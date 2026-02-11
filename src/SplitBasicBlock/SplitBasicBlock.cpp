#include "SplitBasicBlock.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses SplitBasicBlockPass::run(Function &F, FunctionAnalysisManager &AM) {
    if (F.empty() || F.hasFnAttribute(Attribute::OptimizeNone)) {
        return PreservedAnalyses::all();
    }

    std::vector<BasicBlock *> originalBlocks;
    for (BasicBlock &BB : F) {
        originalBlocks.push_back(&BB);
    }

    bool modified = false;
    for (BasicBlock *BB : originalBlocks) {
        int instCount = 0;
        for (Instruction &I : *BB) { instCount++; }

        if (instCount >= 3) {
            int splitPoint = ObfuscatorUtils::Random::generateRandomIntInRange(1, instCount - 2);
            auto it = BB->begin();
            for (int i = 0; i < splitPoint; ++i) { ++it; }
            
            if (!it->isTerminator() && !isa<PHINode>(&*it)) {
                BB->splitBasicBlock(&*it, BB->getName() + ".split");
                modified = true;
            }
        }
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

PassPluginLibraryInfo getSplitBasicBlockPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseSplitBasicBlock", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseSplitBasicBlock") {
                        FPM.addPass(SplitBasicBlockPass());
                        return true;
                    }
                    return false;
                });
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(SplitBasicBlockPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getSplitBasicBlockPluginInfo();
}
