#!/bin/bash
# tested with LLVM 6.0

cd samples

clang -c -emit-llvm -cl-std=CL2.0  -Xclang -finclude-default-header  vector-add-kernel.cl -o vector-add-kernel.bc

echo
echo "Feature evaluation with external tool"
../features -i vector-add-kernel.bc
../features -i vector-add-kernel.bc -fe kofler

echo
echo "Feature evaluation with opt"
opt-6.0 -load ../libfeature_eval.so -feature-eval vector-add-kernel.bc -o vector-add-kernel-afterpass.bc
opt-6.0 -loops -load ../libfeature_eval.so -kofler13-eval  vector-add-kernel.bc -o vector-add-kernel-afterpass.bc

cd ..
