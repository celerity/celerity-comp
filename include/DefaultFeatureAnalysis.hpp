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
struct DefaultFeatureAnalysis : public FeatureAnalysis {

    
 public:
    DefaultFeatureAnalysis(string feature_set = "fan19") { 
      analysis_name="default";
      features = FSRegistry::dispatch(feature_set);      
    }
    virtual ~DefaultFeatureAnalysis(){}

    //static llvm::AnalysisKey Key;

    /*
    /// runs the analysis on a specific function, returns a StringMap
    using Result = ResultFeatureAnalysis;
    ResultFeatureAnalysis run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);

    /// feature extraction for basic block
    virtual void extract(llvm::BasicBlock &bb);	
    /// feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
    /// apply feature postprocessing steps such as normalization
    virtual void finalize();

    //static bool isRequired() { return true; }
    */

}; // end FeatureAnalysis

} // end namespace celerity
