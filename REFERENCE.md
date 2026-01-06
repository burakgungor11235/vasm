# varm Reference Manual

## Architecture

### Registers

| Register | Name | Description |
|----------|------|-------------|
| R0-R7 | - | General purpose (8 registers) |
| R13 | SP | Stack pointer |
| R14 | LR | Link register (return address) |
| R15 | PC | Program counter |
| CPSR | - | Condition flags (NZCV) |

### CPSR Flags (NZCV)

| Bit | Flag | Description |
|-----|------|-------------|
| 31 | N | Negative |
| 30 | Z | Zero |
| 29 | C | Carry |
| 28 | V | Overflow |

### Memory

- Total: 1 MiB (1048576 bytes)
- Text section: starts at 0x00000000
- Data section: starts at 0x00010000

## Instruction Format

All instructions are 32 bits:

```
31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:0 operand
```

- **opcode**: 8-bit operation code
- **cond**: 4-bit condition (see below)
- **rn**: 4-bit source register
- **rd**: 4-bit destination register
- **operand**: 12-bit operand (immediate or register+shift)

## Condition Codes

| Code | Meaning | Flags Tested |
|------|---------|--------------|
| EQ | Equal | Z set |
| NE | Not Equal | Z clear |
| CS/HS | Carry Set | C set |
| CC/LO | Carry Clear | C clear |
| MI | Minus | N set |
| PL | Plus | N clear |
| VS | Overflow | V set |
| VC | No Overflow | V clear |
| HI | Higher | C set and Z clear |
| LS | Lower or Same | C clear or Z set |
| GE | Greater/Equal | N == V |
| LT | Less Than | N != V |
| GT | Greater Than | Z clear and N == V |
| LE | Less/Equal | Z set or N != V |
| AL | Always | Unconditional |

## Assembler Syntax

### Instruction Format

```
[label:]  MNEMONIC{cond}  operands  ; comment
```

### Operands

**Register:** `r0` to `r7`, `sp`, `lr`, `pc`

**Immediate:** `#42`, `#0x2A`, `#0b101010`

**Memory:** `[r0]`, `[r1, #4]`, `[sp, #-4]`

### Directives

| Directive | Syntax | Description |
|-----------|--------|-------------|
| .text | .text | Start code section |
| .data | .data | Start data section |
| .word | .word val, ... | 32-bit values |
| .byte | .byte val, ... | 8-bit values |
| .equ | .equ name, value | Define constant |
| .global | .global name | Export symbol |

## Instruction Set

### Data Processing

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x00 | MOV | MOV Rd, Operand2 | Move value |
| 0x01 | MVN | MVN Rd, Operand2 | Move NOT |
| 0x02 | ADD | ADD Rd, Rn, Operand2 | Add |
| 0x03 | ADC | ADC Rd, Rn, Operand2 | Add with Carry |
| 0x04 | SUB | SUB Rd, Rn, Operand2 | Subtract |
| 0x05 | SBC | SBC Rd, Rn, Operand2 | Subtract with Carry |
| 0x06 | RSB | RSB Rd, Rn, Operand2 | Reverse Subtract |
| 0x07 | RSC | RSC Rd, Rn, Operand2 | Reverse Sub with Carry |
| 0x08 | AND | AND Rd, Rn, Operand2 | Bitwise AND |
| 0x09 | EOR | EOR Rd, Rn, Operand2 | Bitwise XOR |
| 0x0A | ORR | ORR Rd, Rn, Operand2 | Bitwise OR |
| 0x0B | BIC | BIC Rd, Rn, Operand2 | Bit Clear |
| 0x0C | CMP | CMP Rn, Operand2 | Compare (sets flags) |
| 0x0D | CMN | CMN Rn, Operand2 | Compare Negative |
| 0x0E | TST | TST Rn, Operand2 | Test Bits |
| 0x0F | TEQ | TEQ Rn, Operand2 | Test Equivalence |

### Multiply

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x10 | MUL | MUL Rd, Rm, Rs | Multiply |
| 0x11 | MLA | MLA Rd, Rm, Rs, Rn | Multiply-Accumulate |

### Memory

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x20 | LDR | LDR Rt, [Rn, #offset] | Load Word |
| 0x21 | LDRB | LDRB Rt, [Rn, #offset] | Load Byte |
| 0x22 | STR | STR Rt, [Rn, #offset] | Store Word |
| 0x23 | STRB | STRB Rt, [Rn, #offset] | Store Byte |

### Branch

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x30 | B | B label | Branch |
| 0x31 | BL | BL label | Branch with Link |
| 0x32 | BX | BX Rn | Branch and Exchange |

### System

| Opcode | Name | Syntax | Description |
|--------|------|--------|-------------|
| 0x40 | HALT | HALT | Stop execution |
| 0x41 | SWI | SWI #imm | Software Interrupt |
| 0x42 | NOP | NOP | No Operation |

## Operand2

The operand2 field can be:

**Immediate (8-bit rotated):**
```
| rotate*2 (4 bits) | imm8 (8 bits) |
```
Value = rotate_right(imm8, rotate*2)

**Register with shift:**
```
| shift_type (2) | shift_imm (5) | rm (5) |
```

Shift types: LSL, LSR, ASR, ROR

## File Format

### Header (32 bytes)

```
Offset  Size  Field
0       4     Magic: "VARM"
4       4     Text offset
8       4     Text size
12      4     Data offset
16      4     Data size
20      4     Entry point
24      4     Reserved
28      4     Reserved
```

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

### Label and Branch

```asm
        mov r0, #0
loop    add r0, r0, #1
        cmp r0, #10
        ble loop
        halt
```

## Assembly Format

Assembly source files use `.vasm` extension. Compiled bytecode uses `.varm` extension.

```bash
# Assemble
vasm input.vasm -o output.varm

# Run
varm output.varm
```

Or use the convenience script:

```bash
./qol.sh asm input.vasm -o output.varm
./qol.sh run output.varm
```
