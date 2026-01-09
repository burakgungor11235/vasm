# varm Assembler Parser Documentation

## Overview

The varm assembler translates ARM-like assembly code into virtual machine bytecode. This document describes the instruction format, parsing architecture, and endianness conventions.

## Endianness

**All multi-byte values are stored in little-endian byte order.**

This means:
- The least significant byte (LSB) is stored at the lowest memory address
- The most significant byte (MSB) is stored at the highest memory address

Example: The 32-bit instruction `0x00E0082A` is stored in memory as:

```
Address +0: 0x2A  (bits 0-7)
Address +1: 0x08  (bits 8-15)
Address +2: 0xE0  (bits 16-23)
Address +3: 0x00  (bits 24-31)
```

## Instruction Format

All varm instructions are **32 bits (4 bytes)** wide. The format follows a consistent field layout:

```
┌───────────────────────────────────────────────────────────────────────────┐
│                           32-bit Instruction                              │
├───────────┬───────────┬───────────┬───────────┬───────────────────────────┤
│  Opcode   │  Cond     │    Rn     │    Rd     │        Operand            │
│  (8 bit)  │  (4 bit)  │  (4 bit)  │  (4 bit)  │        (12 bit)           │
└───────────┴───────────┴───────────┴───────────┴───────────────────────────┘
 31       24 23       20 19       16 15       12 11                        0
```

### Field Descriptions

| Field  | Bits    | Description                                      |
|--------|---------|--------------------------------------------------|
| Opcode | 31:24   | Operation code (0x00-0xFF) - instruction type    |
| Cond   | 23:20   | Condition code (see COND_* enums)                |
| Rn     | 19:16   | First source register for ALU operations         |
| Rd     | 15:12   | Destination register                             |
| Operand| 11:0    | Second operand (immediate or register reference) |

### Byte Layout (Little-Endian)

```
Memory Address:  [addr+3]  [addr+2]  [addr+1]  [addr+0]
                 ┌────────┬────────┬────────┬────────┐
                 │ Opcode │  Cond  │  Rn/Rd │  Rd/Op │
                 │   00   │    E   │    0   │  08 2A │
                 └────────┴────────┴────────┴────────┘
                 MSB                                LSB
```

### Condition Codes (4 bits, bits 23:20)

| Code | Name  | Description                      | Flags Tested     |
|------|-------|----------------------------------|------------------|
| 0x0  | EQ    | Equal                            | Z = 1            |
| 0x1  | NE    | Not Equal                        | Z = 0            |
| 0x2  | CS/HS | Carry Set / Unsigned Higher      | C = 1            |
| 0x3  | CC/LO | Carry Clear / Unsigned Lower     | C = 0            |
| 0x4  | MI    | Minus / Negative                 | N = 1            |
| 0x5  | PL    | Plus / Non-Negative              | N = 0            |
| 0x6  | VS    | Overflow Set                     | V = 1            |
| 0x7  | VC    | Overflow Clear                   | V = 0            |
| 0x8  | HI    | Unsigned Higher                  | C = 1 && Z = 0   |
| 0x9  | LS    | Unsigned Lower or Same           | C = 0 \|\| Z = 1   |
| 0xA  | GE    | Signed Greater or Equal          | N == V           |
| 0xB  | LT    | Signed Less Than                 | N != V           |
| 0xC  | GT    | Signed Greater Than              | Z = 0 && N == V  |
| 0xD  | LE    | Signed Less or Equal             | Z = 1 \|\| N != V  |
| 0xE  | AL    | Always (unconditional)           | -                |
| 0xF  | NV    | Never (reserved)                 | -                |

## Parser Architecture

### Module Structure

```
src/asm/
├── lexer.c              # Tokenization
├── parser.c             # Main parser with handler functions
├── parser_internal.h    # Internal parser context and declarations
├── directives.c         # Assembler directives
├── symbol_table.c/h     # O(1) label lookup (FNV-1a hash)
├── error.c/h            # Error collection and reporting
└── generated/
    ├── lookup_tables.h           # Public header
    ├── instr_name_lookup.c       # Instruction name → opcode
    └── cond_code_lookup.c        # Condition suffix → code
```

### Parsing Flow

```
            Source Code (.vasm)
                    │
                    ▼
            ┌───────────────────┐
            │     Tokenize      │  lexer.c
            │   (lexer.c)       │  Breaks input into tokens
            └─────────┬─────────┘
                      │
                      ▼ (token stream)
            ┌───────────────────┐
            │      Parse        │  parser.c
            │   (parse())       │  Main entry point
            └─────────┬─────────┘
                      │
                      ▼ (tokens)
    ┌─────────────────┴──────────┐
    │                            │
    ▼                            │          
Label Handler                    │
    │                            ▼
    ▼                   ┌─────────────────┐
┌────────┐              │                 │  parse_move()
│ Symbol │              │  Instruction    │  parse_alu()
│ Table  │              │  Handlers       │  parse_mult()
│ (hash) │              │                 |  parse_load_store()  etc.
└────┬───┘              └─────────────────┘
     │                        │
     │                        ▼
     │               ┌───────────────────┐
     └───────────────│    Emit Binary    │  Encodes to 32-bit
                     │    Instruction    │  instruction word
                     └─────────┬─────────┘
                               │
                               ▼
                     ┌───────────────────┐
                     │   Write .varm     │  write_vm_file()
                     │   File            │  Little-endian output
                     └───────────────────┘
```

### Symbol Table 
Labels are stored in a hash table using FNV-1a hashing:

```
Label: "loop" at address 0x1C
        │
        ▼
┌─────────────────────────────────────────────────────┐
│              Symbol Table (256 slots)               │
├───────────┬─────────────────────────────────────────┤
│ Index     │ Entry                                   │
├───────────┼─────────────────────────────────────────┤
│ 0x00      │ { "main", 0x00 }                        │
│ 0x01      │ { NULL } (empty)                        │
│ ...       │ ...                                     │
│ 0x5A      │ { NULL } (empty)                        │
│ 0x5B      │ { "loop", 0x1C }  ◄── hash("loop")=0x5B │
│ 0x5C      │ { "end", 0x24 }                         │
│ ...       │ ...                                     │
└───────────┴─────────────────────────────────────────┘
```

### Literal Pool (for `ldr rd, =label`)

When loading label addresses that can't be resolved directly, a literal pool is used:

```
Source:
    ldr r1, =message    ; Load address of 'message'
    ...
message: .byte 'H', 'e', 'l', 'l', 'o'

Parsing:
    1. Add reloc for 'message' (address unknown)
    2. Create LDR instruction with PC-relative addressing
    3. Add 'message' to literal pool
    4. Emit LDR instruction
    5. After all code: emit literal pool with actual addresses
    6. Fix up LDR instructions with correct offsets
```

## Instruction Handler Functions

### Parsing Pattern

Each instruction type follows a consistent parsing pattern:

```
┌─────────────────────────────────────────────────────────────────┐
│                    parse_<type>() Function                      │
├─────────────────────────────────────────────────────────────────┤
│ 1. Extract destination register (Rd) from token                 │
│ 2. Expect comma                                                 │
│ 3. Extract source register(s) or immediate value                │
│ 4. Parse optional condition code (.eq, .ne, etc.)               │
│ 5. Encode instruction:                                          │
│    instr = (opcode << 24) | (cond << 20) | (rd << 12) | operand │
│ 6. Emit instruction to text section                             │
└─────────────────────────────────────────────────────────────────┘
```

### Handler Functions

| Function            | Instructions              | Format                          |
|---------------------|---------------------------|---------------------------------|
| `parse_move()`      | MOV, MVN                  | `Rd, <op>`                      |
| `parse_alu()`       | ADD, SUB, AND, ORR, etc.  | `Rd, Rn, <op>`                  |
| `parse_mult()`      | MUL, MLA                  | `Rd, Rm, Rs[, Rn]`              |
| `parse_load_store()`| LDR, STR, LDRB, STRB      | `Rt, [Rn, #offset]`             |
| `parse_branch()`    | B, BL                     | `<label>`                       |
| `parse_system()`    | HALT, NOP, SWI            | `[#imm]`                        |
| `parse_directive()`  | .text, .data, .word, etc.| Directives                      |
| `parse_pseudo_ldr()`| `ldr rd, =label`          | Pseudo-instruction for literals |

### Operand Parsing

```
parse_operand() - Handles operand formats:

    #<immediate>     → Immediate value (0x42, 42, 0b1010)
    <register>       → Register (r0, r1, sp, lr, pc)
    <label>          → Label reference (=message)

Encoding:
    Immediate: bit 11 = 1, bits 0-10 = rotate<<8 | value&0xFF
    Register:  bit 11 = 0, bits 0-3 = register number
```

## Example: Encoding `mov r0, #42`

```
Source:    mov r0, #42

Tokens:    [INSTRUCTION: "mov"] [REGISTER: "r0"] [COMMA] [HASH] [IMMEDIATE: "42"]

Fields:
    opcode = OP_MOV = 0x00
    cond   = COND_AL = 0xE
    rd     = 0 (r0)
    operand = (1 << 11) | immediate(42)
            = 0x800 | 0x2A
            = 0x82A

Encoding:
    instr = (0x00 << 24) | (0xE << 20) | (0 << 12) | 0x82A
          = 0x00E0082A

Memory Layout (little-endian at address 0x20):
    0x20: 2A 08 E0 00
           │  │  │  └─ opcode (0x00)
           │  │  └──── cond (0xE)
           │  └─────── rd (0x00)
           └────────── operand bits 8-11 (0x8)
```

## Example: Encoding `cmp r0, #10`

```
Source:    cmp r0, #10

Fields:
    opcode = OP_CMP = 0x0C
    cond   = COND_AL = 0xE
    rd     = 0x0 (CMP has no destination, always 0)
    rn     = 0 (r0)
    operand = (1 << 11) | immediate(10)
            = 0x800 | 0x0A
            = 0x80A

Encoding:
    instr = (0x0C << 24) | (0xE << 20) | (0 << 16) | (0 << 12) | 0x80A
          = 0x0CE0080A

Memory Layout:
    0x20: 0A 08 E0 0C
```

## Error Handling

Errors are collected throughout parsing and reported at the end:

```
Parser Context (parser_ctx_t):
    ├── text[MAX_INSTRUCTIONS]   # Encoded instructions
    ├── data[65536]              # Data section
    ├── labels (symbol_table)    # Label definitions
    ├── errors (error_context)   # Collected errors
    ├── relocs[]                 # Unresolved references
    ├── literal_pool[]           # Literal pool entries
    └── literal_pool_refs[]      # Fixup references
```

## Generated Lookup Tables

At build time, `scripts/gen_lookup_tables.py` generates optimized C code for:

1. **Instruction Name Lookup** (`instr_name_lookup.c`):
   - Hash table mapping instruction names to opcodes
   - Uses FNV-1a hash with linear probing for collisions

2. **Condition Code Lookup** (`cond_code_lookup.c`):
   - Array mapping condition suffixes to codes
   - Supports both short (eq, ne) and long (hs, lo) forms

```
Generated lookup (instruction names):

INSTR_NAME_TABLE[256] = {
    [0x00] = { "mov", 0x00 },
    [0x01] = { "mvn", 0x01 },
    [0x02] = { "add", 0x02 },
    ...
}
```

## Build Integration

Meson generates lookup tables at build time:

```meson
gen_lookup = custom_target(
  'gen_lookup',
  input: gen_script,
  output: 'instr_name_lookup.c',
  command: [python, '@INPUT@', '--stdout'],
  capture: true,
)
```

## Quick Reference

| Concept          | Value/Format                     |
|------------------|----------------------------------|
| Endianness       | Little-endian                    |
| Instruction Size | 32 bits (4 bytes)                |
| Opcode Position  | Bits 31:24                       |
| Condition        | Bits 23:20 (4 bits, 0xE=always)  |
| Rn               | Bits 19:16                       |
| Rd               | Bits 15:12                       |
| Operand          | Bits 11:0                        |
| Label Lookup     | O(1) via hash table              |
| Max Instructions | 4096                             |
| Max Data Size    | 65536 bytes                      |
