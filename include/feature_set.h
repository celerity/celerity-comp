#pragma once
#include <string>
#include <iostream>
#include <set>
#include <unordered_map>

#include <llvm/IR/Instructions.h>

using namespace std;

namespace celerity {


/* List of supported feature extraction techniques */
enum class feature_set_mode { 	
    GREWE11,    // Follows Grewe et al. CC 2011 paper. Few feautres mainly based on 
    KOFLER13,   // Features based on the OpenCL langauge features (note: [Kofler et al.13] also had dynamic features).
    GPU,        // Features specifically designed for GPU architecture.
    FULL        // Extended feature representation: one feature for each LLVM IR type. Accurate but hard to cover.
};


/* A set of feature, including both raw values and normalized ones. */
class feature_set {
public:
    std::unordered_map<llvm::Function*, std::unordered_map<string,int>>   raw;  // raw features (instruction count)
    std::unordered_map<llvm::Function*, std::unordered_map<string,float>> feat; // feature after normalization
    std::unordered_map<llvm::Function*, int> instructionNum;
    std::unordered_map<llvm::Function*, int> instructionTotContrib;

    void add(llvm::Function* func, const string &feature_name, int contribution = 1){
        int old = raw[func][feature_name];
        raw[func][feature_name] = old + contribution;
        instructionNum[func] += 1;
        instructionTotContrib[func] += contribution;
    }	
    
    void eval(llvm::Instruction &inst, int contribution = 1) {
        llvm::Function *func = inst.getFunction();
        // test if we need to do init
        if (instructionNum.find(func) == instructionNum.end()) {
            raw[func]  = unordered_map<string,int>();
            feat[func] = unordered_map<string,float>();
            instructionNum[func] = 0;
            instructionTotContrib[func] = 0;
        }
        // handling function calls
        if (llvm::CallInst *call = llvm::dyn_cast<llvm::CallInst>(&inst)) {
            if (func != call->getCalledFunction() && call->getCalledFunction()) {
                // ensure this is not a recursion, which is not supported
                int currInstNum = instructionNum[func];
                unordered_map<string, int> fraw = raw[call->getCalledFunction()];
                for (const auto &kv : fraw) {
                    add(func, kv.first, kv.second);
                }
                instructionNum[func] = currInstNum + instructionNum[call->getCalledFunction()];
            }
        } else {
            add(func, eval_instruction(inst), contribution);
        }
    }

    //virtual float get_feature(string &feature_name){ return feat[feature_name]; }
    virtual void print(std::ostream& = std::cerr);
    virtual void print_to_cout();
    virtual void print_to_file(const string&);
    virtual void normalize();
    virtual ~feature_set(){}
protected:
    /* Abstract method that evaluates an llvm instruction in terms of feature representation. */
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1) = 0;
    virtual string get_type_prefix(const llvm::Instruction &inst);
};


/* Feature set based on Fan's work, specifically designed for GPU architecture. */
class gpu_feature_set : public feature_set {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};

/* Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. */
class grewe11_feature_set : public feature_set {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);
}; 

/* Feature set used by Fan, designed for GPU architecture. */
class full_feature_set : public feature_set {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};


/* Memory address space identifiers, used for feature recognition. */
const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace   = 1;
const unsigned globalAddressSpace  = 2;
enum class AddressSpaceType { Private, Local, Global, Unknown };
AddressSpaceType checkAddrSpace(const unsigned addrSpaceId);


} // end namespace celerity
