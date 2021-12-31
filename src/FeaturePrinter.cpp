#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
using namespace llvm;

#include "FeatureAnalysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;

llvm::PreservedAnalyses FeaturePrinterPass::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
    out_stream << "Function: " << fun.getName() << "\n";    
    auto &feature_set = fam.getResult<FeatureAnalysis>(fun);    
    print_features(feature_set, out_stream);
    return PreservedAnalyses::all();
}
