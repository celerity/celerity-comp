#include <unordered_map>
using namespace std;


#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
using namespace llvm;

#include "FeatureAnalysis.hpp"
#include "FeaturePrinter.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;


llvm::AnalysisKey FeatureAnalysis::Key;

FeatureAnalysis::~FeatureAnalysis(){}

void FeatureAnalysis::extract(BasicBlock &bb){
    for(Instruction &i : bb){
        features->eval(i);
    }
}	

void FeatureAnalysis::extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
    for (llvm::BasicBlock &bb : fun) 
        extract(bb);
}

void FeatureAnalysis::finalize(){
    normalize(*features);
}

FeatureAnalysis::Result FeatureAnalysis::run(llvm::Function &fun, llvm::FunctionAnalysisManager &fam){
    std::cout << "function:    " << fun.getName().str() << "\n";
    std::cout << "feature-set: " << features->getName() << "\n";
    extract(fun, fam);
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




//-----------------------------------------------------------------------------
// Pass registration using the new LLVM PassManager
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getFeatureExtractionPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "FeatureAnalysis", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
          // #1 REGISTRATION FOR "opt -passes=print<feature>"
          // Register FeaturePrinterPass so that it can be used when specifying pass pipelines with `-passes=`.
          PB.registerPipelineParsingCallback(
            [&](StringRef Name, FunctionPassManager &FPM, ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "print<feature>") {
                    FPM.addPass(FeaturePrinterPass(llvm::errs()));
                    return true;
                }
                return false;
              });
          // #2 REGISTRATION FOR "-O{1|2|3|s}"
          // Register FeaturePrinterPass as a step of an existing pipeline.
          PB.registerVectorizerStartEPCallback(
              [](llvm::FunctionPassManager &PM, llvm::PassBuilder::OptimizationLevel Level) {
                PM.addPass(FeaturePrinterPass(llvm::errs()));
              });
          // #3 REGISTRATION FOR "FAM.getResult<FeatureAnalysis>(Func)"
          // Register FeatureAnalysis as an analysis pass, so that FeaturePrinterPass can request the results of FeatureAnalysis.
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return FeatureAnalysis(); });
              });
          }
        }; 
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getFeatureExtractionPassPluginInfo();
}
