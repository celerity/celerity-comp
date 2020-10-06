
#include "crel_extractor.h"


using namespace std;
using namespace llvm;


/* Helper class to parse command line arguments. */
class ArgumentParser {
public:
    ArgumentParser (int &argc, char **argv) {
        for (int i=1; i < argc; ++i)
            this->tokens.push_back(string(argv[i]));
    }

    string getCmdOption(const string &option) const {
        vector<string>::const_iterator itr;
        itr =  find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end()) {
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
const string usage = "Usage:\n\t-h help\n\t-i <kernel cl file>\n\t-csv <csv file>\n\t-v verbose\n";
bool verbose = false;
std::string clFileOutput;

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

    clFileOutput = input.getCmdOption("-csv");


    // define a feature set
    celerity::crel_feature_set *fs;
    //fs = new celerity::poly_grewe11_feature_set();
    fs = new celerity::poly_gpu_feature_set();

    std::string extraCompilerArgs = "-DM=1024 -DLSIZE=64 -DCLASS=CLASS_A";

    // Run the extractor
    int isError = celerity::extract_crels(fs, clFileName, true, clFileOutput, extraCompilerArgs);
    if (!isError) {

        // Print to file if outfile is provided
        if (!clFileOutput.empty()){
            fs->print_to_file(clFileOutput);
        }

        // If no output file is provided print to stdout
        if (clFileOutput.empty()) {
            // final printing to either cout or file
            fs->print_to_cout();
        }
    }

    delete fs;
    return isError;
} // end main

