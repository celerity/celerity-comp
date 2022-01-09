#pragma once

#include <llvm/IR/PassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
using namespace llvm;

#include "FeatureAnalysis.hpp"

namespace celerity {

/// An LLVM analysisfunction pass that extract static code features. 
/// The extraction of features from a single instruction is delegated to a feature set class.
/// In this basic implementation, BB's instruction contributions are summed up.
struct DefaultFeatureAnalysis : public FeatureAnalysis, llvm::AnalysisInfoMixin<DefaultFeatureAnalysis> {
    
 public:
    DefaultFeatureAnalysis(string feature_set = "fan19") { 
      analysis_name="default";
      features = FSRegistry::dispatch(feature_set);
      assert(features != nullptr);      
    }
    virtual ~DefaultFeatureAnalysis(){}

  friend struct llvm::AnalysisInfoMixin<DefaultFeatureAnalysis>;   
  static llvm::AnalysisKey Key;

}; // end DefaultFeatureAnalysis

} // end namespace celerity
