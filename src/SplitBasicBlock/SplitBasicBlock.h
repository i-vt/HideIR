#ifndef SPLIT_BASIC_BLOCK_H
#define SPLIT_BASIC_BLOCK_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"

namespace llvm {

class SplitBasicBlockPass : public PassInfoMixin<SplitBasicBlockPass> {
public:
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // SPLIT_BASIC_BLOCK_H
