#pragma once

#include <string>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <cstdint>
using namespace std;

#include <llvm/IR/Instructions.h>

#include "Registry.hpp"

namespace celerity {

// Supported feature sets
enum FeatureSetOptions { fan19, grewe13, full };

/// A set of features, including both raw values and normalized ones. 
/// Abstract class, with different subslasses
class FeatureSet {
public:
    llvm::StringMap<unsigned> raw; // features as counters, before normalization
    llvm::StringMap<float> feat;   // features after normalization, they should not necessarily be the same features as the raw
    int instruction_num;
    int instruction_tot_contrib;
    string name;

public:
    FeatureSet() : name("default"){}
    FeatureSet(string feature_set_name) : name(feature_set_name){}
    virtual ~FeatureSet(){}

    llvm::StringMap<unsigned> getFeatureCounts(){ return raw; }
    llvm::StringMap<float> getFeatureValues(){ return feat; }
    string getName(){ return name; }

    /// set all features to zero
    virtual void reset(){
        for(StringRef rkey : raw.keys())
            raw[rkey] = 0;
        for(StringRef fkey : feat.keys())
            feat[fkey] = 0;
        instruction_num = 0;
        instruction_tot_contrib = 0;
    }

    virtual void add(const string &feature_name, int contribution = 1){
        int old = raw[feature_name];
        raw[feature_name] = old + contribution;
        instruction_num += 1;
        instruction_tot_contrib += contribution;
    }

    virtual void eval(llvm::Instruction &inst, int contribution = 1) = 0;
    virtual void normalize(llvm::Function &fun);
    virtual void print(llvm::raw_ostream &out_stream);     
};


/// Feature set based on Fan's work, specifically designed for GPU architecture. 
class Fan19FeatureSet : public FeatureSet {
 public:
    Fan19FeatureSet() : FeatureSet("fan19"){}
    virtual ~Fan19FeatureSet(){}
    virtual void reset();
    virtual void eval(llvm::Instruction &inst, int contribution = 1);   
     
};

/// Feature set used by Grewe & O'Boyle. It is very generic and mainly designed to catch mem. vs comp. 
class Grewe11FeatureSet : public FeatureSet {
 public:
    Grewe11FeatureSet() : FeatureSet("grewe11"){}
    virtual ~Grewe11FeatureSet(){}
    virtual void reset();
    virtual void eval(llvm::Instruction &inst, int contribution = 1);
    virtual void normalize(llvm::Function &fun);
}; 

/// Feature set used by Fan, designed for GPU architecture. 
class FullFeatureSet : public FeatureSet {
 public:
    FullFeatureSet() : FeatureSet("full"){}
    virtual ~FullFeatureSet(){}
    //virtual void reset(); we are fine the the super class reset()
    virtual void eval(llvm::Instruction &inst, int contribution = 1);
};

/// Registry of feature sets
using FSRegistry = Registry<celerity::FeatureSet*>;


/// Printing utilities
/// Print all feature names in one line
template <typename T>
void print_feature_names(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    stringstream ss;
    for(StringRef &f : keys){
        ss << std::setw(7) << f.str() << " ";
    }
    ss << "\n";
    out_stream << ss.str();
}

/// Print all feature unsigned values in one line
template <typename T>
void print_feature_values(llvm::StringMap<T> &feature_map, llvm::raw_ostream &out_stream){
    auto keys = feature_map.keys();
    stringstream ss;    
    for(StringRef &f : keys){
        if constexpr(std::is_same<float,T>::value){            
            ss << std::setprecision(3) 
               << std::setfill(' ') 
               << std::setw(7);
        }
        else
            ss <<  std::setw(7);  
        ss << feature_map[f] << " ";
    }
    ss << "\n";
    out_stream << ss.str();
}


/// Demangling utilties
string get_demangled_name(const llvm::CallInst &call_inst);


} // end namespace celerity
