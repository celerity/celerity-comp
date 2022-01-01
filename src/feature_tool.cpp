#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
using namespace std;

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IRReader/IRReader.h>
#include "llvm/Analysis/AliasAnalysis.h"
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/CodeGen/CommandFlags.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/InitLLVM.h>
//#include <llvm/Support/DynamicLibrary.h>
using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"
#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
using namespace celerity;


//-----------------------------------------------------------------------------
// Command line parsing
//-----------------------------------------------------------------------------

// fset={...} supported feature sets
cl::opt<FeatureSetOptions> FSet("fset", cl::desc("Specify the feature set:"),
                                cl::values(
                                    clEnumVal(fan19, "Default feature set for GPU used in [Fan et al. ICPP 19]"),
                                    clEnumVal(grewe13, "Feature set used in pGrewe et al. 13]"),
                                    clEnumVal(full, "Feature set mapping all LLVM IR opcode (very large, hard to cover)")));
// fanal={...} supported feature analyses
cl::opt<string> FAnal("fanal", cl::desc("Specify the feature analysis algorithm"), cl::value_desc("feature_analysis"), cl::init("default"));
// fnorm={...} supported normalization
cl::opt<string> FNorm("fnorm", cl::desc("Specify the feature normalization algorithm"), cl::value_desc("feature_norm"), cl::init("default"));
// in case of standalone tool (no opt), we need a positional param for the input IR file
cl::opt<string> IRFilename(cl::Positional, cl::desc("<input_bitcode_file>"), cl::Required);
// help
//cl::opt<bool> Help("h", cl::desc("Enable binary output on terminals"), cl::init(false));
// verbose
cl::opt<bool> Verbose("v", cl::desc("Verbose"), cl::init(false));

Expected<FeatureAnalysisParam> parseAnalysisArguments(int argc, char **argv, bool printErrors)
{
  // LLVM command line parser
  string descr_list = "Specify the feature analysis algorithm. Supported: ";
  for(StringRef l : FARegistry::getKeyList() ) {
    descr_list += " "; descr_list += l;
  }
  FAnal.setDescription(descr_list);
  cl::ParseCommandLineOptions(argc, argv);

  // if we are using the extractor tool, we need the input file
  FeatureAnalysisParam param = {FeatureSetOptions::fan19, "default", "no-norm", "", false, false};
  param.feature_set = FSet;
  param.analysis = FAnal;
  param.normalization = FNorm;
  param.filename = IRFilename;
  //param.help = Help;
  param.verbose = Verbose;
  return param;
}

/// function to load a module from file
std::unique_ptr<Module> load_module(LLVMContext &context, const std::string &fileName, bool verbose) {
    SMDiagnostic error;
    if (verbose)
        cout << "loading module from file" << fileName << endl;
    
    std::unique_ptr<Module> module = llvm::parseIRFile(fileName, error, context);
    if (!module)
    {
        std::string what = error.getMessage().str();
        std::cerr << "error: " << what;
        exit(1);
    } // end if
    if (verbose) {
        cout << "loading complete"  << endl;
        cout << " - name " << module->getName().str() << endl;
        cout << " - number of functions" << module->getFunctionList().size() << endl;
        cout << " - instruction count #" << module->getInstructionCount() << endl;
        
    }
    return module;
}


// Standalone tool that extracts different features representations out of a LLVM-IR program.
int main(int argc, char *argv[]) {
    InitLLVM X(argc, argv);
    LLVMContext context;

    Expected<FeatureAnalysisParam> param = parseAnalysisArguments(argc, argv, true);
    if(!param){
        cerr << "params not set\n";
        exit(0);
    }

    // Module loading
    std::unique_ptr<Module> module_ptr = load_module(context, param->filename, param->verbose);
    
    //DynamicLibrary::

    // Pass management with the new pass pipeline
    PassInstrumentationCallbacks PIC;
    StandardInstrumentations SI(true, false);
    SI.registerCallbacks(PIC);  
    PassBuilder PB(false, nullptr, llvm::PipelineTuningOptions(), llvm::None, &PIC);    
    Expected<PassPlugin> PassPlugin = PassPlugin::Load("./libfeature_pass.so");
    if (!PassPlugin) {
      errs() << "Failed to load passes from libfeature_pass.so plugin\n";
      errs() << "Problem while loading libfeature_pass: " << toString(std::move(PassPlugin.takeError())) ;
      errs() << "\n";
      return 1;
    }
    PassPlugin->registerPassBuilderCallbacks(PB);

    AAManager AA;
    LoopAnalysisManager LAM(true);
    FunctionAnalysisManager FAM(true);
    CGSCCAnalysisManager CGAM(true);
    ModuleAnalysisManager MAM(true);
    // Register the AA manager first so that our version is the one used.
    FAM.registerPass([&] { return std::move(AA); });
    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    ModulePassManager MPM(true);
 
    // Run!
    if(param->verbose) cout << "Pass manager run.." << endl;
    MPM.run(*module_ptr, MAM); 
    if(param->verbose) cout << "Pass manager run completed" << endl;

    return 0;
} // end main
