#!/bin/sh
#
# Check that QBE functions, frames, and narrow values use legal IL widths.

set -eu

nuocc=$1
source_dir=$2
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuocc-qbe-widths.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT
trap 'exit 1' HUP INT TERM

case $nuocc in
    /*) ;;
    */*) nuocc=$(CDPATH= cd -- "$(dirname -- "$nuocc")" && pwd)/$(basename -- "$nuocc") ;;
    *)
        if ! nuocc=$(command -v "$nuocc"); then
            echo "the compiler is not available" >&2
            exit 1
        fi
        ;;
esac
source_dir=$(CDPATH= cd -- "$source_dir" && pwd)

if ! target=$("$nuocc" --target); then
    echo "the compiler could not report its target" >&2
    exit 1
fi
if [ -z "$target" ]; then
    echo "the compiler reported an empty target" >&2
    exit 1
fi
if [ "$target" != qbe ]; then
    echo "the QBE width test does not apply to this target" >&2
    exit 77
fi
if ! command -v cpp >/dev/null 2>&1; then
    echo "cpp is not available" >&2
    exit 77
fi

(
    cd "$work_dir"
    "$nuocc" -S "$source_dir/test/qbe/widths.c" || {
        echo "the compiler could not write QBE output" >&2
        exit 1
    }
    if [ ! -f widths.q ]; then
        echo "the compiler wrote no QBE output" >&2
        exit 1
    fi
    if [ ! -f "$source_dir/test/qbe/widths.q" ]; then
        echo "the recorded QBE output is missing" >&2
        exit 1
    fi
    cmp -s "$source_dir/test/qbe/widths.q" widths.q || {
        diff "$source_dir/test/qbe/widths.q" widths.q | sed -n '1,20p'
        exit 1
    }
)
if ! command -v qbe >/dev/null 2>&1; then
    echo "qbe is not available" >&2
    exit 77
fi
cc=${CC:-cc}

# Check the executable even when CC also supplies arguments for it.
case $cc in
    \"*) cc_command=${cc#\"}; cc_command=${cc_command%%\"*} ;;
    \'*) cc_command=${cc#\'}; cc_command=${cc_command%%\'*} ;;
    *)   cc_command=${cc%%[[:space:]]*} ;;
esac
if ! command -v "$cc_command" >/dev/null 2>&1; then
    echo "$cc_command is not available" >&2
    exit 77
fi

(
    cd "$work_dir"
    "$nuocc" -o widths "$source_dir/test/qbe/widths.c" || {
        echo "the generated program could not be built" >&2
        exit 1
    }
    ./widths || {
        status=$?
        echo "the generated program returned failure: $status" >&2
        exit 1
    }
)
