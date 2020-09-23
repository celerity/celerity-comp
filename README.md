# CELERITY Compiler Repository

## Requirements
  * We use flint library to represetn polynomials. libflint.so needs to be installed
    * sudo apt install libflint-dev
  * clang and llvm needs to be installed, Please refer to https://apt.llvm.org/ for instructions
  * We recommend clang-10 as it has better opencl support
    * sudo apt-get install clang-10 clang-tools-10 clang-10-doc libclang-common-10-dev libclang-10-dev libclang1-10 clang-format-10 clangd-10
    * cd /usr/bin
    * sudo ln -s clang-10 clang 
    * sudo ln -s clang++-10 clang++
