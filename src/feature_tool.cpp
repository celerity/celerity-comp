#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>


#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
//#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IRReader/IRReader.h>
//#include <llvm/Support/MemoryBuffer.h>
//#include <llvm/Support/SourceMgr.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
//#include <llvm/Analysis/LoopInfo.h> // InfoWrapperPass
//#include <llvm/Analysis/CallGraph.h>
//#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Passes/PassBuilder.h>

#include "FeatureSet.hpp"
#include "FeaturePass.hpp"

using namespace std;
using namespace llvm;
using namespace celerity;


llvm::Module *load_module(std::ifstream &stream);

// Helper class to parse command line arguments. 
class ArgumentParser {
public:
    ArgumentParser (int &argc, char **argv){
        for (int i=1; i < argc; ++i){
            tokens.push_back(std::string(argv[i]));            
        }
    }
    string getCmdOption(const string &option) const {
        for(int i=0; i<tokens.size(); i++){
            if(tokens[i] == option)
                if(i+1<tokens.size())
                    return tokens[i+1]; 
        }
        return std::string(); // empty string
        /*
        vector<string>::const_iterator itr;
        itr =  std::find(tokens.begin(), tokens.end(), option);
        if (itr != tokens.end() && ++itr != tokens.end()){
            return *itr;
        }
        const string empty = "";
        return empty;
        */
    }

    bool cmdOptionExists(const string &option) const{
        for(string s : tokens){
            if(s == option)
                return true;
        }
        return false;
        //return std::find(tokens.begin(), tokens.end(), option) != tokens.end();
    }
private:
    std::vector<string> tokens;
};

/// function to load a module from file
std::unique_ptr<Module> load_module(const std::string &fileName){
    LLVMContext context;
    SMDiagnostic error;
    cout << "loading...";
    std::unique_ptr<Module> module = llvm::parseIRFile(fileName, error, context);
    if(!module)  {
        std::string what = error.getMessage().str();       
        std::cerr << "error: " << what;
        exit(1);
    } // end if
    cout << "loading complete" << endl;
    return module;
}


string usage = "Celerity Feature Extractor\nUSAGE:\n\t-h help\n\t-i <kernel bitcode file>\n\t-o <output file>\n\t-fe <feature eval={default|kofler13|polfeat}>\n\t-v verbose\n";
bool verbose = false;
int optimization_level = 1;

// Standalone tool that extracts different features representations out of a LLVM-IR program. 
int main(int argc, char* argv[]) {
    FeatureSetRegistry &registered_fs = FeatureSetRegistry::getInstance();
    string fs_names = "\t-fs <feature set={";
    for(auto key : registered_fs.keys()){
        fs_names += key; 
        fs_names += "|";
    }
    fs_names[fs_names.size()-1] = '}';
    fs_names += ">\n";    
    usage += fs_names; 

    ArgumentParser input(argc, argv);
    if(input.cmdOptionExists("-h")) {
        cout << usage;  exit(0);
    }

    if(input.cmdOptionExists("-v")) { verbose = true; }

    const string &fileName = input.getCmdOption("-i");
    if (fileName.empty()){
        cout << usage << "Error: input filename not given\n";
        exit(1);
    }

    // set a feature extraction technique
    celerity::FeatureExtractionPass *fe;    
    string feat_eval_opt = input.getCmdOption("-fe");
    if(!input.cmdOptionExists("-fe") || feat_eval_opt.empty())
        feat_eval_opt = "default";                 
    if(feat_eval_opt == "kofler13") {
        fe = new celerity::Kofler13ExtractionPass();
    //else if(feat_eval_opt =="cr")
    //    fe = new celerity::costrelation_set(fs);
    } 
    else { // default
        fe = new celerity::FeatureExtractionPass();
    }
    
   // set a feature set       
    string feat_set_opt = input.getCmdOption("-fs");
    if (!feat_set_opt.empty()){         
        if(!registered_fs.count(feat_set_opt)){ // returns 1 if is in the map, 0 oterhwise
            feat_set_opt = "default";
        } 
        fe->setFeatureSet(feat_set_opt);
    }

    if(verbose) {
        cout << "feature-evaluation-technique: " << feat_eval_opt << endl;
        cout << "feature-set: " << fe->getFeatureSet()->getName() << endl;        
    }

    if(verbose) cout << "loading module from file" << endl;     
    std::unique_ptr<Module> module_ptr = load_module(fileName);

/*
    // New pass handling code, now aligned with the new Pass Manager:
    // 1. create pass manager and register our printing pass
    //ModulePassManager MPM;    
    // 2. create the analysis managers
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    // 2. new pass managere builder - this can be customized with debug and TargetMachine
    PassBuilder PB;
    // 3. use the default alias analysis pipeline (AAManager is a manager for alias)
    FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });
    // 4. register our analysis + basic analyses
    MAM.registerPass([&] { return fe; });
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
 */   
    // 5.  Create the pass manager for a typical -O1 optimization pipeline. 
    // TODO XXX support from O1 to O3
//    PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O1);
//    MPM.addPass(FeaturePrinterPass(llvm::errs()));
//    PassBuilder PB;
//    PB.registerPipelineStartEPCallback([&](ModulePassManager &MPM, PassBuilder::OptimizationLevel Level) {
//        MPM.addPass(FooPass());};
    // 6. add our function pass to the pass manager (new PM needs an adaptor)
    //MPM.addPass(createModuleToFunctionPassAdaptor(*fe));    
    
    // 7. and run it over the module
    //Module &module = module_ptr;
    cout << "1" << endl;

    ModuleAnalysisManager MAM;    
    ModulePassManager MPM;
    FunctionPassManager FPM;

    cout << "2" << endl;
    FPM.addPass(FeaturePrinterPass(llvm::outs()));
    
    cout << "3" << endl;
    MPM.addPass(createModuleToFunctionPassAdaptor(std::move(FPM)));  
    cout << "4" << endl;
    MPM.run(*module_ptr, MAM); // note: this also prints the features in cerr
    cout << "5" << endl;
    // final printing to either cout or file
    const string &outFile = input.getCmdOption("-o");
    if (outFile.empty()){       
        print_feature<float>(fe->getFeatureSet()->feat, llvm::outs());
    }

    return 0;
} // end main
