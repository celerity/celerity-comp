#pragma once

#include <string>
#include <map> // need an ordered map

// forward declaration
namespace llvm
{
    class Function;
    class Value;
    class raw_ostream;
}


namespace celerity
{

    /// Invariants recognized in typical OpenCL applications:
    ///   kernel arguments: "a0", "a1", ...
    ///   global sizes: "g0", "g1", "g2" for get_global_size(0), ...
    ///   local sizes:  "l0, "l1", "l2" get_local_size(0), ...
    ///   subgroups:  "sg"  get_sub_group_size(), get_num_sub_groups()
    enum InvariantType
    {
        a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, // support up to 10 arguments
        gs0, gs1, gs2, // get_global_size(uint dimindx)
        ng0, ng1, ng2, // get_num_groups(uint dimindx)
        ls0, ls1, ls2, // get_local_size(uint dimindx)
        nsg, sgs, msgs, // get_num_sub_groups(), get_sub_group_size(), get_max_sub_group_size
        none  // value used for returning invalid invariant
    };

    static unsigned InvariantTypeNum = 24;
    static const char *InvariantTypeName[] = {
        "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9",
        "gs0", "gs1", "gs2",
        "ng0", "ng1", "ng2",
        "ls0", "ls1", "ls2",
        "nsg", "sgs", "msgs",
        "none"};

    /// Struct for collecting all kernel invariants for a given function (e.g., OpenCL kernel).
    /// Invariants are store in a map that associates the InvariantType to a a Value.
    ///  Assume uniform work-group size
    struct KernelInvariant
    {
    private:
        llvm::Function *function;
        std::map<InvariantType, llvm::Value *> invariants;

    public:
        KernelInvariant(llvm::Function &fun);

        /// Return an int for an enumerated invariant type. Important: enumeration starts from x1.
        static unsigned enumerate(enum InvariantType it)
        {
            return it+1;
        }

        /// Return the number of invariant type
        static unsigned numInvariantType()
        {
            return InvariantTypeNum;
        }

        /// Returns a map with all found invariants
        std::map<InvariantType, llvm::Value *> getInvariants();
        
        /// Check whether the input value is a registered kernel invariant
        InvariantType isInvariant(llvm::Value *value);

        /// Kernel invariants print utility
        void print(llvm::raw_ostream &out_stream);

    }; // end struct

} // end namespace
