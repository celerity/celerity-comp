#pragma once

#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/MemoryBuffer.h>

namespace celerity {

/// Support utility functions to deal with memory accesses
bool isCoalescedMemAccess();
bool isGlobalMemoryAccess(const unsigned addrSpaceId);
bool isLocalMemoryAccess(const unsigned addrSpaceId);
bool isConstantMemoryAccess(const unsigned addrSpaceId);

bool isCoalescedMemAccess() {
    // TODO XXX
    return false;
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
