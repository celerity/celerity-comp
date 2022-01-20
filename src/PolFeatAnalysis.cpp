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

#include "KernelInvariant.hpp"
#include "PolFeatAnalysis.hpp"
using namespace celerity;

llvm::AnalysisKey PolFeatAnalysis::Key;


ResultPolFeatSet PolFeatAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
 /// XXX
    
    return ResultPolFeatSet { features->getFeatureCounts(), features->getFeatureValues() };
}

void PolFeatAnalysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &FAM)
{
    ScalarEvolution       &SE = FAM.getResult<ScalarEvolutionAnalysis>(fun);
    LoopInfo              &LI = FAM.getResult<LoopAnalysis>(fun);
    DominatorTree         &DT = FAM.getResult<DominatorTreeAnalysis>(fun);
    AssumptionCache       &AC = FAM.getResult<AssumptionAnalysis>(fun);

    std::cout << "PolFeat IMPOLY\n";
    IMPoly test1(10, KernelInvariant::enumerate(celerity::InvariantType::gs0));
    std::cout << test1;
    IMPoly test2(7,KernelInvariant::enumerate(celerity::InvariantType::a0));
    std::cout << test2;
    test1 += test2;
    std::cout << test1;
}

IMPoly PolFeatAnalysis::loopContribution(const Loop &loop, LoopInfo &LI, ScalarEvolution &SE) {
    return IMPoly();
}