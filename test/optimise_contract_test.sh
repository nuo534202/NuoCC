#!/bin/sh
#
# Check the optimizer contract visible in the final syntax tree.

set -eu

nuocc=$1
work_dir=$(mktemp -d "${TMPDIR:-/tmp}/nuocc-optimise-test.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT

source=$work_dir/contract.c
tree=$work_dir/tree
cat > "$source" <<'SOURCE'
int value;
char small;

int main() {
    value = -3;
    small = 3;

    print value * 2;
    print small * 2;
    print 1 / 0;

    return(0);
}
SOURCE

"$nuocc" -S -T "$source" > "$tree"
sed -n '/^EOF$/,$p' "$tree" > "$tree.semantic"

grep -Fq 'operator * : int' "$tree.semantic" || {
    echo "a signed multiplication was rewritten as a shift" >&2
    exit 1
}

grep -Fq 'operator << : char' "$tree.semantic" || {
    echo "an unsigned char multiplication was not rewritten as a shift" >&2
    exit 1
}

grep -Fq 'operator / : char' "$tree.semantic" || {
    echo "a division by zero was folded or rejected" >&2
    exit 1
}

if grep -Fq 'operator << : int' "$tree.semantic"; then
    echo "a signed multiplication remains rewritten as a shift" >&2
    exit 1
fi
