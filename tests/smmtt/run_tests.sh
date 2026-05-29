#!/bin/bash
set -e

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
NEMU_HOME=${NEMU_HOME:-$(cd "$SCRIPT_DIR/../.." && pwd)}
NEMU_BIN="$NEMU_HOME/build/riscv64-nemu-interpreter"

show_list() {
    echo "================================================================================"
    echo "                    Smmtt Extension Test Suite - Modules List"
    echo "================================================================================"
    printf " %-14s | %-26s | %-33s\n" "Module ID" "Source File" "Description"
    echo "---------------+----------------------------+-----------------------------------"
    for f in "$SCRIPT_DIR"/tests/test_*.c; do
        if [ -f "$f" ]; then
            filename=$(basename "$f")
            mod_id=$(echo "$filename" | sed -E 's/test_([^.]+)\.c/\1/')
            # Extract description from head comments (e.g. "// Description: ...")
            desc=$(grep -E '^//[[:space:]]*Description:[[:space:]]*' "$f" | sed -E 's/^\/\/[[:space:]]*Description:[[:space:]]*//' | head -n 1)
            if [ -z "$desc" ]; then
                desc="No description provided"
            fi
            printf " %-14s | %-26s | %-33s\n" "$mod_id" "tests/$filename" "$desc"
        fi
    done
    echo "================================================================================"
    echo "Usage:"
    echo "  $0 <Module ID>     (e.g., $0 mpt_perm)"
    echo "  $0 all             (Runs all test modules linked together)"
    echo "  $0 -l / --list     (Display this module list)"
    exit 0
}

if [ $# -ne 1 ]; then
    show_list
fi

ARG=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [ "$ARG" = "-l" ] || [ "$ARG" = "--list" ]; then
    show_list
fi

if [ "$ARG" = "all" ]; then
    MODULE="all"
else
    # Check dynamically if tests/test_$ARG.c exists
    if [ -f "$SCRIPT_DIR/tests/test_$ARG.c" ]; then
        MODULE="$ARG"
    else
        echo "Error: Unknown module: $1"
        echo "Use '$0 --list' to view available modules."
        exit 1
    fi
fi

# Build target
echo "Building test module: $MODULE"
make -C "$SCRIPT_DIR" "$MODULE"

TEST_BIN="$SCRIPT_DIR/build/smmtt-$MODULE.bin"

if [ ! -f "$TEST_BIN" ]; then
    echo "Error: Test binary not found: $TEST_BIN"
    exit 1
fi

if [ ! -f "$NEMU_BIN" ]; then
    echo "Error: NEMU binary not found: $NEMU_BIN"
    echo "Please build NEMU first: make -j\$(nproc)"
    exit 1
fi

echo "Running: $NEMU_BIN -b $TEST_BIN"
"$NEMU_BIN" -b "$TEST_BIN"
