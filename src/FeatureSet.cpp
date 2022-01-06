#include <fstream>
#include <string>
#include <set>
using namespace std;

#include <llvm/IR/Instruction.h>

using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureNormalization.hpp"
#include "MemAccessFeature.hpp"
using namespace celerity;


const set<string> INT_ADDSUB   = {"add", "sub"};
const set<string> INT_MUL      = {"mul"};
const set<string> INT_DIV      = {"udiv","sdiv","sdivrem"};
//const set<string> INT_REM    = {"urem","srem"}; // remainder of a division
const set<string> FLOAT_ADDSUB = {"fadd", "fsub"};
const set<string> FLOAT_MUL    = {"fmul"};
const set<string> FLOAT_DIV    = {"fdiv"};
//const set<string> FLOAT_REM  = {"frem"};
//const set<string> CALL         = {"call"}; 
const set<string> FNAME_SPECIAL= {"sqrt", "exp", "log", "abs", "fabs", "max", "pow","floor"};
const set<string> IGNORE_FNAME = {"get_global_id", "get_local_id", "get_num_groups", "get_group_id", "get_max_sub_group_size", "max", "pow"};
const set<string> BITWISE      = {"shl","lshr", "ashr", "and", "or", "xor"};
//const set<string> VECTOR     = {"extractelement","insertelement", "shufflevector"};
//const set<string> AGGREGATE  = {"extractvalue","insertvalue"};
const set<string> INTRINSIC    = {"llvm.fmuladd",  "llvm.canonicalize", "llvm.smul.fix.sat", "‘llvm.umul.fix", "llvm.smul.fix",
                                  "llvm.sqrt", "llvm.powi", "llvm.sin", "llvm.cos", "llvm.pow", "llvm.exp", "llvm.exp2",
                                  "llvm.log", "llvm.log10", "llvm.log2", "llvm.fma", "llvm.fabs","llvm.minnum", "llvm.maxnum",
                                  "llvm.minimum", "llvm.maximum", "llvm.copysign", "llvm.floor", "llvm.ceil", "llvm.trunc",
                                  "llvm.rint", "llvm.nearbyint", "llvm.round", "llvm.lround", "llvm.llround", "llvm.lrint","llvm.llrint"};
const set<string> BARRIER      = {"barrier","sub_group_reduce_add"};
const set<string> CONTROL_FLOW = {"phi","br","brcond","brindirect","brjt"}; 
const set<string> CAST         = {"uitofp","fptosi","sitofp"}; 
const set<string> IGNORE       = {"getelementptr", "alloca", "sext","icmp","zext","trunc","ret"};

static inline bool instr_check(const string &instr_name, const set<string> &instr_set){
    return instr_set.find(instr_name) != instr_set.end();
}

static inline bool instr_start_with(const string &instr_name, const set<string> &instr_set){
    for(const string &s : instr_set){
        if (s.rfind(instr_name, 0) == 0)  
            return true;
    }
    return false;
}


void FeatureSet::print(llvm::raw_ostream &out_stream){
    out_stream << "raw values\n";
    print_features<unsigned>(raw, out_stream);
    out_stream << "feature values\n";
    print_features<float>(feat, out_stream);
}

void FeatureSet::normalize(){
	celerity::normalize(*this);
}


void Fan19FeatureSet::reset(){
    raw["int_add"] = 0;
    raw["int_mul"] = 0;
    raw["int_div"] = 0;
    raw["int_bw"] = 0;
    raw["float_add"] = 0;
    raw["float_mul"] = 0;
    raw["float_div"] = 0;
    raw["sf"] = 0;
    raw["mem_global"] = 0;
    raw["mem_local"] = 0;
}

void Fan19FeatureSet::eval(llvm::Instruction &inst, int contribution){
    string i_name = inst.getOpcodeName();
    if(instr_check(i_name, INT_ADDSUB)){    add("int_add", contribution);   return; }
    if(instr_check(i_name, INT_MUL)){       add("int_mul", contribution);   return; }
    if(instr_check(i_name, INT_DIV)){       add("int_div", contribution);   return; }
    if(instr_check(i_name, BITWISE)){       add("int_bw", contribution);    return; }
    if(instr_check(i_name, FLOAT_ADDSUB)){  add("float_add", contribution); return; }
    if(instr_check(i_name, FLOAT_MUL)){     add("float_mul", contribution); return; }
    if(instr_check(i_name, FLOAT_DIV)){     add("float_div", contribution); return; }
    // special functions
    if (const CallInst *ci = dyn_cast<CallInst>(&inst)){
        Function *func = ci->getCalledFunction();
        // check intrinsic
        if (ci->getIntrinsicID() != Intrinsic::not_intrinsic) {
            string intrinsic_name = Intrinsic::getName(ci->getIntrinsicID()).str();
            if(instr_check(intrinsic_name, INTRINSIC)){     
                add("sf", contribution); 
                return; 
            }
            errs() << "WARNING: fan19: intrinsic " << intrinsic_name << " not recognized\n";
        }
        // handling function calls
        string fun_name = ci->getCalledFunction()->getGlobalIdentifier();
        if(instr_start_with(fun_name, FNAME_SPECIAL))
            add("sf", contribution); 
        else
            errs() << "WARNING: fan19: function " << fun_name << " not recognized\n";
        return; 
    }
    // global & local memory access
    if(const LoadInst *li = dyn_cast<LoadInst>(&inst)) {
        unsigned address_space = li->getPointerAddressSpace();
        if(isLocalMemoryAccess(address_space))
            add("mem_global", contribution); 
        if(isGlobalMemoryAccess(address_space))
            add("mem_local", contribution);
        return;
    } else
    if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        unsigned address_space = si->getPointerAddressSpace();
        if(isLocalMemoryAccess(address_space))
            add("mem_global", contribution); 
        if(isGlobalMemoryAccess(address_space))
            add("mem_local", contribution);
        return;
    }
    // ignore list: control flow
    if(instr_check(i_name, CONTROL_FLOW) || instr_check(i_name, IGNORE) || instr_check(i_name, CAST))
        return;    
    // what about store?
    errs() << "WARNING: fan19: opcode " << i_name << " not recognized\n";
}


void Grewe11FeatureSet::reset(){
    raw["int"]=0;
    raw["int4"]=0;
    raw["float"]=0;
    raw["float4"]=0;
    raw["math"]=0;
    raw["barrier"]=0;
    raw["mem_access"]=0;
    raw["mem_local"]=0;
    //raw["per_local_mem"]=0;
    //raw["per_coalesced"]=0;
    raw["mem_coalesced"]=0;
    //raw["comp_mem_ratio"]=0;
    //raw["data_transfer"]=0;
    //raw["comp_per_data"]=0;
    //raw["workitems"]=0;
}

void Grewe11FeatureSet::eval(llvm::Instruction &inst, int contribution){
    string i_name = inst.getOpcodeName();
    unsigned opcode = inst.getOpcode();    
    outs() << "  OPCODE: " << i_name << "\n";
    // int
    if(instr_check(i_name, INT_ADDSUB)){    add("int", contribution);   return; }
    if(instr_check(i_name, INT_MUL)){       add("int", contribution);   return; }
    if(instr_check(i_name, INT_DIV)){       add("int", contribution);   return; }
    if(instr_check(i_name, BITWISE)){       add("int", contribution);   return; }
    // float
    if(instr_check(i_name, FLOAT_ADDSUB)){  add("float", contribution); return; }
    if(instr_check(i_name, FLOAT_MUL)){     add("float", contribution); return; }
    if(instr_check(i_name, FLOAT_DIV)){     add("float", contribution); return; }
    // math (similar to Fan's special functions)
    if (const CallInst *ci = dyn_cast<CallInst>(&inst)){
        Function *func = ci->getCalledFunction();
        // check intrinsic
        if (ci->getIntrinsicID() != Intrinsic::not_intrinsic) {
            string intrinsic_name = Intrinsic::getName(ci->getIntrinsicID()).str();
            if(instr_check(intrinsic_name, INTRINSIC)){     
                add("math", contribution); 
                return; 
            }
            errs() << "WARNING: fan19: intrinsic " << intrinsic_name << " not recognized\n";
        }
        // handling function calls
        string fun_name = ci->getCalledFunction()->getGlobalIdentifier();
        
        if(instr_start_with(fun_name, FNAME_SPECIAL)) // math
            add("math", contribution); 
        else if(instr_start_with(fun_name, BARRIER)) // barrier
            add("barrier", contribution);
        else
            errs() << "WARNING: fan19: function " << fun_name << " not recognized\n";
    }
    // mem access
    if(const LoadInst *li = dyn_cast<LoadInst>(&inst)) {
        add("mem_access", contribution);
        unsigned address_space = li->getPointerAddressSpace();          
        if(isLocalMemoryAccess(address_space))
            add("mem_local", contribution);
        return;
    }
    // local mem access
    else if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        add("mem_access", contribution);
        unsigned address_space = si->getPointerAddressSpace();          
        if(isLocalMemoryAccess(address_space))
            add("mem_local", contribution);
        return;
    }
    
    // int4 TODO
    // float4 TODO
}


void FullFeatureSet::eval(llvm::Instruction &inst, int contribution){         
    add(inst.getOpcodeName(), contribution);
}


//-----------------------------------------------------------------------------
// Register the available feature sets in the FeatureSet registry
//-----------------------------------------------------------------------------
static celerity::FeatureSet* _static_fs_1_ = new celerity::Fan19FeatureSet();
static bool _registered_fset_1_ = FSRegistry::registerByKey("fan19", _static_fs_1_ ); 
static celerity::FeatureSet* _static_fs_2_ = new celerity::Grewe11FeatureSet();
static bool _registered_fset_2_ = FSRegistry::registerByKey("grewe11", _static_fs_2_ ); 
static celerity::FeatureSet* _static_fs_3_ = new celerity::FullFeatureSet();
static bool _registered_fset_3_ = FSRegistry::registerByKey("full", _static_fs_3_ ); 

