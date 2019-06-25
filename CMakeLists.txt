cmake_minimum_required (VERSION 3.8)
project(CELERITY_COMP CXX)

# Make sure to use g++ to be able to pass linker flags -Wl,-znodelete to fix bug with llvm autoregistring passes
# https://github.com/sampsyo/llvm-pass-skeleton/issues/7#issuecomment-401834287
SET(CMAKE_CXX_COMPILER /usr/bin/g++)

# Requires LLVM
find_package(LLVM REQUIRED CONFIG)

set(CMAKE_VERBOSE_MAKEFILE ON)




include_directories(${LLVM_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/include")
add_definitions(${LLVM_DEFINITIONS})
set(FEATURE_SRC src/feature_eval.cpp src/feature_set.cpp)

# Build the feature extraction tool 
add_executable(features ${FEATURE_SRC} src/feature_tool.cpp)
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(features ${llvm_libs})

# Build the pass for opt
add_library(feature_eval MODULE ${FEATURE_SRC} src/feature_eval.cpp)
#add_llvm_loadable_module(FeatureEvaluationPass feature_eval.cpp)


# Add kernel name pass
# Build the pass for opt
add_library(celerity_interface_pass SHARED src/celerity_interface_pass.cpp)
target_compile_options(celerity_interface_pass PRIVATE -Wl,-znodelete) # workoaround to fix bug with llvm autoregistring passes
target_link_libraries(celerity_interface_pass LLVM ) # We need to link against shared llvm lib otherwise we'll have double registration of passes errors



# move example scripts and examples (make install)
install(PROGRAMS "examples/features_from_OpenCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(PROGRAMS "examples/features_from_SYCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(FILES "examples/example-application-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/matrix-multiply-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/simple-vector-add-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/vector-add-CL.c"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/vector-add-kernel.cl"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
