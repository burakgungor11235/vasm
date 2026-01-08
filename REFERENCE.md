# varm Reference Manual

## Overview

varm (Virtual Abstract Runtime Machine) is a simple ARM-like register-based virtual machine. All values are stored in little-endian format, both in memory and in `.varm` files.

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

### Immediate Form (bit 19 = 1)

```
11:8 rotate  |  7:0 imm8
```

Value = ROR(imm8, rotate * 2). Example: #0xFF000000 = rotate 0xFF by 24.

**Encoding Note:** The immediate flag uses bit 19 (not bit 25 as in ARM) because:
- varm uses 8-bit opcode (bits 24-31), so bits 25-28 overlap with the opcode field
- Bit 19 falls in the rn field, which is unused by MOV/MVN instructions
- This allows clean encoding without corrupting other fields

### Register Form (bit 19 = 0)

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

**Character Literal:** `'A'`, `'H'`, `'\n'` (converts to ASCII value)

**Memory:** `[r0]`, `[r1, #4]`, `[sp, #-4]`

Offset is signed, in bytes.

### Directives

| Directive | Syntax | Description |
|-----------|--------|-------------|
| .text | .text | Start code section |
| .data | .data | Start data section |
| .word | .word val, ... | Emit 32-bit values |
| .byte | .byte val, ... | Emit 8-bit values (supports character literals like `'H'`) |
| .equ | .equ name, value | Define constant |
| .global | .global name | Export symbol |

**Character Literal Examples:**
```asm
.data
msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10  ; 10 = newline
newline: .byte '\n'
hex_val: .byte 0xFF
```

### Opcode Bit Position Defines

The VM source code uses named defines for instruction bit positions:

| Define | Value | Description |
|--------|-------|-------------|
| OPCODE_SHIFT | 24 | Bit position of opcode field |
| COND_SHIFT | 20 | Bit position of condition field |
| RN_SHIFT | 16 | Bit position of first source register |
| RD_SHIFT | 12 | Bit position of destination register |
| ROTATE_SHIFT | 8 | Bit position of rotate in operand2 |
| OFFSET_MASK | 0xFFF | Mask for 12-bit offset field |
| OFFSET_SIGN_BIT | 0x800 | Sign bit for signed offset |
| OPERAND_IMM_FLAG | (1 << 19) | Flag: operand2 is immediate (not register) |

**Usage in VM:**
```c
u8 opcode = (instr >> OPCODE_SHIFT) & 0xFF;
u8 cond = (instr >> COND_SHIFT) & 0xF;
u8 rd = (instr >> RD_SHIFT) & 0xF;
u8 is_immediate = (instr >> 19) & 1;
```

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
| 0x41 | SWI | SWI | Software interrupt (syscall) |
| 0x42 | NOP | NOP | No operation |

### Syscalls (SWI)

Syscalls are invoked via the SWI instruction. The syscall number is passed in R7.

| Number | Name | R0 | R1 | R2 | Return |
|--------|------|----|----|----|--------|
| 1 | EXIT | exit_code | - | - | - |
| 2 | READ | fd | buffer | count | bytes_read |
| 3 | WRITE | fd | buffer | count | bytes_written |

**Syscall conventions:**

- R7 = syscall number (1=EXIT, 2=READ, 3=WRITE)
- R0, R1, R2 = arguments (see table above)
- Return value in R0
- READ/WRITE only support fd 0 (stdin) and 1 (stdout)

**Example - Exit with code 42:**

```asm
mov r0, #42
mov r7, #1
swi
```

**Example - Write to stdout:**

```asm
ldr r1, =msg    ; buffer address
mov r2, #5      ; length
mov r0, #1      ; fd = stdout
mov r7, #3      ; syscall = WRITE
swi
```

**Example - Read from stdin:**

```asm
ldr r1, =buffer ; buffer address
mov r2, #10     ; max bytes
mov r0, #0      ; fd = stdin
mov r7, #2      ; syscall = READ
swi            ; R0 = bytes read
```

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
- Character literal: `'A'`, `'H'`

### Literal Pool and =label Pseudo-Instruction

The assembler uses a **literal pool** to handle 32-bit label addresses. When you use `=label` syntax:

```asm
ldr r1, =msg    ; Load address of msg into r1
```

The assembler:
1. Adds the label value (0x10000) to a literal pool at the end of the text section
2. Emits an LDR instruction with a PC-relative offset to the literal pool entry
3. Fixes up the offset after the literal pool is placed

**Example:**
```asm
.data
msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10

.text
        mov r0, #1          ; fd = stdout
        ldr r1, =msg        ; r1 = address of msg (0x10000)
        mov r2, #7          ; length = 7
        mov r7, #3          ; syscall = WRITE
        swi
```

**Generated instruction sequence:**
```
0x20: MOV r0, #1
0x24: LDR r1, [pc, #24]    ; Points to literal pool at 0x28
0x28: 0x00010000           ; Literal pool entry: msg address
0x2C: MOV r2, #7
0x30: MOV r7, #3
0x34: SWI
```

**Debug the literal pool:**
```bash
./vasm -d program.vasm -o program.varm
# Output includes:
# [POOL] emit_literal_pool: text_size=8, pool_start=32, count=1
# [POOL]   pool[0]: value=0x00010000, offset=32
```

## Examples

### Hello World

```asm
; hello.vasm - Print "Hello!" using syscalls
        .data
msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10

        .text
        mov r0, #1          ; fd = stdout (1)
        ldr r1, =msg        ; r1 = address of msg (0x10000)
        mov r2, #7          ; length = 7 bytes
        mov r7, #3          ; syscall = WRITE (3)
        swi                 ; write(fd=1, buf=0x10000, count=7)

        mov r0, #0          ; exit code = 0
        mov r7, #1          ; syscall = EXIT (1)
        swi                 ; exit(0)
```

**Run it:**
```bash
./qol.sh asmrun examples/hello.vasm
# Output: Hello!
```

**With debug:**
```bash
./qol.sh asmrun -d examples/hello.vasm
# Shows label resolution and literal pool
./varm -d SYSCALL examples/hello.varm
# Shows syscall execution
```

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

### Syscall Examples

**Exit with code 42:**
```asm
mov r0, #42
mov r7, #1
swi
```

**Write to stdout:**
```asm
mov r0, #1          ; fd = stdout
ldr r1, =msg        ; buffer address
mov r2, #5          ; length
mov r7, #3          ; syscall = WRITE
swi
```

**Read from stdin:**
```asm
mov r0, #0          ; fd = stdin
ldr r1, =buffer     ; buffer address
mov r2, #10         ; max bytes
mov r7, #2          ; syscall = READ
swi                 ; R0 = bytes read
```

## Usage

```bash
# Build the project
./qol.sh build

# Assemble a program
./qol.sh asm examples/simple.vasm -o program.varm

# Run a program
./qol.sh run program.varm

# Assemble and run in one command
./qol.sh asmrun examples/hello.vasm

# Run all tests
./qol.sh test

# Run with VM debug output
./qol.sh run -d INSTR program.varm

# Assemble with assembler debug output
./qol.sh asm -d examples/hello.vasm -o hello.varm
```

### qol.sh Color Output

The convenience script color-codes output:

| Color | Prefix | Meaning |
|-------|--------|---------|
| Green | `[stdout]` | Standard output from program |
| Red | `[stderr]` | Standard error (debug output) |

### asmrun Command

The `asmrun` command assembles and immediately runs a program:

```bash
# Basic usage
./qol.sh asmrun program.vasm

# With debug output
./qol.sh asmrun -d program.vasm

# Equivalent to:
./qol.sh asm program.vasm -o /tmp/program.varm
./qol.sh run /tmp/program.varm
rm /tmp/program.varm
```

## Debug Mode

The VM includes a tag-based debug system for tracing execution.

### Options

| Option | Description |
|--------|-------------|
| `-v, --verbose <level>` | Set verbosity level (0-5) |
| `-d, --debug <tag>` | Enable a debug tag |
| `--no-<tag>` | Disable a debug tag |
| `--tags` | List all available debug tags |

### Verbosity Levels

| Level | Name | Description |
|-------|------|-------------|
| 0 | ERROR | Show only errors |
| 1 | WARN | Show warnings and errors |
| 2 | INFO | Show informational messages |
| 3 | DEBUG | Show debug messages |
| 4 | TRACE | Show detailed traces |

### Debug Tags

| Tag | Abbr | Description |
|-----|------|-------------|
| `instr` | INSTR | Trace every instruction execution |
| `regs` | REGS | Dump registers after each instruction |
| `mem` | MEM | Log all memory accesses |
| `syscall` | SYSCALL | Trace system calls |
| `stats` | STATS | Show execution statistics at end |

### Assembler Debug Tags

The assembler (`vasm`) has its own debug tags for tracing the assembly process:

| Tag | Abbr | Description |
|-----|------|-------------|
| `asm` | ASM | General assembler diagnostics |
| `label` | LABEL | Label resolution and =label pseudo-instruction |
| `pool` | POOL | Literal pool allocation and fixup |

**Assembler Debug Usage:**
```bash
# Enable all assembler debug output
./vasm -d examples/hello.vasm -o hello.varm

# Sample output:
# [LABEL] =label pseudo-instr: label='msg' addr=0x10000 rd=1
# [POOL] emit_literal_pool: text_size=8, pool_start=32, count=1
# [POOL]   pool[0]: value=0x00010000, offset=32
# [POOL]   fixup LDR at 4: pool_offset=32, byte_offset=24, offset=24
```

### Examples

```bash
# List available tags
./varm --tags

# Trace instructions
./varm -d INSTR program.varm

# Show registers
./varm -d REGS program.varm

# Trace syscalls
./varm -d SYSCALL program.varm

# Show memory accesses
./varm -d MEM program.varm

# Show execution statistics
./varm -d STATS program.varm

# Multiple tags
./varm -d INSTR -d REGS program.varm

# Verbosity level
./varm -v 3 program.varm

# Disable a tag enabled by verbosity
./varm -v 4 --no-SYSCALL program.varm
```

### Output Format

Instruction trace format:
```
[INSTR] pc=0x00000020 00e0 AL MOV  r0, #0x02a
```

Register dump format:
```
[REGS]
  r0 : 0x00000000  r1 : 0x00000000  r2 : 0x00000000  r3 : 0x00000000
  sp: 0x00000000  lr: 0x00000000  pc: 0x00000024
  cpsr: 0x40000000  N=0 Z=1 C=0 V=0
```

Memory access format:
```
[MEM] read [0x00000020] = 0x00e0002a
[MEM] write [0x00010000] = 0x00000048
```

Syscall format:
```
[SYSCALL] WRITE fd=1 addr=0x10000 count=7
[SYSCALL] EXIT code=0
```

Statistics format:
```
[STATS]
  instructions: 9
  memory reads: 15
  memory writes: 3
  syscalls: 2
```

## Assembly/Run Cycle

```
program.vasm (source) -> vasm (assembler) -> program.varm (bytecode) -> varm (VM)
```

## Troubleshooting

### Common Issues

#### "Unknown syscall" or Incorrect Register Values

**Symptom:** Program prints nothing or behaves incorrectly, r7 shows unexpected values.

**Cause:** Immediate values being incorrectly encoded as register operands.

**Check:** Use syscall debug to verify register values:
```bash
./varm -d SYSCALL program.varm
```

#### Data Section at Wrong Address

**Symptom:** Labels in `.data` section point to wrong addresses, causing segfaults.

**Cause:** Data loaded to file offset instead of virtual address.

**Check:** Verify data section loading:
```bash
./varm -d MEM program.varm | grep 0x10000
```

#### Character Literals Not Working

**Symptom:** `.byte 'H'` causes assembly error.

**Solution:** Ensure you're using single quotes:
```asm
; Correct
.byte 'H', 'e', 'l', 'l', 'o'

; Incorrect - will not work
.byte "Hello"
```

### Debugging Workflow

1. **Enable assembler debug** to trace assembly:
```bash
./vasm -d program.vasm -o program.varm
```

2. **Check instruction encoding**:
```bash
./varm -d INSTR program.varm
```

3. **Trace syscalls** to verify system interaction:
```bash
./varm -d SYSCALL program.varm
```

4. **Dump registers** to see register state:
```bash
./varm -d REGS program.varm
```

5. **Full trace** for complex issues:
```bash
./varm -d INSTR -d REGS -d SYSCALL program.varm
```

### Performance Notes

- Debug overhead: ~5-10% per enabled tag
- Memory overhead: ~200 bytes per debug_config
- **Recommended:** Use `-d SYSCALL` for production tracing (minimal overhead)
- **Warning:** `-d MEM` and `-d INSTR` have higher overhead on large programs

---

## Example Programs

The `examples/` directory contains several demonstration programs:

| Program | Description | Concepts Demonstrated |
|---------|-------------|----------------------|
| `hello.vasm` | Print "Hello!" to stdout | Syscalls, data section, labels |
| `print_char.vasm` | Print a single character | Character literals, syscalls |
| `simple.vasm` | Simple computation | Basic instructions |
| `memory_load.vasm` | Load values from memory | LDR with labels, word loads |
| `multiple_loads.vasm` | Load multiple values | Multiple LDR/LDRB instructions |
| `addition.vasm` | Exit with value 42 | Immediate values, MOV |

### Running Examples

```bash
# Run any example
./qol.sh run examples/hello.varm

# Assemble and run
./qol.sh asmrun examples/hello.vasm

# With debug output
./qol.sh asmrun -d examples/hello.vasm
./varm -d SYSCALL examples/hello.varm
```

### Example: Memory Load

```asm
; memory_load.vasm - Load a value from data section

        .data
value:  .word 12345

        .text
        ldr r0, =value       ; r0 = address of value
        ldr r0, [r0]         ; r0 = *value = 12345
        mov r7, #1           ; exit with value (truncated to 8 bits)
        swi
```

Output: Exit code 57 (12345 & 0xFF = 0x3039 & 0xFF = 0x39 = 57)

### Example: Multiple Loads

```asm
; multiple_loads.vasm - Load multiple values

        .data
val1:   .word 100
val2:   .word 200
val3:   .byte 42

        .text
        ldr r0, =val1
        ldr r0, [r0]         ; r0 = 100
        ldr r1, =val3
        ldrb r1, [r1]        ; r1 = 42
        mov r0, r1           ; exit with 42
        mov r7, #1
        swi
```

Output: Exit code 42

## Known Issues

### Character Literals

Character literals use ASCII values:
```asm
.byte 'A'    ; = 65 (0x41)
.byte 'H'    ; = 72 (0x48)
.byte '\n'   ; = 10 (0x0A)
```

---

**varm Reference Manual** - Virtual Abstract Runtime Machine
