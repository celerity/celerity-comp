#include <unordered_map>
#include <sstream>
#include <vector>
using namespace std;

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Support/CommandLine.h>
using namespace llvm;

#include "FeatureAnalysis.hpp"
#include "Kofler13Analysis.hpp"
#include "FeaturePrinter.hpp"
//#include "FeatureNormalization.hpp"
using namespace celerity;

llvm::AnalysisKey FeatureAnalysis::Key;

FeatureAnalysis::~FeatureAnalysis() {}

void FeatureAnalysis::extract(BasicBlock &bb)
{
  for (Instruction &i : bb)
  {
    features->eval(i);
  }
}

void FeatureAnalysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  for (llvm::BasicBlock &bb : fun)
    extract(bb);
}

void FeatureAnalysis::finalize()
{
  normalize(*features);
}

ResultFeatureAnalysis FeatureAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  outs() << "analysis for function: " << fun.getName().str() 
    << " feature-set: " << features->getName()
    << " analysis-name: " << getName() << "\n";
  extract(fun, fam);
  finalize();
  return ResultFeatureAnalysis { features->getFeatureCounts(), features->getFeatureValues() };
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


