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

From the source directory, simply run

    mkdir build
    cd build
    ccmake ..
    make install

Then you can use the features external tool as is or you can call the pass dynamically from the LLVM optimizer (`opt`).

## Examples

The runtime comes with usage examples working on OpenCL and SYCL.
Those examples are the two scripts  `features_from_OpenCL.sh` and `features_from_SYCL.sh` .


## Commands
  * opt-8 hipsycl_c64eada2e3de8aef-cuda-nvptx64-nvidia-cuda-sm_61.ll -o output.opt
  * opt-8 -load ../cmake-build-debug/libkernel_name_pass.so  hipsycl_c64eada2e3de8aef-cuda-nvptx64-nvidia-cuda-sm_61.ll -o output.opt
  * clang-8 -Xclang -load -Xclang ../cmake-build-debug/libkernel_name_pass.so -g test.cpp
  * clang-8 -Xclang -load -Xclang ../cmake-build-debug/libkernel_name_pass.so -g -mllvm -debug-pass=Structure test.cpp
  
  * /usr/bin/syclcc-clang  -I/home/nadjib/work/git/celerity-runtime/include -I/home/nadjib/work/git/celerity-runtime/vendor -I/usr/include -I/usr/include/mpich -I/home/nadjib/work/git/celerity-runtime/cmake-build-debug/spdlog-src/include  --hipsycl-platform=cuda -g -Xclang -load -Xclang  /home/nadjib/work/git/celerity-comp/cmake-build-debug/libkernel_name_pass.so  --cuda-device-only   -c /home/nadjib/work/git/celerity-runtime/examples/wave_sim/wave_sim.cc


