#pragma once

#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include "FeatureAnalysis.hpp"
#include "FeatureSet.hpp"

using namespace llvm;

namespace celerity {
/*
/// List of supported feature extraction techniques 
enum class feature_pass_mode { 
    DEFAULT,     // Instruction features of each BB are summed up for the program.
    KOFLER13,   // Instruction features of instructions inside loops have a larger contribution.
    POLFEAT     // Instruction features are progated as polynomial cost relations . More accurate, but requires runtime evaluation.
};
*/


/// An LLVM analysis pass to extract features using [Kofler et al., 13] loop heuristics.
/// The heuristic gives more important (x100) to the features inside a loop.
/// It requires the loop analysis pass ("loops") to be executed before of that pass.
struct Kofler13Analysis : public FeatureAnalysis {
 private:
   const int default_loop_contribution = 100;

 public:
    Kofler13Analysis() : FeatureAnalysis() { analysis_name="kofler13"; }
    virtual ~Kofler13Analysis(){}

    /// overwrite feature extraction for function
    virtual void extract(llvm::Function &fun, llvm::FunctionAnalysisManager &fam);
    // calculate del loop contribution of a given loop (assume non nesting, which is calculated later)
    int loopContribution(const llvm::Loop &loop, ScalarEvolution &SE);
};

} // end namespace celerity
