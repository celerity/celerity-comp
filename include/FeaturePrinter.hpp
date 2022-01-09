#pragma once

#include <type_traits>

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
//using namespace celerity;


namespace celerity {

/// Pass that print the results of a FeatureAnalysis
template <typename AnalysisType> 
struct FeaturePrinterPass : public llvm::PassInfoMixin<celerity::FeaturePrinterPass<AnalysisType> > {
   static_assert(std::is_base_of<FeatureAnalysis, AnalysisType>::value, "AnalysisType must derive from FeatureAnalysis");

 public:
   explicit FeaturePrinterPass(llvm::raw_ostream &stream) : out_stream(stream) {}

   llvm::PreservedAnalyses run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam) {
      out_stream.changeColor(llvm::raw_null_ostream::Colors::MAGENTA);
      out_stream << "Print feature for function: " << fun.getName() << "\n";
      out_stream.changeColor(llvm::raw_null_ostream::Colors::YELLOW);

      ResultFeatureAnalysis &feature_set = fam.getResult<AnalysisType>(fun);    
      
      out_stream.changeColor(llvm::raw_null_ostream::Colors::WHITE, true);
      print_feature_names(feature_set.raw, out_stream);
      out_stream.changeColor(llvm::raw_null_ostream::Colors::WHITE, false);
      print_feature_values(feature_set.raw, out_stream);
      out_stream.changeColor(llvm::raw_null_ostream::Colors::WHITE, true);
      print_feature_names(feature_set.feat, out_stream);
      out_stream.changeColor(llvm::raw_null_ostream::Colors::WHITE, false);
      print_feature_values(feature_set.feat, out_stream);
      
      return PreservedAnalyses::all();
   }

   static bool isRequired() { return true; }

 private:
    llvm::raw_ostream &out_stream;
};

} // end namespace celerity
