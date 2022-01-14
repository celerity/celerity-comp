CELERITY Compiler Repository
=========

**Celerity-comp** is a collection of LLVM passes and supporting code that interact with modern predictive optimizer and runtime systems.  It is designed to work both as a standalone library, as supporting external tool, and integrated within the Celerity runtime systems. It supports the new LLVM pass manager and does not implement the legacy one. 
Celerity-comp has been tested with LLVM 12.

### Table of contents
* [Introduction](#introduction)
* [Requirements](#requirements)
  * [Common](#common)
  * [Extractor tools](#extractor)
  * [Celerity integration](#celerityintegration)
* [References](#references)


Introduction
============
xxx

Requirements
============
  * CMake: we suggest to install both cmake and the curses gui
  ```console
  sudo apt install cmake cmake-curses-gui
  ```
  * Clang/LLVM: required at least LLVM version 12.0
  ```console
  sudo apt install clang-12 llvm-12  ```

  * (Optional) Libflint is required for more accurate features (polfeat). Note that older versions of libflint do not support multivariate polynomials.
 ```console
  sudo apt install libflint-2.6.3 libflint-dev zlib1g-dev
  ```



Installation
============  


Getting Started
===============  


References
============

TODO
