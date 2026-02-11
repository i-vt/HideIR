#include "OpaquePredicate.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h" // Required to fix 'incomplete type' error for Module
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses OpaquePredicatePass::run(Function &F, FunctionAnalysisManager &AM) {
    // Check if function is empty. The Attribute::OptimizeNone check is removed 
    // to ensure obfuscation triggers even at -O0.
    if (F.empty()) {
        return PreservedAnalyses::all();
    }

    Module *M = F.getParent();
    LLVMContext &ctx = F.getContext();
    
    // Create or retrieve a global "Key" variable used for the opaque check
    GlobalVariable *key = M->getGlobalVariable("obf.opaque_key");
    if (!key) {
        key = new GlobalVariable(*M, Type::getInt32Ty(ctx), false, 
                                GlobalValue::PrivateLinkage, 
                                ConstantInt::get(Type::getInt32Ty(ctx), 0), 
                                "obf.opaque_key");
    }

    std::vector<BasicBlock *> originalBlocks;
    for (BasicBlock &BB : F) originalBlocks.push_back(&BB);

    bool modified = false;
    for (BasicBlock *BB : originalBlocks) {
        Instruction *term = BB->getTerminator();
        // Skip blocks that don't have standard terminators (like switches or invokes)
        if (!term || isa<SwitchInst>(term) || isa<InvokeInst>(term)) continue;

        IRBuilder<> builder(term);
        
        // Generate random constants for the opaque math identity
        uint32_t val1 = ObfuscatorUtils::Random::generateRandomIntInRange(2, 50);
        uint32_t val2 = ObfuscatorUtils::Random::generateRandomIntInRange(2, 50);
        
        // Use a volatile load so LLVM cannot assume the value of 'key' is always 0.
        // This prevents the compiler from optimizing away the 'false' branch.
        LoadInst *loadKey = builder.CreateLoad(Type::getInt32Ty(ctx), key, true, "op.key");
        Value *mul = builder.CreateMul(loadKey, builder.getInt32(val1), "op.mul");
        Value *add = builder.CreateAdd(mul, builder.getInt32(val2), "op.add");
        
        // Identity check: (key * val1) + val2 == val2 (where key is always 0)
        Value *cmp = builder.CreateICmpEQ(add, builder.getInt32(val2), "op.cmp");

        // Split the block to insert the opaque conditional branch
        BasicBlock *trueBlock = BB->splitBasicBlock(term, "op.true");
        BasicBlock *falseBlock = BasicBlock::Create(ctx, "op.false", &F);

        // The 'false' block is unreachable junk code that just jumps back
        IRBuilder<> falseBuilder(falseBlock);
        falseBuilder.CreateBr(trueBlock);

        // Replace the original unconditional branch with the opaque conditional branch
        BB->getTerminator()->eraseFromParent();
        IRBuilder<> branchBuilder(BB);
        branchBuilder.CreateCondBr(cmp, trueBlock, falseBlock);

        modified = true;
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

// Plugin registration for the LLVM Pass Manager
PassPluginLibraryInfo getOpaquePredicatePluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseOpaquePredicate", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseOpaquePredicate") {
                        FPM.addPass(OpaquePredicatePass());
                        return true;
                    }
                    return false;
                });
            // Register to run at the end of the optimization pipeline
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(OpaquePredicatePass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getOpaquePredicatePluginInfo();
}
