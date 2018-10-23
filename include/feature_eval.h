#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include <llvm/PassSupport.h>
#include <llvm/PassRegistry.h>

#include "feature_set.h"

namespace celerity {

/* List of supported feature extraction techniques */
enum class feature_eval_mode { 
    NORMAL,         // Instruction features of each BB are summed up for the program.
    KOFLER13,       // Instruction features of instructions inside loops have a larger contribution.
    COST_RELATION   // Instruction features are progated as cost relations . More accurate, but requires runtime evaluation.
};


/* 
 * An LLVM function pass to extract features. 
 * The extraction of features from a single instruction is delegated to a feature set class.
 * In this basic implementation, BB's instruction contributions are summed up.
 */
class feature_eval : public llvm::FunctionPass {
 public:
	static char ID; 
    feature_set *features;
    gpu_feature_set default_feature_set;

    feature_eval() : llvm::FunctionPass(ID) {        
        features = &default_feature_set;        
    }
	feature_eval(feature_set *fs) : llvm::FunctionPass(ID) {
        features = fs;
    }
    virtual ~feature_eval() {}

	virtual void eval_BB(const llvm::BasicBlock &bb);	
    virtual void eval_function(const llvm::Function &fun);

    /* Overwrites LLVM FunctionPass method */
	virtual bool runOnFunction(llvm::Function &fun);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const {}    
    void finalize();
};

/*  
 * An LLVM function pass to extract features using [Kofler et al., 13] loop heuristics.
 * The heuristic gives more important (x100) to the features inside a loop.
 * It requires the loop analysis pass ("loops") to be executed before of that pass.
 */
class kofler13_eval : public feature_eval {
 public:
    kofler13_eval() : feature_eval() {}
	kofler13_eval(feature_set *fs) : feature_eval(fs) {}
    virtual ~kofler13_eval() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const;
    virtual void eval_function(const llvm::Function &fun);
};


/*  Feature etraction based on cost realation */
// NOTE(Biagio) for Nadjib: your implementation should be included here.
// class costrelation_eval : public feature_eval {
//    virtual void eval_function(const llvm::Function &fun);
//};
//

} // end namespace celerity
