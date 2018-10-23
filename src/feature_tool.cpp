#include <vector>
#include <algorithm>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/LoopInfo.h> // InfoWrapperPass

#include "feature_set.h"
#include "feature_eval.h"

using namespace std;
using namespace llvm;


/* Helper class to parse command line arguments. */
class ArgumentParser {
public:
    ArgumentParser (int &argc, char **argv){
        for (int i=1; i < argc; ++i)
            this->tokens.push_back(string(argv[i]));
    }

    string getCmdOption(const string &option) const {
        vector<string>::const_iterator itr;
        itr =  find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()){
            return *itr;
        }
        static const string empty = "";
        return empty;
    }

    bool cmdOptionExists(const string &option) const{
        return find(this->tokens.begin(), this->tokens.end(), option)
               != this->tokens.end();
    }
private:
    vector <string> tokens;
};


const string usage = "Usage:\n\t-h help\n\t-i <kernel bitcode file>\n\t-o <output file>\n\t-fs <featureset={gpu|grewe|full}>\n\t-fe <featureeval={norm|kofler|cr}>\n\t-v verbose\n";
bool verbose = false;


/* Standalone tool that extracts different features representation out of a LLVM-IR program. */
int main(int argc, char* argv[]) {

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
        exit(1);
    }

    // Getting LLVM MemoryBuffer and Module
    ErrorOr<unique_ptr<MemoryBuffer>> fileBuffer = MemoryBuffer::getFile(fileName);     
    if (error_code ec = fileBuffer.getError()){
        cerr << "ERROR loading the bitcode file: " << fileName << endl << "Error message: " << ec.message() << endl;
        exit(1);
    }
    else {
	if(verbose) cout << "Bitcode file loaded: " << fileName << endl;
    }

    LLVMContext context;
    MemoryBufferRef memRef = (*fileBuffer)->getMemBufferRef();
    Expected<unique_ptr<Module>> bcModule = parseBitcodeFile(memRef, context);
/*  XXX fixme add error handling
    if(!bcModule()){ // if there is an error
      Error err = bcModule.takeError(); // ?
      cerr << "ERROR loading the module from the bitcode" << endl << "Error message: " << err << endl; 
      exit(1);
    }
*/ 

    // define a feature set   
    celerity::feature_set *fs;
    string feat_set_opt = input.getCmdOption("-fs");
    if (!feat_set_opt.empty()){  // supported flags: {gpu|grewe|full}        
        if(feat_set_opt =="grewe") 
            fs = new celerity::grewe11_feature_set();
        else if(feat_set_opt =="full")
            fs = new celerity::full_feature_set();
        else if(feat_set_opt =="gpu")
            fs = new celerity::gpu_feature_set();
    }
    else {
        // default, no command line flag
        fs = new celerity::gpu_feature_set();
        feat_set_opt = "gpu";
    }

    // define a feature evaluation technique
    celerity::feature_eval *fe;
    string feat_eval_opt = input.getCmdOption("-fe");
    if (!feat_eval_opt.empty()){ // XXX to be supported flags: {normal|kofler|cr}
        if(feat_eval_opt == "kofler")
            fe = new celerity::kofler13_eval(fs);
        //else if(feat_eval_opt =="cr")
        //    fe = new celerity::costrelation_set(fs);
        else if(feat_eval_opt == "normal")
            fe = new celerity::feature_eval(fs);
    }
    else {
        // default, no command line flag
        fe = new celerity::feature_eval(fs);
        feat_eval_opt = "normal";
    }

    cout << "feature-set: " << feat_set_opt << ", feature-evaluation-technique: " << feat_eval_opt << endl;

    // We build a pass manager that load our pass and dependent passes 
    // (e.g., LoopInfoWrapperPass si required by kofler13_eval)    
    legacy::PassManager manager;    
    llvm::Pass *loop_analysis = new llvm::LoopInfoWrapperPass();
    manager.add(loop_analysis);
    manager.add(fe);
    Module &module = *(*bcModule);
    manager.run(module); // note: this also prints the features in cerr

    // final printing to either cout or file
    const string &outFile = input.getCmdOption("-o");
    if (outFile.empty()){
      // do nothing
      // fs->print_to_cout(); 
    }
    else {
      fs->print_to_file(outFile);
    }    

    //the pass manager does these two deallocations:
    //delete fe; delete loop_analysis;
    delete fs;
    return 0;
} // end main

