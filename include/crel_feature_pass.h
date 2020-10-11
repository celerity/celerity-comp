#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include <llvm/PassSupport.h>
#include <llvm/PassRegistry.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>

#include "crel_feature_set.h"

namespace celerity {

/* 
 * An LLVM function pass to extract features. 
 * The extraction of features from a single instruction is delegated to a feature set class.
 * In this basic implementation, BB's instruction contributions are summed up.
 */
//class feature_pass : public llvm::CallGraphSCCPass {
class crel_feature_pass : public llvm::ModulePass {
 public:
    static char ID;
    crel_feature_set *featureSet;
    poly_gpu_feature_set default_feature_set;

    crel_feature_pass() : llvm::ModulePass(ID) {
        featureSet = &default_feature_set;
    }

    crel_feature_pass(crel_feature_set *fs) : llvm::ModulePass(ID) {
        featureSet = fs;
    }
    virtual ~crel_feature_pass() {}

    virtual void eval_BB(llvm::BasicBlock &bb);	
    virtual void eval_function(llvm::Function &fun);

    /* Overrides LLVM CallGraphSCCPass method */
    virtual bool runOnModule(llvm::Module &m);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {};
    void finalize();
};

/*
 * An LLVM pass to extract features using cost relations. Cost relations are
 * multivariate polynomials where the polynomial unknowns are variables that
 * can only be determined at runtime
 * It requires the loop analysis pass ("loops") to be executed before of that pass.
 */
class poly_crel_pass : public crel_feature_pass {
public:
    static char ID;
    poly_crel_pass() : crel_feature_pass() {}
    poly_crel_pass(crel_feature_set *fs) : crel_feature_pass(fs) {}
    virtual ~poly_crel_pass() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const;
    virtual void eval_function(llvm::Function &fun);

private:
    crel_mpoly evaluateValue(const crel_kernel &kernel, llvm::Value *value);
    crel_mpoly evaluateSCEV(llvm::ScalarEvolution &SE, const crel_kernel &kernel, const llvm::Loop &loop, crel_mpoly &poly, const llvm::SCEV *scev);
};


} // end namespace celerity
