#pragma once

/*  
 * An LLVM pass to extract features using multivariate polynomal as cost relation features.
 * The heuristic need 
 * It requires the loop analysis pass ("loops") to be executed before of that pass.
 */
class PolFeatPass : public FeaturePass {
 public:
    static char ID; 
    PolFeatPass() : FeaturePass() {}
    PolFeatPass(FeatureSet *fs) : FeaturePass(fs) {}
    virtual ~PolFeatPass() {}

    virtual void getAnalysisUsage(llvm::AnalysisUsage &info) const {}
    virtual void eval_function(llvm::Function &fun) {}
};

