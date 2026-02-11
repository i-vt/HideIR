#include "AntiDebugging.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/TargetParser/Triple.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses AntiDebuggingPass::run(Module &M, ModuleAnalysisManager &AM) {
    bool modified = false;
    LLVMContext &ctx = M.getContext();
    IRBuilder<> builder(ctx);

    // ==========================================
    // FEATURE 1: OS-Aware Debugger API Trap
    // ==========================================
    FunctionType *funcType = FunctionType::get(Type::getVoidTy(ctx), false);
    Function *antiDebugFunc = Function::Create(funcType, GlobalValue::InternalLinkage, "obf.anti_debug_init", &M);
    antiDebugFunc->addFnAttr(Attribute::NoInline);
    antiDebugFunc->addFnAttr(Attribute::OptimizeNone);

    BasicBlock *entryBlock = BasicBlock::Create(ctx, "entry", antiDebugFunc);
    BasicBlock *trapBlock = BasicBlock::Create(ctx, "trap", antiDebugFunc);
    BasicBlock *retBlock = BasicBlock::Create(ctx, "ret", antiDebugFunc);

    // Trap Block: Crash the program if a debugger is attached
    builder.SetInsertPoint(trapBlock);
    Function *trapIntrinsic = Intrinsic::getDeclaration(&M, Intrinsic::trap);
    builder.CreateCall(trapIntrinsic);
    builder.CreateUnreachable();

    // Ret Block: Normal execution continues
    builder.SetInsertPoint(retBlock);
    builder.CreateRetVoid();

    builder.SetInsertPoint(entryBlock);
    Triple targetTriple(M.getTargetTriple());

    if (targetTriple.isOSWindows()) {
        // Windows: Call IsDebuggerPresent()
        FunctionType *idpType = FunctionType::get(Type::getInt32Ty(ctx), false);
        FunctionCallee idpFunc = M.getOrInsertFunction("IsDebuggerPresent", idpType);
        Value *ret = builder.CreateCall(idpFunc);
        Value *cmp = builder.CreateICmpNE(ret, builder.getInt32(0));
        builder.CreateCondBr(cmp, trapBlock, retBlock);
    } else {
        // Unix (Linux/Mac/Solaris): Call ptrace()
        // On macOS, PT_DENY_ATTACH is 31. On Linux/Solaris, PTRACE_TRACEME is 0.
        int ptrace_request = targetTriple.isMacOSX() ? 31 : 0;
        
        FunctionType *ptraceType = FunctionType::get(Type::getInt64Ty(ctx), {Type::getInt32Ty(ctx), Type::getInt32Ty(ctx), builder.getPtrTy(), builder.getPtrTy()}, false);
        FunctionCallee ptraceFunc = M.getOrInsertFunction("ptrace", ptraceType);
        
        Value *args[] = {
            builder.getInt32(ptrace_request),
            builder.getInt32(0),
            ConstantPointerNull::get(builder.getPtrTy()),
            ConstantPointerNull::get(builder.getPtrTy())
        };
        Value *ret = builder.CreateCall(ptraceFunc, args);
        
        // If ptrace returns -1, we are already being traced
        Value *cmp = builder.CreateICmpEQ(ret, builder.getInt64(-1));
        builder.CreateCondBr(cmp, trapBlock, retBlock);
    }

    // Append to global constructors to run before main()
    appendToGlobalCtors(M, antiDebugFunc, 0);
    modified = true;

    // ==========================================
    // FEATURE 2: Cross-Platform Timing Checks
    // ==========================================
    Function *cycleCounter = Intrinsic::getDeclaration(&M, Intrinsic::readcyclecounter);
    
    for (Function &F : M) {
        if (F.empty() || F.getName().starts_with("obf.")) continue;

        std::vector<BasicBlock *> blocks;
        for (BasicBlock &BB : F) blocks.push_back(&BB);

        for (BasicBlock *BB : blocks) {
            // Randomly inject timing traps (20% probability)
            if (ObfuscatorUtils::Random::generateRandomIntInRange(1, 100) > 20) continue;
            
            Instruction *firstInst = &*BB->getFirstInsertionPt();
            Instruction *termInst = BB->getTerminator();
            if (!termInst || firstInst == termInst || isa<PHINode>(firstInst)) continue;

            // Start timer
            builder.SetInsertPoint(firstInst);
            Value *startCycles = builder.CreateCall(cycleCounter);

            // End timer before block exits
            builder.SetInsertPoint(termInst);
            Value *endCycles = builder.CreateCall(cycleCounter);
            
            Value *diff = builder.CreateSub(endCycles, startCycles);
            
            // 0xFFFFFFF CPU cycles is roughly a fraction of a second.
            // A human manually stepping through assembly in GDB will trigger this easily.
            Value *isStepping = builder.CreateICmpUGT(diff, builder.getInt64(0xFFFFFFF));

            BasicBlock *timeTrapBB = BasicBlock::Create(ctx, "time_trap", &F);
            BasicBlock *timeContBB = BB->splitBasicBlock(termInst, "time_cont");
            
            IRBuilder<> trapBuilder(timeTrapBB);
            trapBuilder.CreateCall(trapIntrinsic);
            trapBuilder.CreateUnreachable();

            BB->getTerminator()->eraseFromParent();
            IRBuilder<> branchBuilder(BB);
            branchBuilder.CreateCondBr(isStepping, timeTrapBB, timeContBB);
            
            modified = true;
        }
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PassPluginLibraryInfo getAntiDebuggingPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseAntiDebugging", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseAntiDebugging") {
                        MPM.addPass(AntiDebuggingPass());
                        return true;
                    }
                    return false;
                });
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(AntiDebuggingPass());
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getAntiDebuggingPluginInfo();
}
