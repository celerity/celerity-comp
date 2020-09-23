//
// Created by nmammeri on 08/09/2020.
//

#ifndef COST_RELATION_CREL_KERNEL_H
#define COST_RELATION_CREL_KERNEL_H

#include <string>
#include <iostream>
#include <set>
#include <unordered_map>

#include <llvm/IR/Instructions.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/Analysis/LoopInfo.h>

#include "feature_set.h"

#include "crel_mpoly.h"

namespace celerity {

    // Naming convention:
    //   kX: for kernel arguments and X represent the kernel arg number or index
    //   g0: for get_global_id(0), g1: for get_global_id(1), g2: for get_global_id(2)
    //   l0: for get_local_id(0), g1: for get_local_id(1), g2: for get_local_id(2)

    struct crel_variable {
        std::string name;
        llvm::Value* value;
        long value_index;
        bool is_kernel_arg;
    };

     /**
     * This class encapsulates a kernel function.
     * Contains a map with all kernel features
     * A vector of positional kernel arguments (order preserved)
     *
     */
    class crel_kernel {
    public:
        crel_kernel() = default;
        explicit crel_kernel(llvm::Function* function);
        ~crel_kernel() = default;

        void add (llvm::BasicBlock* bb, const string &feature_name, uint32_t contribution = 1);
        void add (llvm::BasicBlock* bb, const string &feature_name, const crel_mpoly &poly2);

        std::vector<string> getVarNames();


        llvm::Function* function{}; // reference to the llvm function
        int instructionNum{};
        string name;

        // Intermediate features for blocks and loops
        std::unordered_map<const llvm::BasicBlock*, std::unordered_map<string,crel_mpoly>> bbFeatures;
        std::unordered_map<const llvm::Loop*, crel_mpoly> loopMultipliers;

        // Final features for the kernel
        std::unordered_map<string,crel_mpoly> features;
        // A vector of all kernel runtime variables that the mpolys take into consideration
        std::vector<crel_variable> runtime_vars;

    };

}

#endif //COST_RELATION_CREL_KERNEL_H
