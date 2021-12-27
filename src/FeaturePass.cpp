#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/ScalarEvolution.h>

#include <llvm/Analysis/CallGraph.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include "FeaturePass.h"
#include "FeatureNormalization.h"

using namespace celerity;
using namespace llvm;
using namespace std;


// Initialization of a static member
char FeaturePass::ID = 0;
char Kofler13Pass::ID = 0;

void FeaturePass::eval_BB(BasicBlock &bb){
    for(Instruction &i : bb){
        features->eval(i);
    }
}	

void FeaturePass::eval_function(Function &fun){
    for (llvm::BasicBlock &bb : fun) 
    eval_BB(bb);
}

void FeaturePass::finalize(){
    normalize(*features);
}

bool FeaturePass::runOnModule(Module& m) {
    CallGraph &CG = getAnalysis<CallGraphWrapperPass>().getCallGraph();
  
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

    return false;
}

bool FeaturePass::runOnSCC(CallGraphSCC &SCC) {
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


void Kofler13Pass::getAnalysisUsage(AnalysisUsage &AU) const {
    AU.setPreservesAll();
    AU.addRequired<CallGraphWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequired<ScalarEvolutionWrapperPass>();
}

/*
 * Current limitations:
 *  - it only works on natual loops (nested loops may be missing)
 */
void Kofler13Pass::eval_function(Function &func) {
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
    LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>(func).getLoopInfo();
    ScalarEvolution &SE = getAnalysis<ScalarEvolutionWrapperPass>(func).getSE();
    for(const Loop *topLevelLoop : LI) {
         cerr << "loop " << topLevelLoop->getName().str() << " in function : " << func.getName().str() << endl;
        auto loopnest = topLevelLoop->getLoopsInPreorder();
        for (const Loop *loop : loopnest) {
            cerr << "    Subloop " << loop->getName().str() << " tripCount: " << SE.getSmallConstantTripCount(loop) << "\n";
            unsigned tripCount = SE.getSmallConstantTripCount(loop);
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
/static RegisterPass<FeaturePass> DefaultFP("feature-pass", "Default feature extraction pass",  false /* Only looks at CFG */,  false /* Analysis Pass */);
static RegisterPass<Kofler13Pass> Kofler13FP("kofler13-pass", "Feature extraction pass based on [Kofler et al., ICS'13]",  false /* Only looks at CFG */,  false /* Analysis Pass */);
//static RegisterPass<FeaturePass> FP("hello", "Hello World Pass",  false /* Only looks at CFG */,  false /* Analysis Pass */);
// New-style pass registration
//FUNCTION_PASS("helloworld", HelloWorldPass())
