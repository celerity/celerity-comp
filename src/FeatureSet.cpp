#include <fstream>
#include <string>
#include <set>
using namespace std;

#include <llvm/Support/MemoryBuffer.h>
using namespace llvm;

#include "FeatureSet.hpp"
#include "FeatureNormalization.hpp"
using namespace celerity;


FeatureSet::~FeatureSet(){}

void FeatureSet::print(llvm::raw_ostream &out_stream){
    out_stream << "raw values\n";
    print_features<unsigned>(raw, out_stream);
    out_stream << "feature values\n";
    print_features<float>(feat, out_stream);
}

/* this version handles different Function 
void FeatureSet::print(llvm::raw_ostream &os)){
    for (const auto& kv : raw) {
        Function *func = kv.first;
        if (!raw[func].empty()) {
            os << "Features for function: " << (func->hasName() ? func->getName().str() : "(anonymous)") << endl;
            vector<string> feat_names;
            feat_names.reserve(feat[func].size());
            for(std::pair<string,float> el : feat[func])	
                feat_names.push_back(el.first);	
            std::sort(feat_names.begin(),feat_names.end());
            // print in alphabetical order
            os << "name           raw       feat" << endl;
            for(string &name : feat_names){    
                string p_name = name;
                string p_raw  = std::to_string(raw[func][name]);
                string p_feat = std::to_string(feat[func][name]);
                p_name.resize(15,' ');
                p_raw.resize(10,' ');
                p_feat.resize(10,' ');
                os << p_name << p_raw << p_feat << endl;	
            }
            os << endl;
        }
    }
}*/

/*
void FeatureSet::print_to_file(const string &out_file){
	cout << "Writing to file: " << out_file << endl;
	ofstream outstream;
	outstream.open (out_file);
	print(outstream);
	outstream.close();
}
*/

void FeatureSet::normalize(){
	celerity::normalize(*this);
}


string FeatureSet::get_type_prefix(const llvm::Instruction &inst) {
    Type *t = inst.getType();
    if (t->isHalfTy()) {
        return "f16.";
    } else if (t->isFloatTy()) {
        return "f32.";
    } else if (t->isDoubleTy()) {
        return "f64.";
    } 
    //else if (t->isIntegerTy()) {
    //    return "i" + to_string(t->getIntegerBitWidth()) + ".";
    //}
    return "";
}


Fan19FeatureSet::~Fan19FeatureSet(){}

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

static inline bool instr_check(const set<string> &instr_set, const string &instr_name){
    return instr_set.find(instr_name) != instr_set.end();
}

string Fan19FeatureSet::eval_instruction(const llvm::Instruction &inst, int contribution){
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
    else if(const LoadInst *li = dyn_cast<LoadInst>(&inst)) {
        return "load";
        // checkAddrSpace(li->getPointerAddressSpace()); 
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    }
    else if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        return "store";
        // checkAddrSpace(si->getPointerAddressSpace()); 
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    }
    return "other";
}


Grewe11FeatureSet::~Grewe11FeatureSet(){}

string Grewe11FeatureSet::eval_instruction(const llvm::Instruction &inst, int contribution){
    // TODO FIXME XXX Nadjib
    // implementation missing
    return "";
}


FullFeatureSet::~FullFeatureSet(){}

string FullFeatureSet::eval_instruction(const llvm::Instruction &inst, int contribution){    
    string i_name = inst.getOpcodeName();
    if(instr_check(BIN_OPS,i_name)) {
        return get_type_prefix(inst) + i_name;
    }
    return i_name;
}


AddressSpaceType celerity::checkAddrSpace(const unsigned addrSpaceId) {
    /*
    From AMD's backend: 
    https://llvm.org/docs/AMDGPUUsage.html#amdgpu-address-spaces
    https://llvm.org/docs/AMDGPUUsage.html#address-space-mapping
    we have the following address space definition:
      Address Space - Memory Space
                  1 - Private (Scratch)
                  2 - Local (group/LDS)
                  3 - Global
                 nd - Constant
                 nd - Generic (Flat)
                 nd - Region (GDS)
    Note that some bakends are currently implementing only part of them. 
    We return Global (3) for the the other the currently unmapped address space (constant, generic, region).
    */
    if(addrSpaceId == localAddressSpace) {
        return AddressSpaceType::Local;
    } else if(addrSpaceId == globalAddressSpace) {
        return AddressSpaceType::Global;
    } else if(addrSpaceId == privateAddressSpace) {
        return AddressSpaceType::Private;
    } else {
        //std::cerr << "WARNING: unkwnown address space id: " << addrSpaceId << std::endl;
        return AddressSpaceType::Global;
    }
}

//-----------------------------------------------------------------------------
// Register the feature set in the FeatureSet registry
//-----------------------------------------------------------------------------
static celerity::FeatureSet* _static_fs_1_ = new celerity::Fan19FeatureSet();
static bool _registered_fset_1_ = FSRegistry::registerByKey("fan19", _static_fs_1_ ); 
static celerity::FeatureSet* _static_fs_2_ = new celerity::Grewe11FeatureSet();
static bool _registered_fset_2_ = FSRegistry::registerByKey("fan19", _static_fs_2_ ); 
static celerity::FeatureSet* _static_fs_3_ = new celerity::FullFeatureSet();
static bool _registered_fset_3_ = FSRegistry::registerByKey("fan19", _static_fs_3_ ); 

