#pragma once

#include <string>
#include <iostream>
//#include <set>
//#include <unordered_map>
//#include <map>

#include <llvm/IR/Instructions.h>

using namespace std;

namespace celerity {

/// List of supported feature extraction techniques 
/*
enum class feature_set_mode { 	
    GREWE11,    // Follows Grewe et al. CC 2011 paper. Few feautres mainly based on 
    KOFLER13,   // Features based on the OpenCL langauge features (note: [Kofler et al.13] also had dynamic features).
    FAN19,        // Features specifically designed for GPU architecture.
    FULL        // Extended feature representation: one feature for each LLVM IR type. Accurate but hard to cover.
};
*/

/// A set of feature, including both raw values and normalized ones. 
class FeatureSet {
public:
/* Function is not supported here, but in the pass
    std::unordered_map<llvm::Function*, std::unordered_map<string,int>>   raw;  // raw features (instruction count)
    std::unordered_map<llvm::Function*, std::unordered_map<string,float>> feat; // feature after normalization
    std::unordered_map<llvm::Function*, int> instructionNum;
    std::unordered_map<llvm::Function*, int> instructionTotContrib;
*/
    llvm::StringMap<unsigned> raw;
    llvm::StringMap<float> feat;
    int instruction_num;
    int instruction_tot_contrib;

    llvm::StringMap<float> getFeatureValues(){
        return feat;
    }

    void add(const string &feature_name, int contribution = 1){
        int old = raw[feature_name];
        raw[feature_name] = old + contribution;
        instruction_num += 1;
        instruction_tot_contrib += contribution;
    }

    void eval(llvm::Instruction &inst, int contribution = 1) {
        add(eval_instruction(inst), contribution);
    }

/// NOTE previous version of thi code handles the call invocation case, 
// where a function calls nother one.
 // we shoudl handel this in the upper layer -> pass

    /*void add(llvm::Function* func, const string &feature_name, int contribution = 1){
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
	        for (const auto& kv : fraw) {
	            add(func, kv.first, kv.second);
	        }
		instructionNum[func] = currInstNum + instructionNum[call->getCalledFunction()];
	    }
        } else {
            add(func, eval_instruction(inst), contribution);
        }
    }
    */

    //virtual float get_feature(string &feature_name){ return feat[feature_name]; }
    void print(llvm::raw_ostream &out_stream);    
    //void print_to_file(const string&);
    virtual void normalize();
    virtual ~FeatureSet(){}
protected:
    /* Abstract method that evaluates an llvm instruction in terms of feature representation. */
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1) = 0;
    virtual string get_type_prefix(const llvm::Instruction &inst);
};


/* Feature set based on Fan's work, specifically designed for GPU architecture. */
class Fan19FeatureSet : public FeatureSet {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};

/* Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. */
class Grewe11FeatureSet : public FeatureSet {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);
}; 

/* Feature set used by Fan, designed for GPU architecture. */
class FullFeatureSet : public FeatureSet {
    string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};


/* Memory address space identifiers, used for feature recognition. */
const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace   = 1;
const unsigned globalAddressSpace  = 2;
enum class AddressSpaceType { Private, Local, Global, Unknown };
AddressSpaceType checkAddrSpace(const unsigned addrSpaceId);


/// A registry compainint all supported feature sets
struct FeatureSetRegistry : public llvm::StringMap<FeatureSet*> {
    FeatureSetRegistry(){        
        (*this)["grewe11"] = new Grewe11FeatureSet();
        (*this)["fan19"] = new Fan19FeatureSet();
        (*this)["full"] =  new FullFeatureSet();
        (*this)["default"] = new Fan19FeatureSet();
    }
    ~FeatureSetRegistry(){     
        for(auto key : keys())
            delete (*this)[key];
    }
};

/// Printing functions
template <typename T>
void print_feature(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << f <<": " << feature_map[f] << "\n";
    }
}

template <typename T>
void print_feature_name(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << f;
    }
     out_stream << "\n";
}

template <typename T>
void print_feature_val(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << feature_map[f];
    }
     out_stream << "\n";
}

} // end namespace celerity
