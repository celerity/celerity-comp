#pragma once

namespace celerity {

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
using namespace llvm;

struct ResultCoalescedAnalysis{
    std::map<llvm::Value*, bool> mem_access;
};

struct CoalescedAnalysis : public llvm::AnalysisInfoMixin<CoalescedAnalysis> {
 protected:
    std::map<llvm::Value*, bool> mem_access;    

 public:
    CoalescedAnalysis() { }
    virtual ~CoalescedAnalysis(){}

    using Result = ResultCoalescedAnalysis;
    ResultCoalescedAnalysis run(llvm::Function &fun, llvm::FunctionAnalysisManager &FAM)
    {
        DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(fun);       
        // TODO more accurate coalesced mm=emory access
        return { mem_access };
    }



  friend struct llvm::AnalysisInfoMixin<CoalescedAnalysis>;   
  static llvm::AnalysisKey Key;
}; // end DefaultFeatureAnalysis





}