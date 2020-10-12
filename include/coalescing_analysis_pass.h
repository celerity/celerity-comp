#include "MemAccessDescriptor.h"

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#include <set>
#include <map>
#include <list>
#include <functional>
#include <algorithm>
#include <ostream>

using namespace llvm;

class NDRange {
  private:
    bool isFunc(Instruction* inst, std::string name) {
      if (CallInst* call = dyn_cast<CallInst>(inst)) {
        Function* f = call->getCalledFunction();
        if (f) {
          StringRef fname = f->getName();
          return fname.contains(name);
        }
      }
      return false;
    }
  public:
    bool isTid(Instruction* inst) {
      return isGlobal(inst) || isLocal(inst);
    }
    int getDirection(Instruction* inst) {
      if (CallInst *call = dyn_cast<CallInst>(inst)) {
        if (const ConstantInt *ci = dyn_cast<ConstantInt>(call->getArgOperand(0))) {
          return ci->getSExtValue();
        }
      }
      return -1;
    }
    bool isGlobal(Instruction* inst) {
      return isFunc(inst, "get_global_id");
    }
    bool isLocal(Instruction* inst) {
      return isFunc(inst, "get_local_id");
    }
    bool isGlobalSize(Instruction* inst) {
      return isFunc(inst, "get_global_size");
    }
    bool isLocalSize(Instruction* inst) {
      return isFunc(inst, "get_local_size");
    }
    bool isGroupId(Instruction* inst) {
      return isFunc(inst, "get_group_id");
    }
    bool isGroupsNum(Instruction* inst) {
      return isFunc(inst, "get_num_groups");
    }
};


class CoalescingAnalysisPass : public llvm::FunctionPass {
  private:
    NDRange ndr = NDRange();
    LoopInfo* loopInfo;
    int dimensions;
    Instruction* lastInstruction;
    //std::vector<BasicBlock::Iterator> loopStack;
    std::set<Instruction*> memops;
    std::set<Instruction*> relevantInstructions;
    std::set<Loop *> relevantLoops;
    std::vector<std::map<Instruction*, std::vector<MemAccessDescriptor>>> accessDescriptorStack;
    std::map<StringRef, std::set<int>> accessedCacheLines;

    inst_iterator simulate(inst_iterator inst, Instruction* fwdDef, Loop* innermostLoop);
    PHINode *getInductionVariable(Loop* loop) const;
    void preprocess(Function *function, std::set<Instruction*>& memops, std::set<Instruction*>& relevantInstructions);
    Value * getAccessedSymbolPtr(Value * v);
    StringRef getAccessedSymbolName(Value * v);
    bool isCachedAddressSpace(Instruction * inst);
    std::vector<MemAccessDescriptor> findInStack(Instruction* inst);
    void mergeIntoStack(std::map<Instruction*, std::vector<MemAccessDescriptor>> &defs);
    bool isFwdDef(Instruction* inst);
    std::vector<MemAccessDescriptor> getOperand(Value * v);
    void applyBinaryOp(function<int(int, int)> f, Instruction * inst);

    inline void addToStack(Instruction* inst, std::vector<MemAccessDescriptor> mad) {
      accessDescriptorStack.back().insert(std::pair<Instruction*, std::vector<MemAccessDescriptor>>(inst, mad));
    }
    inline void addToStack(Instruction* inst, MemAccessDescriptor mad) {
      addToStack(inst, std::vector<MemAccessDescriptor>{mad});
    }
  public:
    std::map<Instruction*, int> cacheLinesPerWarp;
    static char ID; 

    CoalescingAnalysisPass() : llvm::FunctionPass(ID) {        
    }

    virtual ~CoalescingAnalysisPass() {}

    void preprocess(Function *function);

    virtual bool runOnFunction(llvm::Function &F);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {au.addRequired<llvm::LoopInfoWrapperPass>();};
    void finalize();
};

