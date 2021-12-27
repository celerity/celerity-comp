#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/Analysis/ScalarEvolution.h>

//#include <llvm/Analysis/CallGraph.h>
//#include <llvm/ADT/SCCIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

#include "FeaturePass.hpp"
#include "FeatureNormalization.hpp"

using namespace celerity;
using namespace llvm;
using namespace std;


llvm::AnalysisKey FeatureExtractionPass::Key;

void FeatureExtractionPass::extract(BasicBlock &bb){
    for(Instruction &i : bb){
        features->eval(i);
    }
}	

void FeatureExtractionPass::extract(Function &fun){
    for (llvm::BasicBlock &bb : fun) 
        extract(bb);
}

void FeatureExtractionPass::finalize(){
    normalize(*features);
}

FeatureExtractionPass::Result FeatureExtractionPass::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
    extract(fun);
    finalize();    
    return features->getFeatureValues();
}

/*
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
*/


llvm::PreservedAnalyses FeaturePrinterPass::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
    auto &feature_set = fam.getResult<FeatureExtractionPass>(fun);
    out_stream << "Printing analysis FeatureExtractionPass for function " << fun.getName() << "\n";
    print_feature(feature_set, out_stream);
    return PreservedAnalyses::all();
}

/*
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
void Kofler13ExtractionPass::extract(llvm::Function &fun) {
    // Current implementation requires that the LoopInfoWrapperPass pass calculates the loop information, 
    // thus it should be ran before ofthis pass.    
    
    // 1. for each BB, we initialize it's "loop multiplier" to 1
    if (fun.isDeclaration())
        return;
    std::unordered_map<const llvm::BasicBlock *, int> multiplier;
    for(const BasicBlock &bb : fun.getBasicBlockList()){
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
//static RegisterPass<FeaturePass> DefaultFP("feature-pass", "Default feature extraction pass",  false /* Only looks at CFG */,  false /* Analysis Pass */);
//static RegisterPass<Kofler13Pass> Kofler13FP("kofler13-pass", "Feature extraction pass based on [Kofler et al., ICS'13]",  false /* Only looks at CFG */,  false /* Analysis Pass */);

/*
llvm::PassPluginLibraryInfo getFeatureExtractionPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "OpcodeCounter", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
          // #1 REGISTRATION FOR "opt -passes=print<opcode-counter>"
          // Register OpcodeCounterPrinter so that it can be used when
          // specifying pass pipelines with `-passes=`.
          PB.registerPipelineParsingCallback(
              [&](StringRef Name, FunctionPassManager &FPM,
                  ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "print<opcode-counter>") {
                  FPM.addPass(OpcodeCounterPrinter(llvm::errs()));
                  return true;
                }
                return false;
              });
          // #2 REGISTRATION FOR "-O{1|2|3|s}"
          // Register OpcodeCounterPrinter as a step of an existing pipeline.
          // The insertion point is specified by using the
          // 'registerVectorizerStartEPCallback' callback. To be more precise,
          // using this callback means that OpcodeCounterPrinter will be called
          // whenever the vectoriser is used (i.e. when using '-O{1|2|3|s}'.
          PB.registerVectorizerStartEPCallback(
              [](llvm::FunctionPassManager &PM,
                 llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(OpcodeCounterPrinter(llvm::errs()));
              });
          // #3 REGISTRATION FOR "FAM.getResult<OpcodeCounter>(Func)"
          // Register OpcodeCounter as an analysis pass. This is required so that
          // OpcodeCounterPrinter (or any other pass) can request the results
          // of OpcodeCounter.
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return OpcodeCounter(); });
              });
          }
        };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOpcodeCounterPluginInfo();
}
*/