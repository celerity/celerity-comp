# CELERITY Compilation

The CELERITY static analysis and modeling implementation.

## Dependencies

* The [LLVM](http://www.llvm.org) compiler (tested with version 6.0)
* [CMake](https://www.cmake.org)
* A C++14 compiler (C++11 should be enough if ComputeCpp is not used)

### Optional

The feature extraction tools works on the LLVM intermediate representation, which can be obtained in different ways.
These dependencies are only required for testing it on SYCL codes or OpenCL kernels.

* An [OpenCL](https://www.khronos.org/opencl) driver (tested with OpenCL version 2.0)
* The [ComputeCpp](https://www.codeplay.com/products/computesuite/computecpp) runtime (tested with version 0.8.0)

## Getting started

From the surce directory, simply run

    mkdir build
    cd build
    ccmake ..
    make install

Then you can use the features external tool as is or you can call the pass dynamically from the LLVM optimizer (`opt`).

## Examples

The runtime comes with usage examples working on OpenCL and SYCL.
Those examples are the two scripts  `features_from_OpenCL.sh` and `features_from_SYCL.sh` .

