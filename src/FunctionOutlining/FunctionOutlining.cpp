#include "FunctionOutlining.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Transforms/Utils/CodeExtractor.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses FunctionOutliningPass::run(Function &F, FunctionAnalysisManager &AM) {
    // Skip empty functions or functions marked with OptimizeNone
    if (F.empty() || F.hasFnAttribute(Attribute::OptimizeNone)) {
        return PreservedAnalyses::all();
    }

    // Do not outline functions that have already been outlined to prevent infinite loops,
    // and skip the decryption stub from the StringEncryption pass.
    if (F.getName().contains("obf.outlined") || F.getName().contains("obf.decrypt_strings")) {
        return PreservedAnalyses::all();
    }

    // The CodeExtractor requires DominatorTree analysis to safely compute inputs/outputs
    auto &DT = AM.getResult<DominatorTreeAnalysis>(F);

    std::vector<BasicBlock *> blocksToOutline;
    
    // Skip the entry block. Outlining the entry block is dangerous because it often 
    // contains AllocaInsts (stack allocations) which must remain in the parent function.
    auto it = F.begin();
    ++it; 
    
    for (; it != F.end(); ++it) {
        BasicBlock *BB = &*it;
        
        // Skip Exception Handling pads, they cannot be cleanly extracted
        if (BB->isEHPad()) continue;
        
        // Collect blocks. In a pure production environment, you could tie this to a probability 
        // config variable. Here we attempt to extract all eligible blocks to maximize scattering.
        blocksToOutline.push_back(BB);
    }

    bool modified = false;

    // CodeExtractor works best on valid dominance regions. We extract valid blocks individually.
    for (BasicBlock *BB : blocksToOutline) {
        std::vector<BasicBlock *> extractionRegion = { BB };
        
        CodeExtractorAnalysisCache CEAC(F);
        
        // Initialize the CodeExtractor. We disable AllowAlloca to prevent it from moving 
        // stack variables into the new function, which can break scope.
        CodeExtractor CE(extractionRegion, &DT, /*AggregateArgs=*/false, nullptr, nullptr, nullptr, 
                         /*AllowVarArgs=*/false, /*AllowAlloca=*/false, 
                         /*AllocationBlock=*/nullptr, "obf.outlined");
        
        // isEligible() automatically verifies that the block doesn't break SSA form or dominance
        if (CE.isEligible()) {
            Function *outlinedFn = CE.extractCodeRegion(CEAC);
            if (outlinedFn) {
                // Add NoInline so standard compiler optimizations (-O2/-O3) don't just 
                // immediately inline the function back into the parent, undoing our work.
                outlinedFn->addFnAttr(Attribute::NoInline);
                modified = true;
            }
        }
    }

    return modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

// Plugin registration for the LLVM Pass Manager
PassPluginLibraryInfo getFunctionOutliningPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseFunctionOutlining", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseFunctionOutlining") {
                        FPM.addPass(FunctionOutliningPass());
                        return true;
                    }
                    return false;
                });
            PB.registerOptimizerLastEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    FunctionPassManager FPM;
                    FPM.addPass(FunctionOutliningPass());
                    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getFunctionOutliningPluginInfo();
}
