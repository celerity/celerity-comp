#!/bin/bash
# tested with LLVM 10.0

cd samples

echo
echo "Generating LLVM IR from OpenCL kernels"
clang -c -x cl -emit-llvm -cl-std=CL2.0 -S  -Xclang -finclude-default-header  vector-add-kernel.cl -o vector-add-kernel.bc

echo
echo "Feature extraction from LLVM IR with the extractor utility"
../features -i vector-add-kernel.bc
../features -i vector-add-kernel.bc -fe kofler13

echo
echo "Feature evaluation from LLVM IR with LLVM moudlar optimizer (opt)"
opt -load ../libfeature_pass.so -feature-pass vector-add-kernel.bc -o vector-add-kernel-afterpass.bc
opt -loops -load ../libfeature_pass.so -kofler13-eval  vector-add-kernel.bc -o vector-add-kernel-afterpass.bc

cd ..
