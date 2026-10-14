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
    expected=$script_dir/expected/$name.out

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
    "$work_dir/out" > "$work_dir/trial.out"

    if cmp -s "$expected" "$work_dir/trial.out"; then
        echo "ok   $name"
        passed=$((passed + 1))
    else
        echo "FAIL $name: the program printed something else"
        diff "$expected" "$work_dir/trial.out" | sed -n '1,10p'
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
echo "$passed passed, $failed failed"

[ "$failed" -eq 0 ]
