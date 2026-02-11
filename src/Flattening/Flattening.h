#ifndef FLATTENING_H
#define FLATTENING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

namespace llvm {

// The core class for our Control Flow Flattening pass
class FlatteningPass : public PassInfoMixin<FlatteningPass> {
public:
    // The main entry point. This runs once for every function in the source code.
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);

    // Tells the compiler this pass shouldn't be skipped by optimizations
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // FLATTENING_H
