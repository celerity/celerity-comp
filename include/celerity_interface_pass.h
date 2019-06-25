//
// Created by nadjib on 20.06.19.
//

#ifndef CELERITY_COMP_CELERITY_INTERFACE_PASS_H
#define CELERITY_COMP_CELERITY_INTERFACE_PASS_H



#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"

#include "llvm/Demangle/Demangle.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"


#include <iostream>
#include <fstream>
#include <string>
#include <regex>

using namespace llvm;
using namespace std;

namespace celerity {

/*
 * An LLVM function pass to extract features.
 * The extraction of features from a single instruction is delegated to a feature set class.
 * In this basic implementation, BB's instruction contributions are summed up.
 */
    class CelerityInterfacePass : public ModulePass {

    public:
        static char ID;
        // By default we go for std:cout as the output stream
        std::ostream* outputstreamPtr = &std::cout;

        CelerityInterfacePass() : ModulePass(ID) {
        }

        ~CelerityInterfacePass() { }

        virtual void getAnalysisUsage(AnalysisUsage &au) const {
            au.setPreservesAll();
        }

        virtual bool runOnModule(Module &m);
        virtual bool runOnFunction(Function &f);

        virtual void printInterfaceHeader();
        virtual void printKernelClass(const std::string& kernelName, Function &f);

        virtual bool isItaniumEncoding(const std::string &MangledName);
        virtual std::string demangle(const std::string &MangledName);

        // Getter and setter for current output stream
        std::ostream& outputstream() {
            return *outputstreamPtr;
        }

        void setOutputStream(std::ostream& outs = std::cout) {
            outputstreamPtr = &outs;
        }


    };
}


#endif //CELERITY_COMP_CELERITY_INTERFACE_PASS_H
