#include "StringEncryption.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include "llvm/Support/raw_ostream.h" // For debug printing
#include "../Utils/Crypto.h"
#include "../Utils/Random.h"
#include <vector>

using namespace llvm;

PreservedAnalyses StringEncryptionPass::run(Module &M, ModuleAnalysisManager &AM) {
    bool modified = false;
    LLVMContext &ctx = M.getContext();
    std::vector<GlobalVariable *> targetStrings;
    std::vector<uint8_t> keys;

    // Iterate through all global variables
    for (GlobalVariable &GV : M.globals()) {
        // Skip metadata and globals without initializers
        if (GV.getName().starts_with("llvm.") || !GV.hasInitializer()) continue;

        Constant *init = GV.getInitializer();
        
        // ConstantDataSequential is the base class for ConstantDataArray (strings)
        auto *CDS = dyn_cast<ConstantDataSequential>(init);
        
        // Only target 8-bit integer sequences (character arrays)
        if (!CDS || !CDS->getElementType()->isIntegerTy(8)) continue;

        StringRef data = CDS->getRawDataValues();
        if (data.empty() || data.size() < 4) continue; // Skip very short strings/padding

        // Generate a random key for this specific string
        uint8_t key = static_cast<uint8_t>(ObfuscatorUtils::Random::generateRandomIntInRange(1, 255));
        
        // Perform XOR encryption
        std::vector<uint8_t> encrypted;
        for (unsigned char c : data) {
            encrypted.push_back(c ^ key);
        }

        // Replace the plaintext with encrypted data
        Constant *newInit = ConstantDataArray::get(ctx, encrypted);
        GV.setInitializer(newInit);
        
        // CRITICAL: Global must be mutable so the decryption stub can write to it
        GV.setConstant(false);
        
        targetStrings.push_back(&GV);
        keys.push_back(key);
        modified = true;
    }

    if (!modified) return PreservedAnalyses::all();

    // Create a decryption function that runs at program startup
    FunctionType *funcType = FunctionType::get(Type::getVoidTy(ctx), false);
    Function *decryptFunc = Function::Create(funcType, GlobalValue::InternalLinkage, "obf.decrypt_strings", &M);
    
    // Prevent the optimizer from removing this logic
    decryptFunc->addFnAttr(Attribute::NoInline);
    decryptFunc->addFnAttr(Attribute::OptimizeNone);

    BasicBlock *entryBlock = BasicBlock::Create(ctx, "entry", decryptFunc);
    IRBuilder<> builder(entryBlock);

    for (size_t i = 0; i < targetStrings.size(); ++i) {
        GlobalVariable *GV = targetStrings[i];
        uint8_t key = keys[i];
        
        Type *valueType = GV->getValueType();
        if (!valueType->isArrayTy()) continue;

        uint64_t arrSize = valueType->getArrayNumElements();
        
        // Decrypt the string in-place
        for (uint64_t j = 0; j < arrSize; ++j) {
            Value *idxList[] = {builder.getInt64(0), builder.getInt64(j)};
            Value *gep = builder.CreateInBoundsGEP(valueType, GV, idxList);
            
            // CRITICAL FIX: Make the load and store VOLATILE (the 'true' argument).
            // This stops LLVM's GlobalOpt from statically evaluating our decryption routine!
            LoadInst *load = builder.CreateLoad(builder.getInt8Ty(), gep, true); 
            Value *xorVal = builder.CreateXor(load, builder.getInt8(key));
            builder.CreateStore(xorVal, gep, true);
        }
    }
    
    builder.CreateRetVoid();
    
    // Append to global constructors to ensure it runs before main()
    appendToGlobalCtors(M, decryptFunc, 0);

    return PreservedAnalyses::none();
}

PassPluginLibraryInfo getStringEncryptionPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION, "EnterpriseStringEncryption", "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseStringEncryption") {
                        MPM.addPass(StringEncryptionPass());
                        return true;
                    }
                    return false;
                });
            
            // Run at the beginning of the pipeline to guarantee we catch the string
            // before standard optimizations inline or fold it.
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(StringEncryptionPass());
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getStringEncryptionPluginInfo();
}
