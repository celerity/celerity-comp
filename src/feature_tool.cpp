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
using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureAnalysis.hpp"
#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
using namespace celerity;



/// utility for parsing command line and plugin params
//llvm::Expected<FeatureAnalysisParam> parseAnalysisArguments(std::string &arguments, bool printErrors, bool passNameCheck);
//llvm::Expected<FeatureAnalysisParam> parseAnalysisArguments(int argc, char **argv,  bool printErrors, bool passNameCheck);
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


/*
Expected<FeatureAnalysisParam> parseAnalysisArguments(string &arguments, bool printErrors, bool passNameCheck)
{
  // string to argv and argc
  bool first_space = true;
  bool first_char = true;
  std::vector<unsigned> argv_index;
  string argv_copy = arguments + " ";
  // from string to argv and argc
  for (int i = 0; i < argv_copy.size(); i++)
  {
    char &current_char = argv_copy[i];
    if (current_char == ' ')
    {
      if (first_space)
        current_char = '\n';
      first_space = false;
      first_char = true;
    } // not a space
    else
    {
      if (first_char)
        argv_index.push_back(i);
      first_char = false;
      first_space = true;
    }
  }
  char str[1024];
  strncpy(str, argv_copy.c_str(), 1024);
  int argc = argv_index.size();
  char *argv[1024];
  for (int i = 0; i < argc; i++)
    argv[i] = str + argv_index[i];
  return parseAnalysisArguments(argc, argv, printErrors, passNameCheck);
}
*/

Expected<FeatureAnalysisParam> parseAnalysisArguments(int argc, char **argv, bool printErrors)
{
  // LLVM command line parser
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
std::unique_ptr<Module> load_module(LLVMContext &context, const std::string &fileName) {
    SMDiagnostic error;
    cout << "loading...";
    std::unique_ptr<Module> module = llvm::parseIRFile(fileName, error, context);
    if (!module)
    {
        std::string what = error.getMessage().str();
        std::cerr << "error: " << what;
        exit(1);
    } // end if
    cout << " complete" << endl;
    return module;
}

/*
string usage = "Celerity Feature Extractor\nUSAGE:\n\t-h help\n\t-i <kernel bitcode file>\n\t-o <output file>\n\t-fe <feature eval={default|kofler13|polfeat}>\n\t-v verbose\n";
*/
bool verbose = false;
int optimization_level = 1;

// Standalone tool that extracts different features representations out of a LLVM-IR program.
int main(int argc, char *argv[]) {
    InitLLVM X(argc, argv);
    LLVMContext context;

    Expected<FeatureAnalysisParam> param = parseAnalysisArguments(argc, argv, true);
    if(!param){
        cerr << "params not set\n";
        exit(0);
    }

/*
    FeatureSetRegistry &registered_fs = FeatureSetRegistry::getInstance();
    string fs_names = "\t-fs <feature set={";
    for (auto key : registered_fs.keys())
    {
        fs_names += key;
        fs_names += "|";
    }
    fs_names[fs_names.size() - 1] = '}';
    fs_names += ">\n";
    usage += fs_names;

    ArgumentParser input(argc, argv);

    if (input.cmdOptionExists("-h"))
    {
        cout << usage;
        exit(0);
    }

    if (input.cmdOptionExists("-v"))
    {
        verbose = true;
    }

    const string &fileName = input.getCmdOption("-i");
    if (fileName.empty())
    {
        cout << usage << "Error: input filename not given\n";
        exit(1);
    }

    // set a feature extraction technique
    celerity::FeatureAnalysis *fe;
    string feat_eval_opt = input.getCmdOption("-fe");
    if (!input.cmdOptionExists("-fe") || feat_eval_opt.empty())
        feat_eval_opt = "default";
    if (feat_eval_opt == "kofler13")
    {
        fe = new celerity::Kofler13Analysis();
        // else if(feat_eval_opt =="cr")
        //     fe = new celerity::costrelation_set(fs);
    }
    else
    { // default
        fe = new celerity::FeatureAnalysis();
    }

    // set a feature set
    string feat_set_opt = input.getCmdOption("-fs");
    if (!feat_set_opt.empty())
    {
        if (!registered_fs.count(feat_set_opt))
        { // returns 1 if is in the map, 0 oterhwise
            feat_set_opt = "default";
        }
        fe->setFeatureSet(feat_set_opt);
    }
*/


    /*
    if (param->verbose)
    {
        cout << "feature-evaluation-technique: " << feat_eval_opt << endl;
        cout << "feature-set: " << fe->getFeatureSet()->getName() << endl;
    }
    */
    
    if (param->verbose)
        cout << "loading module from file" << endl;
    
    std::unique_ptr<Module> module_ptr = load_module(context, param->filename);
    

    // Pass management with the new pass pipeline
    PassInstrumentationCallbacks PIC;
    StandardInstrumentations SI(true, false);
    SI.registerCallbacks(PIC);  
    PassBuilder PB(false,nullptr,llvm::PipelineTuningOptions(),llvm::None, &PIC);    
    Expected<PassPlugin> PassPlugin = PassPlugin::Load("./libfeature_pass.so");
    if (!PassPlugin) {
      errs() << "Failed to load passes from libfeature_pass.so plugin\n";
      errs() << "Problem with division " << toString(std::move(PassPlugin.takeError())) ;
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
    cout << "pre run" << endl;
    MPM.run(*module_ptr, MAM); 
    cout << "post run" << endl;

    exit(0);
    return 0;
} // end main
