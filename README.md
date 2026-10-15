# NuoCC

NuoCC is a small compiler for a subset of the C language. Supported syntax is listed in [Supported Features](docs/supported_features.md), and the grammar itself is written out in the [grammar](docs/bnf.txt).

## Quick Start

This project uses CMake as the build system. Make sure you have CMake and a C++ compiler installed:

```bash
mkdir build
cd build
cmake ..
make
./nuocc input.c
```

The compiler writes the assembly it generates and hands it to the system C
compiler to be assembled and linked. The programs it builds call
`printint()` and `printchar()`, which the runtime in [lib](lib) provides
and which is linked in as well.

| Flag | What it does |
|---|---|
| | compiles, assembles and links the input files into `a.out` |
| `-o outfile` | names the file the program is linked into |
| `-c` | stops at an object file for each input file, and does not link |
| `-S` | stops at an assembly file for each input file, and does not link |
| `-v` | prints each stage and the command it runs |
| `-T` | prints the tokens and the syntax tree of each input file |
| `-M` | prints the names each input file declared |

Several source files may be named at once, and they are linked together:

```bash
./nuocc -o hello main.c helper.c
```

What a compilation produces is written where the compiler is run and named
after the input file, so `nuocc -c src/main.c` writes `main.o`. The object
files made on the way to an executable are removed once it has been linked.

## Targets

The compiler can generate assembly for more than one machine. `NUOCC_TARGET`
picks which, and only that one code generator is built:

```bash
cmake -DNUOCC_TARGET=x86_64 ..   # the default
cmake -DNUOCC_TARGET=arm64 ..
```

Both write the whole program in one file, and neither needs a runtime of its
own beyond `printint`.

| Target | Assembler and linker | Notes |
|---|---|---|
| `x86_64` | GNU or clang, Linux System V | Emits `.globl`, `.type NAME, @function` and `NAME(%rip)`. |
| `arm64` | Apple clang | Emits the Apple spelling `NAME@PAGE` and `NAME@PAGEOFF` for a global variable. A Linux AArch64 assembler wants `adrp NAME` and `add ... :lo12:NAME` instead, which is the only difference. |

## LICENSE

The project is under [MIT LICENSE](LICENSE).
