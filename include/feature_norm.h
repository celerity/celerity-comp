#pragma once

namespace celerity {

/* Supported normalization approches */
enum class feature_norm { 
	NONE,
	SUM1,
	MINMAX_LINEAR,
	MINMAX_LOG
};

}