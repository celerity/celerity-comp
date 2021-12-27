#pragma once

#include "FeatureSet.h"

namespace celerity {

/* Feature normalization approches */
enum class feature_norm {
	NONE,
	SUM,
	MINMAX_LINEAR,
	MINMAX_LOG
};

// TODO TOFIX XXX so far only simple linear normalization is implemented	             
inline 
void normalize(FeatureSet &fs){ 
    for (const auto& kv : fs.raw) {
        llvm::Function *func = kv.first;
        float instructionContribution = 1.0f / float(fs.instructionTotContrib[func]);
        if(fs.instructionTotContrib[func] == 0) 
            instructionContribution = 0.f; // we don't like NAN
        for(std::pair<std::string, int> entry : fs.raw[func]){
            float instTypeNum = float(entry.second);
            fs.feat[func][entry.first] = instTypeNum * instructionContribution;
        }
    }
}
 
} // end namespace celerity 
