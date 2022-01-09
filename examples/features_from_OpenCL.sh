#!/bin/bash

# tested with LLVM 12.0
echo
echo "$(tput setaf 1) Generating LLVM IR from generic C function and OpenCL kernels $(tput sgr 0)"
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/vecadd.cl -o samples/vecadd.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/2mm.cl -o samples/2mm.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/3mm.cl -o samples/3mm.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/coalesced.cl -o samples/coalesced.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/kmeans.cl -o samples/kmeans.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/softmax_loss.cl -o samples/softmax_loss.bc
clang -S -x cl -emit-llvm -cl-std=CL2.0 -Xclang -finclude-default-header samples/parboil.cl -o samples/parboil.bc
clang -S -emit-llvm samples/simple_loop.c -o samples/simple_loop.bc

echo
echo "$(tput setaf 1) Feature evaluation from LLVM IR with LLVM modular optimizer (opt) $(tput sgr 0)"
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/vecadd.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/2mm.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/3mm.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/coalesced.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/kmeans.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/softmax_loss.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/parboil.bc
opt-12 -load-pass-plugin ./libfeature_pass.so  --passes="print<feature>" -disable-output samples/simple_loop.bc

#echo
#echo "$(tput setaf 1) Feature extraction from LLVM IR with the extractor utility $(tput sgr 0)"
#./feature_ext -i samples/vecadd.bc
#./feature_ext -i samples/vecadd.bc -fe kofler13   

