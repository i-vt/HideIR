#include "AntiTampering.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
#include <vector>

using namespace llvm;

static std::pair<Value*, BasicBlock*> createHashLoop(
    LLVMContext &ctx,
    IRBuilder<> &B,
    Function *parentFunc,
    Function *targetFunc,
    BasicBlock *startBlock)
{
    // Create loop blocks
    BasicBlock *loopHeader = BasicBlock::Create(ctx, "hash.loop", parentFunc);
    BasicBlock *loopEnd = BasicBlock::Create(ctx, "hash.end", parentFunc);

    // Branch from startBlock → loopHeader
    IRBuilder<> startBuilder(startBlock);
    startBuilder.CreateBr(loopHeader);

    // Loop body
    IRBuilder<> loopBuilder(loopHeader);

    PHINode *iNode = loopBuilder.CreatePHI(B.getInt32Ty(), 2, "hash.i");
    PHINode *hashNode = loopBuilder.CreatePHI(B.getInt32Ty(), 2, "hash.val");

    Value *startHash = B.getInt32(0x811c9dc5);

    iNode->addIncoming(B.getInt32(0), startBlock);
    hashNode->addIncoming(startHash, startBlock);

    // Cast function to pointer
    Value *funcPtr = loopBuilder.CreatePointerCast(targetFunc, B.getPtrTy());

    // Read byte
    Value *bytePtr = loopBuilder.CreateInBoundsGEP(
        B.getInt8Ty(), funcPtr, iNode);

    Value *byteVal = loopBuilder.CreateLoad(
        B.getInt8Ty(), bytePtr, true);

    Value *byteExt = loopBuilder.CreateZExt(
        byteVal, B.getInt32Ty());

    // FNV-1a
    Value *xorVal = loopBuilder.CreateXor(hashNode, byteExt);
    Value *newHash = loopBuilder.CreateMul(
        xorVal, B.getInt32(16777619));

    Value *nextI = loopBuilder.CreateAdd(
        iNode, B.getInt32(1));

    Value *cond = loopBuilder.CreateICmpSLT(
        nextI, B.getInt32(64));

    loopBuilder.CreateCondBr(cond, loopHeader, loopEnd);

    iNode->addIncoming(nextI, loopHeader);
    hashNode->addIncoming(newHash, loopHeader);

    return { newHash, loopEnd };
}

PreservedAnalyses AntiTamperingPass::run(Module &M, ModuleAnalysisManager &) {
    bool modified = false;
    LLVMContext &ctx = M.getContext();
    IRBuilder<> builder(ctx);

    // Collect functions
    std::vector<Function*> targets;
    for (Function &F : M) {
        if (!F.empty() && !F.getName().starts_with("obf.")) {
            targets.push_back(&F);
        }
    }

    if (targets.empty())
        return PreservedAnalyses::all();

    // ===============================
    // Create a per-function expected hash global so each function's
    // integrity is verified independently.
    // ===============================
    std::vector<GlobalVariable *> expectedHashes;
    for (size_t i = 0; i < targets.size(); ++i) {
        GlobalVariable *gh = new GlobalVariable(
            M,
            builder.getInt32Ty(),
            false,
            GlobalValue::PrivateLinkage,
            builder.getInt32(0),
            "obf.expected_hash." + targets[i]->getName().str());
        expectedHashes.push_back(gh);
    }

    // ===============================
    // Constructor: compute baseline hash for every target function
    // ===============================
    FunctionType *initTy =
        FunctionType::get(Type::getVoidTy(ctx), false);

    Function *initFunc =
        Function::Create(initTy,
                         GlobalValue::InternalLinkage,
                         "obf.tamper_init",
                         &M);

    BasicBlock *currentBlock =
        BasicBlock::Create(ctx, "entry", initFunc);

    for (size_t i = 0; i < targets.size(); ++i) {
        auto [initHash, initEnd] =
            createHashLoop(ctx, builder, initFunc,
                           targets[i], currentBlock);

        IRBuilder<> storeBuilder(initEnd);
        storeBuilder.CreateStore(initHash, expectedHashes[i], true);

        // Chain: each function's hash loop feeds into the next.
        if (i + 1 < targets.size()) {
            currentBlock = BasicBlock::Create(ctx, "init.next", initFunc);
            storeBuilder.CreateBr(currentBlock);
        } else {
            storeBuilder.CreateRetVoid();
        }
    }

    appendToGlobalCtors(M, initFunc, 0);

    // ===============================
    // Runtime checks: each function hashes *itself*
    // ===============================
    Function *trap =
        Intrinsic::getDeclaration(&M, Intrinsic::trap);

    for (size_t i = 0; i < targets.size(); ++i) {
        Function *F = targets[i];
        BasicBlock &entry = F->getEntryBlock();
        Instruction *insertPt = &*entry.getFirstInsertionPt();

        // Split entry
        BasicBlock *cont =
            entry.splitBasicBlock(insertPt, "tamper.cont");

        // Remove default branch
        entry.getTerminator()->eraseFromParent();

        // Hash this function's own bytes at runtime
        auto [runtimeHash, endBlock] =
            createHashLoop(ctx, builder, F,
                           F, &entry);

        IRBuilder<> checkBuilder(endBlock);

        Value *stored =
            checkBuilder.CreateLoad(
                builder.getInt32Ty(),
                expectedHashes[i],
                true);

        Value *valid =
            checkBuilder.CreateICmpEQ(runtimeHash, stored);

        BasicBlock *trapBlock =
            BasicBlock::Create(ctx, "tamper.trap", F);

        IRBuilder<> trapBuilder(trapBlock);
        trapBuilder.CreateCall(trap);
        trapBuilder.CreateUnreachable();

        checkBuilder.CreateCondBr(valid, cont, trapBlock);

        modified = true;
    }

    return modified
           ? PreservedAnalyses::none()
           : PreservedAnalyses::all();
}

// Plugin registration
PassPluginLibraryInfo getAntiTamperingPluginInfo() {
    return {
        LLVM_PLUGIN_API_VERSION,
        "EnterpriseAntiTampering",
        "1.0",
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "EnterpriseAntiTampering") {
                        MPM.addPass(AntiTamperingPass());
                        return true;
                    }
                    return false;
                });
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM,
                   OptimizationLevel) {
                    MPM.addPass(AntiTamperingPass());
                });
        }};
}

extern "C" LLVM_ATTRIBUTE_WEAK
::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return getAntiTamperingPluginInfo();
}
