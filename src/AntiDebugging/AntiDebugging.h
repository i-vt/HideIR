#ifndef ANTI_DEBUGGING_H
#define ANTI_DEBUGGING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

namespace llvm {

class AntiDebuggingPass : public PassInfoMixin<AntiDebuggingPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // ANTI_DEBUGGING_H
