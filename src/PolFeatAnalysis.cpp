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


IMPoly::IMPoly(){   
    KernelInvariant::numInvariantType();
    fmpz_mpoly_ctx_init(ctx, KernelInvariant::numInvariantType(), ordering_t::ORD_DEGLEX);
    fmpz_mpoly_init(mpoly, ctx);
}
IMPoly::IMPoly(unsigned constant, unsigned inv_id){
    KernelInvariant::numInvariantType();
    fmpz_mpoly_ctx_init(ctx, KernelInvariant::numInvariantType(), ordering_t::ORD_DEGLEX);
    fmpz_mpoly_init(mpoly, ctx);
    fmpz_mpoly_set_term_coeff_ui(mpoly, inv_id, constant, ctx);
}
IMPoly::~IMPoly(){
    fmpz_mpoly_clear(mpoly, ctx);
    fmpz_mpoly_ctx_clear(ctx);
}   

IMPoly& IMPoly::operator += (IMPoly const &rhs){
    fmpz_mpoly_add(mpoly, mpoly, rhs.mpoly, ctx);
    return *this;    
}
IMPoly& IMPoly::operator *= (IMPoly const &rhs){
    fmpz_mpoly_mul(mpoly, mpoly, rhs.mpoly, ctx);
    return *this; 
}

IMPoly IMPoly::max(IMPoly const &poly1, IMPoly const &poly2){
    IMPoly res;
    // TODO XXX
    return res;
}
// Assignment operators
//IMPoly& IMPoly::operator=(IMPoly &poly){}
//IMPoly& IMPoly::operator=(unsigned constant){}   
// Output operators

string IMPoly::str() const{
    string pretty = string(fmpz_mpoly_get_str_pretty(mpoly, InvariantTypeName, ctx));
    return pretty;
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