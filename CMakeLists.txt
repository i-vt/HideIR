cmake_minimum_required(VERSION 3.16)
project(EnterpriseObfuscator C CXX)

# Set C++ standard required by modern LLVM
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the LLVM installation on your system
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM's custom CMake utilities
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

# Add LLVM include directories and definitions globally
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Delegate to the source and test directories
add_subdirectory(src)
add_subdirectory(tests)
