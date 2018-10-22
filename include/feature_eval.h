#pragma once

#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include "feature_set.h"

namespace celerity {

/* List of supported feature extraction techniques 
enum class feature_eval_mode { 
	RAW, 	      // Absolute values, not normalized. 
	GREWE11,      // Follows Grewe et al. CC 2011 paper. No loop, only sum of BB values 
	KOFLER13,     // Follows Kofler et al. ICS 2013 paper. Loop use the "unroll100" heuristic 
	FAN18,        // An extension of Grewe et al. with more features, designed to
	COST_RELATION // Advanced representation where loop count are propagated in a cost relation feature form. It requires runtime feature evaluation, but is the most accurate.
};
*/

/* List of supported feature extraction techniques */
enum class feature_eval_mode { 
    NORMAL,         // Instruction features of each BB are summed up for the program.
    KOFLER13,       // Instruction features of instructions inside loops have a larger contribution.
    COST_RELATION   // Instruction features are progated as cost relations . More accurate, but requires runtime evaluation.
};


/* 
 * An LLVM function pass to extract features. 
 * The extraction of features from a single instruction is delegated to
 * a feature set class.
 * In this basic implementation, BB'instruction are summed up (ass in Grewe 2011).
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
    void finalize();
};

/*  An LLVM function pass to extract features using [Kofler et al., 13] loop heuristics.  */
class kofler13_eval : public feature_eval {
 public:
    kofler13_eval() : feature_eval() {}
	kofler13_eval(feature_set *fs) : feature_eval(fs) {}
    virtual ~kofler13_eval() {}

    virtual void eval_function(const llvm::Function &fun);
};

/*  Feature etraction based on cost realation */
// NOTE(Biagio) for Nadjib: your implementation should be included here.
// class costrelation_eval : public feature_eval {
//    virtual void eval_function(const llvm::Function &fun);
//};
//

} // end namespace celerity
