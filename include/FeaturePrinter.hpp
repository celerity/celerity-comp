#pragma once

#include <type_traits>

//#include <llvm/Analysis/LoopInfo.h>
//#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
//#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
//#include <llvm/Pass.h>
//#include <llvm/Passes/PassBuilder.h>
//#include <llvm/Passes/PassPlugin.h>
//#include <llvm/Transforms/IPO/PassManagerBuilder.h>
using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
//using namespace celerity;


namespace celerity {

/// Pass that print the results of a FeatureAnalysis
template <typename AnalysisType> //  = FeatureAnalysis
struct FeaturePrinterPass : public llvm::PassInfoMixin<celerity::FeaturePrinterPass<AnalysisType> > {
   static_assert(std::is_base_of<FeatureAnalysis, AnalysisType>::value, "AnalysisType must derive from FeatureAnalysis");

 public:
   explicit FeaturePrinterPass(llvm::raw_ostream &stream) : out_stream(stream) {}

   llvm::PreservedAnalyses run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam) {
      out_stream << "Function: " << fun.getName() << "\n";    
      auto &feature_set = fam.getResult<AnalysisType>(fun);    
      print_features(feature_set, out_stream);
      return PreservedAnalyses::all();
   }

   static bool isRequired() { return true; }

 private:
    llvm::raw_ostream &out_stream;
};

} // end namespace celerity
