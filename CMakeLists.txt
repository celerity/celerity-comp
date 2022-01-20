cmake_minimum_required (VERSION 3.13.4)
project(Celerity_Comp)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

# Requires LLVM
find_package(LLVM 12.0.0 REQUIRED CONFIG PATHS /usr/lib/llvm-12/lib )
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Found LLVM Tools in ${LLVM_TOOLS_BINARY_DIR}")
llvm_map_components_to_libnames(llvm_libs support passes core irreader analysis)

#separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
#add_definitions(${LLVM_DEFINITIONS_LIST})

# optional: enable support for polynomial cost relations
option(EXTRACTOR_TOOL "Build the external feature extractor tool" ON)
# optional: support more advanced and accurate feature peresentation
option(POLFEAT "Support for polynomial features" ON)
# optional: install sample scripts for several 
option(SAMPLE_SCRIPTS "Install sample scripts for C functions, OpenCL and SYCL" ON)
# optional: Celerity runtime integration 
option(CELERITY_RUNTIME "Install the integration layer for Celerity (requires existing Celerity Runtime installation)" OFF)

# Sources
set(FEATURE_SRC 
  src/FeatureSet.cpp           src/FeaturePrinter.cpp      src/FeatureAnalysisPlugin.cpp  
  src/FeatureAnalysis.cpp      src/Kofler13Analysis.cpp    src/DefaultFeatureAnalysis.cpp  )

# Support for polynomial features 
if(POLFEAT)
  find_package(FLINT REQUIRED)
  set(EXTRA_INCLUDE        ${FLINT_INCLUDE_DIRS}  )
  set(FEATURE_SRC          ${FEATURE_SRC}     src/KernelInvariant.cpp  src/PolFeatAnalysis.cpp  src/IMPoly.cpp)
  set(EXTRA_LIB            ${FLINT_LIBRARIES})
  # impoly test function 
  add_executable(test_impoly ${FEATURE_SRC} src/test_impoly.cpp)
  target_link_libraries(test_impoly ${llvm_libs} ${EXTRA_LIB})
  target_compile_options(test_impoly PUBLIC -Wl,-znodelete)
endif(POLFEAT)
 
# Add includes
include_directories(${LLVM_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include" ${EXTRA_INCLUDE})

# Build the feature extraction tool 
if(EXTRACTOR_TOOL)
  add_executable(feature_ext ${FEATURE_SRC} src/feature_tool.cpp) 
  target_link_libraries(feature_ext ${llvm_libs} ${EXTRA_LIB})
  target_compile_options(feature_ext PUBLIC -Wl,-znodelete)
  #target_include_directories(feature_ext ${LLVM_INCLUDE_DIRS} ${FLINT_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}/include")
 endif(EXTRACTOR_TOOL)

# Build the LLVM pass to be used with the optimizer
add_library(feature_pass MODULE ${FEATURE_SRC})

#target_include_directories(feature_pass ${LLVM_INCLUDE_DIRS} ${FLINT_INCLUDE_DIR} "${PROJECT_SOURCE_DIR}/include")

# Build the integration layer to be used with the Celerity runtime
if(CELERITY_RUNTIME)
  # build a Celerity  kernel name pass  
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
  make_directory("${CMAKE_BINARY_DIR}/samples")
  install(PROGRAMS "examples/features_from_OpenCL.sh"    DESTINATION "${CMAKE_BINARY_DIR}" )
  install(PROGRAMS "examples/features_from_SYCL.sh"      DESTINATION "${CMAKE_BINARY_DIR}" )
  install(FILES "examples/example-application-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/matrix-multiply-sycl.cpp"      DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/simple-vector-add-sycl.cpp"    DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/vecadd.cl"                     DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/2mm.cl"                        DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/3mm.cl"                        DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/kmeans.cl"                     DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/softmax_loss.cl"               DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/coalesced.cl"                  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/parboil.cl"                    DESTINATION "${CMAKE_BINARY_DIR}/samples" )
  install(FILES "examples/simple_loop.c"                 DESTINATION "${CMAKE_BINARY_DIR}/samples" )
endif(SAMPLE_SCRIPTS)
