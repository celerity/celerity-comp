cmake_minimum_required (VERSION 3.8)
project(CELERITY_COMP CXX)

# Requires LLVM
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})
set(FEATURE_SRC src/feature_eval.cpp src/feature_set.cpp)

# Build the feature extraction tool 
add_executable(features ${FEATURE_SRC} src/feature_tool.cpp)
llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(features ${llvm_libs})

# Build the pass for opt
add_library(feature_eval MODULE feature_eval.cpp)
# add_llvm_loadable_module(FeatureEvaluationPass feature_eval.cpp)

