# varm - Virtual Abstract Runtime Machine

A simple educational ARM-like register-based virtual machine written in C.

## What is varm?

varm is a minimal virtual machine implementing a simplified ARM instruction set. It is designed for learning how virtual machines and assembly language work.

## Quick Start

```bash
# Build the project
./qol.sh build

# Assemble a program
./qol.sh examples/simple.asm -o program.varm

# Run a program
./qol.sh run program.varm

# Run all tests
./qol.sh test
```

## Hello World

```asm
mov r0, #42      ; Set r0 to 42
halt             ; Stop execution
```

## Project Structure

```
varm/
  examples/      - Example programs
  include/       - Header files
  src/           - Source code
    vm/          - VM core (memory, registers, instructions)
    asm/         - Assembler (lexer, parser)
    debug/       - Debugger (not yet implemented)
  tests/         - Test suite
  qol.sh         - Convenience script
```

## Build System

- **Meson** for build configuration
- **C99** standard
- **No external dependencies**

## Usage

| Command | Description |
|---------|-------------|
| `./qol.sh build` | Build varm and vasm |
| `./qol.sh asm <file.vasm> -o <file.varm>` | Assemble to bytecode |
| `./qol.sh run <file.varm>` | Execute bytecode |
| `./qol.sh test` | Run all tests |
| `./qol.sh clean` | Clean build directory |
| `./qol.sh help` | Show help |

## Examples

Located in `examples/`:

| File | Description |
|------|-------------|
| `simple.vasm` | Basic register move |
| `test.vasm` | Arithmetic operations |

## Further Reading

See [REFERENCE.md](docs/REFERENCE.md) for complete documentation of the instruction set, architecture, and assembly syntax.
