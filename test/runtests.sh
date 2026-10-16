#!/bin/sh
#
# Run the NuoCC regression suite.
#
# A program in test/testN.c is compiled and linked by the compiler itself,
# run, and what it printed is compared against the recorded output in
# test/expected/testN.out. A program in test/errors is the other kind of
# test: the compiler has to refuse it, and the message it is refused with is
# compared against the recorded one beside it.
#
# A target which gives a value a width of its own, or which holds the bytes
# of a value in an order of its own, prints something else for a program
# which asks about them. Such a target records what it prints under
# test/expected/<target>/, and that file is what its run is compared
# against. A program which cannot be built for a target at all, because the
# value it prints does not fit the width that target gives an int, is named
# in test/<target>/skip.txt and is reported as not applying to it.
#
# Usage: test/runtests.sh [path to the nuocc binary]
#
# The binary to test is the one named on the command line, or the one which
# has been built for this host: an Apple host assembles the arm64 target
# and every other host assembles the x86_64 one, which is also the target
# CMake builds by default. Naming one the host cannot assemble shows up as
# assembly which will not build. The assembler and the linker the compiler
# calls can be named with the CC environment variable.

set -u

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(dirname -- "$script_dir")
if [ $# -gt 0 ]; then
    nuocc=$1
else
    case $(uname -s) in
        Darwin) first=$root_dir/build-arm64/nuocc
                second=$root_dir/build/nuocc ;;
        *)      first=$root_dir/build/nuocc
                second=$root_dir/build-arm64/nuocc ;;
    esac

    if [ -x "$first" ]; then
        nuocc=$first
    elif [ -x "$second" ]; then
        nuocc=$second
    else
        nuocc=$first
    fi
fi

if [ ! -x "$nuocc" ]; then
    echo "error: $nuocc is not an executable" >&2
    echo "build the compiler first, or name one on the command line" >&2
    exit 2
fi

# The compiler is run from a directory of its own, so the name it was given
# has to be an absolute one by the time it is used.
case $nuocc in
    /*) ;;
    *) nuocc=$(CDPATH= cd -- "$(dirname -- "$nuocc")" && pwd)/$(basename -- "$nuocc") ;;
esac

# Which target this compiler was built for, which says which of the tests
# apply to it, what it prints, and what runs the programs it builds.
target=$("$nuocc" --target)
target_expected=$script_dir/expected/$target
skip_file=$script_dir/$target/skip.txt

# A program for the machine this compiler runs on is run by the system. One
# for another machine is run by an emulator of it, which has to be on the
# path, the way the tool which assembles the code is.
case $target in
    m6809) runner=${NUOCC_RUNNER:-emu6809} ;;
    *)     runner="" ;;
esac

skipped=0

# The compiler writes its assembly next to itself, so it is run in a
# directory of its own and nothing is left behind in the project.
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

passed=0
failed=0

# The programs which have to compile and run, in numeric order.
programs=$(
    for expected in "$script_dir"/expected/*.out; do
        [ -f "$expected" ] || continue
        name=$(basename "$expected" .out)
        echo "$(echo "$name" | tr -cd 0-9) $name"
    done | sort -n | cut -d' ' -f2
)

for name in $programs; do
    # A program this target cannot build is not one it fails.
    if [ -f "$skip_file" ] &&
       grep -q "^$name\b" "$skip_file"; then
        skipped=$((skipped + 1))
        continue
    fi

    # What this target prints for the program, which is what the shared
    # output says unless the target recorded one of its own.
    expected=$script_dir/expected/$name.out
    if [ -f "$target_expected/$name.out" ]; then
        expected=$target_expected/$name.out
    fi

    # A test is one source file, or several when it is about what more than
    # one file of a program sees: they are named testN.c, testN_2.c and so
    # on, and they are all compiled together.
    set -- "$script_dir/$name.c"
    for extra in "$script_dir/$name"_*.c; do
        [ -f "$extra" ] || continue
        set -- "$@" "$extra"
    done

    if ! (cd "$work_dir" && "$nuocc" -o out "$@" >compiler.log 2>&1); then
        echo "FAIL $name: the compiler could not build it"
        sed -n '1,3p' "$work_dir/compiler.log"
        failed=$((failed + 1))
        continue
    fi

    # What the program itself returns is not checked: a test is free to
    # end without a return statement.
    if [ -n "$runner" ]; then
        "$runner" "$work_dir/out" > "$work_dir/trial.out" 2>"$work_dir/emu.log"
    else
        "$work_dir/out" > "$work_dir/trial.out"
    fi

    if cmp -s "$expected" "$work_dir/trial.out"; then
        echo "ok   $name"
        passed=$((passed + 1))
    else
        echo "FAIL $name: the program printed something else"
        diff "$expected" "$work_dir/trial.out" | sed -n '1,10p'
        failed=$((failed + 1))
    fi
done

# The code a target writes may be recorded as well, and what the compiler
# writes is compared with it. This is how the pass a target has over the code
# it wrote is tested: the pass of the 6809 is the peephole optimiser, which
# is not allowed to change what a program does, so a program of that target
# says nothing about it, and the code it leaves does.
for expected in "$script_dir/$target"/*.s; do
    [ -f "$expected" ] || continue
    name=$(basename -- "$expected" .s)

    if ! (cd "$work_dir" && "$nuocc" -S "$script_dir/$target/$name.c" \
            >/dev/null 2>&1); then
        echo "FAIL $name: the compiler could not build it"
        failed=$((failed + 1))
        continue
    fi

    if cmp -s "$expected" "$work_dir/$name.s"; then
        echo "ok   $name (the code the target writes)"
        passed=$((passed + 1))
    else
        echo "FAIL $name: the code the target writes is not the recorded one"
        diff "$expected" "$work_dir/$name.s" | sed -n '1,10p'
        failed=$((failed + 1))
    fi
done

# The programs which have to be refused, and what the compiler has to say.
for source in "$script_dir"/errors/*.c; do
    [ -f "$source" ] || continue
    name=$(basename "$source" .c)
    expected=$script_dir/errors/$name.err

    if [ ! -f "$expected" ]; then
        echo "FAIL $name: no recorded error message"
        failed=$((failed + 1))
        continue
    fi

    # Nothing has to be assembled or linked to see that the source is
    # refused, so only the assembly is asked for.
    if (cd "$work_dir" && "$nuocc" -S "$source" >/dev/null 2>trial.err); then
        echo "FAIL $name: the compiler accepted it"
        failed=$((failed + 1))
        continue
    fi

    if cmp -s "$expected" "$work_dir/trial.err"; then
        echo "ok   $name"
        passed=$((passed + 1))
    else
        echo "FAIL $name: the compiler said something else"
        diff "$expected" "$work_dir/trial.err" | sed -n '1,10p'
        failed=$((failed + 1))
    fi
done

echo
echo "$passed passed, $failed failed, $skipped not applying to $target"

[ "$failed" -eq 0 ]
