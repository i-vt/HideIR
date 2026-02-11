#include "APIHiding.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/TargetParser/Triple.h"
#include <vector>

using namespace llvm;

PreservedAnalyses APIHidingPass::run(Module &M, ModuleAnalysisManager &AM) {
    bool modified = false;
    LLVMContext &ctx = M.getContext();
    IRBuilder<> builder(ctx);
    Triple targetTriple(M.getTargetTriple());

    // Step 1: Declare OS-specific dynamic loading functions
    FunctionCallee resolveFunc;
    if (targetTriple.isOSWindows()) {
        FunctionType *loadLibTy = FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false);
        FunctionCallee loadLib = M.getOrInsertFunction("LoadLibraryA", loadLibTy);
        
        FunctionType *getProcTy = FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
        resolveFunc = M.getOrInsertFunction("GetProcAddress", getProcTy);
    } else {
        // Unix (Linux/Mac/Solaris): dlsym(void* handle, const char* symbol)
        FunctionType *dlsymTy = FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false);
        resolveFunc = M.getOrInsertFunction("dlsym", dlsymTy);
    }

    std::vector<CallInst *> targetCalls;

    for (Function &F : M) {
        if (F.empty() || F.getName().starts_with("obf.")) continue;

        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                if (auto *callInst = dyn_cast<CallInst>(&I)) {
                    Function *callee = callInst->getCalledFunction();
                    
                    // Only target direct calls to external, non-intrinsic functions
                    if (callee && callee->isDeclaration() && !callee->isIntrinsic() && 
                        callee->getName() != "dlsym" && callee->getName() != "GetProcAddress" &&
                        callee->getName() != "LoadLibraryA") {
                        targetCalls.push_back(callInst);
                    }
                }
            }
        }
    }

    // Step 2: Replace direct calls with dynamic resolution
    for (CallInst *CI : targetCalls) {
        Function *callee = CI->getCalledFunction();
        StringRef funcName = callee->getName();

        builder.SetInsertPoint(CI);
        
        // Create a global string for the function name
        Constant *funcNameStr = builder.CreateGlobalStringPtr(funcName, "obf.api." + funcName.str());

        Value *resolvedPtr = nullptr;
        if (targetTriple.isOSWindows()) {
            // GetProcAddress(GetModuleHandleA(NULL), "funcName")
            FunctionType *getModTy = FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false);
            FunctionCallee getMod = M.getOrInsertFunction("GetModuleHandleA", getModTy);
            Value *hModule = builder.CreateCall(getMod, {ConstantPointerNull::get(builder.getPtrTy())});
            resolvedPtr = builder.CreateCall(resolveFunc, {hModule, funcNameStr});
        } else {
            // dlsym(RTLD_DEFAULT, "funcName")
            // RTLD_DEFAULT is usually 0 on Linux/Solaris, and -2 on macOS
            intptr_t rtld_default_val = targetTriple.isMacOSX() ? -2 : 0;
            // FIXED: Changed getIntToPtr to CreateIntToPtr
            Value *handle = builder.CreateIntToPtr(builder.getInt64(rtld_default_val), builder.getPtrTy());
            resolvedPtr = builder.CreateCall(resolveFunc, {handle, funcNameStr});
        }

        // Replace the direct call with an indirect call
        FunctionType *calleeType = callee->getFunctionType();
        std::vector<Value *> args(CI->args().begin(), CI->args().end());
        
        CallInst *indirectCall = builder.CreateCall(calleeType, resolvedPtr, args);
        
        if (!CI->getType()->isVoidTy()) {
            CI->replaceAllUsesWith(indirectCall);
        }
        CI->eraseFromParent();
        modified = true;
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PassPluginLibraryInfo getAPIHidingPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseAPIHiding", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseAPIHiding") {
                        MPM.addPass(APIHidingPass());
                        return true;
                    }
                    return false;
                });
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(APIHidingPass());
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getAPIHidingPluginInfo();
}
