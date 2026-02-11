#include "Flattening.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Constants.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/Local.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses FlatteningPass::run(Function &F, FunctionAnalysisManager &AM) {
    if (F.empty() || F.hasFnAttribute(Attribute::OptimizeNone)) {
        return PreservedAnalyses::all();
    }

    // 1. SSA Demotion: Required to prevent cross-block register uses in the flattened CFG.
    
    // Step A: Demote all PHIs to stack slots.
    std::vector<PHINode *> phis;
    for (BasicBlock &BB : F) 
        for (Instruction &I : BB) 
            if (auto *phi = dyn_cast<PHINode>(&I)) 
                phis.push_back(phi);
    
    for (PHINode *P : phis) DemotePHIToStack(P);

    // Step B: Iteratively demote any instruction that is used in a different block.
    // This is crucial because DemotePHIToStack creates new LoadInsts that may 
    // themselves have cross-block uses that break dominance after flattening.
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<Instruction *> toDemote;
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (isa<AllocaInst>(&I)) continue;
                for (User *U : I.users()) {
                    if (auto *UI = dyn_cast<Instruction>(U)) {
                        if (UI->getParent() != &BB) {
                            toDemote.push_back(&I);
                            break;
                        }
                    }
                }
            }
        }
        if (!toDemote.empty()) {
            for (Instruction *I : toDemote) DemoteRegToStack(*I);
            changed = true;
        }
    }

    // 2. Prepare the entry trampoline.
    // We must keep all AllocaInsts in the actual entry block to satisfy the Verifier.
    BasicBlock *entryBlock = &F.getEntryBlock();
    BasicBlock *firstBlock = BasicBlock::Create(F.getContext(), "entry_logic", &F);
    
    // Move all non-alloca instructions from the original entry to a new logic block.
    for (auto it = entryBlock->begin(); it != entryBlock->end(); ) {
        Instruction &I = *it++;
        if (!isa<AllocaInst>(&I)) {
            I.moveBefore(*firstBlock, firstBlock->end());
        }
    }

    // 3. Setup Dispatcher Structure.
    BasicBlock *loopEntry = BasicBlock::Create(F.getContext(), "dispatch_header", &F);
    BasicBlock *loopEnd = BasicBlock::Create(F.getContext(), "loop_end", &F);
    BasicBlock *dispatchBlock = BasicBlock::Create(F.getContext(), "indirect_dispatch", &F);

    std::vector<BasicBlock *> originalBlocks;

    for (BasicBlock &BB : F) {
        if (&BB == entryBlock || &BB == loopEntry || &BB == loopEnd || &BB == dispatchBlock) continue;
        originalBlocks.push_back(&BB);
    }

    if (originalBlocks.empty()) return PreservedAnalyses::all();

    // Setup Entry Trampoline: Initialize state with the BlockAddress of the first block.
    IRBuilder<> entryBuilder(entryBlock);
    
    // Allocate space to hold a pointer (the block address)
    AllocaInst *stateVar = entryBuilder.CreateAlloca(entryBuilder.getPtrTy(), nullptr, "state_var");
    
    // Store the memory address of the first logical block into the state variable
    entryBuilder.CreateStore(BlockAddress::get(firstBlock), stateVar);
    entryBuilder.CreateBr(loopEntry);

    // Central Dispatcher Logic.
    IRBuilder<> builder(loopEntry);
    builder.CreateBr(dispatchBlock);
    
    builder.SetInsertPoint(loopEnd);
    builder.CreateBr(loopEntry);

    // Create the Indirect Branch instruction
    builder.SetInsertPoint(dispatchBlock);
    LoadInst *loadState = builder.CreateLoad(builder.getPtrTy(), stateVar, "load_state");
    IndirectBrInst *indirectBr = builder.CreateIndirectBr(loadState, originalBlocks.size());

    // 4. Re-route Original Blocks back into the dispatcher using their BlockAddresses.
    for (BasicBlock *BB : originalBlocks) {
        // Register the block as a valid destination for the indirect branch
        indirectBr->addDestination(BB);
        
        Instruction *term = BB->getTerminator();
        
        // Functions ending in return or resume leave the dispatcher naturally.
        if (!term || isa<ReturnInst>(term) || isa<ResumeInst>(term)) continue;

        builder.SetInsertPoint(term);
        if (auto *br = dyn_cast<BranchInst>(term)) {
            if (br->isConditional()) {
                // Update state variable with the BlockAddress based on the branch condition.
                Value *select = builder.CreateSelect(br->getCondition(),
                    BlockAddress::get(br->getSuccessor(0)),
                    BlockAddress::get(br->getSuccessor(1)));
                builder.CreateStore(select, stateVar);
            } else {
                // Update state variable with the unconditional BlockAddress.
                builder.CreateStore(BlockAddress::get(br->getSuccessor(0)), stateVar);
            }
            builder.CreateBr(loopEnd);
            term->eraseFromParent();
        }
    }

    return PreservedAnalyses::none();
}

PassPluginLibraryInfo getFlatteningPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseFlattening", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseFlattening") {
                        FPM.addPass(FlatteningPass());
                        return true;
                    }
                    return false;
                });
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(FlatteningPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getFlatteningPluginInfo();
}
