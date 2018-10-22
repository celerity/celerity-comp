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
	std::unordered_map<string,int> raw;       // raw features (instruction count)
	std::unordered_map<string,float> feat;    // feature after normalization
	int instructionNum	= 0;
	
	void add(const string &feature_name, int contribution = 1){
		int old = raw[feature_name];
		raw[feature_name] = old + contribution;
        instructionNum++;
	}	
    
    /* Abstract method that evaluates an llvm instruction in terms of feature representation. */
    virtual void eval_instruction(const llvm::Instruction &inst, int contribution = 1) = 0;        

    virtual float get_feature(string &feature_name){ return feat[feature_name]; }
	virtual void print(std::ostream&);
	virtual void print_to_cout();
	virtual void print_to_file(const string&);
    virtual void normalize();
};


/* Feature set based on Fan's work, specifically designed for GPU architecture. */
class gpu_feature_set : public feature_set {
    void eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};

/* Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. */
class grewe11_feature_set : public feature_set {
    void eval_instruction(const llvm::Instruction &inst, int contribution = 1);
}; 

/* Feature set used by Fan, designed for GPU architecture. */
class full_feature_set : public feature_set {
    void eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};


/* Memory address space identifiers, used for feature recognition. */
const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace   = 1;
const unsigned globalAddressSpace  = 2;
enum class AddressSpaceType { Private, Local, Global, Unknown };
AddressSpaceType checkAddrSpace(const unsigned addrSpaceId);


} // end namespace celerity
