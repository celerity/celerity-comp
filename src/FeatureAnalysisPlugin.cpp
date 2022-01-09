#include <unordered_map>
#include <sstream>
#include <vector>
using namespace std;

#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/Scalar/IndVarSimplify.h>
#include <llvm/Transforms/Utils/LCSSA.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/CommandLine.h>
using namespace llvm;

#include "DefaultFeatureAnalysis.hpp"
#include "PolFeatAnalysis.hpp"
#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
using namespace celerity;

//-----------------------------------------------------------------------------
// Pass registration using the new LLVM PassManager
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getFeatureExtractionPassPluginInfo()
{
  return {
      LLVM_PLUGIN_API_VERSION, "FeatureAnalysis", LLVM_VERSION_STRING,
      [](PassBuilder &PB)
      {
        //outs() << "plugin pass registration \n";
        // #1 REGISTRATION FOR "opt -passes=print<feature>"
        // Register FeaturePrinterPass so that it can be used when specifying pass pipelines with `-passes=`.
        PB.registerPipelineParsingCallback(
            [&](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>)
            {
              // outs() << " * plugin input " << Name << "\n";
              if (Name == "print<feature>")
              {
                FPM.addPass(FeaturePrinterPass<DefaultFeatureAnalysis>(llvm::outs())); 
                FPM.addPass(LCSSAPass());                
                FPM.addPass(FeaturePrinterPass<Kofler13Analysis>(llvm::outs())); // 
                return true;
              }
              return false;
            });
        // #2 REGISTRATION FOR "-O{1|2|3|s}"
        // Register FeaturePrinterPass as a step of an existing pipeline.
        PB.registerVectorizerStartEPCallback(
            [](llvm::FunctionPassManager &PM, llvm::PassBuilder::OptimizationLevel Level)
            {
              PM.addPass(FeaturePrinterPass<DefaultFeatureAnalysis>(llvm::outs()));
              PM.addPass(LCSSAPass());                
              PM.addPass(FeaturePrinterPass<Kofler13Analysis>(llvm::outs()));
            });
        // #3 REGISTRATION FOR "FAM.getResult<FeatureAnalysis>(Func)"
        // Register FeatureAnalysis as an analysis pass, so that FeaturePrinterPass can request the results of FeatureAnalysis.
        PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM)
            {
              FAM.registerPass([&] { return DefaultFeatureAnalysis("grewe11"); });              
              FAM.registerPass([&] { return Kofler13Analysis(); });
              FAM.registerPass([&] { return PolFeatAnalysis(); });
            });
      }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return getFeatureExtractionPassPluginInfo();
}
