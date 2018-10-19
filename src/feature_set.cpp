#include <fstream>

#include <llvm/Support/MemoryBuffer.h>

#include "feature_set.h"
#include "feature_norm.h"

using namespace celerity;
using namespace llvm;
using namespace std;

void feature_set::print(ostream &os){
	vector<string> feat_names;
	feat_names.reserve(feat.size());
	for(std::pair<string,float> el : feat)	
		feat_names.push_back(el.first);	
	std::sort(feat_names.begin(),feat_names.end());
        // print in alphabetical order
	cout << "name, raw, feat" << endl;
	for(string name : feat_names){
	  os << name << "," << raw[name] <<","<< feat[name] << endl;	
	}
	cout << endl;
}

void feature_set::print_to_cout(){
	feature_set::print(cout);
}

void feature_set::print_to_file(const string &out_file){
	cout << "Writing to file: " << out_file << endl;
	ofstream outstream;
	outstream.open (out_file);
	print(outstream);
	outstream.close();
}

void feature_set::normalize(){
	celerity::normalize(*this);
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

static inline bool instr_check(const set<string> &instr_set, const string &instr_name){
    return instr_set.find(instr_name) != instr_set.end();
}

void gpu_feature_set::eval_instruction(const llvm::Instruction &inst){
    string i_name = inst.getOpcodeName();
    if(instr_check(BIN_OPS,i_name)) {        
        if(instr_check(INT_ADDSUB, i_name)){
            add("int_add_sub");
        }
        else if(instr_check(INT_MUL, i_name)){        
            add("int_mul");
        }
        else if(instr_check(INT_DIV, i_name)){        
            add("int_div");
        }
        else if(instr_check(INT_REM, i_name)){        
            add("int_rem");
        }
        else if(instr_check(FLOAT_ADDSUB, i_name)){        
            add("float_add_Sub");
        }
        else if(instr_check(FLOAT_MUL, i_name)){        
            add("float_mul");
        }
        else if(instr_check(FLOAT_DIV, i_name)){        
            add("float_div");
        }
        else if(instr_check(FLOAT_REM, i_name)){        
            add("float_rem");
        }
        else if(instr_check(SPECIAL, i_name)){        
            add("float_rem");
        }
    }
    else if(instr_check(BITWISE, i_name)){    
        add("bitwise");
    }
    else if(instr_check(AGGREGATE, i_name)){    
        add("aggregate");
    }
    else if(instr_check(VECTOR, i_name)){    
        add("vector");    
    }     
    else if(const LoadInst *li = dyn_cast<LoadInst>(&inst)) {
        add("load");
        checkAddrSpace(li->getPointerAddressSpace()); 
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    }
    else if (const StoreInst *si = dyn_cast<StoreInst>(&inst)) {
        add("store");
        checkAddrSpace(si->getPointerAddressSpace()); 
        // TODO: distinguish local from global memory
        // add("load_local"), add("load_global")
    } else {
        add("other");
    }    
}

void full_feature_set::eval_instruction(const llvm::Instruction &inst){    
    string i_name = inst.getOpcodeName();
    add(i_name);
}

void grewe11_feature_set::eval_instruction(const llvm::Instruction &inst){
    // TODO FIXME XXX Nadjib
    // implementation missing
}

AddressSpaceType celerity::checkAddrSpace(const unsigned addrSpaceId) {
    if(addrSpaceId == localAddressSpace) {
        return AddressSpaceType::Local;
    } else if(addrSpaceId == globalAddressSpace) {
        return AddressSpaceType::Global;
    } else if(addrSpaceId == privateAddressSpace) {
        return AddressSpaceType::Private;
    } else {
        std::cerr << "WARNING: unkwnown address space id: " << addrSpaceId << std::endl;
        return AddressSpaceType::Unknown;
    }
}

