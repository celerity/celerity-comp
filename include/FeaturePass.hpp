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
    FeatureSetRegistry registered_feature_sets;
    FeatureSet * features = &registered_feature_sets["default"];
    // TODO normalization must handled in the same way
    // Normalization
    

 public:
    /// this methods allow to change the underlying feature set
    void setFeatureSet(string &featureSetName){
        features = &registered_feature_sets["default"];
    }

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


/// Pass that print the results of a FeatureExtractionPass
struct FeaturePrinterPass : public llvm::PassInfoMixin<FeaturePrinterPass> {
 public:
    explicit FeaturePrinterPass(llvm::raw_ostream &stream) : out_stream(stream) {}

    llvm::PreservedAnalyses run(llvm::Function &func, llvm::FunctionAnalysisManager &fam);

    static bool isRequired() { return true; }

 private:
    llvm::raw_ostream &out_stream;
};


/// An LLVM analysis pass to extract features using [Kofler et al., 13] loop heuristics.
/// The heuristic gives more important (x100) to the features inside a loop.
/// It requires the loop analysis pass ("loops") to be executed before of that pass.
struct Kofler13ExtractionPass : public FeatureExtractionPass {
 private:
    //function_ref<LoopInfo &(Function &)> LookupLoopInfo;
 public:
    //explicit Kofler13ExtractionPass(function_ref<LoopInfo &(Function &)> LookupLoopInfo) 
    //    : LookupLoopInfo(LookupLoopInfo) {}

    /// overwrite feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
};


} // end namespace celerity
