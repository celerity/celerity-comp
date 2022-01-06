#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
using namespace llvm;

#include "KernelInvariant.hpp"
#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"


namespace celerity {

/// Feature set based on Fan's work, specifically designed for GPU architecture. 
class PolFeatFeatureSet : public FeatureSet {
 private:
    FeatureSet *reference;
 public:
    PolFeatFeatureSet() {}
    virtual ~PolFeatFeatureSet();
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};


/// An LLVM analysis to extract features using multivariate polynomal as cost relation features.
struct PolFeatAnalysis : public FeatureAnalysis, llvm::AnalysisInfoMixin<PolFeatAnalysis> {
 public:
   PolFeatAnalysis(string feature_set = "fan19") : FeatureAnalysis() { 
      analysis_name="kofler13"; 
      features = FSRegistry::dispatch(feature_set);
   }
   virtual ~PolFeatAnalysis(){}

   //PreservedAnalyses run(Loop &L, LoopAnalysisManager &AM, LoopStandardAnalysisResults &AR, LPMUpdater &U);

   /// overwrite feature extraction for function
   virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
   // calculate the loop contribution of a given loop (assume non nesting, which is calculated later)
   //Polynomial loopContribution(const Loop &loop, LoopInfo &LI, ScalarEvolution &SE);
   
   friend struct llvm::AnalysisInfoMixin<PolFeatAnalysis>;   
   static llvm::AnalysisKey Key;
};

} // namespace


