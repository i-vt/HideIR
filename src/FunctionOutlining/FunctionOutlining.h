#ifndef FUNCTION_OUTLINING_H
#define FUNCTION_OUTLINING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

namespace llvm {

// The core class for the Function Outlining pass
class FunctionOutliningPass : public PassInfoMixin<FunctionOutliningPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

    // Tells the compiler this pass shouldn't be skipped by optimizations
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // FUNCTION_OUTLINING_H
