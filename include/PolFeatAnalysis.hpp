#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
using namespace llvm;

#include "IMPoly.hpp"
#include "FeatureSet.hpp"



namespace celerity {


/// Feature set representation based multivariate polynomials. 
class PolFeatSet {
 private:
   llvm::StringMap<IMPoly> raw;   // features as multivariate polynomial counters, before normalization
   llvm::StringMap<float> feat;   // features after normalization and runtime resolution
   int instruction_num;
   /// int instruction_tot_contrib; NOTE normalization after ?
   string name;
 public:
   PolFeatSet() : name("default") {}
   PolFeatSet(string feature_set_name) : name(feature_set_name){}
   virtual ~PolFeatSet();

   llvm::StringMap<IMPoly> getFeatureCounts(){ return raw; }
   llvm::StringMap<float> getFeatureValues(){ return feat; }
   string getName(){ return name; }

   /// Set all features to zero
   virtual void reset(){
        for(StringRef rkey : raw.keys())
            raw[rkey] = IMPoly();
        for(StringRef fkey : feat.keys())
            feat[fkey] = 0;
      instruction_num = 0;
      //instruction_tot_contrib = 0; TO FIX XXX ???
    }
   
   /// Add a feature contribution to the feature set
   virtual void add(const string &feature_name, IMPoly &contribution /*= 1*/){
        raw[feature_name] += contribution;
        instruction_num += 1;
        //instruction_tot_contrib += contribution; TO FIX XXX ???
   }

   virtual void eval(llvm::Instruction &inst, IMPoly &contribution /*= 1*/) {}
   
   virtual void normalize(llvm::Function &fun){}
   
   virtual void print(llvm::raw_ostream &out_stream){}    
};


  /// Results of a PolFeat feature analysis
  struct ResultPolFeatSet
  {
    llvm::StringMap<IMPoly> raw;
    llvm::StringMap<float> feat;
  };


/// An LLVM analysis to extract features using multivariate polynomal as cost relation features.
struct PolFeatAnalysis : public llvm::AnalysisInfoMixin<PolFeatAnalysis> {
 protected:
  PolFeatSet *features;
  string analysis_name;
 public:
   PolFeatAnalysis(string feature_set = "fan19") { 
      analysis_name ="polfeat"; 
      //features = FSRegistry::dispatch(feature_set); TODO FIX
   }
   virtual ~PolFeatAnalysis(){}

  /// runs the analysis on a specific function, returns a StringMap
  using Result = ResultPolFeatSet;
  ResultPolFeatSet run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);

   /// overwrite feature extraction for function
   virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
   
   // calculate the loop contribution of a given loop (assume non nesting, which is calculated later)
   IMPoly loopContribution(const Loop &loop, LoopInfo &LI, ScalarEvolution &SE);
   
   friend struct llvm::AnalysisInfoMixin<PolFeatAnalysis>;   
   static llvm::AnalysisKey Key;

};


} // namespace

