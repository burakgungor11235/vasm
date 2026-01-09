.. _directives:

==========
Directives
==========

.. warning::
   varm is not a stable project. Directives, syntax, and behavior may change
   without notice. This documentation reflects the current implementation.

Assembler directives (also called pseudo-ops) control the assembly process
and define data. They are distinguished from instructions by starting with
a dot (``.``).

.. contents:: Table of Contents
   :local:

Section Control Directives
==========================

.text - Code Section
--------------------

**Syntax:** ``.text``

**Description:** Switches to the text (code) section. All subsequent
instructions are placed in the text section at address 0x00000000.

**Example:**

.. code-block:: asm

   .data
   message: .byte 'H', 'e', 'l', 'l', 'o'

   .text
   mov r0, #1
   ldr r1, =message

**Notes:**

- The text section is where instructions are stored
- The first instruction in the text section is loaded at virtual address 0x00000000
- Entry point defaults to the start of the text section

.data - Data Section
--------------------

**Syntax:** ``.data``

**Description:** Switches to the data section. All subsequent directives
that emit data place values in the data section at address 0x00010000.

**Example:**

.. code-block:: asm

   .data
   value:  .word 12345
   buffer: .byte 0, 0, 0, 0

**Notes:**

- The data section is for initialized data
- Data is loaded at virtual address 0x00010000
- Labels in the data section resolve to addresses in the 0x00010000 range

Data Definition Directives
==========================

.word - Define 32-bit Words
---------------------------

**Syntax:** ``.word val1, val2, ...``

**Description:** Emits one or more 32-bit big-endian values into the current
section. Each value is stored as a 32-bit word.

**Example:**

.. code-block:: asm

   .data
   value:      .word 12345           ; single word
   numbers:    .word 1, 2, 3, 4      ; multiple words
   address:    .word 0x00010000      ; literal address
   negative:   .word -1              ; two's complement

**Encoding in Data Section:**

For ``.word 12345`` (0x00003039):

+--------+--------+--------+--------+
| Offset | Byte 0 | Byte 1 | Byte 2+3|
+--------+--------+--------+--------+
| 0      | 0x39   | 0x30   | 0x0000 |
+--------+--------+--------+--------+

**Supported Value Formats:**

- Decimal: ``.word 42``
- Hexadecimal: ``.word 0x2A``
- Binary: ``.word 0b101010``
- Character: ``.word 'A'`` (ASCII value)
- Expressions: ``.word 1 + 2``

**Complexity:** O(n) time and space for n words

.byte - Define Bytes
--------------------

**Syntax:** ``.byte val1, val2, ...``

**Description:** Emits one or more 8-bit values into the current section.

**Example:**

.. code-block:: asm

   .data
   char:   .byte 'A'           ; ASCII 'A' = 65
   msg:    .byte 'H', 'e', 'l', 'l', 'o'
   hex:    .byte 0xFF, 0x00
   nums:   .byte 1, 2, 3, 4

**Character Literals:**

Character literals are converted to their ASCII integer values:

+-----------+-------+-------+
| Character | ASCII | Hex   |
+-----------+-------+-------+
| 'A'       | 65    | 0x41  |
| 'a'       | 97    | 0x61  |
| '0'       | 48    | 0x30  |
| '\n'      | 10    | 0x0A  |
| '\t'      | 9     | 0x09  |
+-----------+-------+-------+

**Example with String:**

.. code-block:: asm

   .data
   msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10  ; 10 = newline

**Complexity:** O(n) time and space for n bytes

Constant Definition Directives
==============================

.equ - Define Constant
----------------------

**Syntax:** ``.equ name, value``

**Description:** Defines a constant symbol that is substituted during assembly.
The value is evaluated at assembly time.

**Example:**

.. code-block:: asm

   .equ BUFFER_SIZE, 256
   .equ STACK_TOP, 0x000FFFFF

   .text
   mov r0, #BUFFER_SIZE    ; assembles as mov r0, #256
   ldr r1, =STACK_TOP      ; loads address 0x000FFFFF

**Equivalent Form:**

The ``.set`` directive is a synonym for ``.equ``:

.. code-block:: asm

   .set MAX_SIZE, 1000     ; same as .equ

**Notes:**

- Constants are replaced textually during assembly
- Cannot be redefined
- Only numeric values are supported
- Useful for magic numbers and configuration values

**Complexity:** O(1) time, O(1) space

Label Definition
================

Labels
------

**Syntax:** ``label:``

**Description:** Defines a symbol at the current address. Labels are resolved
during the second pass of assembly.

**Example:**

.. code-block:: asm

   .text
   start:                      ; address 0x00000000
       mov r0, #42
       b end                   ; branch to end

   loop:                       ; address 0x00000008
       add r0, r0, #1
       b loop

   end:                        ; address 0x00000010
       halt

**Label Types:**

- **Code labels:** Labels in the ``.text`` section
- **Data labels:** Labels in the ``.data`` section

**Address Resolution:**

+------+--------------------+-------------------+
| Type | Assembly Address   | Runtime Address   |
+------+--------------------+-------------------+
| Text | 0x00000000 + offset| 0x00000000 + offset|
| Data | 0x00010000 + offset| 0x00010000 + offset|
+------+--------------------+-------------------+

**Example with Data:**

.. code-block:: asm

   .data
   message: .byte 'H', 'i', '!', 10    ; at 0x00010000
   value:   .word 42                   ; at 0x00010004

   .text
   ldr r0, =message    ; r0 = 0x00010000
   ldr r1, =value      ; r1 = 0x00010004

**Label Visibility:**

Labels are local by default and only visible within the same assembly file.
There is no ``.global`` directive for exporting symbols.

Literal Pool and =label Pseudo-Instruction
==========================================

ldr rd, =label
--------------

**Syntax:** ``LDR Rd, =label``

**Description:** Pseudo-instruction that loads the address of a label into Rd.
The assembler places the address value in a literal pool and generates an
LDR instruction with PC-relative addressing.

**Example:**

.. code-block:: asm

   .data
   message: .byte 'H', 'e', 'l', 'l', 'o', '!', 10

   .text
   ldr r1, =message    ; r1 = 0x00010000 (address of message)
   mov r0, #1          ; fd = stdout
   mov r2, #7          ; length = 7
   mov r7, #3          ; syscall = WRITE
   swi

**Generated Instructions:**

The assembler generates:

+------+--------+--------------------------------------------+
| Addr | Opcode | Instruction                                 |
+------+--------+--------------------------------------------+
| 0x00 | MOV    | r0, #1                                      |
| 0x04 | LDR    | r1, [pc, #24]  ; points to literal pool     |
| 0x08 | MOV    | r2, #7                                      |
| 0x0C | MOV    | r7, #3                                      |
| 0x10 | SWI    |                                             |
| 0x14 | (pool) | 0x00010000  ; literal pool entry: message   |
+------+--------+--------------------------------------------+

**Notes:**

- The literal pool is placed at the end of the text section
- Only one literal pool entry per unique address value
- The offset is calculated as: ``literal_pool_addr - (current_pc + 4)``

Directive Summary Table
=======================

+-----------+------------------+------------------------------------------+
| Directive | Syntax           | Description                              |
+-----------+------------------+------------------------------------------+
| .text     | .text            | Switch to code section                   |
| .data     | .data            | Switch to data section                   |
| .word     | .word val, ...   | Emit 32-bit word(s)                      |
| .byte     | .byte val, ...   | Emit 8-bit byte(s)                       |
| .equ      | .equ name, val   | Define constant                          |
| .set      | .set name, val   | Define constant (synonym for .equ)       |
| Label     | name:            | Define label at current address          |
+-----------+------------------+------------------------------------------+

Assembly Process
================

The varm assembler uses a two-pass algorithm:

**Pass 1: Tokenization and Symbol Collection**

1. Tokenize source code (instructions, registers, immediates, labels, directives)
2. Collect labels and their addresses
3. Store directives that emit data

**Pass 2: Instruction Encoding**

1. Encode instructions with resolved label addresses
2. Generate literal pool for ``=label`` pseudo-instructions
3. Resolve branch offsets
4. Emit final bytecode

**Symbol Table:**

Labels are stored in a hash table using FNV-1a hashing:

- O(1) average lookup time
- 256 slots maximum
- Linear probing for collision resolution

**Maximum Limits:**

+--------------------------+-------+
| Item                     | Limit |
+--------------------------+-------+
| Instructions             | 4096  |
| Data size                | 65536 |
| Literal pool entries     | 256   |
| Labels                   | 256   |
+--------------------------+-------+

Number Formats
==============

The assembler accepts several number formats:

+-----------+------------------+------------------------+
| Format    | Example          | Value                  |
+-----------+------------------+------------------------+
| Decimal   | 42               | 42                     |
| Hex       | 0x2A, 0x2a       | 42                     |
| Binary    | 0b101010         | 42                     |
| Character | 'A', '\n'        | ASCII value            |
+-----------+------------------+------------------------+

**Examples:**

.. code-block:: asm

   .byte 42              ; decimal
   .byte 0x2A            ; hexadecimal
   .byte 0b101010        ; binary
   .byte 'A'             ; character literal
   .byte '\n'            ; escape sequence
