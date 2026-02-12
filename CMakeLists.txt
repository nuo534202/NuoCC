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

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE SRC ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(nuocc ${SRC})