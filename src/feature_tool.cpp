#include <vector>
#include <algorithm>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>

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

    const string& getCmdOption(const string &option) const{
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


const string usage = "Usage:\n\t-h help\n\t-i <kernel bitcode file>\n\t-o <output file>\n\t-fs <featureset={gpu|grewe|full}>\n\t-fe <featureeval={norm|grewe}>\n\t-v verbose\n";
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
    
    celerity::feature_set *fs;
    const string &feat_set_opt = input.getCmdOption("-fs");
    if (feat_set_opt.empty()){    
        // supported flags: {gpu|grewe|full}
        if(feat_set_opt =="grewe")
            fs = new celerity::grewe11_feature_set();
        else if(feat_set_opt =="full")
            fs = new celerity::full_feature_set();
        else // "gpu" is the default
            fs = new celerity::gpu_feature_set();
    }

    celerity::feature_eval *fe;
    const string &feat_eval_opt = input.getCmdOption("-fe");
    if (feat_eval_opt.empty()){
        // XXX to be supported flags: {normal|kofler|cr}
        if(feat_set_opt == "kofler")
            fe = new celerity::kofler13_eval(fs);
        //else if(feat_set_opt =="cr")
        //    fe = new celerity::costrelation_set(fs);
        else // "normal" is the default
            fe = new celerity::feature_eval(fs);
    }

    // for each function in the module
    auto &module = (*bcModule)->getFunctionList();
    for(Function &f : module){	
        fe->eval_function(f); 
    }
    fe->finalize(); // this includes normalization

    const string &outFile = input.getCmdOption("-o");
    if (outFile.empty()){
      fs->print_to_cout();
    }
    else {
      fs->print_to_file(outFile);
    }
    
    delete fe;
    delete fs;
    return 0;
} // end main

