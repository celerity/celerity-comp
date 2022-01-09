#pragma once

#include <llvm/IR/Argument.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Support/MemoryBuffer.h>

#include "FeatureSet.hpp" // for demanglng utility


namespace celerity {

/// Check wheter mem access are coalesced
struct CoalescedMemAccess {
    int mem_access;
    int mem_coalesced;
};

/// Uses a simple heuristics to calculate how many mem access are coalesced
CoalescedMemAccess getCoalescedMemAccess(Function &fun) {
    CoalescedMemAccess cma = {0, 0};
    outs() << "getCoalescedMemAccess\n"; 
    // 1. Search for pointer arguments   
    std::set<GetElementPtrInst*> gep_set;
    for (unsigned i=0; i< fun.arg_size(); i++) {            
        llvm::Argument* arg = fun.getArg(i);
        // if we have a pointer, it cannot be used for loop bound analysis, thus we skip it
        if(arg->getType()->isPointerTy()){
            //outs() << "argument" << i << " user\n";
            // 2. Collect  uses of the pointer argument
            for (User *user : arg->users()) {
                if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(user)) {
                    //outs() << "  gep:" << *gep << "\n";
                    gep_set.insert(gep);
                }
            }          
        }
    }
    cma.mem_access = gep_set.size();
    // 3. Find get_global_id()
    for (BasicBlock &bb : fun) {
        for (Instruction &inst: bb) {
            if (const CallInst *call_inst = dyn_cast<CallInst>(&inst)){
                string fun_name = call_inst->getCalledFunction()->getGlobalIdentifier();
                if(fun_name.find("get_global_id")!= std::string::npos){
                    //outs() << "global_id\n";
                    // 4. For each use..                    
                    for (const User *user : call_inst->users()) {
                        //outs() << "  *uses* " << *user << "\n";     
                        // 5. ...we check if corresponds to the gep operand
                        for(GetElementPtrInst* gep : gep_set){
                            if(user == gep->getOperand(1)){
                                outs() << "  *coalesced for global id* " << "\n";     
                                cma.mem_coalesced++;
                            }
                        }
                    }
                }
            }
        }
    }
    // 6. Further check gep with constant index
    for(GetElementPtrInst* gep : gep_set){
        const Value *gep_op = gep->getOperand(1);
        if(const ConstantInt *cont_int = dyn_cast<ConstantInt>(gep_op)){
            outs() << "  *coalesced for constant* " << "\n";     
            cma.mem_coalesced++; 
        }
    }
    return cma;
}


/// Enum identifying OpenCL address spaces
enum class cl_address_space_type { Generic, Global, Region, Local, Constant, Private };

/// Mapping between LLVM address space qualifiers and OpenCL address space types. 
/// https://llvm.org/docs/AMDGPUUsage.html#amdgpu-amdhsa-memory-model
cl_address_space_type get_cl_address_space_type(const unsigned addrSpaceId) {
    switch(addrSpaceId){
        case 0: return cl_address_space_type::Generic;
        case 1: return cl_address_space_type::Global;
        case 2: return cl_address_space_type::Region;
        case 3: return cl_address_space_type::Local;
        case 4: return cl_address_space_type::Constant;
        case 5: return cl_address_space_type::Private;
        default: 
            errs() << "WARNING: unkwnown address space id: " << addrSpaceId << "\n";
            return cl_address_space_type::Generic;
    }
}

/// Support utility functions to deal with memory accesses
bool isGlobalMemoryAccess(const unsigned addrSpaceId){        
    return get_cl_address_space_type(addrSpaceId) == cl_address_space_type::Local;
}

bool isLocalMemoryAccess(const unsigned addrSpaceId){    
    return get_cl_address_space_type(addrSpaceId) == cl_address_space_type::Global;
}

bool isConstantMemoryAccess(const unsigned addrSpaceId){    
    return get_cl_address_space_type(addrSpaceId) == cl_address_space_type::Global;
}


} // celerity
