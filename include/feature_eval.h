#pragma once

#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Pass.h>


namespace celerity {

/* List of supported feature extraction techniques */
enum class feature_eval_mode { 
	RAW, 	      // Absolute values, not normalized. 
	GREWE11,      // Follows Grewe et al. CC 2011 paper. No loop, only sum of BB values 
	KOFLER13,     // Follows Kofler et al. ICS 2013 paper. Loop use the "unroll100" heuristic 
	FAN18,        // An extension of Grewe et al. with more features, designed to
	COST_RELATION // Advanced representation where loop count are propagated in a cost relation feature form. It requires runtime feature evaluation, but is the most accurate.
};


/* Memory address space identifier, used for feature recognition. */
const unsigned privateAddressSpace = 0;
const unsigned localAddressSpace   = 1;
const unsigned globalAddressSpace  = 2;

/* An LLVM pass to extract features. */
class feature_eval : public llvm::FunctionPass {
 public:
	static char ID;
	FeatureEval() : llvm::FunctionPass(ID) {}

	virtual void evalInstruction(const llvm::Instruction &inst, FeatureSet &feat) = 0;
	virtual void evalBB(const llvm::BasicBlock &bb, FeatureSet &feat) = 0;	
	virtual void evalFunction(const llvm::Function &fun, FeatureSet &feat){
	  for (const llvm::BasicBlock &bb : fun) 
	    evalBB(bb, feat);
	}

};


/* Utility function to check the address space of a buffer */
bool checkAddrSpace(const unsigned addrSpaceId, FeatureSet &feat);


/* Feature evaluation schema used by Fan et al. in XXX */
class Fan18FeatureEval : public FeatureEval {
 public:
	FeatureSet features;
	
	Fan18FeatureEval() : FeatureEval() {}
	// override FeatureEval
	void evalInstruction(const llvm::Instruction &inst, FeatureSet &feat);
	void evalBB(const llvm::BasicBlock &bb, FeatureSet &feat);	
	// overrides FunctionPass
	bool runOnFunction(llvm::Function &F);

	void finalize();

	// List of LLVM instruction mapped into a specific feature
	const set<string> BIN_OPS = {"add","fadd", "sub", "fsub", "mul", "fmul", "udiv", "sdiv", "fdiv", "urem", "srem", "frem"};//rem- remainder of a division by...
	const set<string> BIN_OPS_INT_ADDSUB = {"add", "sub"};
	const set<string> BIN_OPS_INT_MUL = {"mul"};
	const set<string> BIN_OPS_INT_DIV = {"udiv","sdiv"};
	const set<string> BIN_OPS_INT_REM = {"urem","srem"};
	const set<string> BIN_OPS_FLOAT_ADDSUB = {"fadd", "fsub"};
	const set<string> BIN_OPS_FLOAT_MUL = {"fmul"};
	const set<string> BIN_OPS_FLOAT_DIV = {"fdiv"};
	const set<string> BIN_OPS_FLOAT_REM = {"frem"};
	const set<string> BIN_OPS_SPECIAL = {"call"};
	const set<string> BITBIN_OPS = {"shl","lshr", "ashr", "and", "or", "xor"};
	const set<string> VEC_OPS = {"extractelement","insertelement", "shufflevector"};
	const set<string> AGG_OPS = {"extractvalue","insertvalue"};
};



} // end namespace celerity