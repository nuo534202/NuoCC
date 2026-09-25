cmake_minimum_required(VERSION 3.16)

project(NuoCC CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE debug)
# set(CMAKE_BUILD_TYPE release)

# Warnings
add_compile_options(-Wall)

# The machine the compiler generates assembly for. Every target provides
# MakeCodegen() in its own file under src/codegen.
set(NUOCC_TARGET "x86_64" CACHE STRING "the target to generate code for")

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)

# The code generators are picked one at a time, so remove them all first.
list(FILTER SRC EXCLUDE REGEX "/codegen/")

if(NUOCC_TARGET STREQUAL "x86_64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_x86.cpp)
elseif(NUOCC_TARGET STREQUAL "arm64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_arm64.cpp)
else()
    message(FATAL_ERROR "NUOCC_TARGET must be x86_64 or arm64, not ${NUOCC_TARGET}")
endif()

message(STATUS "NuoCC will generate ${NUOCC_TARGET} assembly")

add_executable(nuocc ${SRC})
