#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
using namespace llvm;

#include "FeatureSet.hpp"
#include "PolFeatAnalysis.hpp"

//#include "FeatureAnalysis.hpp"
//#include "FeaturePrinter.hpp"
//#include "FeatureNormalization.hpp"

namespace celerity {


// Pass printing the results of a polynomial feature analysis. 
struct PolFeatPrinterPass : public llvm::PassInfoMixin<PolFeatPrinterPass> {

 public:
   explicit PolFeatPrinterPass(llvm::raw_ostream &stream) : out_stream(stream) {}

   llvm::PreservedAnalyses run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam) {
      out_stream.changeColor(llvm::raw_null_ostream::Colors::MAGENTA);
      out_stream << "Print polynomial features for function: " << fun.getName() << "\n";
      out_stream.changeColor(llvm::raw_null_ostream::Colors::YELLOW);

      ResultPolFeatSet &feature_set = fam.getResult<PolFeatAnalysis>(fun);    
      
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


} // celerity
