//#include <llvm/RegisterPass.h>

#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>

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
	normalize(*features);
}

bool feature_eval::runOnFunction(llvm::Function &function) {
	eval_function(function);
	finalize();	
	features->print(cerr); // LLVM doesn't like cout :)
    return false; // no changes have been done to the Function
}

void kofler13_eval::eval_function(const llvm::Function &fun) {
    cout << "kofler 1" << endl;

    // 1. for each BB, we initialize it's "loop multiplier" to 1
    std::unordered_map<const llvm::BasicBlock *, int> multiplier;
    for(const BasicBlock &bb : fun.getBasicBlockList()){
        multiplier[&bb] = 1;
    }	

    cout << "kofler 2" << endl;
    // 2. for each BB in a loop, we multiply that "loop multiplier" times 100
    const int loop_contribution = 100;
    llvm::Function &fun2 = const_cast<llvm::Function &>(fun); // un-const hack
    //const LoopInfo &LI = getAnalysis<LoopInfo>(fun2);  
    const LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(fun2).getLoopInfo();  
    for(const Loop *loop : LI){
        for(const BasicBlock *bb : loop->getBlocks()) {
            multiplier[bb] = multiplier[bb] * loop_contribution;
        }
    }

    cout << "kofler 3" << endl;
    /// 3. evaluation
    //feature_eval::eval_function(fun);
   	for (const llvm::BasicBlock &bb : fun) {
        int mult = multiplier[&bb];
        cout << "BB mult: " << mult;
       	for(const Instruction &i : bb){
		    features->eval_instruction(i, mult);            
	    }        
    }

}
