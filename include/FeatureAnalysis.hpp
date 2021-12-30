#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include "FeatureSet.hpp"

using namespace llvm;

namespace celerity {

using ResultFeatureAnalysis = llvm::StringMap<float>;

/// An LLVM analysisfunction pass that extract static code features. 
/// The extraction of features from a single instruction is delegated to a feature set class.
/// In this basic implementation, BB's instruction contributions are summed up.
struct FeatureAnalysis : public llvm::AnalysisInfoMixin<FeatureAnalysis> {

 protected:
    FeatureSet * features;
    // TODO normalization must handled here in the samo way (Normalization)
    
 public:
    FeatureAnalysis(){
      FeatureSetRegistry &registry = FeatureSetRegistry::getInstance();
      features = registry["default"];
    }
    virtual ~FeatureAnalysis();

    /// this methods allow to change the underlying feature set
    void setFeatureSet(string &featureSetName){
        FeatureSetRegistry &registry = FeatureSetRegistry::getInstance();
        features = registry[featureSetName];
    }
    FeatureSet * getFeatureSet(){ return features; }

    /// runs the analysis on a specific function, returns a StringMap
    using Result = ResultFeatureAnalysis;
    ResultFeatureAnalysis run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);

    /// feature extraction for basic block
    virtual void extract(llvm::BasicBlock &bb);	
    /// feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
    /// apply feature postprocessing steps such as normalization
    virtual void finalize();

    static bool isRequired() { return true; }
 
  private:
    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<FeatureAnalysis>;

};

} // end namespace celerity
