#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
VARM="$BUILD_DIR/src/varm"
VASM="$BUILD_DIR/src/vasm"

usage() {
    echo "Usage: $0 <command> [args]"
    echo ""
    echo "Commands:"
    echo "  build        Build the VM and assembler"
    echo "  run <file>   Run a .varm program"
    echo "  asm <file>   Assemble a .vasm file to .varm"
    echo "  asmrun <file> Assemble and run a .vasm file immediately"
    echo "  test         Run all tests"
    echo "  clean        Clean build directory"
    echo "  help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 run program.varm"
    echo "  $0 asm program.vasm -o program.varm"
    echo "  $0 asmrun program.vasm"
    echo "  $0 test"
}

build() {
    echo "Building VM and assembler..."
    cd "$SCRIPT_DIR"
    rm -rf "$BUILD_DIR"
    meson setup "$BUILD_DIR"
    meson compile -C "$BUILD_DIR"
    echo "Build complete."
}

run() {
    if [ -z "$1" ]; then
        echo "Error: No program specified"
        echo "Usage: $0 run <program.vm>"
        exit 1
    fi

    if [ ! -f "$VARM" ]; then
        echo "VM not built. Run '$0 build' first."
        exit 1
    fi

    cd "$SCRIPT_DIR"

    if [ ! -f "$1" ]; then
        echo "Error: File not found: $1"
        exit 1
    fi

    "$VARM" "$@"
}

asm() {
    if [ -z "$1" ]; then
        echo "Error: No input file specified"
        echo "Usage: $0 asm <input.vasm> [-o <output.varm>]"
        exit 1
    fi

    if [ ! -f "$VASM" ]; then
        echo "Assembler not built. Run '$0 build' first."
        exit 1
    fi

    cd "$SCRIPT_DIR"

    "$VASM" "$@"
}

asmrun() {
    if [ -z "$1" ]; then
        echo "Error: No input file specified"
        echo "Usage: $0 asmrun <input.vasm>"
        exit 1
    fi

    if [ ! -f "$VASM" ]; then
        echo "Assembler not built. Run '$0 build' first."
        exit 1
    fi

    if [ ! -f "$VARM" ]; then
        echo "VM not built. Run '$0 build' first."
        exit 1
    fi

    cd "$SCRIPT_DIR"

    if [ ! -f "$1" ]; then
        echo "Error: File not found: $1"
        exit 1
    fi

    temp_file=$(mktemp /tmp/varm_XXXXXX.varm)
    trap "rm -f '$temp_file'" EXIT

    "$VASM" "$1" -o "$temp_file"
    "$VARM" "$temp_file"
}

test() {
    if [ ! -f "$VARM" ]; then
        echo "VM not built. Run '$0 build' first."
        exit 1
    fi

    echo "Running tests..."

    cd "$SCRIPT_DIR"

    echo ""
    echo "=== Unit Tests ==="
    if [ -f "$BUILD_DIR/src/unit_lexer" ]; then
        "$BUILD_DIR/src/unit_lexer"
        UNIT_RESULT=$?
        if [ $UNIT_RESULT -ne 0 ]; then
            echo "Unit tests failed!"
            exit 1
        fi
    else
        echo "Unit tests not built. Run '$0 build' first."
        exit 1
    fi

    if [ -f "$BUILD_DIR/src/unit_parser" ]; then
        "$BUILD_DIR/src/unit_parser"
        UNIT_RESULT=$?
        if [ $UNIT_RESULT -ne 0 ]; then
            echo "Unit tests failed!"
            exit 1
        fi
    else
        echo "Parser unit tests not built. Run '$0 build' first."
        exit 1
    fi

    echo ""
    echo "=== Integration Tests (Example Programs) ==="

    FAILED=0
    PASSED=0

    for asm_file in examples/*.vasm; do
        if [ -f "$asm_file" ]; then
            basename=$(basename "$asm_file" .vasm)
            vm_file="/tmp/test_${basename}.varm"

            if "$VASM" "$asm_file" -o "$vm_file" > /dev/null 2>&1; then
                if "$VARM" "$vm_file" > /dev/null 2>&1; then
                    echo "  PASS: $basename"
                    PASSED=$((PASSED + 1))
                else
                    echo "  FAIL: $basename (runtime error)"
                    FAILED=$((FAILED + 1))
                fi
            else
                echo "  FAIL: $basename (assembly error)"
                FAILED=$((FAILED + 1))
            fi
        fi
    done

    echo ""
    echo "Results: $PASSED passed, $FAILED failed"

    if [ $FAILED -gt 0 ]; then
        exit 1
    fi
}

clean() {
    echo "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    echo "Clean complete."
}

case "$1" in
    build)
        build
        ;;
    run)
        shift
        run "$@"
        ;;
    asm)
        shift
        asm "$@"
        ;;
    asmrun)
        shift
        asmrun "$@"
        ;;
    test)
        test
        ;;
    clean)
        clean
        ;;
    help|--help|-h|"")
        usage
        ;;
    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;
esac
