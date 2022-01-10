#include <unordered_map>

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Dominators.h>
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
void Kofler13Analysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &FAM)
{
    ScalarEvolution       &SE = FAM.getResult<ScalarEvolutionAnalysis>(fun);
    LoopInfo              &LI = FAM.getResult<LoopAnalysis>(fun);
    DominatorTree         &DT = FAM.getResult<DominatorTreeAnalysis>(fun);
    AssumptionCache       &AC = FAM.getResult<AssumptionAnalysis>(fun);
   
    LoopAnalysisManager *LAM = nullptr;
    if (auto *LAMProxy = FAM.getCachedResult<LoopAnalysisManagerFunctionProxy>(fun))
        LAM = &LAMProxy->getManager();  
    auto &MAMProxy = FAM.getResult<ModuleAnalysisManagerFunctionProxy>(fun);
    bool changed = false;
    for (auto &loop : LI) {
        changed |= simplifyLoop(loop, &DT, &LI, &SE, &AC, nullptr, false);
        changed |= formLCSSARecursively(*loop, DT, &LI, &SE);
    }
   outs() << " loop in LCSSA form (loop has changed:" << changed << ")\n";

    // loop checks
    for (Loop *loop : LI.getLoopsInPreorder()) {
        if (!loop->isLoopSimplifyForm()) errs() << " WARNING Loop is not in normal form\n";
        //if (!loop->isCanonical(SE)) errs() << " WARNING Loop is not canonical\n";
        if (!loop->isLCSSAForm(DT)) errs() << " WARNING Loop is not in LCSSA form\n";
        //else                        errs() << " Loop is in LCSSA form\n";
        BasicBlock *Latch = loop->getLoopLatch();
        if (loop->getExitingBlock() != Latch)  errs() << " WARNING Loop: Exiting and latch block are different\n";
    }

    // 1. For each BB, we initialize it's "loop multiplier" to 1
    std::unordered_map<const llvm::BasicBlock *, unsigned> multiplier;
    for (const BasicBlock &bb : fun.getBasicBlockList()) {
        multiplier[&bb] = 1.0f;
    }
    // 2. For each, we cacluate the loop contribution
    std::map<Loop*, int> loop_cost;
    for (Loop *loop : LI.getLoopsInPreorder()) {
        loop_cost[loop] = loopContribution(*loop, LI, SE);
    }
    // 3. For each BB in a loop, we multiply that "loop multiplier" times the loop cost
    for (Loop *loop : LI.getLoopsInPreorder()) {
        for (BasicBlock *bb : loop->getBlocks()) { // TODO: shold we only count the body?
            multiplier[bb] *= loop_cost[loop];
        }
    } 
    // 4. Final evaluation
    for (llvm::BasicBlock &bb : fun) {
        int mult = multiplier[&bb];
        //outs() << "BB mult: " << mult << "\n";
        for (Instruction &i : bb) {
            features->eval(i, mult);
        }
    }
}

int Kofler13Analysis::loopContribution(const Loop &loop, LoopInfo &LI, ScalarEvolution &SE) {
    // print loop info
    PHINode *ind_var = loop.getInductionVariable(SE);
    if(ind_var == nullptr){
        outs() << "  WARNING: induction variable not found, counting default loop contribution\n";
        return default_loop_contribution;
    }

    Optional<Loop::LoopBounds> bounds = Loop::LoopBounds::getBounds(loop, *ind_var, SE);
    if (!bounds) {
        outs() << "  WARNING: loop bound not found, counting default loop contribution\n";
        return default_loop_contribution;
    }

    // calculate loop cost
    // case 1: uv is an integer and constant
    Value &final = bounds->getFinalIVValue();
    if (ConstantInt *ci = dyn_cast<ConstantInt>(&final)) {
        if (ci->getBitWidth() <= 32) {
            int int_val = ci->getSExtValue();
            outs() << "  CONST loop size is " << int_val << "\n";
            return int_val;
        }
    }
    // case 2: uv is not a constant, then we use the default loop contribution
    outs() << "  Not finding a constant int for finalIVValue, counting default loop contribution\n";
    return default_loop_contribution;
}
