#!/bin/sh
#
# Check that the driver preserves argv entries which a shell would interpret.

set -eu

nuocc=$1
source_dir=$2
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuocc-tool-test.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT
trap 'exit 1' HUP INT TERM

tool_dir="$work_dir/tools '; \$HOME \`marker\`"
mkdir "$tool_dir"

fake_cpp=$tool_dir/cpp
cat > "$fake_cpp" <<'SCRIPT'
#!/bin/sh
[ -z "${NUOCC_CPP_LOG:-}" ] || printf '%s\n' "$@" > "$NUOCC_CPP_LOG"
if [ "${NUOCC_CPP_EXIT:-0}" -ne 0 ]; then
    exit "$NUOCC_CPP_EXIT"
fi
printf 'int main() { return(0); }\n'
SCRIPT

fake_cc=$tool_dir/cc
cat > "$fake_cc" <<'SCRIPT'
#!/bin/sh
printf '%s\n' "$@" > "$NUOCC_CC_LOG"
exit "${NUOCC_CC_EXIT:-0}"
SCRIPT

fake_qbe=$tool_dir/qbe
cat > "$fake_qbe" <<'SCRIPT'
#!/bin/sh
[ -z "${NUOCC_QBE_LOG:-}" ] || printf '%s\n' "$@" > "$NUOCC_QBE_LOG"
while [ "$#" -gt 0 ]; do
    if [ "$1" = "-o" ]; then
        shift
        : > "$1"
        exit "${NUOCC_QBE_EXIT:-0}"
    fi
    shift
done
exit 1
SCRIPT

fake_lwasm=$tool_dir/lwasm
cat > "$fake_lwasm" <<'SCRIPT'
#!/bin/sh
printf '%s\n' "$@" > "$NUOCC_LWASM_LOG"
output=
while [ "$#" -gt 0 ]; do
    if [ "$1" = "-o" ]; then
        shift
        output=$1
    fi
    shift
done
if [ "${NUOCC_LWASM_EXIT:-0}" -ne 0 ]; then
    exit "$NUOCC_LWASM_EXIT"
fi
tee "$NUOCC_LWASM_STDIN" > "$output"
SCRIPT
chmod 700 "$fake_cpp" "$fake_cc" "$fake_qbe" "$fake_lwasm"

target=$("$nuocc" --target)
stem="source '; \$HOME \`marker\`"
source=$work_dir/$stem.c
printf 'int main() { return(0); }\n' > "$source"
cpp_log=$work_dir/cpp-arguments
expected=$work_dir/expected
printf '%s\n' '-nostdinc' "-I$source_dir/lib/include" "$source" > "$expected"

(
    cd "$work_dir"
    PATH="$tool_dir:$PATH" NUOCC_CPP_LOG="$cpp_log" \
        "$nuocc" -S "$source"
)
cmp -s "$expected" "$cpp_log" || {
    echo "the preprocessor received different arguments" >&2
    exit 1
}

if (
    cd "$work_dir"
    PATH="$tool_dir:$PATH" NUOCC_CPP_LOG="$cpp_log" \
        NUOCC_CPP_EXIT=9 "$nuocc" -S "$source"
) >/dev/null 2>&1; then
    echo "a preprocessor failure was accepted" >&2
    exit 1
fi

preserved=$work_dir/$stem.s
printf 'keep this file\n' > "$preserved"
if (
    cd "$work_dir"
    PATH="$tool_dir:$PATH" NUOCC_CPP_LOG="$cpp_log" \
        NUOCC_CPP_EXIT=9 "$nuocc" -S "$source"
) >/dev/null 2>&1; then
    echo "a preprocessor failure was accepted" >&2
    exit 1
fi
if [ "$(cat "$preserved")" != "keep this file" ]; then
    echo "a preexisting output file was removed" >&2
    exit 1
fi

if [ "$target" != "m6809" ]; then
    for bad_command in "''" "   " "'cc" 'cc\'; do
        if (
            cd "$work_dir"
            PATH="$tool_dir:$PATH" CC="$bad_command" "$nuocc" -c "$source"
        ) >/dev/null 2>&1; then
            echo "an invalid tool command was accepted" >&2
            exit 1
        fi
    done
fi

if [ "$target" = "m6809" ]; then
    output=$work_dir/program
    lwasm_log=$work_dir/lwasm-arguments
    lwasm_input=$work_dir/lwasm-input
    printf '%s\n' '--format=raw' '-o' "$output" '-' > "$expected"
    (
        cd "$work_dir"
        PATH="$tool_dir:$PATH" NUOCC_LWASM_LOG="$lwasm_log" \
            NUOCC_LWASM_STDIN="$lwasm_input" "$nuocc" -o "$output" "$source"
    )
    cmp -s "$expected" "$lwasm_log" || {
        echo "lwasm received different arguments" >&2
        exit 1
    }
    runtime=$source_dir/lib/6809/runtime.s
    runtime_size=$(wc -c < "$runtime" | tr -d ' ')
    dd if="$lwasm_input" of="$work_dir/runtime-prefix" bs=1 \
        count="$runtime_size" 2>/dev/null
    cmp -s "$runtime" "$work_dir/runtime-prefix" || {
        echo "lwasm input did not begin with the runtime" >&2
        exit 1
    }
    if [ "$(wc -c < "$lwasm_input")" -le "$runtime_size" ]; then
        echo "lwasm input held no generated code" >&2
        exit 1
    fi
    if (
        cd "$work_dir"
        PATH="$tool_dir:$PATH" NUOCC_LWASM_LOG="$lwasm_log" \
            NUOCC_LWASM_EXIT=6 "$nuocc" -o "$work_dir/failed" "$source"
    ) >/dev/null 2>&1; then
        echo "an lwasm failure was accepted" >&2
        exit 1
    fi
    exit 0
fi

cc_log=$work_dir/cc-arguments
cc_expected=$work_dir/cc-expected
qbe_log=$work_dir/qbe-arguments
printf '%s\n' '-c' "$stem.s" '-o' "$stem.o" > "$cc_expected"
(
    cd "$work_dir"
    PATH="$tool_dir:$PATH" CC="\"$fake_cc\"" NUOCC_CC_LOG="$cc_log" \
        NUOCC_QBE_LOG="$qbe_log" "$nuocc" -c "$source"
)
cmp -s "$cc_expected" "$cc_log" || {
    echo "the assembler received different arguments" >&2
    exit 1
}

printf 'preexisting assembly\n' > "$work_dir/$stem.s"
(
    cd "$work_dir"
    PATH="$tool_dir:$PATH" CC="\"$fake_cc\"" NUOCC_CC_LOG="$cc_log" \
        NUOCC_QBE_LOG="$qbe_log" "$nuocc" -c "$source"
)
if [ ! -f "$work_dir/$stem.s" ]; then
    echo "a preexisting assembly file was removed" >&2
    exit 1
fi

if [ "$target" = "qbe" ]; then
    printf '%s\n' '-o' "$stem.s" "$stem.q" > "$expected"
    cmp -s "$expected" "$qbe_log" || {
        echo "qbe received different arguments" >&2
        exit 1
    }
fi

link_output=$work_dir/program
link_expected=$work_dir/link-expected
printf '%s\n' '-o' "$link_output" "$stem.o" \
    "$source_dir/lib/printint.c" > "$link_expected"
(
    cd "$work_dir"
    PATH="$tool_dir:$PATH" CC="\"$fake_cc\"" NUOCC_CC_LOG="$cc_log" \
        NUOCC_QBE_LOG="$qbe_log" "$nuocc" -o "$link_output" "$source"
)
cmp -s "$link_expected" "$cc_log" || {
    echo "the linker received different arguments" >&2
    exit 1
}
if [ "$target" = "qbe" ]; then
    printf '%s\n' '-o' "$stem.s" "$stem.q" > "$expected"
    cmp -s "$expected" "$qbe_log" || {
        echo "qbe did not produce the link input" >&2
        exit 1
    }
    if (
        cd "$work_dir"
        PATH="$tool_dir:$PATH" CC="\"$fake_cc\"" \
            NUOCC_QBE_EXIT=5 "$nuocc" -c "$source"
    ) >/dev/null 2>&1; then
        echo "a qbe failure was accepted" >&2
        exit 1
    fi
fi

ccache_dir=$work_dir/ccache-path
quoted_cc_dir="$work_dir/compiler path"
mkdir "$ccache_dir" "$quoted_cc_dir"
cat > "$ccache_dir/ccache" <<'SCRIPT'
#!/bin/sh
exec "$@"
SCRIPT
cp "$fake_cc" "$quoted_cc_dir/cc"
chmod 700 "$ccache_dir/ccache" "$quoted_cc_dir/cc"
(
    cd "$work_dir"
    PATH="$ccache_dir:$tool_dir:$PATH" \
        CC="ccache '$quoted_cc_dir/cc'" NUOCC_CC_LOG="$cc_log" \
        "$nuocc" -c "$source"
)
cmp -s "$cc_expected" "$cc_log" || {
    echo "the wrapped compiler received different arguments" >&2
    exit 1
}

newline='
'
continued_cc="\"$fake_cc\"\\$newline"
(
    cd "$work_dir"
    PATH="$tool_dir:$PATH" CC="$continued_cc" \
        NUOCC_CC_LOG="$cc_log" "$nuocc" -c "$source"
)
cmp -s "$cc_expected" "$cc_log" || {
    echo "a continued command was not joined" >&2
    exit 1
}

if (
    cd "$work_dir"
    PATH="$tool_dir:$PATH" CC="\"$fake_cc\"" NUOCC_CC_LOG="$cc_log" \
        NUOCC_CC_EXIT=8 "$nuocc" -c "$source"
) >/dev/null 2>&1; then
    echo "an assembler failure was accepted" >&2
    exit 1
fi
