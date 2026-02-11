#ifndef ANTI_TAMPERING_H
#define ANTI_TAMPERING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

namespace llvm {

class AntiTamperingPass : public PassInfoMixin<AntiTamperingPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // ANTI_TAMPERING_H
