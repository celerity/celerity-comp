#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
using namespace llvm;

#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;


/// Feature extraction based on Kofler et al. 13 loop heuristics
/// Requires LoopAnalysis and ScalarEvolutionAnalysis.
/// Current limitations:
///  - it only works on natual loops (in some case nested loops may be missing)
///  - analysis is more accurate on canonical loops
///  - support up to 3 nested level
void Kofler13Analysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam) {
    std::cerr << "extract on function " << fun.getName().str() << "\n";
    LoopInfo &LI = fam.getResult<LoopAnalysis>(fun);
    ScalarEvolution &SCEV = fam.getResult<ScalarEvolutionAnalysis>(fun);

    // skip the function if is only a declaration    
    if (fun.isDeclaration()) return;

    // 1. for each BB, we initialize it's "loop multiplier" to 1
    std::unordered_map<const llvm::BasicBlock *, unsigned> multiplier;
    for(const BasicBlock &bb : fun.getBasicBlockList()){
        multiplier[&bb] = 1.0f;
    }	

    cerr << "loops:\n";
    int count = 0;
    for(const Loop *loop : LI) {        
        cerr << " * " << count
             << "\n  - name:" << loop->getName().str()
             << "\n  - depth:" << loop->getLoopDepth()
             << "\n  - canonical:" << loop->isCanonical(SCEV)
             << "\n";
        count++;
    }

    // 2. for each BB in a loop, we multiply that "loop multiplier" times 100
    const int default_loop_contribution = 100;
    for(const Loop *loop1 : LI) {
        cerr << "loop 1 " << loop1->getName().str() <<  "\n";
        // calculate the contributoin for the loop
        int contrib1 = loopContribution(*loop1, SCEV);
        // apply to each basic block in the loop
        for(BasicBlock *bb : loop1->getBlocks()){ // TODO: shold we only count the body?
            multiplier[bb] *= contrib1;
        }
        //iterate on sub-loop (2)       
        for (const Loop *loop2 : loop1->getLoopsInPreorder()) {
            cerr << "loop 2 " << loop2->getName().str() <<  "\n";        
            int contrib2 = loopContribution(*loop2, SCEV);        
            for(BasicBlock *bb : loop2->getBlocks()){
                multiplier[bb] *= contrib2;
            }        
        }
    /*
        for (const Loop *loop2 : loopnest) {
            cerr << "    subloop " << loop2->getName().str() << " tripCount: " << SCEV.getSmallConstantTripCount(loop) << "\n";
            unsigned tripCount = SCEV.getSmallConstantTripCount(loop);
            for(const BasicBlock *bb : loop2->getBlocks()) {
                int contribution;
		if (tripCount > 1 && bb == loop2->getExitingBlock()) {
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
    */
    }

    /// 3. evaluation
    for (llvm::BasicBlock &bb : fun) {
        int mult = multiplier[&bb];
        /// cerr << "BB mult: " << mult << endl;
        for(Instruction &i : bb){
            features->eval(i, mult);
        }
    }

}

int Kofler13Analysis::loopContribution(const llvm::Loop &loop, ScalarEvolution &SCEV){
    // if canonical, we can try a more accurate guess
    if(loop.isCanonical(SCEV)){ 
        auto iv =loop.getInductionVariable(SCEV);
        Optional<Loop::LoopBounds> opt_bounds = Loop::LoopBounds::getBounds(loop, *iv, SCEV);
        if(opt_bounds){
            Loop::LoopBounds &bounds = opt_bounds.getValue();
            // if canonical, we assume the loop starts at 0 and ends at _ub_
            llvm::Value &uv = bounds.getFinalIVValue() ;
            // case 1: uv is an integer and constant
            cerr << "checking uv for " << uv.getName().str() << "\n";            
            // int
            if (ConstantInt* ci = dyn_cast<ConstantInt>(&uv)) {
                if (ci->getBitWidth() <= 32) {
                    int int_val = ci->getSExtValue();
                    cerr << "loop size is " << int_val << "\n";
                    return int_val;
                }
            }
            // case 2: uv is not a constant, then we use the default_loop_contribution         
        } else  
            cerr << "LoopBounds opt. not found\n";
    }
    return default_loop_contribution;
}
            
//-----------------------------------------------------------------------------
// Pass registration using the new LLVM PassManager
//-----------------------------------------------------------------------------
/*
llvm::PassPluginLibraryInfo getFeatureExtractionPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "Kofler13FeatureExtraction", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
          // #1 REGISTRATION FOR "opt -passes=print<feature-extraction>"
          // Register FeaturePrinterPass so that it can be used when
          // specifying pass pipelines with `-passes=`.
          PB.registerPipelineParsingCallback(
            [&](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "print<kofler13feature>") {
                    FPM.addPass(FeaturePrinterPass(llvm::errs()));
                    return true;
                }
                return false;
              });
          // #2 REGISTRATION FOR "-O{1|2|3|s}"
          // Register FeaturePrinterPass as a step of an existing pipeline.
          // The insertion point is specified by using the
          // 'registerVectorizerStartEPCallback' callback. To be more precise,
          // using this callback means that FeaturePrinterPass will be called
          // whenever the vectoriser is used (i.e. when using '-O{1|2|3|s}'.
          PB.registerVectorizerStartEPCallback(
              [](llvm::FunctionPassManager &PM, llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(FeaturePrinterPass(llvm::errs()));
              });
          // #3 REGISTRATION FOR "FAM.getResult<FeatureExtractionPass>(Func)"
          // Register FeatureExtractionPass as an analysis pass. This is required so that
          // FeaturePrinterPass (or any other pass) can request the results
          // of FeatureExtractionPass.

          // #4 Registration for ScalarEvolution and LoopInfo pass
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return FeatureExtractionPass(); });
              });
          
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return  LoopAnalysis(); });
              });
          }

        }; 
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFeatureExtractionPassPluginInfo();
}
*/