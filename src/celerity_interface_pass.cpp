
#include "../include/celerity_interface_pass.h"

using namespace celerity;

// ------------------ Help functions

std::string stripBeforeAndInsideBracketPair(char openingBracket, char closingBracket, const std::string &symbol,
                                            std::string &strippedPart) {
    std::string result = symbol;

    if (!result.empty()) {
        size_t openPos = result.find_first_of(openingBracket);
        size_t closePos = result.find_first_of(closingBracket);

        if ((openPos != std::string::npos) && (closePos != std::string::npos)) {
            strippedPart = result.substr(openPos);
            //result = result.substr(openPos, result.length()-closePos);
            result = result.substr(closePos + 1);
        }
    }
    return result;
}

/*
 * Get File Name from a Path with or without extension
 */
std::string extractFileName(std::string filePath, char seperator = '/')
{
    // Get last dot position
    std::size_t dotPos = filePath.rfind('.');
    std::size_t sepPos = filePath.rfind(seperator);

    // If there was no separator found start from the begining of the string
    if(sepPos == std::string::npos) sepPos = -1;
    // If there was no dot found go till the end of the string
    if(dotPos == std::string::npos) dotPos = filePath.size();

    return filePath.substr(sepPos + 1, dotPos);
}


/**
 * Module run method
 */
bool CelerityInterfacePass::runOnModule(Module &m) {

    // Parse the module name and use it to name the output filename
    std::string filename = extractFileName(m.getModuleIdentifier());
    filename = extractFileName(filename); // done twice to make sure
    filename = filename + "_celerity_interface.h";

    outputstream() << "Generating celerity interface header ... " << endl;
    outputstream() << "Module: " << m.getModuleIdentifier() << endl;
    outputstream() << "Output filename: " << filename << endl;
    outputstream() << "Module Triple: " << m.getTargetTriple() << endl;

    // Create the file output stream
    ofstream fileOutputStream(filename);
    // Set the output stream to the file. std:cout is used If this line is commented out
    setOutputStream(fileOutputStream);

    // Generate the header
    printInterfaceHeader();

    // Then try looking for function representing sycl kernel and generate their corresponding templated class
    for (Module::iterator mi = m.begin(), me = m.end(); mi != me; ++mi) {
        runOnFunction(*mi);
    }

    // Make sure to close the output stream
    fileOutputStream.close();

    return false;
}

/**
 * This is the main function for generating the celerity runtime interface
 * This method runs on every llvm function and tries to extract the sycl kernel name by demangling the llvm function name.
 * It uses some regular expressions to extract the kernel name.
 * If kernel name is found, it runs some other passes to extract static features then generates the final celerity_interface.h
 */
bool CelerityInterfacePass::runOnFunction(Function &f) {

    // Regular expression constants
    const std::regex SYCL_DISPATCH_REGEXPR("^void cl::sycl::detail::dispatch.*void celerity::handler::.*");
    const std::regex SYCL_KERNEL_NAME_REGEXPR(".*const::(\\w+),.*");

    const string SYCL_DISPATCH_EXPR = "void cl::sycl::detail::dispatch";
    //const string SYCL_DISPATCH_EXPR = "mat_mul";

    // demanlge the function name.
    string demangledName = demangle(f.getName());

    // Identify the kernel dispatch function by grepping for void cl::sycl::detail::dispatch.*void celerity::handler::
    // There should be only one function defintion dispatching the kernel but can be called multiple times
    if (std::regex_match(demangledName, SYCL_DISPATCH_REGEXPR)) {


        std::smatch match;
        std::string kernelName;
        if (std::regex_search(demangledName, match, SYCL_KERNEL_NAME_REGEXPR) && match.size() > 1) {
            kernelName = match.str(1);

            // Generate the kernel interface templated class
            printKernelClass(kernelName, f);

        }
    }

    return false;
}

/**
 * This method is responsible for generating the celerity interface code for a certain kernel
 * @param kernelName
 */
void CelerityInterfacePass::printKernelClass(const std::string& kernelName, Function &f) {

    outputstream() << "// efficiency_predictor for kernel: " <<  kernelName << endl;
    outputstream() << "template<>" << endl;
    outputstream() << "class efficiency_predictor<" << kernelName << "> {" << endl;
    outputstream() << "     float operator(const runtime_features& rf) {" << endl;
    outputstream() << "          return 1.0f; // TODO: replace this with a call to an internal modeling function" << endl;
    outputstream() << "     }" << endl;
    outputstream() << "}" << endl;

}
/**
 * Print the header of the generated celerity_interface.h
 * Prints any common code
 */
void CelerityInterfacePass::printInterfaceHeader() {

    std::time_t now_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    outputstream() << "// " << endl;
    outputstream() << "// This file is autogenerated by the CelerityInterfacePass " << endl;
    outputstream() << "// " << std::ctime(&now_time) << endl;

    // Includes
    outputstream() << "#include <CL/sycl.hpp>" << endl << endl;

    // Structs
    outputstream() << "// buffer_access  struct" << endl;
    outputstream() << "struct buffer_access {" << endl;
    outputstream() << "     cl::sycl::access::mode mode;" << endl;
    outputstream() << "     std::size_t range[3];" << endl;
    outputstream() << "}" << endl;

    outputstream() << "// runtime_features  struct" << endl;
    outputstream() << "struct runtime_features {" << endl;
    outputstream() << "     std::size_t global_size[3];" << endl;
    outputstream() << "     std::size_t local_size[3];" << endl;
    outputstream() << "     std::vector<buffer_access> buffer_accesses;" << endl;
    outputstream() << "}" << endl << endl;

}


/**
 * Demangles the name of a function
 * @param MangledName
 * @return
 */
std::string CelerityInterfacePass::demangle(const std::string &MangledName) {
    char *Demangled;
    if (isItaniumEncoding(MangledName))
        Demangled = itaniumDemangle(MangledName.c_str(), nullptr, nullptr, nullptr);
    else
        Demangled = microsoftDemangle(MangledName.c_str(), nullptr, nullptr, nullptr);

    if (!Demangled)
        return MangledName;

    std::string Ret = Demangled;
    free(Demangled);
    return Ret;
}

/**
 * This checks whether we are using a ItaniumEncoding usually used by most posix compilers
 * @param MangledName
 * @return
 */
bool CelerityInterfacePass::isItaniumEncoding(const std::string &MangledName) {
    size_t Pos = MangledName.find_first_not_of('_');
    // A valid Itanium encoding requires 1-4 leading underscores, followed by 'Z'.
    return Pos > 0 && Pos <= 4 && MangledName[Pos] == 'Z';
}

// ------------------ Pass registration code

// Pass loading stuff
// To use, run: clang -Xclang -load -Xclang <your-pass>.so <other-args> ...
static void registerKernelNamePass(const PassManagerBuilder &Builder, legacy::PassManagerBase &PM) {
    PM.add(new CelerityInterfacePass());
}

// Pass info
// LLVM uses the address of this static member to identify the pass
char CelerityInterfacePass::ID = 0;
// This registers this pass with -kernel-name command line argument for opt
static RegisterPass<CelerityInterfacePass> X("celerity-interface", "CelerityInterfacePass");

//static RegisterStandardPasses RegisterMyPass(PassManagerBuilder::EP_EarlyAsPossible, KernelNamePass::registerMyPass);

// Note: The location EP_OptimizerLast places this pass at the end of the list
// of *optimizations*. That means on -O0, it does not get run.
//
// In general, adding your pass twice will cause it to run twice, but in this
// particular case, the two are disjoint (EP_EnabledOnOptLevel0 only runs if
// you're in -O0, and EP_OptimizerLast only runs if you're not). You can check
// include/llvm/Transforms/IPO/PassManagerBuilder.h header and
// lib/Transforms/IPO/PassManagerBuilder.cpp file for the exact behavior.

// These constructors add our pass to a list of global extensions.
static RegisterStandardPasses celerityInterfacePassLoader_Ox(PassManagerBuilder::EP_ModuleOptimizerEarly,
                                                             registerKernelNamePass);
static RegisterStandardPasses celerityInterfacePassLoader_O0(PassManagerBuilder::EP_EnabledOnOptLevel0,
                                                             registerKernelNamePass);



