
#include "crel_extractor.h"


using namespace std;
using namespace llvm;
using namespace celerity;




/* Standalone tool that extracts different features representation out of a LLVM-IR program. */

/**
 *
 * @param fs
 * @param clFileInput
 * @param outFile
 * @param compilerArgs:  can be used to pass extra cland compiler args for example to specify defines. Eg:
 *                       -DM=1024 -DLSIZE=64 -DCLASS=CLASS_A
 * @param verbose
 * @return
 */
int celerity::extract_crels(celerity::crel_feature_set *fs,
                            const std::string clFileInput,
                            const bool verbose,
                            const std::string csvFileOtput,
                            const std::string extraClangArgs) {

    // Check that we have required clang and llvm commands
    if (std::system("clang-10 --version > /dev/null") ||
        std::system("opt-10 --version > /dev/null") ||
        std::system("llvm-dis-10 --version > /dev/null") ) {
        cerr << "ERROR make sure to have clang version 10 and llvm version 10 (clang-10, opt-10 and llvm-dis-10) installed and in path." << endl;
        exit(1);
    }

    // Parse .cl MemoryBuffer
    ErrorOr<unique_ptr<MemoryBuffer>> clFileBuffer = MemoryBuffer::getFile(clFileInput);
    if (error_code ec = clFileBuffer.getError()){
        cerr << "ERROR loading the cl file: " << clFileInput << endl << "Error message: " << ec.message() << endl;
        return(1);
    }
    else {
	    if(verbose) cout << "kernel cl file loaded: " << clFileInput << endl;
    }


    // Compile .cl to LLVM IR
    // This assumes clang++ exist
    std::string opts_FileName = clFileInput + "opts.txt";
    std::string noopt_bcFileName = clFileInput + ".bc";
    std::string noopt_llFileName = clFileInput + ".ll";
    std::string bcFileName = clFileInput + "_opt.bc"; // optimised file
    std::string llFileName = clFileInput + "_opt.ll";

    // Assumes clang exists in PATH and points to latest eg: clang10
    // -O0 : doesn't inline non-kernel functions which is required by our tool
    // -O3: does also loop unrolling not good when we are testing loop analysis
    std::string bc_comp_command = "clang-10 -c -x cl -emit-llvm -cl-std=CL1.1 -Xclang -finclude-default-header -O3 -target amdgcn-amd " + extraClangArgs + " -foptimization-record-file=" + opts_FileName + " " + clFileInput + " -o " + noopt_bcFileName;
    std::string opt_comp_command = "opt-10 --loop-simplify --adce --always-inline --amdgpu-always-inline --strip-dead-prototypes " + noopt_bcFileName + " -o " + bcFileName;
    // Disassembly for debugging
    std::string ll_noopt_comp_command = "llvm-dis-10 " + noopt_bcFileName + " -o " + noopt_llFileName;
    std::string ll_opt_comp_command   = "llvm-dis-10 " + bcFileName + " -o " + llFileName;

    // Generate bitcode .bc
    if (verbose) cout << "Executing ...  " << bc_comp_command << endl;
    if (std::system(bc_comp_command.c_str())) {
        cerr << "ERROR while executing:  " << bc_comp_command << endl;
        return(1);
    }

    // Optimise bitcode .bc
    if (verbose) cout << "Executing ...  " << opt_comp_command << endl;
    if (std::system(opt_comp_command.c_str())) {
        cerr << "ERROR while executing:  " << opt_comp_command << endl;
        return(1);
    }

    // Generate IR .ll for debugging
    if (verbose) cout << "Executing ...  " << ll_noopt_comp_command << endl;
    std::system(ll_noopt_comp_command.c_str());
    if (verbose) cout << "Executing ...  " << ll_opt_comp_command << endl;
    std::system(ll_opt_comp_command.c_str());


    // Parse LLVM Bitcode to MemoryBuffer
    ErrorOr<unique_ptr<MemoryBuffer>> bcFileBuffer = MemoryBuffer::getFile(bcFileName);
    if (error_code ec = bcFileBuffer.getError()){
        cerr << "ERROR loading the kernel bitcode IR file: " << bcFileName << endl << "Error message: " << ec.message() << endl;
        return (1);
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
      return(1);
    }
*/

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


    // Delete all generated files
    //if (!verbose) {
        if( remove(opts_FileName.c_str()) != 0 ) perror( "Error deleting file" );
        if( remove(noopt_bcFileName.c_str()) != 0 ) perror( "Error deleting file" );
        if( remove(noopt_llFileName.c_str()) != 0 ) perror( "Error deleting file" );
        if( remove(bcFileName.c_str()) != 0 ) perror( "Error deleting file" );
        if( remove(llFileName.c_str()) != 0 ) perror( "Error deleting file" );
    //}

    //the pass manager does these two deallocations:
    //delete fe; delete loop_analysis;

    return 0;
}

