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
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/CommandLine.h>
using namespace llvm;

#include "DefaultFeatureAnalysis.hpp"
#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
//#include "FeatureNormalization.hpp"
using namespace celerity;


llvm::AnalysisKey DefaultFeatureAnalysis::Key;

//-----------------------------------------------------------------------------
// Register the analysis in a FeatureAnalysis registry
//-----------------------------------------------------------------------------
static celerity::FeatureAnalysis* _static_dfa_ptr_ = new celerity::DefaultFeatureAnalysis; // dynamic_cast<celerity::FeatureAnalysis*>(&_static_fa_);
static bool _registered_feature_analysis_ = FARegistry::registerByKey("default", _static_dfa_ptr_ ); 

//-----------------------------------------------------------------------------
// Pass registration using the new LLVM PassManager
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getFeatureExtractionPassPluginInfo()
{
  return {
      LLVM_PLUGIN_API_VERSION, "FeatureAnalysis", LLVM_VERSION_STRING,
      [](PassBuilder &PB)
      {
        outs() << "plugin pass registration \n";
        // #1 REGISTRATION FOR "opt -passes=print<feature>"
        // Register FeaturePrinterPass so that it can be used when specifying pass pipelines with `-passes=`.
        PB.registerPipelineParsingCallback(
            [&](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>)
            {
              // outs() << " * plugin input " << Name << "\n";
              if (Name == "print<feature>")
              {
                //outs() << "   * FeaturePrinterPass registration for opt - " << Name << " *\n";
                // first we nned to make our loop canonical              
                FPM.addPass(LoopSimplifyPass());                
                // then we print all features
                FPM.addPass(FeaturePrinterPass<DefaultFeatureAnalysis>(llvm::outs()));
                FPM.addPass(FeaturePrinterPass<Kofler13Analysis>(llvm::outs()));
                return true;
              }
              return false;
            });
        // #2 REGISTRATION FOR "-O{1|2|3|s}"
        // Register FeaturePrinterPass as a step of an existing pipeline.
        PB.registerVectorizerStartEPCallback(
            [](llvm::FunctionPassManager &PM, llvm::PassBuilder::OptimizationLevel Level)
            {
              //outs() << "   * FeaturePrinterPass registration as a step of an existing pipeline *\n";
              // first we nned to make our loop canonical              
              PM.addPass(LoopSimplifyPass());
              PM.addPass(createFunctionToLoopPassAdaptor(IndVarSimplifyPass()));              
              // then we print all features
              PM.addPass(FeaturePrinterPass<DefaultFeatureAnalysis>(llvm::outs()));
              PM.addPass(FeaturePrinterPass<Kofler13Analysis>(llvm::outs()));
            });
        // #3 REGISTRATION FOR "FAM.getResult<FeatureAnalysis>(Func)"
        // Register FeatureAnalysis as an analysis pass, so that FeaturePrinterPass can request the results of FeatureAnalysis.
        PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM)
            {
              //outs() << "   * analysis registration *\n";
              //FAM.registerPass([&] { return ScalarEvolution(); });
              FAM.registerPass([&] { return DefaultFeatureAnalysis(); });
              FAM.registerPass([&] { return Kofler13Analysis(); });
            });

      }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return getFeatureExtractionPassPluginInfo();
}
