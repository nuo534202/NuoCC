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
# against. A program which does not apply to a target, because the value it
# prints does not fit the width that target gives an int or because it calls
# something the target has not got, is named in test/<target>/skip.txt with
# the reason, and a run of the suite reports it as not applying.
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
if ! target=$("$nuocc" --target); then
    echo "error: $nuocc could not report its target" >&2
    exit 2
fi
if [ -z "$target" ]; then
    echo "error: $nuocc reported an empty target" >&2
    exit 2
fi
# Keep this list in sync with NUOCC_TARGET in CMakeLists.txt.
case $target in
    x86_64|arm64|qbe|m6809) ;;
    *)
        echo "error: $nuocc reported an unsupported target: $target" >&2
        exit 2
        ;;
esac
target_expected=$script_dir/expected/$target
skip_file=$script_dir/$target/skip.txt
if ! code_suffix=$("$nuocc" --code-suffix); then
    echo "error: $nuocc could not report its code suffix" >&2
    exit 2
fi
if [ -z "$code_suffix" ]; then
    echo "error: $nuocc reported an empty code suffix" >&2
    exit 2
fi

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
if ! work_dir=$(mktemp -d); then
    echo "error: could not create a test work directory" >&2
    exit 2
fi
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
       grep -Eq "^$name([[:space:]]|$)" "$skip_file"; then
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

    # A failed program or runner must not pass by printing matching output.
    run_status=0
    if [ -n "$runner" ]; then
        "$runner" "$work_dir/out" > "$work_dir/trial.out" \
            2>"$work_dir/run.log" || run_status=$?
    else
        "$work_dir/out" > "$work_dir/trial.out" \
            2>"$work_dir/run.log" || run_status=$?
    fi
    if [ "$run_status" -ne 0 ]; then
        echo "FAIL $name: the program or runner exited with status $run_status"
        sed -n '1,3p' "$work_dir/run.log"
        failed=$((failed + 1))
        continue
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
# writes is compared with it. This is how the pass a machine target has over
# the code it wrote is tested: the peephole optimiser is not allowed to
# change what a program does, so a program of that target says nothing about
# it, and the code it leaves does.
# Every source in a target directory is a recording fixture. It must have one
# companion recording with the target's code suffix, and every recording must
# have its source.
recorded_stems=$work_dir/recorded_stems
for path in "$script_dir/$target"/*.c \
            "$script_dir/$target"/*"$code_suffix"; do
    [ -f "$path" ] || continue
    name=${path##*/}
    echo "${name%.*}"
done | sort -u > "$recorded_stems"
if [ ! -s "$recorded_stems" ]; then
    echo "FAIL $target: no code recordings found in $script_dir/$target"
    failed=$((failed + 1))
fi

while IFS= read -r name; do
    source=$script_dir/$target/$name.c
    if [ ! -f "$source" ]; then
        echo "FAIL $name: recording has no source fixture: $source"
        failed=$((failed + 1))
        continue
    fi

    expected=$script_dir/$target/$name$code_suffix
    if [ ! -f "$expected" ]; then
        echo "FAIL $name: no recording found beside $source"
        failed=$((failed + 1))
        continue
    fi

    if ! fixture_dir=$(mktemp -d "$work_dir/code.XXXXXX"); then
        echo "error: could not create a work directory for $name" >&2
        exit 2
    fi
    if ! (cd "$fixture_dir" && "$nuocc" -S "$source" \
            >/dev/null 2>&1); then
        echo "FAIL $name: the compiler could not build it"
        failed=$((failed + 1))
        continue
    fi

    generated_file=$fixture_dir/$name$code_suffix
    if [ ! -f "$generated_file" ]; then
        echo "FAIL $name: the compiler wrote no $code_suffix code file"
        failed=$((failed + 1))
        continue
    fi

    if cmp -s "$expected" "$generated_file"; then
        echo "ok   $name (the code the target writes)"
        passed=$((passed + 1))
    else
        echo "FAIL $name: the code the target writes is not the recorded one"
        diff "$expected" "$generated_file" | sed -n '1,10p'
        failed=$((failed + 1))
    fi
done < "$recorded_stems"

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
