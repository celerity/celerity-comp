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

/*
// Helper class to parse command line arguments.
class ArgumentParser
{
public:
    ArgumentParser(int &argc, char **argv)
    {
        for (int i = 1; i < argc; ++i)
        {
            tokens.push_back(std::string(argv[i]));
        }
    }
    string getCmdOption(const string &option) const
    {
        for (int i = 0; i < tokens.size(); i++)
        {
            if (tokens[i] == option)
                if (i + 1 < tokens.size())
                    return tokens[i + 1];
        }
        return std::string(); // empty string
    }

    bool cmdOptionExists(const string &option) const
    {
        for (string s : tokens)
        {
            if (s == option)
                return true;
        }
        return false;
    }

private:
    std::vector<string> tokens;
};
*/
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

    Expected<FeatureAnalysisParam> param = parseAnalysisArguments(argc, argv, true, false);
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
 
    //cl::PrintOptionValues();

    // Run!
    cout << "pre run" << endl;
    MPM.run(*module_ptr, MAM); 
    cout << "post run" << endl;

    exit(0);
    return 0;
} // end main
