//#include <llvm/RegisterPass.h>

#include "feature_eval.h"
#include "feature_norm.h"

using namespace celerity;
using namespace llvm;
using namespace std;

// The following defines the name of the pass, used by <opt> to run this pass on a IR file (registering dynamically loaded passes).
static RegisterPass<feature_eval> feature_eval_pass("feature_eval", "Feature evaluation");

// Initialization of a static member
char feature_eval::ID = 0;

void feature_eval::eval_BB(const BasicBlock &bb){
	for(const Instruction &i : bb){
		features->eval_instruction(i);
	}
}	

void feature_eval::eval_function(const llvm::Function &fun){
	for (const llvm::BasicBlock &bb : fun) 
        eval_BB(bb);
}

void feature_eval::finalize(){
	//std::unordered_map<string,int> &raw = features->raw;
	//features->instructionNum = raw.size();
        //raw["binOps"] + raw["bitbinOps"] + raw["vecOps"] + raw["aggOps"] + raw["otherOps"] + raw["loadOps"] + raw["storeOps"];
	normalize(*features);
}

bool feature_eval::runOnFunction(llvm::Function &function) {
	eval_function(function);
	finalize();	
	features->print(cerr); // LLVM doesn't like cout :)
    return false; // no changes have been done to the Function
}

void kofler13_eval::eval_function(const llvm::Function &fun) {
    // TODO fixme bug Nadjib
    // Implementation is currenty using the standard one.
    // Need to implement the loop heuristic.
    feature_eval::eval_function(fun);
}
