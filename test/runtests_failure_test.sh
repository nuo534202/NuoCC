#!/bin/sh
# Check that execution and temporary-directory failures fail the harness.

set -eu

source_dir=$1
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuocc-harness-test.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT
trap 'exit 1' HUP INT TERM

suite=$work_dir/test
mkdir -p "$suite/expected" "$suite/arm64" "$suite/m6809" "$work_dir/tools"
cp "$source_dir/test/runtests.sh" "$suite/runtests.sh"
printf 'expected output\n' > "$suite/expected/test1.out"
: > "$suite/test1.c"
for target in arm64 m6809; do
    : > "$suite/$target/recording.c"
    printf 'recorded code\n' > "$suite/$target/recording.s"
done

compiler=$work_dir/compiler
cat > "$compiler" <<'SCRIPT'
#!/bin/sh
case $1 in
    --target) printf '%s\n' "${NUOCC_TEST_TARGET-arm64}" ;;
    --code-suffix) printf '.s\n' ;;
    -S) printf 'recorded code\n' > recording.s ;;
    -o)
        cat > "$2" <<'PROGRAM'
#!/bin/sh
if [ "${NUOCC_TEST_OUTPUT:-matching}" = matching ]; then
    printf 'expected output\n'
fi
if [ "${NUOCC_TEST_EXIT:-0}" -ne 0 ]; then
    echo 'native diagnostic' >&2
fi
exit "${NUOCC_TEST_EXIT:-0}"
PROGRAM
        if [ "${NUOCC_TEST_EXECUTABLE:-1}" -eq 1 ]; then
            chmod +x "$2"
        fi
        ;;
    *) exit 1 ;;
esac
SCRIPT

runner=$work_dir/runner
cat > "$runner" <<'SCRIPT'
#!/bin/sh
if [ "${NUOCC_TEST_RUNNER_EXIT:-0}" -ne 0 ]; then
    if [ "${NUOCC_TEST_OUTPUT:-matching}" = matching ]; then
        printf 'expected output\n'
    fi
    echo 'runner diagnostic' >&2
    exit "$NUOCC_TEST_RUNNER_EXIT"
fi
exec "$@"
SCRIPT

NUOCC_TEST_REAL_MKTEMP=$(command -v mktemp)
export NUOCC_TEST_REAL_MKTEMP
cat > "$work_dir/tools/mktemp" <<'SCRIPT'
#!/bin/sh
if [ "${NUOCC_TEST_MKTEMP_FAIL:-}" = work ] ||
   { [ "${NUOCC_TEST_MKTEMP_FAIL:-}" = fixture ] && [ "$#" -gt 1 ]; }; then
    exit 1
fi
exec "$NUOCC_TEST_REAL_MKTEMP" "$@"
SCRIPT
chmod +x "$compiler" "$runner" "$work_dir/tools/mktemp"

run_harness() {
    expected_status=$1
    expected_message=$2
    shift 2
    actual_status=0
    env NUOCC_TEST_TARGET=arm64 NUOCC_TEST_OUTPUT=matching NUOCC_TEST_EXIT=0 \
        NUOCC_TEST_RUNNER_EXIT=0 NUOCC_TEST_EXECUTABLE=1 \
        NUOCC_TEST_MKTEMP_FAIL= "$@" \
        /bin/sh "$suite/runtests.sh" "$compiler" \
        > "$work_dir/result" 2>&1 || actual_status=$?
    if [ "$actual_status" -ne "$expected_status" ] ||
       ! grep -Fq "$expected_message" "$work_dir/result"; then
        echo "unexpected harness result; expected status $expected_status and: $expected_message" >&2
        cat "$work_dir/result" >&2
        exit 1
    fi
    if [ "$expected_status" -eq 1 ] &&
       grep -Fxq 'ok   test1' "$work_dir/result"; then
        echo 'a failed program was counted as passing' >&2
        cat "$work_dir/result" >&2
        exit 1
    fi
}

run_harness 0 '2 passed, 0 failed, 0 not applying to arm64'
run_harness 0 '2 passed, 0 failed, 0 not applying to m6809' \
    NUOCC_TEST_TARGET=m6809 NUOCC_RUNNER="$runner"
for output in matching empty; do
    run_harness 1 'FAIL test1: the program or runner exited with status 7' \
        NUOCC_TEST_EXIT=7 NUOCC_TEST_OUTPUT="$output"
    grep -Fxq 'native diagnostic' "$work_dir/result"
    run_harness 1 'FAIL test1: the program or runner exited with status 9' \
        NUOCC_TEST_TARGET=m6809 NUOCC_RUNNER="$runner" \
        NUOCC_TEST_RUNNER_EXIT=9 NUOCC_TEST_OUTPUT="$output"
    grep -Fxq 'runner diagnostic' "$work_dir/result"
done

run_harness 1 'FAIL test1: the program or runner exited with status 127' \
    NUOCC_TEST_TARGET=m6809 NUOCC_RUNNER="$work_dir/missing-runner"
run_harness 1 'FAIL test1: the program or runner exited with status 126' \
    NUOCC_TEST_EXECUTABLE=0
run_harness 2 'error: could not create a test work directory' \
    PATH="$work_dir/tools:$PATH" NUOCC_TEST_MKTEMP_FAIL=work
run_harness 2 'error: could not create a work directory for recording' \
    PATH="$work_dir/tools:$PATH" NUOCC_TEST_MKTEMP_FAIL=fixture

echo 'harness failure checks passed'
