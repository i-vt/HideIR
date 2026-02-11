#ifndef OPAQUE_PREDICATE_H
#define OPAQUE_PREDICATE_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

namespace llvm {

class OpaquePredicatePass : public PassInfoMixin<OpaquePredicatePass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // OPAQUE_PREDICATE_H
