#pragma once
#include <string>
#include <iostream>
#include <set>
#include <unordered_map>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/Analysis/LoopInfo.h>

#include "feature_set.h"

#include "crel_mpoly.h"
#include "crel_kernel.h"


using namespace std;

namespace celerity {

/* A set of feature, including both raw values and normalized ones. */
class crel_feature_set {

public:
    vector<string> feat_names; // a vector of all feature names
    std::unordered_map<string, crel_kernel>  kernels;  // raw features per kernel

    crel_feature_set() {

    };

    void add(llvm::Function* func, llvm::BasicBlock* bb, const string &feature_name, uint32_t contribution = 1) {
        // Add a constant to our feature
        kernels[func->getGlobalIdentifier()].add(bb, feature_name, contribution);
    }

    void add(llvm::Function* func, llvm::BasicBlock* bb, const string &feature_name, const crel_mpoly poly2) {
        kernels[func->getGlobalIdentifier()].add(bb, feature_name, poly2); // Add a constant to our feature
    }
    
    void eval(llvm::Instruction &inst, uint32_t contribution = 1) {
        llvm::Function * func = inst.getFunction();
        llvm::BasicBlock* bb = inst.getParent();

        // Add the feature only if the returned string is not empty
        auto feature = eval_instruction(inst);
        if (!feature.empty())
            add(func, bb, feature, contribution);
    }

    //virtual float get_feature(string &feature_name){ return feat[feature_name]; }
    virtual void print(std::ostream&);
    virtual void print_to_cout();
    virtual void print_csv(std::ostream&);
    virtual void print_to_file(const string&);
    virtual ~crel_feature_set()= default;
protected:
    /* Abstract method that evaluates an llvm instruction in terms of feature representation. */
    virtual string eval_instruction(const llvm::Instruction &inst) = 0;
    virtual string get_type_prefix(const llvm::Instruction &inst);

};


/* Feature set based on Fan's work, specifically designed for GPU architecture. */
class poly_gpu_feature_set : public crel_feature_set {
public:
    poly_gpu_feature_set();
    string eval_instruction(const llvm::Instruction &inst) override;
};

/* Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. */
class poly_grewe11_feature_set : public crel_feature_set {
public:
    poly_grewe11_feature_set();
    string eval_instruction(const llvm::Instruction &inst) override;
};

/* Feature set used by Fan, designed for GPU architecture. */
class poly_full_feature_set : public crel_feature_set {
public:
    poly_full_feature_set();
    string eval_instruction(const llvm::Instruction &inst) override;
};


} // end namespace celerity
