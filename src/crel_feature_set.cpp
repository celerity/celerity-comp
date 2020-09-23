#include <fstream>
#include <string>

#include <llvm/IR/Intrinsics.h>

#include <llvm/Support/MemoryBuffer.h>

#include "crel_feature_set.h"
#include "crel_llvm_helper.h"

using namespace celerity;
using namespace llvm;
using namespace std;

void crel_feature_set::print(ostream &os) {
    for (const auto& kv : kernels) {
        string funcID = kv.first;
        if (!kernels[funcID].features.empty()) {
            os << "Features for kernel-function: " << funcID << endl;

            // Get all feature names and sort them
            vector<string> feat_names;
            feat_names.reserve(kernels[funcID].features.size());
            for(auto el : kernels[funcID].features)
                feat_names.push_back(el.first);
            std::sort(feat_names.begin(),feat_names.end());

            // print in alphabetical order
            os << "name           raw" << endl;
            for(string &name : feat_names) {
                string p_name = name;
                string p_raw  = kernels[funcID].features[name].toString(kernels[funcID].getVarNames());
                p_name.resize(15,' ');
                //p_raw.resize(200,' ');
                os << p_name << p_raw << endl;
            }
            os << endl;
        }
    }
}

void crel_feature_set::print_to_cout(){
    crel_feature_set::print(cout);
}

void crel_feature_set::print_to_file(const string &out_file){
	cout << "Writing to file: " << out_file << endl;
	ofstream outstream;
	outstream.open (out_file);
	print(outstream);
	outstream.close();
}

string crel_feature_set::get_type_prefix(const llvm::Instruction &inst) {
    Type *t = inst.getType();
    if (t->isHalfTy()) {
        return "f16.";
    } else if (t->isFloatTy()) {
        return "f32.";
    } else if (t->isDoubleTy()) {
        return "f64.";
    } /*else if (t->isIntegerTy()) {
        return "i" + to_string(t->getIntegerBitWidth()) + ".";
    }*/
    return "";
}


// List of LLVM instructions mapped into a specific feature
const set<string> BIN_OPS = {"add","fadd", "sub", "fsub", "mul", "fmul", "udiv", "sdiv", "fdiv", "urem", "srem", "frem"};//rem- remainder of a division by...
const set<string> INT_ADDSUB = {"add", "sub"};
const set<string> INT_MUL = {"mul"};
const set<string> INT_DIV = {"udiv","sdiv"};
const set<string> INT_REM = {"urem","srem"}; // remainder of a division
const set<string> FLOAT_ADDSUB = {"fadd", "fsub"};
const set<string> FLOAT_MUL = {"fmul"};
const set<string> FLOAT_DIV = {"fdiv"};
const set<string> FLOAT_REM = {"frem"};
const set<string> SPECIAL = {"call"};
const set<string> BITWISE = {"shl","lshr", "ashr", "and", "or", "xor"};
const set<string> VECTOR = {"extractelement","insertelement", "shufflevector"};
const set<string> AGGREGATE = {"extractvalue","insertvalue"};

// Bins used by Grewe featureset
// getelementptr performs address calculation only and be considered a comp instruction
// ‘fsub‘ instruction is used to represent the ‘fneg‘ instruction present in most other intermediate representations.
const set<string> COMP_SET = {"getelementptr", "add","fadd", "sub", "fsub", "fneg", "mul", "fmul", "udiv", "sdiv", "fdiv", "urem", "srem", "frem"};//rem- remainder of a division by...
const set<string> COMP_INTINSICS = {"llvm.fmuladd",  "llvm.canonicalize",
                                    "llvm.smul.fix.sat", "‘llvm.umul.fix", "llvm.smul.fix",
                                    "llvm.sqrt", "llvm.powi", "llvm.sin", "llvm.cos", "llvm.pow", "llvm.exp", "llvm.exp2",
                                    "llvm.log", "llvm.log10", "llvm.log2", "llvm.fma", "llvm.fabs","llvm.minnum", "llvm.maxnum",
                                    "llvm.minimum", "llvm.maximum", "llvm.copysign", "llvm.floor", "llvm.ceil", "llvm.trunc",
                                    "llvm.rint", "llvm.nearbyint", "llvm.round", "llvm.lround", "llvm.llround", "llvm.lrint","llvm.llrint",
};

const set<string> RATIONAL_SET = {"icmp",  "fcmp"};
// https://llvm.org/docs/LangRef.html#memory-access-and-addressing-operations
const set<string> ATOMIC_SET = {"cmpxchg", "atomicrmw"};
// Ignore set
// Casting inst: trunc, bitcast, sitofp
// Zero and sign extending: zext, sext
// Allocate memory on stack: alloca
// Phi instructions: phi
const set<string> IGNORE_SET = {"ret",  "br", "bitcast", "trunc", "sitofp", "zext", "sext", "alloca", "phi"};

static inline bool instr_check(const set<string> &instr_set, const string &instr_name){
    return instr_set.find(instr_name) != instr_set.end();
}

/**
 * Implements gpu_feature_set
 */

poly_gpu_feature_set::poly_gpu_feature_set() {
    feat_names.assign({"int_addsub", "int_mul", "int_div", "int_rem", "addsub", "mul", "div", "rem", "call",
                       "bitwise", "aggregate", "vector", "load", "store", "other"});
}

string poly_gpu_feature_set::eval_instruction(const llvm::Instruction &inst){
    string i_name = inst.getOpcodeName();
    if(instr_check(BIN_OPS,i_name)) {        
        if(instr_check(INT_ADDSUB, i_name)){
            return "int_addsub";
        }
        else if(instr_check(INT_MUL, i_name)){        
            return "int_mul";
        }
        else if(instr_check(INT_DIV, i_name)){        
            return "int_div";
        }
        else if(instr_check(INT_REM, i_name)){        
            return "int_rem";
        }
        else if(instr_check(FLOAT_ADDSUB, i_name)){        
            return get_type_prefix(inst) + "addsub";
        }
        else if(instr_check(FLOAT_MUL, i_name)){        
            return get_type_prefix(inst) + "mul";
        }
        else if(instr_check(FLOAT_DIV, i_name)){        
            return get_type_prefix(inst) + "div";
        }
        else if(instr_check(FLOAT_REM, i_name)){        
            return get_type_prefix(inst) + "rem";
        }
        else if(instr_check(SPECIAL, i_name)){        
            return get_type_prefix(inst) + "call";
        }
    }
    else if(instr_check(BITWISE, i_name)){    
        return "bitwise";
    }
    else if(instr_check(AGGREGATE, i_name)){    
        return "aggregate";
    }
    else if(instr_check(VECTOR, i_name)){    
        return "vector";
    }     
    else if(const auto *li = dyn_cast<LoadInst>(&inst)) {
        return "load";
        // getOpenclAddrSpaceType(li->getPointerAddressSpace());
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    }
    else if (const auto *si = dyn_cast<StoreInst>(&inst)) {
        return "store";
        // getOpenclAddrSpaceType(si->getPointerAddressSpace());
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    }
    return "other";
}

/**
 * Implements full_feature_set
 * adds all types of instructions as features
 */
poly_full_feature_set::poly_full_feature_set() {
    feat_names.assign({"add","fadd", "sub", "fsub", "mul", "fmul", "udiv", "sdiv", "fdiv", "urem", "srem", "frem"});
}
string poly_full_feature_set::eval_instruction(const llvm::Instruction &inst){
    string i_name = inst.getOpcodeName();
    if(instr_check(BIN_OPS,i_name)) {
        return get_type_prefix(inst) + i_name;
    }
    return i_name;
}

/**
 * Implements grewe_feature_set
 */
poly_grewe11_feature_set::poly_grewe11_feature_set() {
    feat_names.assign({"comp", "rational", "mem", "localmem", "coalesced", "atomic"});
}
string poly_grewe11_feature_set::eval_instruction(const llvm::Instruction &inst) {
    string i_name = inst.getOpcodeName();

    // Grewe features don't differentiate between loads and stores
    // mem: are global load & stores
    // localmem are local load & stores
    if (auto *li = dyn_cast<LoadInst>(&inst)) {
        if (getOpenclAddrSpaceType(li->getPointerAddressSpace()) == OpenclAddressSpaceType::Constant ||
            getOpenclAddrSpaceType(li->getPointerAddressSpace()) == OpenclAddressSpaceType::Global) {
            return "mem";
        } else if (getOpenclAddrSpaceType(li->getPointerAddressSpace()) == OpenclAddressSpaceType::Local) {
            return "localmem";
        }
    } else if (auto *si = dyn_cast<StoreInst>(&inst)) {
        if (getOpenclAddrSpaceType(si->getPointerAddressSpace()) == OpenclAddressSpaceType::Constant ||
            getOpenclAddrSpaceType(si->getPointerAddressSpace()) == OpenclAddressSpaceType::Global) {
            return "mem";
        } else if (getOpenclAddrSpaceType(si->getPointerAddressSpace()) == OpenclAddressSpaceType::Local) {
            return "localmem";
        }
    // Comp instr
    } else if (instr_check(COMP_SET, i_name) || instr_check(BITWISE, i_name)) {
        return "comp";

    // Compare instr
    } else if (instr_check(RATIONAL_SET, i_name)) {
        return "rational";

        // Atomic instr
    } else if (instr_check(ATOMIC_SET, i_name)) {
        return "atomic";

    // We need to also handle llvm intrinsic functions
    } else if (instr_check(IGNORE_SET, i_name)) {
        return "";

    // Handling functions call
    } else if (const CallInst *ci = dyn_cast<CallInst>(&inst)) {

        // Check if this is an intrinsic call
        if ( ci->getIntrinsicID() != Intrinsic::not_intrinsic) {
            // Count this intrinsic as a comp instruction if it belongs the COMP_INTINSICS set
            if (instr_check(COMP_INTINSICS, Intrinsic::getName(ci->getIntrinsicID()).str())) {
                //std::cout << "intrinsic call --> " << inst.getFunction()->getGlobalIdentifier() << " " << Intrinsic::getName(ci->getIntrinsicID()).str() << endl;
                return "comp";
            }

        } else {
            // handling function calls
            llvm::Function *func = ci->getCalledFunction();
            string calledFuncName = func->getGlobalIdentifier();

            // Check if we have calls to get_global_id, get_local_id, get_global_size, get_local_size
            std::size_t found_get_global_id = calledFuncName.find("get_global_id");
            std::size_t found_get_local_id = calledFuncName.find("get_local_id");
            std::size_t found_get_global_size = calledFuncName.find("get_global_size");
            std::size_t found_get_local_size = calledFuncName.find("get_local_size");

            // If any of these calls was found
            if (found_get_global_id!=std::string::npos || found_get_local_id!=std::string::npos ||
                found_get_global_size!=std::string::npos || found_get_local_size!=std::string::npos) {
                //std::cout << "found get_global_id function call --> " << inst.getFunction()->getGlobalIdentifier() << " " << func->getGlobalIdentifier() << endl;
            } else {
                std::cerr << "WARNINIG: found non-kernel function call --> " << inst.getFunction()->getGlobalIdentifier() << " " << func->getGlobalIdentifier() << endl;
            }

        }
        // By default return nothing
        return "";

    } else {
        return i_name;
    }

    // By default return nothing
    return "";
}