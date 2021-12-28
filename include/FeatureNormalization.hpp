#pragma once

#include "FeatureSet.hpp"

namespace celerity {

/* Feature normalization approches 
enum class feature_norm {
	NONE,
	SUM,
	MINMAX_LINEAR,
	MINMAX_LOG
};
*/

// TODO TOFIX XXX so far only simple linear normalization is implemented	              
inline
void normalize(FeatureSet &fs){     
    float instructionContribution = 1.0f / float(fs.instruction_tot_contrib);
    if(fs.instruction_tot_contrib == 0) 
        instructionContribution = 0.f; // we don't like NAN
        
    //for(auto entry : fs.raw){
    //for(llvm::StringMap<unsigned>::const_iterator it = fs.raw.begin();
    //    it != fs.raw.end();
    //    it++)
    auto f_names = fs.raw.keys();
    for(auto feature_name : f_names)
    {
        //for(std::pair<std::string, unsigned> entry : fs.raw){            
        //const string feature_name = it->first; 
        //float instTypeNum = float(it->second);
        int inst_int_val = fs.raw[feature_name];
        float inst_flt_val = float(inst_int_val);
        fs.feat[feature_name] = inst_flt_val * instructionContribution;
    }
}

 
} // end namespace celerity 
