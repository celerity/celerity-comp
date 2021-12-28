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
//#include <llvm/Analysis/LoopAnalysisManager.h>
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
            tokens.push_back(string(argv[i]));            
        }
    }
    string getCmdOption(const string &option) const {
        vector<string>::const_iterator itr;
        itr =  std::find(tokens.begin(), tokens.end(), option);
        if (itr != tokens.end() && ++itr != tokens.end()){
            return *itr;
        }
        const string empty = "";
        return empty;
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
    vector<string> tokens;
};

/// function to load a module from file
std::unique_ptr<Module> load_module(const std::string &fileName){
    LLVMContext context;
    SMDiagnostic error;
    std::unique_ptr<Module> module = llvm::parseIRFile(fileName, error, context);
    if(!module)  {
        std::string what;
        llvm::raw_string_ostream os(what);
        error.print("error after ParseIR()", os);
        std::cerr << what;
        exit(1);
    } // end if
    return module;
}


const string usage = "Celerity Feature Extractor\nUSAGE:\n\t-h help\n\t-i <kernel bitcode file>\n\t-o <output file>\n\t-fs <featureset={fan|grewe|full}>\n\t-fe <featureeval={default|kofler|polfeat}>\n\t-v verbose\n";
bool verbose = false;
int optimization_level = 1;

// Standalone tool that extracts different features representations out of a LLVM-IR program. 
int main(int argc, char* argv[]) {
    celerity::FeatureSetRegistry registered_fs; // XXX TODO this should be a singleton

    ArgumentParser input(argc, argv);
    if(input.cmdOptionExists("-h")) {
        cout << usage;
        exit(0);
    }
    if(input.cmdOptionExists("-v")) {
        verbose = true;
    }
    const string &fileName = input.getCmdOption("-i");
    if (fileName.empty()){
        cout << usage;
        cout << "Error: input filename not given";
        exit(1);
    }


    // define a feature extraction technique
    celerity::FeatureExtractionPass *fe;
    string feat_eval_opt = input.getCmdOption("-fe");
    if (!feat_eval_opt.empty()){                
        // XXX TODO to be supported flags: {default|kofler|polfeat}
        if(feat_eval_opt == "kofler13") {
            fe = new celerity::Kofler13ExtractionPass();
        //else if(feat_eval_opt =="cr")
        //    fe = new celerity::costrelation_set(fs);
        } else //if(feat_eval_opt == "default") 
        {
            fe = new celerity::FeatureExtractionPass();
        }
    }


   // define a feature set   
    celerity::FeatureSet *fs;
    string feat_set_opt = input.getCmdOption("-fs");
    if (!feat_set_opt.empty()){  // supported flags: {gpu|grewe|full}        
        if(registered_fs.count(feat_set_opt)){ // returns 1 if is in the map, 0 oterhwise
            fs = registered_fs[feat_set_opt];
        } 
        else {
            fs = registered_fs["default"];
        }
        fe->setFeatureSet(feat_set_opt);
    }


    if(verbose) cout << "feature-set: " << feat_set_opt << ", feature-evaluation-technique: " << feat_eval_opt << endl;

    if(verbose) cout << "loading module from file" << endl; 
    
    //std::ifstream stream("hello.bc", std::ios_base::binary);
    std::unique_ptr<Module> module_ptr = load_module(fileName);

    // New pass handling code, now aligned with the new Pass Manager:
    // 1. create the analysis managers
    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;
    // 2. new pass managere builder - this can be customized with debug and TargetMachine
    PassBuilder PB;
    // 3. use the default alias analysis pipeline (AAManager is a manager for alias)
    FAM.registerPass([&] { return PB.buildDefaultAAPipeline(); });
    // 4. register basic analyses
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    // 5.  Create the pass manager for a typical -O1 optimization pipeline. 
    // TODO XXX support from O1 to O3
    ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::PassBuilder::OptimizationLevel::O1);
    /*
   
    // We build a pass manager that load our pass and dependent passes 
    // (e.g., LoopInfoWrapperPass is required by kofler13_eval)    
    legacy::PassManager manager;    
    llvm::Pass *loop_analysis = new llvm::LoopInfoWrapperPass();
    manager.add(loop_analysis);
    llvm::Pass *call_graph_wrapper_pass = new llvm::CallGraphWrapperPass();
    manager.add(call_graph_wrapper_pass);
    */
   /*
    if(feat_eval_opt == "kofler13") {
        llvm::Pass *scev = new llvm::ScalarEvolutionWrapperPass();
        manager.add(scev);
    }
    */
    // 6. add our pass to the manager
    MPM.addPass(fe);    
    // 7. and run it over the module
    //Module &module = module_ptr;
    MPM.run(*module_ptr, MAM); // note: this also prints the features in cerr

    // final printing to either cout or file
    const string &outFile = input.getCmdOption("-o");
    if (outFile.empty()){       
        print_feature<float>(fe->getFeatureSet()->feat, llvm::outs());
    }
    else {
        // fs->print_to_file(outFile);
    }    

    //the pass manager does these two deallocations:
    delete fe; // delete loop_analysis;
    //delete fs;
    return 0;
} // end main
