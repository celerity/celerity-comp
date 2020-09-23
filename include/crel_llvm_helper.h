//
// Created by nmammeri on 26/08/2020.
//

#ifndef COST_RELATION_LLVM_HELPER_H
#define COST_RELATION_LLVM_HELPER_H

namespace celerity {

// Enum identifying OpenCL address spaces
    enum class OpenclAddressSpaceType {
        Generic, Global, Region, Local, Constant, Private,
    };

//OpenclAddressSpaceType getOpenclAddrSpaceType(const unsigned addrSpaceId);

/**
 * This method implements mapping between LLVM address space qualifiers and OpenCL
 * address space types. It assumes an amdbackend was used when generating the bitcode
 * https://llvm.org/docs/AMDGPUUsage.html#amdgpu-address-spaces
 */
    inline OpenclAddressSpaceType getOpenclAddrSpaceType(const unsigned addrSpaceId) {

        if (addrSpaceId == 0) {
            return OpenclAddressSpaceType::Generic;
        } else if (addrSpaceId == 1) {
            return OpenclAddressSpaceType::Global;
        } else if (addrSpaceId == 2) {
            return OpenclAddressSpaceType::Region;
        } else if (addrSpaceId == 3) {
            return OpenclAddressSpaceType::Local;
        } else if (addrSpaceId == 4) {
            return OpenclAddressSpaceType::Constant;
        } else if (addrSpaceId == 5) {
            return OpenclAddressSpaceType::Private;
        } else {
            //std::cerr << "WARNING: unkwnown address space id: " << addrSpaceId << std::endl;
            return OpenclAddressSpaceType::Generic;
        }
    }

/**
 * This method returns whether this a kernel function or not. We use callingConv to determine if it's a kernel or not
 */
    inline bool isKernelFunction(llvm::Function &func) {
        if (func.getCallingConv() == llvm::CallingConv::AMDGPU_KERNEL ||
            func.getCallingConv() == llvm::CallingConv::PTX_Kernel ||
            func.getCallingConv() == llvm::CallingConv::SPIR_KERNEL) {
            return true;
        } else {
            return false;
        }
    }

}
#endif //COST_RELATION_LLVM_HELPER_H
