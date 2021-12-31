#!/bin/bash
# tested with LLVM 12.0

CL_KERNEL="samples/vector_add_kernel.cl"
CL_IR="samples/vector_add_kernel.bc"

echo
echo "Generating LLVM IR from generic C function and OpenCL kernels"
clang -c -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header  $CL_KERNEL -o $CL_IR
clang -c -S samples/simple_loop.c -o samples/simple_loop.bc

echo
echo "Feature evaluation from LLVM IR with LLVM modular optimizer (opt)"
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output $CL_IR
#opt -load ../libfeature_pass.so -feature-pass vector-add-kernel.bc -o vector-add-kernel-afterpass.bc
#opt -loops -load ../libfeature_pass.so -kofler13-eval  vector-add-kernel.bc -o vector-add-kernel-afterpass.bc

echo
echo "Feature extraction from LLVM IR with the extractor utility"
feature_ext -i $CL_IR
feature_ext -i $CL_IR -fe kofler13   

