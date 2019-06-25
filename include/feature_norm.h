#pragma once
#include "feature_set.h"

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
void normalize(feature_set &fs){ 
    float instructionContribution = 1.0f / float(fs.instructionTotContrib);
    if(fs.instructionTotContrib == 0) 
        instructionContribution = 0.f; // we don't like NAN
    for(std::pair<std::string, int> entry : fs.raw){
        float instTypeNum = float(entry.second);
        fs.feat[entry.first] = instTypeNum * instructionContribution;
    }
}
 
} // end namespace celerity 