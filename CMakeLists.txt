# The Obi-wan Project
#
# Copyright (c) 2020, Johns Hopkins University - Order Lab.
#
#    All rights reserved.
#    Licensed under the Apache License, Version 2.0 (the "License");
#

cmake_minimum_required(VERSION 3.4.3)
# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed.
  You should create a build directory for building the tool.")
endif()

project(orbit-compiler)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall -Wextra -Wno-unused-parameter")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "LLVM binaries: ${LLVM_TOOLS_BINARY_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_DIR}")

set(ROOT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${ROOT_SOURCE_DIR}/cmake")

include("clang-format")

include(LLVMConfig)
include(AddLLVM)

message(STATUS "LLVM include dir: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries dir: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

include_directories(${LLVM_INCLUDE_DIRS})
message(STATUS "Looking for headers in: ${LLVM_INCLUDE_DIRS}")

link_directories(${LLVM_LIBRARY_DIRS})
message(STATUS "Looking for libraries in: ${LLVM_LIBRARY_DIRS}")

add_definitions(${LLVM_DEFINITIONS})

if (${LLVM_PACKAGE_VERSION} VERSION_GREATER "3.4")
	llvm_map_components_to_libnames(llvm_core core)
	llvm_map_components_to_libnames(llvm_irreader irreader)
	llvm_map_components_to_libnames(llvm_bitwriter bitwriter)
	llvm_map_components_to_libnames(llvm_analysis analysis)
	llvm_map_components_to_libnames(llvm_support support)
	llvm_map_components_to_libnames(llvm_transformutils transformutils)
else()
	llvm_map_components_to_libraries(llvm_core core)
	llvm_map_components_to_libraries(llvm_irreader irreader)
	llvm_map_components_to_libraries(llvm_bitwriter bitwriter)
	llvm_map_components_to_libraries(llvm_analysis analysis)
	llvm_map_components_to_libraries(llvm_support support)
endif()

include_directories(include)
include_directories(lib)

add_subdirectory(runtime)
link_directories(runtime)

add_subdirectory(lib)
add_subdirectory(tools)
