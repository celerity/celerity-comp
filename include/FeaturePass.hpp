#pragma once


#include <llvm/IR/PassManager.h>
//#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

#include <llvm/PassSupport.h>
#include <llvm/PassRegistry.h>

#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/LoopInfo.h>

#include "feature_set.h"

using namespace llvm;

namespace celerity {

/* List of supported feature extraction techniques */
enum class feature_pass_mode { 
    NORMAL,     // Instruction features of each BB are summed up for the program.
    KOFLER13,   // Instruction features of instructions inside loops have a larger contribution.
    POLFEAT     // Instruction features are progated as polynomial cost relations . More accurate, but requires runtime evaluation.
};


/* 
 * An LLVM function pass that extract static code features. 
 * The extraction of features from a single instruction is delegated to a feature set class.
 * In this basic implementation, BB's instruction contributions are summed up.
 */
class FeaturePass :  PassInfoMixin<FeaturePass> {
//public llvm::ModulePass {
 public:
    static char ID;     
    Fan19FeatureSet default_feature_set;
    FeatureSet *features = &default_feature_set;

    virtual void setFeatureSet(FeatureSet &fs){
        features = &fs;
    }
/*
    FeaturePass() : llvm::ModulePass(ID) {        
        features = &default_feature_set;        
    }

    FeaturePass(FeatureSet *fs) : llvm::ModulePass(ID) {
        features = fs;
    }
    virtual ~FeaturePass() {}
*/
    virtual void eval_BB(llvm::BasicBlock &bb);	
    virtual void eval_function(llvm::Function &fun);

    /* Overrides LLVM CallGraphSCCPass method */
    virtual bool runOnModule(llvm::Module &m);
    virtual bool runOnSCC(llvm::CallGraphSCC &SCC);
    virtual void getAnalysisUsage(llvm::AnalysisUsage &au) const {au.addRequired<llvm::CallGraphWrapperPass>();};
    void finalize();
};



/*  
 * An LLVM pass to extract features using [Kofler et al., 13] loop heuristics.
 * The heuristic gives more important (x100) to the features inside a loop.
 * It requires the loop analysis pass ("loops") to be executed before of that pass.
 */
class Kofler13Pass : public PassInfoMixin<Kofler13Pass> {
 public:
   static char ID; 
/*   Kofler13Pass() : FeaturePass() {}
    Kofler13Pass(FeatureSet *fs) : FeaturePass(fs) {}
    virtual ~Kofler13Pass() {}
*/
    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const;
    virtual void eval_function(llvm::Function &fun);
};

//ExtractorFeaturePassRegister<FeaturePass>
//ExtractorFeaturePassRegister<Kofler13Pass>

} // end namespace celerity
