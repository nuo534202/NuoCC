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

# The sources are found when the build is configured, and looked for
# again whenever the build runs, so that a new file of the compiler is
# picked up without having to configure it by hand.
file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cpp)

# The code generators are picked one at a time, so remove them all first.
# The part which writes the assembly text is shared by the two machine
# targets, so it is added back with whichever of them is picked.
list(FILTER SRC EXCLUDE REGEX "/codegen/")

if(NUOCC_TARGET STREQUAL "x86_64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_assembly.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_x86.cpp)
elseif(NUOCC_TARGET STREQUAL "arm64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_assembly.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_arm64.cpp)
elseif(NUOCC_TARGET STREQUAL "qbe")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_qbe.cpp)
elseif(NUOCC_TARGET STREQUAL "m6809")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_m6809.cpp)
    # The peephole optimiser is the pass this target has over the code it
    # wrote, so only this target is built with it.
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_peephole.cpp)
else()
    message(FATAL_ERROR
        "NUOCC_TARGET must be x86_64, arm64, qbe or m6809, not ${NUOCC_TARGET}")
endif()

message(STATUS "NuoCC will generate ${NUOCC_TARGET} assembly")

add_executable(nuocc ${SRC})

# The runtime the generated programs are linked with, which holds printint()
# and printchar(): the language may call them but does not define them. The
# path is fixed here so that the compiler can be run from anywhere. The
# pre-processor and the directory our own header files live in are fixed the
# same way, so that a program may #include one of them from anywhere too.
# The QBE target has the program put through qbe, and the 6809 target has
# it put through lwasm; both are looked for on the path, the way the
# assembler and linker are. The 6809 runtime is the assembly every program
# of that target is built with.
target_compile_definitions(nuocc PRIVATE
    NUOCC_RUNTIME="${PROJECT_SOURCE_DIR}/lib/printint.c"
    NUOCC_6809_RUNTIME="${PROJECT_SOURCE_DIR}/lib/6809/runtime.s"
    NUOCC_CPP="cpp"
    NUOCC_INCLUDE_DIR="${PROJECT_SOURCE_DIR}/lib/include")
