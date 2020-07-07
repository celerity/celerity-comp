#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include <llvm/PassSupport.h>
#include <llvm/PassRegistry.h>

#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>

#include "feature_set.h"

namespace celerity {

/* List of supported feature extraction techniques */
enum class feature_pass_mode { 
    NORMAL,         // Instruction features of each BB are summed up for the program.
    KOFLER13,       // Instruction features of instructions inside loops have a larger contribution.
    COST_RELATION   // Instruction features are progated as cost relations . More accurate, but requires runtime evaluation.
};


/* 
 * An LLVM function pass to extract features. 
 * The extraction of features from a single instruction is delegated to a feature set class.
 * In this basic implementation, BB's instruction contributions are summed up.
 */
class feature_pass : public llvm::CallGraphSCCPass {
//class feature_pass : public llvm::ModulePass {
 public:
    static char ID; 
    feature_set *features;
    gpu_feature_set default_feature_set;

    feature_pass() : llvm::CallGraphSCCPass(ID) {        
        features = &default_feature_set;        
    }

    feature_pass(feature_set *fs) : llvm::CallGraphSCCPass(ID) {
        features = fs;
    }
    virtual ~feature_pass() {}

    virtual void eval_BB(llvm::BasicBlock &bb);	
    virtual void eval_function(llvm::Function &fun);

    /* Overrides LLVM CallGraphSCCPass method */
    virtual bool runOnSCC(llvm::CallGraphSCC &SCC);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {au.addRequired<llvm::CallGraphWrapperPass>();};
    void finalize();
};

/*  
 * An LLVM pass to extract features using [Kofler et al., 13] loop heuristics.
 * The heuristic gives more important (x100) to the features inside a loop.
 * It requires the loop analysis pass ("loops") to be executed before of that pass.
 */
class kofler13_pass : public feature_pass {
 public:
    static char ID; 
    kofler13_pass() : feature_pass() {}
    kofler13_pass(feature_set *fs) : feature_pass(fs) {}
    virtual ~kofler13_pass() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const;
    virtual void eval_function(llvm::Function &fun);
};

/*  Feature extraction based on cost realation */
// NOTE(Biagio) for Nadjib: your implementation should be included here.
// class costrelation_eval : public feature_eval {
//    virtual void eval_function(const llvm::Function &fun);
//};
//

} // end namespace celerity
