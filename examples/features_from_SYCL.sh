#!/bin/bash

SYCL_INCLUDE=$COMPUTECPP_DIR/include
SYCL_FILES="example-application-sycl.cpp matrix-multiply-sycl.cpp simple-vector-add-sycl.cpp"

cd samples

# SYCL compilation
for SYCL_FILE in ${SYCL_FILES}; do
echo "SYCLFILE $SYCL_FILE"
compute++ -O2 -mllvm -inline-threshold=1000 -sycl -intelspirmetadata -sycl-target spir64 -I$SYCL_INCLUDE $SYCL_FILE 
done

# feature extraction from bitcode
for bc in *.bc; do
    echo "--- extracing features from $bc ---"
    ../features -i $bc 
    ../features -i $bc -fe kofler
    ../features -i $bc -fs full
done

cd ..
