#!/bin/bash
# tested with LLVM 6.0

cd samples
#clang-6.0 -emit-spirv -x cl -cl-std=CL2.0 -S  vector-add-kernel.cl  -o vector-add-kernel.spirv
#clang-6.0 -cc1 -emit-llvm -triple=spir-unknown-unknown -cl-std=CL2.0 -x cl  vector-add-kernel.cl -o vector-add-kernel.spirv
clang -c -emit-llvm -cl-std=CL2.0  -Xclang -finclude-default-header  vector-add-kernel.cl -o vector-add-kernel.bc

echo "Feature evaluation with external tool"
../features -i vector-add-kernel.bc
../features -i vector-add-kernel.bc -fe kofler
echo "Feature evaluation with opt"
opt-6.0 -load ../libfeature_eval.so -feature-eval vector-add-kernel.bc


cd ..