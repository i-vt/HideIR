#ifndef STRING_ENCRYPTION_H
#define STRING_ENCRYPTION_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

namespace llvm {

// Note: StringEncryption operates on the whole module since it manipulates global strings
class StringEncryptionPass : public PassInfoMixin<StringEncryptionPass> {
public:
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
    static bool isRequired() { return true; } 
};

} // namespace llvm

#endif // STRING_ENCRYPTION_H
