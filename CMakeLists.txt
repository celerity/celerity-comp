cmake_minimum_required (VERSION 3.8)
#pt iproject(CELERITY_COMP CXX) #remove project to not have problems when added as submodule in other projects

# Make sure to use g++ to be able to pass linker flags -Wl,-znodelete to fix bug with llvm autoregistring passes
# https://github.com/sampsyo/llvm-pass-skeleton/issues/7#issuecomment-401834287
SET(CMAKE_CXX_COMPILER /usr/bin/g++)

SET(CMAKE_CXX_FLAGS "-Wall -fno-rtti")


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Requires LLVM
#find_package(LLVM REQUIRED CONFIG NO_DEFAULT_PATH)
find_package(LLVM REQUIRED CONFIG) # removed NO_DEFAULT_PATH to make it find llvm
# Polynomial library
find_package(FLINT REQUIRED)


set(CMAKE_VERBOSE_MAKEFILE ON)


## Always use relative paths because this project is intended to be used within other projects
include_directories(${LLVM_INCLUDE_DIRS} include)
add_definitions(${LLVM_DEFINITIONS})
set(FEATURE_SRC src/feature_pass.cpp src/feature_set.cpp)

# Build the feature extraction tool
add_executable(features ${FEATURE_SRC} src/feature_tool.cpp)
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(features ${llvm_libs})
target_compile_options(features PUBLIC -fno-rtti)

# Build the pass for opt
#add_library(feature_eval MODULE ${FEATURE_SRC} src/feature_eval.cpp)
#add_library(feature_pass MODULE ${FEATURE_SRC} src/feature_pass.cpp)
#add_llvm_loadable_module(FeatureEvaluationPass feature_eval.cpp)
add_library(feature_pass MODULE ${FEATURE_SRC} src/feature_pass.cpp)


# Add kernel name pass
# Build the pass for opt
add_library(celerity_interface_pass SHARED src/celerity_interface_pass.cpp include/feature_set.h include/crel_llvm_helper.h)
target_compile_options(celerity_interface_pass PRIVATE -Wl,-znodelete) # workoaround to fix bug with llvm autoregistring passes
if (LLVM IN_LIST LLVM_AVAILABLE_LIBS)
  target_link_libraries(celerity_interface_pass LLVM)
else()
  target_link_libraries(celerity_interface_pass LLVMSupport)
endif()


#---------- crel extraction pass target ------------------------------

add_definitions(${LLVM_DEFINITIONS})

# Build the feature extraction tool
add_executable(crel-extract-tool src/crel_mpoly.cpp src/crel_kernel.cpp src/crel_extract_tool.cpp src/crel_feature_set.cpp src/crel_feature_pass.cpp)
target_include_directories(crel-extract-tool  PUBLIC ${LLVM_INCLUDE_DIRS} )
llvm_map_components_to_libnames(llvm_libs support core irreader analysis)
target_link_libraries(crel-extract-tool ${llvm_libs} ${FLINT_LIBRARIES})
target_compile_options(features PUBLIC -fno-rtti)

#---------- Install targets ------------------------------

# move example scripts and examples (make install)
install(PROGRAMS "examples/features_from_OpenCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(PROGRAMS "examples/features_from_SYCL.sh" DESTINATION "${CMAKE_BINARY_DIR}" )
install(FILES "examples/example-application-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/matrix-multiply-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/simple-vector-add-sycl.cpp"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/vector-add-CL.c"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
install(FILES "examples/vector-add-kernel.cl"  DESTINATION "${CMAKE_BINARY_DIR}/samples" )
