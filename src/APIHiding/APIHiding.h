#ifndef API_HIDING_H
#define API_HIDING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

namespace llvm {

class APIHidingPass : public PassInfoMixin<APIHidingPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // API_HIDING_H
