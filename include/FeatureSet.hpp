#pragma once

#include <string>
#include <iostream>
using namespace std;

#include <llvm/IR/Instructions.h>

#include "Registry.hpp"

namespace celerity {

// Supported feature sets
enum FeatureSetOptions { fan19, grewe13, full };

/// A set of feature, including both raw values and normalized ones. 
/// Abstract class, with different subslasses
class FeatureSet {
public:
    llvm::StringMap<unsigned> raw;
    llvm::StringMap<float> feat;
    int instruction_num;
    int instruction_tot_contrib;
    string name;

public:
    FeatureSet() : name("default"){}
    FeatureSet(string feature_set_name) : name(feature_set_name){}
    virtual ~FeatureSet();

    llvm::StringMap<float> getFeatureValues(){
        return feat;
    }

    string getName(){
        return name;
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
    string get_type_prefix(const llvm::Instruction &inst);
    void print(llvm::raw_ostream &out_stream);    
    //void print_to_file(const string&);
    void normalize();

protected:
    /* Abstract method that evaluates an llvm instruction in terms of feature representation. */
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1) = 0;
    
};


/* Feature set based on Fan's work, specifically designed for GPU architecture. */
class Fan19FeatureSet : public FeatureSet {
 public:
    Fan19FeatureSet() : FeatureSet("fan19"){}
    virtual ~Fan19FeatureSet();
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};

/* Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. */
class Grewe11FeatureSet : public FeatureSet {
 public:
    Grewe11FeatureSet() : FeatureSet("grewe13"){}
    virtual ~Grewe11FeatureSet();
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1);
}; 

/* Feature set used by Fan, designed for GPU architecture. */
class FullFeatureSet : public FeatureSet {
 public:
    FullFeatureSet() : FeatureSet("full"){}
    virtual ~FullFeatureSet();
    virtual string eval_instruction(const llvm::Instruction &inst, int contribution = 1);    
};


/* Memory address space identifiers, used for feature recognition. */
const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace   = 1;
const unsigned globalAddressSpace  = 2;
enum class AddressSpaceType { Private, Local, Global, Unknown };
AddressSpaceType checkAddrSpace(const unsigned addrSpaceId);

/*
template <typename T>
struct Registry : public llvm::StringMap<T*> {
    Registry(){}
    ~Registry(){}    
 public:
    Registry(StaticRegistry const&) = delete;
    void operator=(StaticRegistry const&) = delete;
    
    static StaticRegistry& getInstance() {
        static StaticRegistry instance; // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    T get(const llvm::String &name){
        return this->operator[name];
    }
  

}; */

/*
/// A registry containing all supported feature sets. Singleton struct
struct FeatureSetRegistry : public llvm::StringMap<FeatureSet*> {
 private:
    FeatureSetRegistry(){        
        (*this)["grewe11"] = new Grewe11FeatureSet();
        (*this)["fan19"] = new Fan19FeatureSet();
        (*this)["full"] =  new FullFeatureSet();
    }

    ~FeatureSetRegistry(){     
        for(auto key : keys())
            delete (*this)[key];
    }

 public:
    FeatureSetRegistry(FeatureSetRegistry const&) = delete;
    void operator=(FeatureSetRegistry const&) = delete;
    
    static FeatureSetRegistry& getInstance() {
        static FeatureSetRegistry instance; // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }
};
*/

using FSRegistry = Registry<celerity::FeatureSet*>;

/// Printing functions

/// Print all features in a nicely formatted table
template <typename T>
void print_features(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << "  " << f <<": " << feature_map[f] << "\n";
    }
}


/// Print all feature names in one line
template <typename T>
void print_feature_names(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << f << "  ";
    }
     out_stream << "\n";
}


/// Print all feature values in one line
template <typename T>
void print_feature_values(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    for(auto f : keys){
        out_stream << feature_map[f] << "  ";
    }
     out_stream << "\n";
}


} // end namespace celerity
