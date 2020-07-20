#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/ScalarEvolution.h>

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

void feature_pass::eval_function(Function &fun){
    for (llvm::BasicBlock &bb : fun) 
    eval_BB(bb);
}

void feature_pass::finalize(){
    normalize(*features);
}

bool feature_pass::runOnModule(Module& m) {
    CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
    //bool Changed = doInitialization(CG);
  
    // Walk the callgraph in bottom-up SCC order.
    scc_iterator<CallGraph*> CGI = scc_begin(&CG);
  
    CallGraphSCC CurSCC(CG, &CGI);
    while (!CGI.isAtEnd()) {
        // Copy the current SCC and increment past it so that the pass can hack
        // on the SCC if it wants to without invalidating our iterator.
        const std::vector<CallGraphNode *> &NodeVec = *CGI;
        CurSCC.initialize(NodeVec);
        runOnSCC(CurSCC);
        ++CGI;
    }
    /*CallGraph& cg = getAnalysis<CallGraphWrapperPass>().getCallGraph();

    scc_iterator<CallGraph*> cgSccIter = scc_begin(&cg);
    CallGraphSCC curSCC(cg, (void*) &(m.getContext()));
    while (!cgSccIter.isAtEnd())
    {
        const vector<CallGraphNode*>& nodeVec = *cgSccIter;
        curSCC.initialize(nodeVec);
        runOnSCC(curSCC);
        ++cgSccIter;
    }*/

    return false;
}

bool feature_pass::runOnSCC(CallGraphSCC &SCC) {
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
    AU.addRequired<CallGraphWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
}

/*
 * Current limitations:
 *  - it only works on natual loops (nested loops may be missing)
 */
void kofler13_pass::eval_function(Function &func) {
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
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(func).getSE();
    for(const Loop *topLevelLoop : LI) {
         cerr << "loop " << topLevelLoop->getName().str() << " in function : " << func.getName().str()
        //      << " with smallConstantTripCount: " << SE.getSmallConstantTripCount(topLevelLoop)
        //      << " smallConstantMaxTripCount: " << SE.getSmallConstantMaxTripCount(topLevelLoop)
        //      << " smallConstantTripMultiple: " << SE.getSmallConstantTripMultiple(topLevelLoop)
        //      //<< " exiting block: " << topLevelLoop->getExitingBlock()->getName().str()
              << endl;
        auto loopnest = topLevelLoop->getLoopsInPreorder();
        for (const Loop *loop : loopnest) {
            cerr << "    Subloop " << loop->getName().str() << " tripCount: " << SE.getSmallConstantTripCount(loop) << "\n";
            unsigned tripCount = SE.getSmallConstantTripCount(loop);
            // int contribution = default_loop_contribution; // = tripCount > 1 ? tripCount : default_loop_contribution;
            for(const BasicBlock *bb : loop->getBlocks()) {
                int contribution;
		if (tripCount > 1 && bb == loop->getExitingBlock()) {
		    contribution = tripCount;
		} else if (tripCount > 1) {
		    contribution = tripCount - 1;
		} else {
		    contribution = default_loop_contribution;
		}
                multiplier[bb] = multiplier[bb] * contribution;
                // cerr << "        BB " << bb->getName().str() << " contribution " << contribution << " total " << multiplier[bb] << "\n";
            }
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


//INITIALIZE_PASS_BEGIN(kofler13_pass, "kofler13_pass", "Kofler's feature evaluation", false, false)
//INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
//INITIALIZE_PASS_END(kofler13_pass, "kofler13_pass", "Kofler's feature evaluation", false, false)

