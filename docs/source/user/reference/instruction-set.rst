.. _instruction-set:

===============
Instruction Set
===============

.. warning::
   varm is not a stable project. Instruction encodings, behavior, and system
   interfaces may change without notice. This documentation reflects the
   current implementation as of the latest version.

This reference documents all instructions available in the varm virtual machine.
Each instruction is documented with syntax, encoding, flags affected, operational
semantics, and examples.

.. contents:: Table of Contents
   :local:

Instruction Format
==================

All varm instructions are fixed-width 32-bit (4 bytes) little-endian values.

Primary Format (Data Processing)
--------------------------------

::

   31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:0 operand
   ┌──────────┬──────────┬──────────┬──────────┬────────────────────────┐
   │  Opcode  │  Cond    │    Rn    │    Rd    │        Operand         │
   │  8 bit   │  4 bit   │  4 bit   │  4 bit   │        12 bit          │
   └──────────┴──────────┴──────────┴──────────┴────────────────────────┘
    ↑                                                      ↑
   MSB                                                    LSB

**Field Descriptions:**

+-----------+-------+--------------------------------------------+
| Field     | Bits  | Description                                |
+-----------+-------+--------------------------------------------+
| Opcode    | 31:24 | Operation code (0x00-0xFF)                 |
| Cond      | 23:20 | Condition code (see Condition Codes)       |
| Rn        | 19:16 | First source register                      |
| Rd        | 15:12 | Destination register                       |
| Operand   | 11:0  | Second operand (immediate or register)     |
+-----------+-------+--------------------------------------------+

Multiply Format
---------------

Multiplication uses a special encoding in the operand field::

   31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:8 rm  |  7:4 rs  |  3:0 -
   ┌──────────┬──────────┬──────────┬──────────┬─────────┬─────────┬──────────────┐
   │  Opcode  │  Cond    │    Rn    │    Rd    │   Rm    │   Rs    │      -       │
   │  8 bit   │  4 bit   │  4 bit   │  4 bit   │  4 bit  │  4 bit  │    4 bit     │
   └──────────┴──────────┴──────────┴──────────┴─────────┴─────────┴──────────────┘

Branch Format
-------------

Branch instructions encode a signed offset::

   31:24 opcode  |  23:20 cond  |  19:0 offset
   ┌──────────┬──────────┬─────────────────────┐
   │  Opcode  │  Cond    │       Offset        │
   │  8 bit   │  4 bit   │       20 bit        │
   └──────────┴──────────┴─────────────────────┘

Operand2 Encoding
=================

The 12-bit operand field can encode either an immediate value or a register
with optional shift operations.

Immediate Form (bit 11 = 1)
---------------------------

::

   11      │  10:8        │  7:0
   ────────┼──────────────┼─────────────────
   1       │   rotate     │     imm8

Value = ROR(imm8, rotate × 2), where ROR is rotate right.

The assembler automatically computes the correct rotate value for immediates
that can be represented in this format.

Register Form (bit 11 = 0)
--------------------------

::

   11:10    │  9:5         │  4:0
   ─────────┼──────────────┼────────────────
   shift    │   shift_imm  │      rm

**Shift Types:**

+-------+------+-------------------------------------------+
| Value | Name | Description                               |
+-------+------+-------------------------------------------+
| 0     | LSL  | Logical Shift Left                        |
| 1     | LSR  | Logical Shift Right                       |
| 2     | ASR  | Arithmetic Shift Right (sign-extend)      |
| 3     | ROR  | Rotate Right                              |
+-------+------+-------------------------------------------+

Condition Codes
===============

Condition codes determine whether an instruction executes based on the
CPSR flags (NZCV).

+------+------------------+--------+-------------------------+
| Code | Name             | Value  | Flags Tested            |
+------+------------------+--------+-------------------------+
| EQ   | Equal            | 0x0    | Z = 1                   |
| NE   | Not Equal        | 0x1    | Z = 0                   |
| CS/HS| Carry Set        | 0x2    | C = 1                   |
| CC/LO| Carry Clear      | 0x3    | C = 0                   |
| MI   | Minus/Negative   | 0x4    | N = 1                   |
| PL   | Plus/Non-Negative| 0x5    | N = 0                   |
| VS   | Overflow Set     | 0x6    | V = 1                   |
| VC   | Overflow Clear   | 0x7    | V = 0                   |
| HI   | Higher           | 0x8    | C = 1 && Z = 0          |
| LS   | Lower or Same    | 0x9    | C = 0 || Z = 1          |
| GE   | Greater/Equal    | 0xA    | N == V                  |
| LT   | Less Than        | 0xB    | N != V                  |
| GT   | Greater Than     | 0xC    | Z = 0 && N == V         |
| LE   | Less or Equal    | 0xD    | Z = 1 || N != V         |
| AL   | Always           | 0xE    | Unconditional           |
| NV   | Reserved         | 0xF    | -                       |
+------+------------------+--------+-------------------------+

**CPSR Flags:**

+-----+------+-----------------------------------------------+
| Bit | Flag | Description                                   |
+-----+------+-----------------------------------------------+
| 31  | N    | Negative (sign bit of result)                 |
| 30  | Z    | Zero (result equals zero)                     |
| 29  | C    | Carry (unsigned overflow/borrow)              |
| 28  | V    | Overflow (signed overflow)                    |
+-----+------+-----------------------------------------------+

Data Processing Instructions
============================

MOV - Move
----------

**Opcode:** 0x00

**Syntax:** ``MOV Rd, Operand2``

**Description:** Copies the value of Operand2 into Rd. Operand2 can be an
immediate value or a register (with optional shift).

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x00     │   cond   │    0    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← value(Operand2)``

**Example:**

.. code-block:: asm

   mov r0, #42        ; r0 = 42
   mov r1, r0         ; r1 = r0
   mov r2, r1, LSL #2 ; r2 = r1 << 2

**Complexity:** O(1) time, O(1) space

MVN - Move NOT
--------------

**Opcode:** 0x01

**Syntax:** ``MVN Rd, Operand2``

**Description:** Performs bitwise NOT on Operand2 and stores result in Rd.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x01     │   cond   │    0    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← ¬value(Operand2)``

**Example:**

.. code-block:: asm

   mvn r0, #0         ; r0 = 0xFFFFFFFF
   mvn r1, r0         ; r1 = 0x00000000

**Complexity:** O(1) time, O(1) space

ADD - Add
---------

**Opcode:** 0x02

**Syntax:** ``ADD Rd, Rn, Operand2``

**Description:** Adds Rn and Operand2, stores result in Rd.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x02     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Rn + Operand2``

**Example:**

.. code-block:: asm

   add r0, r1, #10    ; r0 = r1 + 10
   add r2, r3, r4     ; r2 = r3 + r4

**Complexity:** O(1) time, O(1) space

ADC - Add with Carry
--------------------

**Opcode:** 0x03

**Syntax:** ``ADC Rd, Rn, Operand2``

**Description:** Adds Rn, Operand2, and the Carry flag. Used for multi-word
addition.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x03     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Rn + Operand2 + C``

**Example:**

.. code-block:: asm

   ; 64-bit addition: r1:r0 + r3:r2 → r5:r4
   adds r4, r0, r2    ; lower 32 bits, set C
   adc r5, r1, r3     ; upper 32 bits with carry

**Complexity:** O(1) time, O(1) space

SUB - Subtract
--------------

**Opcode:** 0x04

**Syntax:** ``SUB Rd, Rn, Operand2``

**Description:** Subtracts Operand2 from Rn, stores result in Rd.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x04     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Rn - Operand2``

**Example:**

.. code-block:: asm

   sub r0, r1, #5     ; r0 = r1 - 5
   sub sp, sp, #16    ; decrement stack pointer

**Complexity:** O(1) time, O(1) space

SBC - Subtract with Carry
-------------------------

**Opcode:** 0x05

**Syntax:** ``SBC Rd, Rn, Operand2``

**Description:** Subtracts Operand2 and NOT(Carry) from Rn. Used for multi-word
subtraction.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x05     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Rn - Operand2 - (1 - C)``

**Example:**

.. code-block:: asm

   subs r4, r0, r2    ; lower 32 bits, set C
   sbc r5, r1, r3     ; upper 32 bits with borrow

**Complexity:** O(1) time, O(1) space

RSB - Reverse Subtract
----------------------

**Opcode:** 0x06

**Syntax:** ``RSB Rd, Rn, Operand2``

**Description:** Subtracts Rn from Operand2 (reversed order).

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x06     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Operand2 - Rn``

**Example:**

.. code-block:: asm

   rsb r0, r1, #0     ; r0 = -r1 (negation)
   rsb r0, r1, r2     ; r0 = r2 - r1

**Complexity:** O(1) time, O(1) space

RSC - Reverse Subtract with Carry
---------------------------------

**Opcode:** 0x07

**Syntax:** ``RSC Rd, Rn, Operand2``

**Description:** Reverse subtract with carry.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x07     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``Rd ← Operand2 - Rn - (1 - C)``

**Complexity:** O(1) time, O(1) space

AND - Bitwise AND
-----------------

**Opcode:** 0x08

**Syntax:** ``AND Rd, Rn, Operand2``

**Description:** Bitwise AND of Rn and Operand2.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x08     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← Rn ∧ Operand2``

**Example:**

.. code-block:: asm

   and r0, r1, #0xF   ; extract lower nibble
   and r2, r3, r4     ; r2 = r3 & r4

**Complexity:** O(1) time, O(1) space

EOR - Bitwise Exclusive OR
--------------------------

**Opcode:** 0x09

**Syntax:** ``EOR Rd, Rn, Operand2``

**Description:** Bitwise XOR of Rn and Operand2.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x09     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← Rn ⊕ Operand2``

**Example:**

.. code-block:: asm

   eor r0, r1, r1     ; r0 = 0 (XOR with self)
   eor r2, r3, #0xFF  ; invert lower byte

**Complexity:** O(1) time, O(1) space

ORR - Bitwise OR
----------------

**Opcode:** 0x0A

**Syntax:** ``ORR Rd, Rn, Operand2``

**Description:** Bitwise OR of Rn and Operand2.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0A     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← Rn ∨ Operand2``

**Example:**

.. code-block:: asm

   orr r0, r1, #0xF   ; set lower nibble
   orr r2, r3, r4     ; r2 = r3 | r4

**Complexity:** O(1) time, O(1) space

BIC - Bit Clear
---------------

**Opcode:** 0x0B

**Syntax:** ``BIC Rd, Rn, Operand2``

**Description:** Bitwise AND of Rn with NOT Operand2 (clears specified bits).

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0B     │   cond   │   Rn    │   Rd    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``Rd ← Rn ∧ ¬Operand2``

**Example:**

.. code-block:: asm

   bic r0, r1, #0xF   ; clear lower nibble
   bic r2, r3, r4     ; r2 = r3 & ~r4

**Complexity:** O(1) time, O(1) space

Comparison Instructions
=======================

CMP - Compare
-------------

**Opcode:** 0x0C

**Syntax:** ``CMP Rn, Operand2``

**Description:** Subtracts Operand2 from Rn and updates flags. Does not store
result.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0C     │   cond   │   Rn    │    0    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``flags ← Rn - Operand2``

**Example:**

.. code-block:: asm

   cmp r0, #10        ; compare r0 with 10
   beq .equal         ; branch if equal

**Complexity:** O(1) time, O(1) space

CMN - Compare Negative
----------------------

**Opcode:** 0x0D

**Syntax:** ``CMN Rn, Operand2``

**Description:** Adds Rn and Operand2 and updates flags. Tests if sum equals zero.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0D     │   cond   │   Rn    │    0    │   Operand2

**Flags Affected:** N, Z, C, V

**Operational Semantics:** ``flags ← Rn + Operand2``

**Example:**

.. code-block:: asm

   cmn r0, r1         ; test if r0 == -r1

**Complexity:** O(1) time, O(1) space

TST - Test Bits
---------------

**Opcode:** 0x0E

**Syntax:** ``TST Rn, Operand2``

**Description:** Bitwise AND of Rn and Operand2, updates flags. Tests if
specified bits are set.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0E     │   cond   │   Rn    │    0    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``flags ← Rn ∧ Operand2``

**Example:**

.. code-block:: asm

   tst r0, #0xF       ; test if lower nibble is non-zero

**Complexity:** O(1) time, O(1) space

TEQ - Test Equivalence
----------------------

**Opcode:** 0x0F

**Syntax:** ``TEQ Rn, Operand2``

**Description:** Bitwise XOR of Rn and Operand2, updates flags. Tests for
equality without affecting bits.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x0F     │   cond   │   Rn    │    0    │   Operand2

**Flags Affected:** N, Z

**Operational Semantics:** ``flags ← Rn ⊕ Operand2``

**Example:**

.. code-block:: asm

   teq r0, r1         ; test if r0 == r1

**Complexity:** O(1) time, O(1) space

Multiplication Instructions
===========================

MUL - Multiply
--------------

**Opcode:** 0x10

**Syntax:** ``MUL Rd, Rm, Rs``

**Description:** Multiplies Rm and Rs, stores low 32 bits in Rd.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │  11:8  │  7:4  │  3:0
   ─────────┼──────────┼─────────┼─────────┼────────┼───────┼────────
   0x10     │   cond   │    0    │   Rd    │   Rm   │  Rs   │   0

**Flags Affected:** N, Z (C and V are undefined)

**Operational Semantics:** ``Rd ← Rm × Rs``

**Example:**

.. code-block:: asm

   mov r0, #6
   mov r1, #7
   mul r2, r0, r1    ; r2 = 42

**Complexity:** O(1) time, O(1) space

**Note:** Multiplication result is modulo 2³². For signed multiplication,
results may differ from hardware ARM.

MLA - Multiply Accumulate
-------------------------

**Opcode:** 0x11

**Syntax:** ``MLA Rd, Rm, Rs, Rn``

**Description:** Multiplies Rm and Rs, adds Rn, stores result in Rd.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │  11:8  │  7:4  │  3:0
   ─────────┼──────────┼─────────┼─────────┼────────┼───────┼────────
   0x11     │   cond   │   Rn    │   Rd    │   Rm   │  Rs   │   0

**Flags Affected:** N, Z (C and V are undefined)

**Operational Semantics:** ``Rd ← (Rm × Rs) + Rn``

**Example:**

.. code-block:: asm

   mov r0, #3
   mov r1, #4
   mov r2, #10
   mla r3, r0, r1, r2  ; r3 = (3*4) + 10 = 22

**Complexity:** O(1) time, O(1) space

Load/Store Instructions
=======================

LDR - Load Register (Word)
--------------------------

**Opcode:** 0x20

**Syntax:** ``LDR Rt, [Rn, #offset]``

**Description:** Loads a 32-bit word from memory into Rt. Address = Rn + offset.
Address must be word-aligned (multiple of 4).

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x20     │   cond   │   Rn    │   Rt    │    offset (signed)

**Flags Affected:** None

**Operational Semantics:** ``Rt ← Mem32[Rn + offset]``

**Example:**

.. code-block:: asm

   ldr r0, [r1, #4]   ; r0 = *(r1 + 4)
   ldr r1, [sp]       ; r1 = *sp

**Complexity:** O(1) time, O(1) space

**Note:** Unaligned access results in undefined behavior.

LDRB - Load Register (Byte)
---------------------------

**Opcode:** 0x21

**Syntax:** ``LDRB Rt, [Rn, #offset]``

**Description:** Loads an 8-bit byte from memory into Rt, zero-extended.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x21     │   cond   │   Rn    │   Rt    │    offset (signed)

**Flags Affected:** None

**Operational Semantics:** ``Rt ← zero_extend(Mem8[Rn + offset])``

**Example:**

.. code-block:: asm

   ldrb r0, [r1]      ; r0 = *(r1) (byte, zero-extended)

**Complexity:** O(1) time, O(1) space

STR - Store Register (Word)
---------------------------

**Opcode:** 0x22

**Syntax:** ``STR Rt, [Rn, #offset]``

**Description:** Stores a 32-bit word from Rt to memory.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x22     │   cond   │   Rn    │   Rt    │    offset (signed)

**Flags Affected:** None

**Operational Semantics:** ``Mem32[Rn + offset] ← Rt``

**Example:**

.. code-block:: asm

   str r0, [r1, #4]   ; *(r1 + 4) = r0
   str r1, [sp]       ; *sp = r1

**Complexity:** O(1) time, O(1) space

STRB - Store Register (Byte)
----------------------------

**Opcode:** 0x23

**Syntax:** ``STRB Rt, [Rn, #offset]``

**Description:** Stores low 8 bits of Rt to memory.

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │     11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────────
   0x23     │   cond   │   Rn    │   Rt    │    offset (signed)

**Flags Affected:** None

**Operational Semantics:** ``Mem8[Rn + offset] ← Rt[7:0]``

**Example:**

.. code-block:: asm

   strb r0, [r1]      ; *(r1) = r0 (byte)

**Complexity:** O(1) time, O(1) space

Branch Instructions
===================

B - Branch
----------

**Opcode:** 0x30

**Syntax:** ``B{cond} label``

**Description:** Branches to the specified label.

**Encoding:**::

   31:24    │  23:20   │           19:0
   ─────────┼──────────┼──────────────────────────────
   0x30     │   cond   │   signed_offset (word units)

**Offset Calculation:**::

   target = PC + 4 + (sign_extend(offset) << 2)

The offset is a signed 20-bit value in word units (multiplied by 4).

**Flags Affected:** None

**Example:**

.. code-block:: asm

   b loop           ; unconditional branch
   beq .equal       ; branch if equal
   bne .not_equal   ; branch if not equal

**Complexity:** O(1) time, O(1) space

BL - Branch with Link
---------------------

**Opcode:** 0x31

**Syntax:** ``BL{cond} label``

**Description:** Branches to label, saving return address in LR (R14).

**Encoding:**::

   31:24    │  23:20   │           19:0
   ─────────┼──────────┼──────────────────────────────
   0x31     │   cond   │   signed_offset (word units)

**Offset Calculation:** Same as B.

**Flags Affected:** None

**Operational Semantics:**::

   LR ← PC
   PC ← target

**Example:**

.. code-block:: asm

   bl subroutine     ; call subroutine
   bx lr             ; return

**Complexity:** O(1) time, O(1) space

BX - Branch and Exchange
------------------------

**Opcode:** 0x32

**Syntax:** ``BX Rn``

**Description:** Branches to address in Rn, optionally switching ARM/Thumb state
(Thumb not supported in varm).

**Encoding:**::

   31:24    │  23:20   │  19:16  │  15:12  │  11:0
   ─────────┼──────────┼─────────┼─────────┼─────────────
   0x32     │   cond   │    0    │    0    │     Rn

**Flags Affected:** None

**Operational Semantics:** ``PC ← Rn``

**Example:**

.. code-block:: asm

   mov lr, pc        ; save return address
   bx r0             ; branch to address in r0

**Complexity:** O(1) time, O(1) space

System Instructions
===================

HALT - Halt Execution
---------------------

**Opcode:** 0x40

**Syntax:** ``HALT``

**Description:** Stops execution and exits with code 0.

**Encoding:**::

   31:24    │  23:20   │
   ─────────┼──────────┘
   0x40     │   cond

**Flags Affected:** None

**Example:**

.. code-block:: asm

   halt              ; exit with code 0

**Complexity:** O(1) time, O(1) space

NOP - No Operation
------------------

**Opcode:** 0x42

**Syntax:** ``NOP``

**Description:** No operation. Does nothing.

**Encoding:**::

   31:24    │  23:20   │
   ─────────┼──────────┘
   0x42     │   cond

**Flags Affected:** None

**Example:**

.. code-block:: asm

   nop               ; do nothing

**Complexity:** O(1) time, O(1) space

SWI - Software Interrupt
------------------------

**Opcode:** 0x41

**Syntax:** ``SWI [#imm]``

**Description:** Triggers a software interrupt for syscalls. The syscall number
is passed in R7, with arguments in R0, R1, R2.

**Encoding:**::

   31:24    │  23:20   │     11:0
   ─────────┼──────────┼─────────────────
   0x41     │   cond   │    imm (unused)

**Flags Affected:** None

**Example:**

.. code-block:: asm

   mov r7, #1        ; syscall = EXIT
   mov r0, #0        ; exit code = 0
   swi               ; invoke syscall

**Complexity:** O(1) time, O(1) space

See :ref:`syscalls` for available syscalls.

Load Multiple Instructions
==========================

LDM - Load Multiple
-------------------

**Opcode:** 0x24

**Syntax:** ``LDM Rn, {Rt, ...}``

**Description:** Loads multiple words from memory starting at address in Rn.
The register list is stored in the operand field as a 12-bit bitmask.

**Encoding:**::

   31:24    │  23:20   │  19:16  │     11:0
   ─────────┼──────────┼─────────┼─────────────────────
   0x24     │   cond   │   Rn    │   register_list

**Flags Affected:** None

**Register List Encoding:**::

   Bit 0:  R0
   Bit 1:  R1
   ...
   Bit 12: R12

**Example:**

.. code-block:: asm

   ldm r0, {r1-r3}   ; load r1, r2, r3 from [r0]

**Complexity:** O(n) time, O(1) space, where n = number of registers

STM - Store Multiple
--------------------

**Opcode:** 0x25

**Syntax:** ``STM Rn, {Rt, ...}``

**Description:** Stores multiple words to memory starting at address in Rn.
The register list is stored in the operand field as a 12-bit bitmask.

**Encoding:**::

   31:24    │  23:20   │  19:16  │     11:0
   ─────────┼──────────┼─────────┼─────────────────────
   0x25     │   cond   │   Rn    │   register_list

**Flags Affected:** None

**Example:**

.. code-block:: asm

   stm r0, {r1-r3}   ; store r1, r2, r3 to [r0]

**Complexity:** O(n) time, O(1) space, where n = number of registers

Instruction Summary Table
=========================

+-------------+-------+-------+-------+-------+-------------------------+
| Instruction | Opcode| Flags | Rd/Rt | Rn    | Description             |
+-------------+-------+-------+-------+-------+-------------------------+
| MOV         | 0x00  | NZ    | Y     | -     | Move                    |
| MVN         | 0x01  | NZ    | Y     | -     | Move NOT                |
| ADD         | 0x02  | NZCV  | Y     | Y     | Add                     |
| ADC         | 0x03  | NZCV  | Y     | Y     | Add with Carry          |
| SUB         | 0x04  | NZCV  | Y     | Y     | Subtract                |
| SBC         | 0x05  | NZCV  | Y     | Y     | Subtract with Carry     |
| RSB         | 0x06  | NZCV  | Y     | Y     | Reverse Subtract        |
| RSC         | 0x07  | NZCV  | Y     | Y     | Reverse Sub w/ Carry    |
| AND         | 0x08  | NZ    | Y     | Y     | Bitwise AND             |
| EOR         | 0x09  | NZ    | Y     | Y     | Bitwise XOR             |
| ORR         | 0x0A  | NZ    | Y     | Y     | Bitwise OR              |
| BIC         | 0x0B  | NZ    | Y     | Y     | Bit Clear               |
| CMP         | 0x0C  | NZCV  | -     | Y     | Compare                 |
| CMN         | 0x0D  | NZCV  | -     | Y     | Compare Negative        |
| TST         | 0x0E  | NZ    | -     | Y     | Test Bits               |
| TEQ         | 0x0F  | NZ    | -     | Y     | Test Equivalence        |
| MUL         | 0x10  | NZ    | Y     | -     | Multiply                |
| MLA         | 0x11  | NZ    | Y     | Y     | Multiply Accumulate     |
| LDR         | 0x20  | -     | Y     | Y     | Load Word               |
| LDRB        | 0x21  | -     | Y     | Y     | Load Byte               |
| STR         | 0x22  | -     | Y     | Y     | Store Word              |
| STRB        | 0x23  | -     | Y     | Y     | Store Byte              |
| LDM         | 0x24  | -     | Y     | Y     | Load Multiple           |
| STM         | 0x25  | -     | -     | Y     | Store Multiple          |
| B           | 0x30  | -     | -     | -     | Branch                  |
| BL          | 0x31  | -     | -     | -     | Branch with Link        |
| BX          | 0x32  | -     | -     | Y     | Branch and Exchange     |
| HALT        | 0x40  | -     | -     | -     | Halt                    |
| SWI         | 0x41  | -     | -     | -     | Software Interrupt      |
| NOP         | 0x42  | -     | -     | -     | No Operation            |
+-------------+-------+-------+-------+-------+-------------------------+
