#include <unordered_map>
#include <sstream>
#include <vector>
using namespace std;

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/CommandLine.h>
using namespace llvm;

#include "FeatureAnalysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;

llvm::AnalysisKey FeatureAnalysis::Key;

FeatureAnalysis::~FeatureAnalysis() {}

void FeatureAnalysis::extract(BasicBlock &bb)
{
  for (Instruction &i : bb)
  {
    features->eval(i);
  }
}

void FeatureAnalysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  for (llvm::BasicBlock &bb : fun)
    extract(bb);
}

void FeatureAnalysis::finalize()
{
  normalize(*features);
}

FeatureAnalysis::Result FeatureAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  std::cout << "analysis for function: " << fun.getName().str() << "\n";
  std::cout << "feature-set: " << features->getName() << "\n";
  extract(fun, fam);
  finalize();
  return features->getFeatureValues();
}

/*
bool FeaturePass::runOnModule(Module& m) {
    CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    // Walk the callgraph in bottom-up SCC order.
    scc_iterator<CallGraph*> CGI = scc_begin(&CG);

    CallGraphSCC CurSCC(CG, &CGI);
    while (!CGI.isAtEnd()) {
        // Copy the current SCC and increment past it so that the pass can hack
        // on the SCC if it wants to without invalidating our iterator.
        const std::vector<CallGraphNode *> &NodeVec = *CGI;
        CurSCC.initialize(NodeVec);
        runOnSCC(CurSCC);
        ++CGI;
    }

    return false;
}

bool FeaturePass::runOnSCC(CallGraphSCC &SCC) {
    for (auto &cgnode : SCC) {
        Function *func = cgnode->getFunction();
        if (func) {
            //cout << "eval function: " << (func->hasName() ? func->getName().str() : "anonymous") << "\n";
            eval_function(*func);
          finalize();
            //features->print();
        }
    }
    return false;
}
*/

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



Expected<FeatureAnalysisParam> celerity::parseAnalysisArguments(string &arguments, bool printErrors, bool passNameCheck)
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


Expected<FeatureAnalysisParam> celerity::parseAnalysisArguments(int argc, char **argv, bool printErrors, bool passNameCheck)
{
  // if a pass name is provided, we are using the plugin from opt and we should check the pass name "print<feature>"
  if (passNameCheck)
  {
    string first = string(argv[0]);
    if (first != "print<feature>")
      return createStringError(inconvertibleErrorCode(), "warning: feature printer pass not specified (expected \"print<feature>\")");
  }

  // LLVM command line parser
  cl::ResetCommandLineParser();
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
              outs() << " * plugin input " << Name << "\n";
              if (Name == "print<feature>")
              {
                outs() << "   * FeaturePrinterPass registration - " << Name << "\n";
                FPM.addPass(FeaturePrinterPass(llvm::errs()));
                return true;
              }
              return false;
            });
        // #2 REGISTRATION FOR "-O{1|2|3|s}"
        // Register FeaturePrinterPass as a step of an existing pipeline.
        PB.registerVectorizerStartEPCallback(
            [](llvm::FunctionPassManager &PM, llvm::PassBuilder::OptimizationLevel Level)
            {
              PM.addPass(FeaturePrinterPass(llvm::errs()));
            });
        // #3 REGISTRATION FOR "FAM.getResult<FeatureAnalysis>(Func)"
        // Register FeatureAnalysis as an analysis pass, so that FeaturePrinterPass can request the results of FeatureAnalysis.
        PB.registerAnalysisRegistrationCallback(
            [](FunctionAnalysisManager &FAM)
            {
              FAM.registerPass([&]
                               { return FeatureAnalysis(); });
            });
      }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo()
{
  return getFeatureExtractionPassPluginInfo();
}
