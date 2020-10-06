//
// Created by nmammeri on 05/10/2020.
//
#pragma once

#include <llvm/PassSupport.h>
#include <llvm/PassRegistry.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Analysis/ScalarEvolutionExpressions.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/IR/LegacyPassManager.h>


#include "crel_feature_set.h"
#include "crel_feature_pass.h"

namespace celerity {

    int extract_crels(celerity::crel_feature_set *fs,
                      std::string clFileInput,
                      bool verbose = false,
                      std::string csvFileOtput = "",
                      std::string compilerArgs = "");

}