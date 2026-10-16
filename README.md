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

The compiler can also be built to write the intermediate language of
[QBE](https://c9x.me/compile/) rather than the assembly of a machine, in
which case it has QBE compile that down to the assembly of the machine the
program is built for (see [Targets](#targets)).

| Flag | What it does |
|---|---|
| | compiles, assembles and links the input files into `a.out` |
| `-o outfile` | names the file the program is linked into |
| `-c` | stops at an object file for each input file, and does not link |
| `-S` | stops at the code the target writes, and does not link |
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

The compiler can generate code for more than one machine, and in more than
one way. `NUOCC_TARGET` picks which, and only that one code generator is
built:

```bash
cmake -DNUOCC_TARGET=x86_64 ..   # the default
cmake -DNUOCC_TARGET=arm64 ..
cmake -DNUOCC_TARGET=qbe ..
cmake -DNUOCC_TARGET=m6809 ..
```

The first two write the whole program in one file, and neither needs a
runtime of its own beyond `printint`.

| Target | Assembler and linker | Notes |
|---|---|---|
| `x86_64` | GNU or clang, Linux System V | Emits `.globl`, `.type NAME, @function` and `NAME(%rip)`. |
| `arm64` | Apple clang | Emits the Apple spelling `NAME@PAGE` and `NAME@PAGEOFF` for a global variable. A Linux AArch64 assembler wants `adrp NAME` and `add ... :lo12:NAME` instead, which is the only difference. |
| `qbe` | QBE, then the same assembler and linker as the machine it targets | Writes the intermediate language of [QBE](https://c9x.me/compile/) into a file ending in `.q`, has QBE compile that into assembly, and carries on as any other target. QBE has to be on the path (`qbe`), and it is QBE which decides which machine the assembly is for. |
| `m6809` | `lwasm`, from [lwtools](http://www.lwtools.ca/) | Writes the assembly of the 6809, which is built by joining it with the runtime in [lib/6809](lib/6809) and assembling the two of them together, so a program of this target is built by one command rather than assembled and linked. `lwasm` has to be on the path, `-c` is not something it has, and a program of more than one source file is the code of each of them joined with the runtime. |

Every machine target puts the code it wrote through the peephole
optimiser once the whole of it has been written, which takes away the
instructions the machine has no need of. A rule of the pass is written in
the instructions of one machine, so each of those targets has its own; the
QBE target leaves this to QBE, which works the code over itself.

The QBE target writes a language in which every value has a name, so a
program is compiled without the compiler working out which register a value
goes in: QBE builds the SSA form and allocates the registers itself, and the
assembly it leaves is smaller than the assembly the machine targets write.

The 6809 target is the other way round: the machine has one accumulator and
an instruction which always works with it, so where the other targets hand
out registers the 6809 gives each of the four registers the walk asks for a
place in the frame of the function, and the arithmetic the machine has no
instruction for is worked out by the runtime. It gives an `int` two bytes
and a pointer two as well, since it reaches no more than 64K of memory.
The code this target writes is put through a peephole optimiser once the
whole of a program has been written, which takes away the instructions the
machine has no need of and leaves a smaller program which does the same
thing.

## Tests

The regression suite compiles, links and runs every program in `test/` and
compares what it printed with what is recorded beside it. It is run against
a compiler which has already been built:

```bash
test/runtests.sh build-arm64/nuocc
test/runtests.sh build-qbe/nuocc   # needs qbe on the path
```

A target which cannot run what it builds, such as the 6809, needs an
emulator of its machine on the path as well, and the suite runs the
programs with it. Which target a compiler was built for is what the
compiler itself says:

```bash
./build-m6809/nuocc --target
```

## LICENSE

The project is under [MIT LICENSE](LICENSE).
