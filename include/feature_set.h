#pragma once
#include <string>
#include <iostream>
#include <set>
#include <unordered_map>

#include <llvm/IR/BasicBlock.h>

namespace celerity {

/* A set of feature, including both raw values and normalized ones. */
class feature_set {
 public:
	std::unordered_map<string,int> raw;           // raw features
	std::unordered_map<string,float> features;    // feature after normalization
	int instructionNum;
	feature_eval_mode feature_type = feature_eval_mode::FAN18;       // by default, Fan's method is used
	
	void add(const string &feature_name){
		int old = raw[feature_name];
		raw[feature_name] = old + 1;
	}

	virtual void normalize(){ // XXX so far only simple linear normalization is implemented	             
            float instructionContribution = 1.0f / float(instructionNum);
	    if(instructionNum == 0) instructionContribution = 0.f; // we don't like NAN
            for(std::pair<std::string, int> entry : raw){
              float instNum = float(entry.second);
              features[entry.first] = instNum * instructionContribution;
            }
	}

	virtual float get_feature(string &feature_name){ return features[feature_name]; }
	virtual void print(std::ostream&);
	virtual void print_to_cout();
	virtual void print_to_file(const string&);
};


} // end namespace celerity