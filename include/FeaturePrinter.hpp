#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/PassManager.h>
using namespace llvm;

#include "FeatureSet.hpp"


namespace celerity {

/// Pass that print the results of a FeatureExtractionPass
struct FeaturePrinterPass : public llvm::PassInfoMixin<FeaturePrinterPass> {
 public:
    explicit FeaturePrinterPass(llvm::raw_ostream &stream) : out_stream(stream) {}

    llvm::PreservedAnalyses run(llvm::Function &func, llvm::FunctionAnalysisManager &fam);

    static bool isRequired() { return true; }

 private:
    llvm::raw_ostream &out_stream;
};

} // end namespace celerity
