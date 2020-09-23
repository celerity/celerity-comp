//
// Created by nmammeri on 08/09/2020.
//

#include "crel_kernel.h"

using namespace celerity;

// Constructor
crel_kernel::crel_kernel(llvm::Function* function) : function(function), instructionNum(0), name(function->getGlobalIdentifier()) {

    // Total number of runtime variables is:
    // + 3: g0, g1, g2 (global id)
    // + 3: l0, l1, l2 (local id)
    // + 3: s0, s1, s2 (global size)
    // + 3: z0, z1, z2 (local size)
    // + kn : n number of kernel args

    // We need to loop through all arguments of this function
   for (uint32_t i =0; i< function->arg_size(); i++) {
       // Create the runtime variable and add it to the list
       string varName = "k"+to_string(i);
       crel_variable var = {varName, function->getArg(i), i, true};
       runtime_vars.push_back(var);

   }

   // Check if we have calls to get_global_id, get_local_id, get_global_size, get_local_size
    for (llvm::BasicBlock &bb : function->getBasicBlockList()) {
        // Go through all instructions of thi BasicBlock
        for (llvm::Instruction &inst : bb) {

            if (auto *ci = llvm::dyn_cast<llvm::CallInst>(&inst)) {
                // handling function calls
                llvm::Function *func = ci->getCalledFunction();
                string calledFuncName = func->getGlobalIdentifier();

                // Check if the function name contains: get_global_id
                std::size_t found_get_global_id = calledFuncName.find("get_global_id");
                std::size_t found_get_local_id = calledFuncName.find("get_local_id");
                std::size_t found_get_global_size = calledFuncName.find("get_global_size");
                std::size_t found_get_local_size = calledFuncName.find("get_local_size");

                // If any of these calls was found
                if (found_get_global_id!=std::string::npos || found_get_local_id!=std::string::npos ||
                    found_get_global_size!=std::string::npos || found_get_local_size!=std::string::npos) {

                    auto *arg = llvm::dyn_cast<llvm::ConstantInt>(ci->getOperand(0));
                    auto *ciValue = llvm::dyn_cast<llvm::Value>(ci);
                    auto argConstantInt = arg->getValue().getSExtValue();

                    // Add to runtime variables
                    if (found_get_global_id!=std::string::npos) {
                        crel_variable var = { "g"+to_string(argConstantInt), ciValue, argConstantInt, false};
                        runtime_vars.push_back(var);

                    } else if (found_get_local_id!=std::string::npos) {
                        crel_variable var = { "l"+to_string(argConstantInt), ciValue, argConstantInt, false};
                        runtime_vars.push_back(var);

                    } else if (found_get_global_size!=std::string::npos) {
                        crel_variable var = { "s"+to_string(argConstantInt), ciValue, argConstantInt, false};
                        runtime_vars.push_back(var);

                    } else if (found_get_local_size!=std::string::npos) {
                        crel_variable var = { "z"+to_string(argConstantInt), ciValue, argConstantInt, false};
                        runtime_vars.push_back(var);
                    }
                }

            }
        }
    }

   for (const auto& var: runtime_vars) {
       cout << " var: " << var.name << " value: " << var.value << " arg_index: " << var.value_index << endl;
   }
   cout << endl;

}

void crel_kernel::add (llvm::BasicBlock* bb, const string &feature_name, uint32_t contribution) {
    // Add a constant to our feature
    bbFeatures[bb][feature_name].add(contribution);
    // Increase number of instructions
    instructionNum++;
}

void crel_kernel::add (llvm::BasicBlock* bb, const string &feature_name, const crel_mpoly &poly2) {
    // Add a constant to our feature
    bbFeatures[bb][feature_name].add(poly2);
    // Increase number of instructions
    instructionNum++;
}

vector<string> crel_kernel::getVarNames() {

    vector<string> runtime_vars_names;

    for(auto &runtime_var : runtime_vars)
        runtime_vars_names.push_back(runtime_var.name);

    return runtime_vars_names;
}
