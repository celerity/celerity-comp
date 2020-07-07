#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/CallGraphSCCPass.h>

#include <llvm/Analysis/CallGraph.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>


#include "feature_pass.h"
#include "feature_norm.h"

using namespace celerity;
using namespace llvm;
using namespace std;


// Initialization of a static member
char feature_pass::ID = 0;
char kofler13_pass::ID = 0;

void feature_pass::eval_BB(BasicBlock &bb){
    for(Instruction &i : bb){
        features->eval(i);
    }
}	

void feature_pass::eval_function(llvm::Function &fun){
    for (llvm::BasicBlock &bb : fun) 
    eval_BB(bb);
}

void feature_pass::finalize(){
    normalize(*features);
}

bool feature_pass::runOnSCC(llvm::CallGraphSCC &SCC) {
    for (auto &cgnode : SCC) {
        Function *func = cgnode->getFunction();
        if (func) {
            //cout << "eval function: " << (func->hasName() ? func->getName().str() : "anonymous") << "\n";
            eval_function(*func);
	    finalize();
            //features->print();
        }
    }
    return false;
}


void kofler13_pass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<llvm::CallGraphWrapperPass>();
    //AU.addRequired<LoopInfoWrapperPass>();
}
/*
 * Current limitations:
 *  - it only works on natual loops (nested loops may be missing)
 *  - it does not implement yet the static loop bound check (multiplier = static loop bound)
 */
void kofler13_pass::eval_function(llvm::Function &func) {
    // Current implementation requires that the LoopInfoWrapperPass pass calculates the loop information, 
    // thus it should be ran before ofthis pass.    
    
    // 1. for each BB, we initialize it's "loop multiplier" to 1
    if (func.isDeclaration())
        return;
    std::unordered_map<const llvm::BasicBlock *, int> multiplier;
    for(const BasicBlock &bb : func.getBasicBlockList()){
        multiplier[&bb] = 1;
    }	

    // 2. for each BB in a loop, we multiply that "loop multiplier" times 100
    const int default_loop_contribution = 100;
    //getAnalysis<LoopInfo>(F);
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(func).getLoopInfo();
    for(const Loop *loop : LI) {
        // cerr << "loop found" << endl;
        for(const BasicBlock *bb : loop->getBlocks()) {
            // cerr << "BB in loop!" << endl;
            multiplier[bb] = multiplier[bb] * default_loop_contribution;
        }
    }
    /// 3. evaluation
    //feature_eval::eval_function(func);
    for (llvm::BasicBlock &bb : func) {
        int mult = multiplier[&bb];
        /// cerr << "BB mult: " << mult << endl;
        for(Instruction &i : bb){
            features->eval(i, mult);
        }
    }

}


// Pass registration.

// Old-style pass registration for <opt> (registering dynamically loaded passes).
static RegisterPass<feature_pass> feature_eval_pass("feature-pass", "Feature evaluation");
static RegisterPass<kofler13_pass> kofler13_eval_pass("kofler13-pass", "Kofler13 feature evaluation");
//static RegisterPass<costrelation_eval> cr_eval_pass("costrelation_eval", "Cost relation feature evaluation");

