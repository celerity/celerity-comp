#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>


#include "feature_eval.h"
#include "feature_norm.h"

using namespace celerity;
using namespace llvm;
using namespace std;


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
	finalize();	        // normalization here
	features->print();  // print the features on cerr (LLVM doesn't like cout)
    return false;       // no changes have been done to the Function
}


void kofler13_eval::getAnalysisUsage(AnalysisUsage &AU) const {    
    AU.setPreservesAll();
    AU.addRequired<LoopInfoWrapperPass>();
}
/*
 * Current limitations:
 *  - it only works on natual loops (nested loops may be missing)
 *  - it does not implement yet the static loop bound check (multiplier = static loop bound)
 */
void kofler13_eval::eval_function(const llvm::Function &fun) {
    // Current implementation requires that the LoopInfoWrapperPass pass calculates the loop information, 
    // thus it should be ran before ofthis pass.    
    
    // 1. for each BB, we initialize it's "loop multiplier" to 1
    std::unordered_map<const llvm::BasicBlock *, int> multiplier;
    for(const BasicBlock &bb : fun.getBasicBlockList()){
        multiplier[&bb] = 1;
    }	

    // 2. for each BB in a loop, we multiply that "loop multiplier" times 100
    const int loop_contribution = 100;
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();        
    for(const Loop *loop : LI) {        
        // cerr << "loop found" << endl;         
        for(const BasicBlock *bb : loop->getBlocks()) {               
            // cerr << "BB in loop!" << endl;         
            multiplier[bb] = multiplier[bb] * loop_contribution;
        }
    }
    
    /// 3. evaluation
    //feature_eval::eval_function(fun);
   	for (const llvm::BasicBlock &bb : fun) {
        int mult = multiplier[&bb];
        /// cerr << "BB mult: " << mult << endl;
       	for(const Instruction &i : bb){
		    features->eval_instruction(i, mult);            
	    }        
    }

}


// Pass registration.

// Old-style pass registration for <opt> (registering dynamically loaded passes).
static RegisterPass<feature_eval> feature_eval_pass("feature-eval", "Feature evaluation");
static RegisterPass<kofler13_eval> kofler13_eval_pass("kofler13-eval", "Kofler13 feature evaluation");
//static RegisterPass<costrelation_eval> cr_eval_pass("costrelation_eval", "Cost relation feature evaluation");

// Pass registration with declared dependencies.
//INITIALIZE_PASS_BEGIN(kofler13_eval, "kofler13_eval", "Kofler's feature evaluation", false, false)
//INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
//INITIALIZE_PASS_END(kofler13_eval, "kofler13_eval", "Kofler's feature evaluation", false, false)
