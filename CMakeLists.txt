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

# The peephole optimiser is the pass a target has over the whole of the code
# it wrote, and every machine target has one: a rule is the instructions of
# one machine, so each of those targets brings its own rules and the pass is
# built with it. The QBE target writes QBE's intermediate language rather
# than the code of a machine, and has no rules of its own.
if(NUOCC_TARGET STREQUAL "x86_64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_assembly.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_x86.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_peephole.cpp)
elseif(NUOCC_TARGET STREQUAL "arm64")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_assembly.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_arm64.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_peephole.cpp)
elseif(NUOCC_TARGET STREQUAL "qbe")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_qbe.cpp)
elseif(NUOCC_TARGET STREQUAL "m6809")
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_codegen_m6809.cpp)
    list(APPEND SRC ${PROJECT_SOURCE_DIR}/src/codegen/nuocc_peephole.cpp)
else()
    message(FATAL_ERROR
        "NUOCC_TARGET must be x86_64, arm64, qbe or m6809, not ${NUOCC_TARGET}")
endif()

message(STATUS "NuoCC will generate ${NUOCC_TARGET} assembly")

add_executable(nuocc ${SRC})

include(CTest)

if(BUILD_TESTING)
    find_package(Threads REQUIRED)

    add_executable(scanner_whitespace_test
        test/scanner_whitespace_test.cpp
        src/nuocc_scanner.cpp)
    target_include_directories(scanner_whitespace_test PRIVATE
        ${PROJECT_SOURCE_DIR}/include)
    target_link_libraries(scanner_whitespace_test PRIVATE Threads::Threads)

    add_test(NAME scanner_whitespace
        COMMAND scanner_whitespace_test)
    set_tests_properties(scanner_whitespace PROPERTIES TIMEOUT 10)

    add_test(NAME scanner_unterminated_character_cr
        COMMAND scanner_whitespace_test --unterminated-character-cr)
    set_tests_properties(scanner_unterminated_character_cr PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: unterminated character literal!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_unterminated_character_eof
        COMMAND scanner_whitespace_test --unterminated-character-eof)
    set_tests_properties(scanner_unterminated_character_eof PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: unterminated character literal!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_unterminated_string_cr
        COMMAND scanner_whitespace_test --unterminated-string-cr)
    set_tests_properties(scanner_unterminated_string_cr PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: unterminated string literal!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_unterminated_string_eof
        COMMAND scanner_whitespace_test --unterminated-string-eof)
    set_tests_properties(scanner_unterminated_string_eof PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: unterminated string literal!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_trailing_escape_crlf
        COMMAND scanner_whitespace_test --trailing-escape-crlf)
    set_tests_properties(scanner_trailing_escape_crlf PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: expected a character after a backslash!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_trailing_escape_eof
        COMMAND scanner_whitespace_test --trailing-escape-eof)
    set_tests_properties(scanner_trailing_escape_eof PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "lexical error: expected a character after a backslash!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")

    add_test(NAME scanner_inline_directive
        COMMAND scanner_whitespace_test --inline-directive)
    set_tests_properties(scanner_inline_directive PROPERTIES
        TIMEOUT 10
        PASS_REGULAR_EXPRESSION
            "Error: unrecognized character #!"
        FAIL_REGULAR_EXPRESSION
            "scanner accepted invalid input")
endif()

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
