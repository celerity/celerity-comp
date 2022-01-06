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
#include "KernelInvariant.hpp"
using namespace celerity;



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
  KernelInvariant ki(fun);
  ki.print(llvm::outs());
  
  for (llvm::BasicBlock &bb : fun)
    extract(bb);
}

void FeatureAnalysis::finalize()
{
  normalize(*features);
}

ResultFeatureAnalysis FeatureAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  outs() << "function: " << fun.getName().str() << " feature-set: " << features->getName() << " analysis-name: " << getName() << "\n";
  features->reset();
  extract(fun, fam);
  finalize();
  return ResultFeatureAnalysis { features->getFeatureCounts(), features->getFeatureValues() };
}
