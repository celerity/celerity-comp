#pragma once

//#include <flint/fmpz_mpoly.h>
#include <flint/fmpz_mpoly.h>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
using namespace llvm;


#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"


namespace celerity {

const uint32_t MAX_IMPOLY_DEGREE = 3;

/// Multivariate Polinomial with variable based on invariants
class IMPoly {
 private:
   fmpz_mpoly_t mpoly;
   fmpz_mpoly_ctx_t ctx;
 
 public:
   IMPoly();
   IMPoly(unsigned constant, unsigned inv_id);
   ~IMPoly();
   

   IMPoly& operator += (IMPoly const &poly);
   IMPoly& operator *= (IMPoly const &poly);
   IMPoly max(IMPoly const &poly1, IMPoly const &poly2);

   string str() const;

   friend std::ostream &operator<<(std::ostream &output, const IMPoly &rhs){
      return output << rhs.str();
   }

};



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
   IMPoly loopContribution(const Loop &loop, LoopInfo &LI, ScalarEvolution &SE);
   
   friend struct llvm::AnalysisInfoMixin<PolFeatAnalysis>;   
   static llvm::AnalysisKey Key;
};

} // namespace

