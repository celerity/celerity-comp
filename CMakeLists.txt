cmake_minimum_required (VERSION 3.8)
project(CELERITY_COMP CXX)

# Requires LLVM
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})
set(FEATURE_SRC src/feature_eval.cpp src/feature_set.cpp)

# Build the feature extraction tool 
add_executable(features ${FEATURE_SRC} src/feature_tool.cpp)
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(features ${llvm_libs})

# Build the pass for opt
add_library(feature_eval MODULE src/feature_eval.cpp)
# add_llvm_loadable_module(FeatureEvaluationPass feature_eval.cpp)

# move example scripts (make install)
install(PROGRAMS "examples/features_from_OpenCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(PROGRAMS "examples/features_from_OpenCL_with_opt.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(PROGRAMS "examples/features_from_SYCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )