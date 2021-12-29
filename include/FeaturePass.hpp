#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include "FeatureSet.hpp"

using namespace llvm;

namespace celerity {

/*
/// List of supported feature extraction techniques 
enum class feature_pass_mode { 
    DEFAULT,     // Instruction features of each BB are summed up for the program.
    KOFLER13,   // Instruction features of instructions inside loops have a larger contribution.
    POLFEAT     // Instruction features are progated as polynomial cost relations . More accurate, but requires runtime evaluation.
};
*/
using ResultFeatureExtraction = llvm::StringMap<float>;

/// An LLVM analysisfunction pass that extract static code features. 
/// The extraction of features from a single instruction is delegated to a feature set class.
/// In this basic implementation, BB's instruction contributions are summed up.
struct FeatureExtractionPass : public llvm::AnalysisInfoMixin<FeatureExtractionPass> {

 protected:
    FeatureSet * features;
    // TODO normalization must handled here in the samo way (Normalization)
    
 public:
    FeatureExtractionPass(){
      FeatureSetRegistry &registry = FeatureSetRegistry::getInstance();
      features = registry["default"];
    }
    virtual ~FeatureExtractionPass(){}

    /// this methods allow to change the underlying feature set
    void setFeatureSet(string &featureSetName){
        FeatureSetRegistry &registry = FeatureSetRegistry::getInstance();
        features = registry[featureSetName];
    }
    FeatureSet * getFeatureSet(){ return features; }

    /// runs the analysis on a specific function, returns a StringMap
    using Result = ResultFeatureExtraction;
    ResultFeatureExtraction run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);

    /// feature extraction for basic block
    virtual void extract(llvm::BasicBlock &bb);	
    /// feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
    /// apply feature postprocessing steps such as normalization
    virtual void finalize();

    static bool isRequired() { return true; }
 
  private:
    static llvm::AnalysisKey Key;
    friend struct llvm::AnalysisInfoMixin<FeatureExtractionPass>;

};




/// An LLVM analysis pass to extract features using [Kofler et al., 13] loop heuristics.
/// The heuristic gives more important (x100) to the features inside a loop.
/// It requires the loop analysis pass ("loops") to be executed before of that pass.
struct Kofler13ExtractionPass : public FeatureExtractionPass {
 private:
   const int default_loop_contribution = 100;

 public:
    Kofler13ExtractionPass(){}
    virtual ~Kofler13ExtractionPass(){}

    /// overwrite feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
    // calculate del loop contribution of a given loop (assume non nesting, which is calculated later)
    int loopContribution(const llvm::Loop &loop, ScalarEvolution &SE);
};


} // end namespace celerity
