#include <vector>
#include <algorithm>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LLVMContext.h>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Analysis/LoopInfo.h> // InfoWrapperPass
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/ScalarEvolution.h>

#include "crel_feature_set.h"
#include "crel_feature_pass.h"


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


// Global variables
const string usage = "Usage:\n\t-h help\n\t-i <kernel cl file>\n\t-o <output file>\n\t-v verbose\n";
bool verbose = false;
std::string outFile;


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
    const string &clFileName = input.getCmdOption("-i");
    if (clFileName.empty()){
        cout << usage;
        exit(1);
    }

    outFile = input.getCmdOption("-o");
    if (outFile.empty()){
        // do nothing
        // fs->print_to_cout();
    }
    else {
        outFile = "output.cl";
    }

    // Parse .cl MemoryBuffer
    ErrorOr<unique_ptr<MemoryBuffer>> clFileBuffer = MemoryBuffer::getFile(clFileName);
    if (error_code ec = clFileBuffer.getError()){
        cerr << "ERROR loading the cl file: " << clFileName << endl << "Error message: " << ec.message() << endl;
        exit(1);
    }
    else {
	    if(verbose) cout << "kernel cl file loaded: " << clFileName << endl;
    }


    // Compile .cl to LLVM IR
    // This assumes clang++ exist
    std::string opts_FileName = clFileName + "opts.txt";
    std::string noopt_bcFileName = clFileName + ".bc";
    std::string noopt_llFileName = clFileName + ".ll";
    std::string bcFileName = clFileName + "_opt.bc"; // optimised file
    std::string llFileName = clFileName + "_opt.ll";

    // Assumes clang exists in PATH and points to latest eg: clang10
    // -O0 : doesn't inline non-kernel functions which is required by our tool
    // -O3: does also loop unrolling not good when we are testing loop analysis
    std::string bc_comp_command = "clang-10 -c -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header -O3 -target amdgcn-amd  -foptimization-record-file="+opts_FileName+" " + clFileName + " -o " + noopt_bcFileName;
    std::string opt_comp_command = "opt-10 --loop-simplify --adce --always-inline --amdgpu-always-inline --strip-dead-prototypes " + noopt_bcFileName + " -o " + bcFileName;
    // Disassembly for debugging
    std::string ll_noopt_comp_command = "llvm-dis-10 " + noopt_bcFileName + " -o " + noopt_llFileName;
    std::string ll_opt_comp_command   = "llvm-dis-10 " + bcFileName + " -o " + llFileName;

    // Generate bitcode .bc
    if (verbose) cout << "Executing ...  " << bc_comp_command << endl;
    std::system(bc_comp_command.c_str());

    // Optimise bitcode .bc
    if (verbose) cout << "Executing ...  " << opt_comp_command << endl;
    std::system(opt_comp_command.c_str());

    // Generate IR .ll for debugging
    if (verbose) cout << "Executing ...  " << ll_noopt_comp_command << endl;
    std::system(ll_noopt_comp_command.c_str());
    if (verbose) cout << "Executing ...  " << ll_opt_comp_command << endl;
    std::system(ll_opt_comp_command.c_str());


    // Parse LLVM Bitcode to MemoryBuffer
    ErrorOr<unique_ptr<MemoryBuffer>> bcFileBuffer = MemoryBuffer::getFile(bcFileName);
    if (error_code ec = bcFileBuffer.getError()){
        cerr << "ERROR loading the kernel bitcode IR file: " << bcFileName << endl << "Error message: " << ec.message() << endl;
        exit(1);
    }
    else {
        if(verbose) cout << endl << "kernel bitcode IR file loaded: " << bcFileName << endl;
    }

    LLVMContext context;
    MemoryBufferRef memRef = (*bcFileBuffer)->getMemBufferRef();
    Expected<unique_ptr<Module>> bcModule = parseBitcodeFile(memRef, context);
/*  XXX fixme add error handling
    if(!bcModule()){ // if there is an error
      Error err = bcModule.takeError(); // ?
      cerr << "ERROR loading the module from the bitcode" << endl << "Error message: " << err << endl; 
      exit(1);
    }
*/ 

    // define a feature set   
    celerity::crel_feature_set *fs;
    fs = new celerity::poly_grewe11_feature_set();
    //fs = new celerity::poly_gpu_feature_set();

    // define a feature evaluation technique
    auto *crelPass = new celerity::poly_crel_pass(fs);


    // We build a pass manager that load our pass and dependent passes
    // (e.g., LoopInfoWrapperPass is required by kofler13_eval)
    legacy::PassManager manager;
    llvm::Pass *loop_analysis = new llvm::LoopInfoWrapperPass();
    manager.add(loop_analysis);
    llvm::Pass *scev = new llvm::ScalarEvolutionWrapperPass();
    manager.add(scev);

    //manager.add(fe);
    manager.add(crelPass);


    Module &module = *(*bcModule);
    if(verbose) cout << "Running cost relation pass ... " << endl << endl;
    manager.run(module); // note: this also prints the features in cerr

    // final printing to either cout or file
    fs->print_to_cout();

    //the pass manager does these two deallocations:
    //delete fe; delete loop_analysis;
    delete fs;
    return 0;
} // end main

