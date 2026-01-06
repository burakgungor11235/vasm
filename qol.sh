#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
ASM_VM="$BUILD_DIR/src/asm-vm"
VASM="$BUILD_DIR/src/vasm"

usage() {
    echo "Usage: $0 <command> [args]"
    echo ""
    echo "Commands:"
    echo "  build        Build the VM and assembler"
    echo "  run <file>   Run a .vm program"
    echo "  asm <file>   Assemble an .asm file to .vm"
    echo "  test         Run all tests"
    echo "  clean        Clean build directory"
    echo "  help         Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0 build"
    echo "  $0 run program.vm"
    echo "  $0 asm program.asm -o program.vm"
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

    if [ ! -f "$ASM_VM" ]; then
        echo "VM not built. Run '$0 build' first."
        exit 1
    fi

    cd "$SCRIPT_DIR"

    if [ ! -f "$1" ]; then
        echo "Error: File not found: $1"
        exit 1
    fi

    "$ASM_VM" "$@"
}

asm() {
    if [ -z "$1" ]; then
        echo "Error: No input file specified"
        echo "Usage: $0 asm <input.asm> [-o <output.vm>]"
        exit 1
    fi

    if [ ! -f "$VASM" ]; then
        echo "Assembler not built. Run '$0 build' first."
        exit 1
    fi

    cd "$SCRIPT_DIR"

    "$VASM" "$@"
}

test() {
    if [ ! -f "$ASM_VM" ]; then
        echo "VM not built. Run '$0 build' first."
        exit 1
    fi

    echo "Running tests..."

    cd "$SCRIPT_DIR"

    FAILED=0
    PASSED=0

    for asm_file in examples/*.asm; do
        if [ -f "$asm_file" ]; then
            basename=$(basename "$asm_file" .asm)
            vm_file="/tmp/test_${basename}.vm"

            if "$VASM" "$asm_file" -o "$vm_file" > /dev/null 2>&1; then
                if "$ASM_VM" "$vm_file" > /dev/null 2>&1; then
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
