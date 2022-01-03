#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Utils/LoopSimplify.h>
using namespace llvm;

#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;


llvm::AnalysisKey Kofler13Analysis::Key;

/// Feature extraction based on Kofler et al. 13 loop heuristics
/// Requires LoopAnalysis and ScalarEvolutionAnalysis.
/// Current limitations:
///  - it only works on natual loops (in some case nested loops may be missing)
///  - analysis is more accurate on canonical loops
///  - support up to 3 nested level
void Kofler13Analysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam) {
    std::cerr << "extract on function " << fun.getName().str() << "\n";
    //fam.getResult<LoopSimplifyPass>(fun);    
    LoopInfo &LI = fam.getResult<LoopAnalysis>(fun);    
    ScalarEvolution &SE = fam.getResult<ScalarEvolutionAnalysis>(fun);
    
    // skip the function if is only a declaration    
    if (fun.isDeclaration()) return;

    // 1. for each BB, we initialize it's "loop multiplier" to 1
    std::unordered_map<const llvm::BasicBlock *, unsigned> multiplier;
    for(const BasicBlock &bb : fun.getBasicBlockList()){
        multiplier[&bb] = 1.0f;
    }	
    
    cerr << "loops:\n";
    int count = 0;
    for(Loop *loop : LI.getLoopsInPreorder()) {             
        outs() << " * " << (count+1)
            //<< "\n  -        id:" << loop->
            << " - depth:" << loop->getLoopDepth()
            << " - canonical:" << loop->isCanonical(SE)
            //<< " - indvar " << loop->getCanonicalInductionVariable()->getName()
            << "\n";       
        count++;        
    }

    

    // 2. for each BB in a loop, we multiply that "loop multiplier" times 100
    const int default_loop_contribution = 100;
    for(const Loop *loop : LI.getLoopsInPreorder()) {
        // calculate the contributoin for the loop
        int contrib = loopContribution(*loop, SE);        
        // apply to each basic block in the loop
        for(BasicBlock *bb : loop->getBlocks()){ // TODO: shold we only count the body?
            multiplier[bb] *= contrib;
        }
        /*
        //iterate on sub-loop (2)       
        for (const Loop *loop2 : loop1->getLoopsInPreorder()) {
            outs() << "loop 2 " << loop2->getName().str() <<  "\n";        
            int contrib2 = loopContribution(*loop2, SE);        
            for(BasicBlock *bb : loop2->getBlocks()){
                multiplier[bb] *= contrib2;
            }        
        }
        */
    } // for

    /// 3. evaluation
    for (llvm::BasicBlock &bb : fun) {
        int mult = multiplier[&bb];
        outs() << "BB mult: " << mult << "\n";
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
// Register the Kofler13 analysis in the FeatureAnalysis registry
//-----------------------------------------------------------------------------
static celerity::FeatureAnalysis* _static_kfa_ptr_ = new celerity::Kofler13Analysis; // dynamic_cast<celerity::FeatureAnalysis*>(&_static_fa_);
static bool _registered_feature_analysis_ = FARegistry::registerByKey("kofler13", _static_kfa_ptr_ ); 
