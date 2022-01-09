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

void FeatureAnalysis::finalize(llvm::Function &fun)
{
  //normalize(*features);
  features->normalize(fun);
}

ResultFeatureAnalysis FeatureAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam)
{
  // nicely printing analysis params
  llvm::raw_ostream &debug = outs();
  debug.changeColor(llvm::raw_null_ostream::Colors::YELLOW, true);
  debug << "function: ";
  debug.changeColor(llvm::raw_null_ostream::Colors::WHITE, false);
  debug << fun.getName().str();
  debug.changeColor(llvm::raw_null_ostream::Colors::YELLOW, true);
  debug << " feature-set: ";
  debug.changeColor(llvm::raw_null_ostream::Colors::WHITE, false);
  debug << features->getName();
  debug.changeColor(llvm::raw_null_ostream::Colors::YELLOW, true);
  debug << " analysis-name: ";
  debug.changeColor(llvm::raw_null_ostream::Colors::WHITE, false);
  debug << getName() << "\n";
  
  // reset all feature values
  features->reset();
  // feature extraction
  extract(fun, fam);
  // feature post-processing (e.g., normalization)
  finalize(fun);
  return ResultFeatureAnalysis { features->getFeatureCounts(), features->getFeatureValues() };
}
