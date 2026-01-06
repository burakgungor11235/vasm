# varm Reference Manual

## Overview

varm (Virtual Abstract Runtime Machine) is a simple educational ARM-like register-based virtual machine. All values are stored in little-endian format, both in memory and in `.varm` files.

## Architecture

### Registers

| Register | Name | Description |
|----------|------|-------------|
| R0-R12 | - | General purpose (13 registers) |
| R13 | SP | Stack pointer |
| R14 | LR | Link register (return address) |
| R15 | PC | Program counter |
| CPSR | - | Condition flags (NZCV) |

### CPSR Flags (NZCV)

| Bit | Flag | Description |
|-----|------|-------------|
| 31 | N | Negative (sign bit of result) |
| 30 | Z | Zero (result equals zero) |
| 29 | C | Carry (unsigned overflow) |
| 28 | V | Overflow (signed overflow) |

### Memory

- Total: 1 MiB (1048576 bytes)
- Addressable as bytes or 32-bit words
- All memory access is byte-addressable
- Text section: load address 0x00000000
- Data section: load address 0x00010000
- Stack: starts at 0x000FFFFF, grows downward

### Endianness

All `.varm` files and in-memory values are little-endian. 32-bit words are stored with least-significant byte first.

## Instruction Format

### Primary Format (Data Processing)

All instructions are 32 bits:

```
31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:0 operand
```

| Field | Bits | Description |
|-------|------|-------------|
| opcode | 8 | Operation code (0x00-0xFF) |
| cond | 4 | Condition (see below) |
| rn | 4 | First source register |
| rd | 4 | Destination register |
| operand | 12 | Second operand (immediate or register) |

### Multiply Format (Opcode 0x10, 0x11)

Multiply uses a special bit layout:

```
31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:8 rm  |  7:4 rs  |  3:0 -
```

| Field | Bits | Description |
|-------|------|-------------|
| opcode | 8 | 0x10 for MUL, 0x11 for MLA |
| cond | 4 | Condition |
| rn | 4 | Accumulate register (MLA only, ignored for MUL) |
| rd | 4 | Destination register |
| rm | 4 | First multiply operand |
| rs | 4 | Second multiply operand |

### Branch Format (Opcode 0x30, 0x31)

```
31:24 opcode  |  23:20 cond  |  19:0 offset
```

The offset is a signed 20-bit word offset. Target = PC + 4 + (sign_extend(offset) << 2).

## Condition Codes

| Code | Meaning | Flags Tested | Value |
|------|---------|--------------|-------|
| EQ | Equal | Z set | 0x0 |
| NE | Not Equal | Z clear | 0x1 |
| CS/HS | Carry Set | C set | 0x2 |
| CC/LO | Carry Clear | C clear | 0x3 |
| MI | Minus | N set | 0x4 |
| PL | Plus | N clear | 0x5 |
| VS | Overflow | V set | 0x6 |
| VC | No Overflow | V clear | 0x7 |
| HI | Higher | C set and Z clear | 0x8 |
| LS | Lower or Same | C clear or Z set | 0x9 |
| GE | Greater/Equal | N == V | 0xA |
| LT | Less Than | N != V | 0xB |
| GT | Greater Than | Z clear and N == V | 0xC |
| LE | Less/Equal | Z set or N != V | 0xD |
| AL | Always | Unconditional | 0xE |
| - | Reserved | - | 0xF |

## Operand2

The operand2 field can be either an immediate value or a register with optional shift.

### Immediate Form (bit 25 = 1)

```
11:8 rotate  |  7:0 imm8
```

Value = ROR(imm8, rotate * 2). Example: #0xFF000000 = rotate 0xFF by 24.

### Register Form (bit 25 = 0)

```
11:10 shift_type  |  9:5 shift_imm  |  4:0 rm
```

| shift_type | Name | Description |
|------------|------|-------------|
| 0 | LSL | Logical Shift Left |
| 1 | LSR | Logical Shift Right |
| 2 | ASR | Arithmetic Shift Right |
| 3 | ROR | Rotate Right |

## Assembler Syntax

### Instruction Format

```
[label:]  MNEMONIC{cond}  operands  ; comment
```

### Operands

**Register:** `r0` to `r12`, `sp`, `lr`, `pc`

**Immediate:** `#42`, `#0x2A`, `#0b101010`

**Memory:** `[r0]`, `[r1, #4]`, `[sp, #-4]`

Offset is signed, in bytes.

### Directives

| Directive | Syntax | Description |
|-----------|--------|-------------|
| .text | .text | Start code section |
| .data | .data | Start data section |
| .word | .word val, ... | Emit 32-bit values |
| .byte | .byte val, ... | Emit 8-bit values |
| .equ | .equ name, value | Define constant |
| .global | .global name | Export symbol |

### Label Resolution

The assembler uses a two-pass approach:
1. First pass: collect all labels and their addresses
2. Second pass: resolve label references in instructions

## Instruction Set

### Data Processing

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x00 | MOV | MOV Rd, Operand2 | Move value |
| 0x01 | MVN | MVN Rd, Operand2 | Move NOT (bitwise complement) |
| 0x02 | ADD | ADD Rd, Rn, Operand2 | Add |
| 0x03 | ADC | ADC Rd, Rn, Operand2 | Add with Carry |
| 0x04 | SUB | SUB Rd, Rn, Operand2 | Subtract |
| 0x05 | SBC | SBC Rd, Rn, Operand2 | Subtract with Carry |
| 0x06 | RSB | RSB Rd, Rn, Operand2 | Reverse Subtract (Operand2 - Rn) |
| 0x07 | RSC | RSC Rd, Rn, Operand2 | Reverse Sub with Carry |
| 0x08 | AND | AND Rd, Rn, Operand2 | Bitwise AND |
| 0x09 | EOR | EOR Rd, Rn, Operand2 | Bitwise XOR |
| 0x0A | ORR | ORR Rd, Rn, Operand2 | Bitwise OR |
| 0x0B | BIC | BIC Rd, Rn, Operand2 | Bit Clear (AND NOT) |
| 0x0C | CMP | CMP Rn, Operand2 | Compare (sets NZCV, no result) |
| 0x0D | CMN | CMN Rn, Operand2 | Compare Negative (Rn + Operand2) |
| 0x0E | TST | TST Rn, Operand2 | Test Bits (AND, set NZCV) |
| 0x0F | TEQ | TEQ Rn, Operand2 | Test Equivalence (XOR, set NZCV) |

### Multiply

| Opcode | Name | Syntax | Bit Layout | Description |
|--------|------|--------|------------|-------------|
| 0x10 | MUL | MUL Rd, Rm, Rs | rd=Dst, rm=Src1, rs=Src2 | Rd = Rm * Rs |
| 0x11 | MLA | MLA Rd, Rm, Rs, Rn | rd=Dst, rm=Src1, rs=Src2, rn=Acc | Rd = Rm * Rs + Rn |

### Memory

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x20 | LDR | LDR Rt, [Rn, #offset] | Load 32-bit word from memory |
| 0x21 | LDRB | LDRB Rt, [Rn, #offset] | Load 8-bit byte (zero-extended) |
| 0x22 | STR | STR Rt, [Rn, #offset] | Store 32-bit word to memory |
| 0x23 | STRB | STRB Rt, [Rn, #offset] | Store 8-bit byte |

**Alignment:** LDR/STR require word-aligned addresses (multiple of 4). Misaligned access results in undefined behavior.

**Offset:** Signed byte offset added to base register.

### Branch

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x30 | B | B label | Branch to label |
| 0x31 | BL | BL label | Branch with Link (save PC+4 to LR) |
| 0x32 | BX | BX Rn | Branch and Exchange (PC = Rn) |

**Branch offset calculation:**
```
offset = sign_extend(instr[19:0])  ; 20-bit signed
target = PC + 4 + (offset << 2)    ; word offset, PC is address of next instruction
```

**Example:** If B is at 0x00000100, offset = 10, then target = 0x00000104 + (10 << 2) = 0x00000104 + 40 = 0x0000012C.

### System

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x40 | HALT | HALT | Stop execution, exit with code 0 |
| 0x41 | SWI | SWI #imm | Software interrupt (reserved) |
| 0x42 | NOP | NOP | No operation |

## Calling Convention

varm uses a simple calling convention:

- **Caller-saved:** R0-R3
- **Callee-saved:** R4-R12
- **SP:** Stack pointer (R13)
- **LR:** Link register (R14)
- **PC:** Program counter (R15)

**Function call (BL):**
1. Caller pushes any registers that need to be preserved
2. BL saves return address (PC+4) to LR
3. Function executes
4. Return via BX LR

**Stack:**
- Initial SP = 0x000FFFFF (top of memory)
- Stack grows downward (toward lower addresses)
- PUSH decrements SP, then stores
- POP loads, then increments SP

## File Format

### Header (32 bytes, all values little-endian)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0 | 4 | Magic | ASCII "VARM" (0x4D524156) |
| 4 | 4 | Text offset | File offset to text section (always 32) |
| 8 | 4 | Text size | Size of text section in bytes |
| 12 | 4 | Data offset | File offset to data section |
| 16 | 4 | Data size | Size of data section in bytes |
| 20 | 4 | Entry point | Virtual address of entry point (absolute) |
| 24 | 4 | Version | Version (currently 1) |
| 28 | 4 | Flags | Reserved (set to 0) |

**Loading:** On start, the VM loads:
- Text section to address 0x00000000
- Data section to address 0x00010000
- Sets PC to the entry point address

## Assembler Behavior

The assembler (`vasm`) performs:

1. **Lexical analysis:** Tokenize input (instructions, registers, immediates, labels, directives)
2. **First pass:** Build symbol table (labels and their addresses)
3. **Second pass:** Encode instructions, resolve label references
4. **Output:** Write `.varm` file with header and sections

**Supported number formats:**
- Decimal: `42`
- Hexadecimal: `0x2A`
- Binary: `0b101010`

## Examples

### Simple Register Move

```asm
mov r0, #42
halt
```

### Arithmetic

```asm
mov r0, #42
mov r1, #10
add r2, r0, r1
halt
```

### Loop with Branch

```asm
        mov r0, #0          ; r0 = 0
loop    add r0, r0, #1      ; r0++
        cmp r0, #10         ; compare r0 with 10
        ble loop            ; branch if r0 <= 10
        halt                ; exit
```

### Function Call

```asm
        mov r0, #5          ; argument = 5
        bl factorial        ; call factorial
        halt                ; result in r0

factorial:
        cmp r0, #1          ; if n <= 1
        ble .base           ;   return 1
        push {lr}           ; save return address
        sub r1, r0, #1      ; r1 = n - 1
        bl factorial        ; recursive call
        mul r0, r0, r1      ; result = result * (n-1)
        pop {pc}            ; return

.base   mov r0, #1          ; return 1
        bx lr
```

## Usage

```bash
# Build the project
./qol.sh build

# Assemble a program
./qol.sh examples/simple.vasm -o program.varm

# Run a program
./qol.sh run program.varm

# Run all tests
./qol.sh test
```

## Assembly/Run Cycle

```
program.vasm (source) -> vasm (assembler) -> program.varm (bytecode) -> varm (VM)
```

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01-07 | Initial specification |
