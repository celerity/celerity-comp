cmake_minimum_required (VERSION 3.13.4)
project(Celerity_Comp)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Requires LLVM
find_package(LLVM REQUIRED CONFIG PATHS /usr/lib/llvm-10/lib )
#find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found LLVM Tools in ${LLVM_TOOLS_BINARY_DIR}")

include_directories(${LLVM_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# optional: enable support for polynomial cost relations
option(EXTRACTOR_TOOL "Build the external feature extractor tool" ON)
# optional: install sample scripts for several 

option(POLFEAT "Support for polynomial features" ON)
# optional: install sample scripts for several 
option(SAMPLE_SCRIPTS "Install sample scripts for C functions, OpenCL and SYCL" ON)
# optional: Celerity runtime integration 
option(CELERITY_RUNTIME "Install the integration layer for Celerity (requires existing Celerity Runtime installation)" OFF)
# TODO: 3opts: only LLVM passes, only external tool, both

# sources
set(FEATURE_SRC src/feature_pass.cpp src/feature_set.cpp)

# Build the feature extraction tool 
if(EXTRACTOR_TOOL)
  add_executable(feature_ext ${FEATURE_SRC} src/feature_tool.cpp)
  llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
  target_link_libraries(feature_ext ${llvm_libs})
  #target_compile_options(feature_ext PUBLIC -fno-rtti)
endif(EXTRACTOR_TOOL)


# Build the LLVM pass to be used with the optimizer
#add_library(feature_eval MODULE ${FEATURE_SRC} src/feature_eval.cpp)
#add_library(feature_pass MODULE ${FEATURE_SRC} src/feature_pass.cpp)
#add_llvm_loadable_module(FeatureEvaluationPass feature_eval.cpp)
add_library(feature_pass MODULE ${FEATURE_SRC} src/feature_pass.cpp)


# Build the integratoin layer to be used with the Celerity runtime
if(CELERITY_RUNTIME)
  # Add kernel name pass  
  add_library(celerity_interface_pass SHARED src/celerity_interface_pass.cpp)
  target_compile_options(celerity_interface_pass PRIVATE -Wl,-znodelete) # workoaround to fix bug with llvm autoregistring passes
  if (LLVM IN_LIST LLVM_AVAILABLE_LIBS)
    target_link_libraries(celerity_interface_pass LLVM clangTooling)
  else()
    target_link_libraries(celerity_interface_pass LLVMSupport clangTooling)
  endif()
endif(CELERITY_RUNTIME)

# Install example scripts (make install)
if(SAMPLE_SCRIPTS)
  install(PROGRAMS "examples/features_from_OpenCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
  install(PROGRAMS "examples/features_from_SYCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
  install(FILES "examples/example-application-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/matrix-multiply-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/simple-vector-add-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/vector-add-CL.c"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/vector-add-kernel.cl"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
endif(SAMPLE_SCRIPTS)
