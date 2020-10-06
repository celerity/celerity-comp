//
// Created by nmammeri on 26/08/2020.
//

#ifndef COST_RELATION_LLVM_HELPER_H
#define COST_RELATION_LLVM_HELPER_H

namespace celerity {

// Enum identifying OpenCL address spaces
    enum class cl_address_space_type {
        Generic, Global, Region, Local, Constant, Private,
    };

//OpenclAddressSpaceType getOpenclAddrSpaceType(const unsigned addrSpaceId);

/**
 * This method implements mapping between LLVM address space qualifiers and OpenCL
 * address space types. It assumes an amdbackend was used when generating the bitcode
 * https://llvm.org/docs/AMDGPUUsage.html#amdgpu-address-spaces
 */
    inline cl_address_space_type get_cl_address_space_type(const unsigned addrSpaceId) {

        if (addrSpaceId == 0) {
            return cl_address_space_type::Generic;
        } else if (addrSpaceId == 1) {
            return cl_address_space_type::Global;
        } else if (addrSpaceId == 2) {
            return cl_address_space_type::Region;
        } else if (addrSpaceId == 3) {
            return cl_address_space_type::Local;
        } else if (addrSpaceId == 4) {
            return cl_address_space_type::Constant;
        } else if (addrSpaceId == 5) {
            return cl_address_space_type::Private;
        } else {
            //std::cerr << "WARNING: unkwnown address space id: " << addrSpaceId << std::endl;
            return cl_address_space_type::Generic;
        }
    }

/**
 * This method returns whether this a kernel function or not. We use callingConv to determine if it's a kernel or not
 */
    inline bool is_cl_kernel_function(llvm::Function &func) {
        if (func.getCallingConv() == llvm::CallingConv::AMDGPU_KERNEL ||
            func.getCallingConv() == llvm::CallingConv::PTX_Kernel ||
            func.getCallingConv() == llvm::CallingConv::SPIR_KERNEL) {
            return true;
        } else {
            return false;
        }
    }

    inline bool is_cl_khr_base_atomics(const std::string &funcName) {
        if (funcName.find("atom_add") != std::string::npos ||
            funcName.find("atom_sub") != std::string::npos ||
            funcName.find("atom_inc") != std::string::npos ||
            funcName.find("atom_dec") != std::string::npos ||
            funcName.find("atom_xchg") != std::string::npos ||
            funcName.find("atom_cmpxchg") != std::string::npos ) {
            return true;
        }
        return false;
    }

    inline bool is_cl_builtin_atomics(const std::string& funcName) {
        if (funcName.find("atomic_add") != std::string::npos  ||
            funcName.find("atomic_and") != std::string::npos  ||
            funcName.find("atomic_cmpxchg") != std::string::npos  ||
            funcName.find("atomic_compare_exchange_strong") != std::string::npos  ||
            funcName.find("atomic_compare_exchange_strong_explicit") != std::string::npos  ||
            funcName.find("atomic_compare_exchange_weak") != std::string::npos  ||
            funcName.find("atomic_compare_exchange_weak_explicit") != std::string::npos  ||
            funcName.find("atomic_dec") != std::string::npos  ||
            funcName.find("atomic_exchange") != std::string::npos  ||
            funcName.find("atomic_exchange_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_add") != std::string::npos  ||
            funcName.find("atomic_fetch_add_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_and") != std::string::npos  ||
            funcName.find("atomic_fetch_and_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_max") != std::string::npos  ||
            funcName.find("atomic_fetch_max_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_min") != std::string::npos  ||
            funcName.find("atomic_fetch_min_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_or") != std::string::npos  ||
            funcName.find("atomic_fetch_or_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_sub") != std::string::npos  ||
            funcName.find("atomic_fetch_sub_explicit") != std::string::npos  ||
            funcName.find("atomic_fetch_xor") != std::string::npos  ||
            funcName.find("atomic_fetch_xor_explicit") != std::string::npos  ||
            funcName.find("atomic_flag_clear") != std::string::npos  ||
            funcName.find("atomic_flag_clear_explicit") != std::string::npos  ||
            funcName.find("atomic_flag_test_and_set") != std::string::npos  ||
            funcName.find("atomic_flag_test_and_set_explicit") != std::string::npos  ||
            funcName.find("atomic_inc") != std::string::npos  ||
            funcName.find("atomic_init") != std::string::npos  ||
            funcName.find("atomic_load") != std::string::npos  ||
            funcName.find("atomic_load_explicit") != std::string::npos  ||
            funcName.find("atomic_max") != std::string::npos  ||
            funcName.find("atomic_min") != std::string::npos  ||
            funcName.find("atomic_or") != std::string::npos  ||
            funcName.find("atomic_store") != std::string::npos  ||
            funcName.find("atomic_store_explicit") != std::string::npos  ||
            funcName.find("atomic_sub") != std::string::npos  ||
            funcName.find("atomic_work_item_fence") != std::string::npos  ||
            funcName.find("atomic_xchg") != std::string::npos  ||
            funcName.find("atomic_xor") != std::string::npos ) {
            return true;
        }

        return false;
    }

}
#endif //COST_RELATION_LLVM_HELPER_H
