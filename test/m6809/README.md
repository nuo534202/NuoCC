# Running the 6809 tests

The compiler built with `-DNUOCC_TARGET=m6809` writes the assembly of the
6809 rather than of the machine it runs on, so a program it builds cannot
be run by the system the compiler runs on. It is run by an emulator of the
machine instead, which is written by the Fuzix project:

```sh
git clone https://github.com/DoctorWkt/Fuzemsys.git
cd Fuzemsys/emulators
make emu6809
```

Put `emu6809` on the path, or name it in `NUOCC_RUNNER` when the suite is
run, and build and run the tests:

```sh
cmake -DNUOCC_TARGET=m6809 ..
test/runtests.sh build-m6809/nuocc
```

`runtests.sh` asks the compiler which target it was built for with
`--target`, and runs the programs it builds with `emu6809` when that target
is `m6809`. The emulator is what the runtime in `lib/6809/runtime.s` is
written for: it writes a character, writes a value out in decimal and
leaves the program by watching the memory it is told to write, which is
what a machine of its own would find in its operating system instead.

## What differs from the other targets

The 6809 gives an `int` two bytes and a pointer two as well, since it
reaches no more than 64K of memory, and it holds the bytes of a value with
the high one first. Two kinds of test therefore say something else:

* A program which asks for the room a value takes, or which reads the
  bytes of one, prints what the machine it was run on says. Those programs
  have a recorded output of their own under `test/expected/m6809/`, and
  the recorded output of the other targets is what they are compared
  against.
* A program which works with a value that does not fit the two bytes this
  machine gives an int cannot be built for it at all. Those programs are
  named in `skip.txt`, with the value which is too wide, and a run of the
  suite reports them as not applying to this target.

Everything else is the same for every target, so a test which is not in
either of those places has to print exactly what the other targets print.

## The code the target writes

The pass this target has over the code it writes is the peephole
optimiser, and the pass is not allowed to change what a program prints, so
the programs the suite runs say nothing about whether it works. What says
something about it is the code itself: `optimise.c` is a program the
optimiser is written for, it is compiled with `-S`, and the assembly which
comes out is compared with `optimise.s` beside it, which is the code the
pass leaves. A change to the pass, or one which stops it firing, is caught
there.
