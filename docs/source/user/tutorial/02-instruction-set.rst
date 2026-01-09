.. _instruction-set:

==============
Instruction Set
==============

.. warning::
   varm is not stable. The instruction set, semantics, and behavior may change
   in future versions. All examples in this documentation work with the current
   version, but verify behavior in your specific installation.

Introduction to Instructions
============================

What Is an Instruction?
-----------------------

In high-level programming languages like Python, Java, or C++, you write code
using statements that the compiler translates into machine code. For example,
when you write ``x = y + 5``, the compiler generates multiple low-level
operations: load values from memory, perform addition, and store the result.
This translation happens automatically, and you never see the intermediate
steps.

Assembly language gives you direct access to these low-level operations. Each
individual operation that a processor can perform is called an **instruction**,
and assembly language provides a human-readable representation (called a
**mnemonic**) for each instruction. When you write assembly code, you are
writing a sequence of instructions that the processor will execute one at a
time.

Think of it like this: if high-level code is like writing a recipe in natural
language ("first cream the butter and sugar, then add eggs one at a time..."),
assembly language is like giving someone step-by-step physical instructions
("pick up the spatula, move your hand to the bowl, rotate your wrist 90
degrees..."). The instructions are simpler but far more numerous.

In varm, every instruction occupies exactly 4 bytes (one **word**) in memory.
This means the processor can efficiently fetch and decode instructions because
their size is predictable. The instruction set follows ARM conventions, where
instructions have a consistent structure that makes them easier to decode.

Anatomy of a varm Instruction
-----------------------------

Every varm instruction follows a consistent structure that determines what
operation it performs and on which data. Understanding this structure helps you
read and write assembly code effectively.

The general format for most varm instructions is:

.. code-block:: text

    ┌─────┬───────┬───────┬─────────────┬─────────────┐
    │cond │ opcode │  rd   │     rn      │  operand2   │
    │(4)  │  (4)   │  (4)  │    (4)      │    (16)     │
    └─────┴───────┴───────┴─────────────┴─────────────┘
     MSB                                                    LSB

The fields are:

* **cond** (bits 31-28): Condition code. Determines whether the instruction
  executes based on the current processor flags. If omitted, execution is
  unconditional (always runs).

* **opcode** (bits 27-24): The operation to perform (ADD, SUB, MOV, etc.).

* **rd** (bits 23-20): Destination register. Where the result of the operation
  will be stored.

* **rn** (bits 19-16): First source operand register. Many operations take
  two operands; this is typically the first one.

* **operand2** (bits 15-0): Second operand. Can be a register, an immediate
  value, or a register with a shift applied.

For example, the instruction ``ADD r0, r1, r2`` means:
"Add the value in register r1 to the value in register r2, and store the result
in register r0."

We'll explore each field in detail as we cover different instruction types.
For now, remember that most instructions specify a destination register (rd)
and one or more source operands.

Data Movement Instructions
==========================

Data movement instructions copy values between registers and immediate values.
These are fundamental because they allow you to set up values before performing
other operations.

MOV: Copy Values
----------------

The ``MOV`` instruction copies a value from one location to another. The source
can be a register or an immediate value (a constant written directly in the
instruction).

**Syntax:**

.. code-block:: text

    MOV  rd, <source>

**Examples:**

.. code-block:: arm

    MOV  r0, r1        @ Copy value from r1 to r0
    MOV  r2, #42       @ Load immediate value 42 into r2
    MOV  r3, r0        @ Copy r0 to r3

**Operational Semantics:**

.. math::

    \text{MOV}(rd, \text{src}) \equiv rd \leftarrow \text{contents}(\text{src})

**Complexity:** O(1) - single cycle operation

**Starter Code:**

.. code-block:: arm

    @ demonstrate_mov.asm - Understanding data movement
    @ Run with: varm demonstrate_mov.asm

    .text
    .global _start

    _start:
        MOV  r0, #100        @ Load immediate value 100 into r0
        MOV  r1, r0          @ Copy r0 to r1 (r1 now equals 100)
        MOV  r2, #200        @ Load 200 into r2
        MOV  r0, r2          @ Copy r2 to r0 (r0 now equals 200)

    @ Program ends here - r0 contains the exit code

MVN: Move with Negation
-----------------------

The ``MVN`` (Move Not) instruction copies the bitwise complement of a value.
That is, it inverts every bit: 0 becomes 1, and 1 becomes 0.

**Syntax:**

.. code-block:: text

    MVN  rd, <source>

**Examples:**

.. code-block:: arm

    MVN  r0, r1        @ Copy bitwise NOT of r1 to r0
    MVN  r2, #0xFF     @ Copy bitwise NOT of 255 to r2

**Explanation:** If r1 contains the binary value ``00000000000000000000000000001111``
(decimal 15), then ``MVN r0, r1`` would store ``11111111111111111111111111110000``
(decimal -16 or 4294967280 in unsigned representation).

**Operational Semantics:**

.. math::

    \text{MVN}(rd, \text{src}) \equiv rd \leftarrow \neg \text{contents}(\text{src})

**Complexity:** O(1) - single cycle operation

**Starter Code:**

.. code-block:: arm

    @ demonstrate_mvn.asm - Bitwise NOT operation
    @ Run with: varm demonstrate_mvn.asm

    .text
    .global _start

    _start:
        MOV  r0, #0b00001111    @ Binary: load 15 into r0
        MVN  r1, r0             @ r1 = bitwise NOT of r0
                               @ r1 = 0b11110000 (240 unsigned, -16 signed)

Arithmetic Instructions
======================

Arithmetic instructions perform mathematical operations on values. These
operations form the foundation of computation.

ADD: Addition
-------------

The ``ADD`` instruction adds two values and stores the result in a destination
register.

**Syntax:**

.. code-block:: text

    ADD  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    ADD  r0, r1, r2        @ r0 = r1 + r2
    ADD  r3, r4, #10       @ r3 = r4 + 10
    ADD  r5, r6, r7        @ r5 = r6 + r7

**Operational Semantics:**

.. math::

    \text{ADD}(rd, rn, op2) \equiv rd \leftarrow \text{contents}(rn) + \text{contents}(op2)

**Complexity:** O(1) - single cycle operation

SUB: Subtraction
----------------

The ``SUB`` instruction subtracts the second operand from the first.

**Syntax:**

.. code-block:: text

    SUB  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    SUB  r0, r1, r2        @ r0 = r1 - r2
    SUB  r3, r4, #5        @ r3 = r4 - 5

**Operational Semantics:**

.. math::

    \text{SUB}(rd, rn, op2) \equiv rd \leftarrow \text{contents}(rn) - \text{contents}(op2)

**Complexity:** O(1) - single cycle operation

ADC: Addition with Carry
------------------------

The ``ADC`` (Add with Carry) instruction adds two values plus the carry flag.
This allows you to chain additions across multiple words for numbers larger
than 32 bits.

**Syntax:**

.. code-block:: text

    ADC  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    ADC  r0, r1, r2        @ r0 = r1 + r2 + Carry

SBC: Subtraction with Carry
---------------------------

The ``SBC`` (Subtract with Carry) instruction subtracts the second operand and
the NOT of the carry flag. Useful for multi-word subtraction.

**Syntax:**

.. code-block:: text

    SBC  rd, rn, <operand2>

RSB: Reverse Subtraction
------------------------

The ``RSB`` (Reverse Subtract) instruction subtracts the first operand from
the second, effectively computing ``operand2 - rn``.

**Syntax:**

.. code-block:: text

    RSB  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    RSB  r0, r1, r2        @ r0 = r2 - r1

RSC: Reverse Subtract with Carry
--------------------------------

The ``RSC`` (Reverse Subtract with Carry) instruction combines RSB with carry.

How Arithmetic Instructions Affect Flags
----------------------------------------

The **NZCV** flags in the Condition Code Register (CCR) indicate the result of
arithmetic operations:

* **N (Negative):** Set when the result is negative (bit 31 is 1)
* **Z (Zero):** Set when the result is exactly zero
* **C (Carry):** Set when unsigned overflow occurs (result too large for 32 bits)
* **V (Overflow):** Set when signed overflow occurs

By default, arithmetic instructions update these flags. To perform arithmetic
without affecting flags, use the ``S`` suffix (e.g., ``ADDS``, ``SUBS``).

**Starter Code:**

.. code-block:: arm

    @ demonstrate_arithmetic.asm - Basic arithmetic operations
    @ Run with: varm demonstrate_arithmetic.asm

    .text
    .global _start

    _start:
        MOV   r0, #10          @ Load 10 into r0
        MOV   r1, #5           @ Load 5 into r1

        ADD   r2, r0, r1       @ r2 = 10 + 5 = 15
        SUB   r3, r0, r1       @ r3 = 10 - 5 = 5
        RSB   r4, r0, r1       @ r4 = 5 - 10 = -5

    @ r2 = 15, r3 = 5, r4 = -5

Logical Instructions
====================

Logical instructions perform bitwise operations on values. These are essential
for working with individual bits, masks, and boolean logic.

AND: Bitwise AND
----------------

The ``AND`` instruction performs a bitwise AND between two values.

**Syntax:**

.. code-block:: text

    AND  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    AND  r0, r1, r2        @ r0 = r1 AND r2
    AND  r3, r4, #0xFF     @ Keep only the lowest 8 bits of r4

**Truth Table:**

=== === =====
A   B   A AND B
=== === =====
0   0     0
0   1     0
1   0     0
1   1     1
=== === =====

ORR: Bitwise OR
---------------

The ``ORR`` (OR) instruction performs a bitwise OR between two values.

**Syntax:**

.. code-block:: text

    ORR  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    ORR  r0, r1, r2        @ r0 = r1 OR r2
    ORR  r3, r4, #0x0F     @ Set the lowest 4 bits of r4

**Truth Table:**

=== === =====
A   B   A OR B
=== === =====
0   0     0
0   1     1
1   0     1
1   1     1
=== === =====

EOR: Bitwise Exclusive OR
-------------------------

The ``EOR`` (XOR) instruction performs a bitwise exclusive OR.

**Syntax:**

.. code-block:: text

    EOR  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    EOR  r0, r1, r2        @ r0 = r1 XOR r2

**Truth Table:**

=== === =====
A   B   A XOR B
=== === =====
0   0     0
0   1     1
1   0     1
1   1     0
=== === =====

BIC: Bit Clear
--------------

The ``BIC`` instruction clears (sets to 0) specific bits. It performs
``rd = rn AND NOT operand2``.

**Syntax:**

.. code-block:: text

    BIC  rd, rn, <operand2>

**Examples:**

.. code-block:: arm

    BIC  r0, r1, #0x0F     @ Clear the lowest 4 bits of r1

**Starter Code:**

.. code-block:: arm

    @ demonstrate_logical.asm - Bitwise operations
    @ Run with: varm demonstrate_logical.asm

    .text
    .global _start

    _start:
        MOV   r0, #0b11110000    @ 240 in decimal
        MOV   r1, #0b00001111    @ 15 in decimal

        AND   r2, r0, r1        @ r2 = 0b00000000 (0)
        ORR   r3, r0, r1        @ r3 = 0b11111111 (255)
        EOR   r4, r0, r1        @ r4 = 0b11111111 (255)
        BIC   r5, r0, #0b10000000 @ Clear bit 7 of r0

Comparison Instructions
=======================

Comparison instructions examine values and update the condition flags, but do
not store a result. They are essential for making decisions in your program.

CMP: Compare
------------

The ``CMP`` instruction subtracts the second operand from the first and
updates the flags, but discards the result.

**Syntax:**

.. code-block:: text

    CMP  rn, <operand2>

**Examples:**

.. code-block:: arm

    CMP  r0, r1        @ Compare r0 and r1
    CMP  r2, #100      @ Compare r2 with 100

**Effect on flags:**

* **Z = 1** if rn == operand2 (values are equal)
* **N = 1** if rn < operand2 (signed comparison)
* **C = 0** if rn < operand2 (unsigned comparison)
* **C = 1** if rn >= operand2 (unsigned comparison)

CMN: Compare Negated
--------------------

The ``CMN`` instruction adds the second operand to the first and updates flags.

**Syntax:**

.. code-block:: text

    CMN  rn, <operand2>

**Effect:** Tests if rn + operand2 equals zero.

TST: Test Bits
--------------

The ``TST`` instruction performs a bitwise AND and updates flags.

**Syntax:**

.. code-block:: text

    TST  rn, <operand2>

**Use case:** Check if specific bits are set.

**Example:**

.. code-block:: arm

    TST  r0, #0b00001000    @ Is bit 3 set in r0?

TEQ: Test Equivalence
---------------------

The ``TEQ`` instruction performs a bitwise XOR and updates flags.

**Syntax:**

.. code-block:: text

    TEQ  rn, <operand2>

**Use case:** Check if two values have identical bits.

**Starter Code:**

.. code-block:: arm

    @ demonstrate_comparison.asm - Using comparison instructions
    @ Run with: varm demonstrate_comparison.asm

    .text
    .global _start

    _start:
        MOV   r0, #50          @ r0 = 50
        MOV   r1, #100         @ r1 = 100

        CMP   r0, r1           @ Compare 50 vs 100
        @ After CMP: Z=0, N=1, C=0

    @ Comparison doesn't change r0 or r1, only flags

Multiplication Instructions
===========================

Multiplication instructions multiply values together.

MUL: Multiply
-------------

The ``MUL`` instruction multiplies two 32-bit values to produce a 32-bit result.

**Syntax:**

.. code-block:: text

    MUL  rd, rm, rs

**Examples:**

.. code-block:: arm

    MUL  r0, r1, r2        @ r0 = r1 * r2

**Warning:** The result is modulo 2^32. If the true product exceeds 32 bits,
the high bits are discarded.

**Complexity:** O(1) - single cycle operation

MLA: Multiply and Accumulate
----------------------------

The ``MLA`` instruction multiplies two values and adds a third.

**Syntax:**

.. code-block:: text

    MLA  rd, rm, rs, rn

**Effect:** ``rd = (rm * rs) + rn``

**Examples:**

.. code-block:: arm

    MLA  r0, r1, r2, r3    @ r0 = (r1 * r2) + r3

**Starter Code:**

.. code-block:: arm

    @ demonstrate_multiplication.asm - Multiplying values
    @ Run with: varm demonstrate_multiplication.asm

    .text
    .global _start

    _start:
        MOV   r0, #6           @ r0 = 6
        MOV   r1, #7           @ r1 = 7

        MUL   r2, r0, r1       @ r2 = 6 * 7 = 42

        MOV   r3, #10          @ r3 = 10
        MLA   r4, r0, r1, r3   @ r4 = (6 * 7) + 10 = 52

Branch Instructions
===================

Branch instructions change the flow of execution by modifying the program
counter (PC). Normally, the processor executes instructions sequentially,
fetching each instruction from the next consecutive address. A branch
instruction can redirect execution to a different location.

B: Unconditional Branch
-----------------------

The ``B`` instruction transfers execution to the specified label.

**Syntax:**

.. code-block:: text

    B    <label>

**Examples:**

.. code-block:: arm

    B    loop            @ Jump to 'loop' label
    B    end             @ Jump to 'end' label

**Effect:** PC ← address of label

**Starter Code:**

.. code-block:: arm

    @ demonstrate_branch.asm - Basic branching
    @ Run with: varm demonstrate_branch.asm

    .text
    .global _start

    _start:
        MOV   r0, #0           @ Initialize counter

    loop:
        CMP   r0, #5           @ Have we reached 5?
        BEQ   done             @ If yes, branch to done

        ADD   r0, r0, #1       @ Increment counter
        B     loop             @ Loop back

    done:
    @ Program continues here when r0 = 5

BL: Branch with Link
--------------------

The ``BL`` instruction branches to a subroutine and saves the return address
in the Link Register (LR, which is register r14).

**Syntax:**

.. code-block:: text

    BL    <label>

**Examples:**

.. code-block:: arm

    BL    my_function     @ Call my_function, return address in LR

**Use case:** Call functions or subroutines. The callee can return by
branching back to the address saved in LR.

**Starter Code:**

.. code-block:: arm

    @ demonstrate_bl.asm - Function calls with BL
    @ Run with: varm demonstrate_bl.asm

    .text
    .global _start

    _start:
        MOV   r0, #10          @ r0 = 10
        BL    double           @ Call double function
        @ After return, r0 contains 20

    double:
        ADD   r0, r0, r0       @ r0 = r0 * 2
        MOV   pc, lr           @ Return to caller (PC = LR)

Conditional Branches
--------------------

Conditional branches execute only if specific flags meet a condition. The
condition code is appended to the instruction.

**Condition Codes:**

=== ===== ====================
Code Meaning                  Flags Required
=== ===== ====================
EQ   Equal                    Z = 1
NE   Not Equal                Z = 0
GT   Greater Than (signed)    Z = 0, N = V
LT   Less Than (signed)       N ≠ V
GE   Greater or Equal (signed) N = V
LE   Less or Equal (signed)   Z = 1 or N ≠ V
HI   Higher (unsigned)        C = 1, Z = 0
HS   Higher or Same (unsigned) C = 1
LO   Lower (unsigned)         C = 0
LS   Lower or Same (unsigned) C = 0 or Z = 1
=== ===== ====================

**Examples:**

.. code-block:: arm

    BEQ   equal_label       @ Branch if equal
    BNE   not_equal_label   @ Branch if not equal
    BGT   greater_label     @ Branch if greater (signed)
    BLO   loop_label        @ Branch if lower (unsigned)

System Instructions
===================

System instructions interact with the virtual machine or request services.

HALT: Stop Execution
--------------------

The ``HALT`` instruction stops the virtual machine. When the program reaches
this instruction, execution terminates.

**Syntax:**

.. code-block:: text

    HALT

**Example:**

.. code-block:: arm

    HALT                    @ Stop the program

NOP: No Operation
-----------------

The ``NOP`` instruction does nothing but consume a cycle. It can be used for
padding or timing adjustments.

**Syntax:**

.. code-block:: text

    NOP

SWI: Software Interrupt
-----------------------

The ``SWI`` instruction triggers a software interrupt, used to make system
calls (requesting services from the operating system). We cover syscalls
in detail in :ref:`syscalls`.

**Syntax:**

.. code-block:: text

    SWI  <number>

**Example:**

.. code-block:: arm

    SWI  0x9001              @ Make syscall number 0x9001

**Starter Code:**

.. code-block:: arm

    @ demonstrate_system.asm - System instructions
    @ Run with: varm demonstrate_system.asm

    .text
    .global _start

    _start:
        NOP                     @ Do nothing
        NOP                     @ Another NOP
        HALT                    @ Stop execution
