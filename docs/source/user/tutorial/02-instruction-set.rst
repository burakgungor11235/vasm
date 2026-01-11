===============
Instruction Set
===============

Essential varm instructions for learning assembly programming.

Data Movement
=============

``mov rd, src``: Copy value

.. code-block:: varm

   mov r0, #42     ; Immediate
   mov r1, r0      ; Register copy

``mvn rd, src``: Bitwise NOT

Arithmetic
==========

``add rd, rn, rm``: Addition

.. code-block:: varm

   add r2, r0, r1  ; r2 = r0 + r1

``sub rd, rn, rm``: Subtraction
``mul rd, rn, rm``: Multiplication
``rsb rd, rn, imm``: Reverse subtract (imm - rn)
``adc``: Add with carry
``sbc``: Subtract with carry

Logical
=======

``and rd, rn, rm``: Bitwise AND
``orr rd, rn, rm``: Bitwise OR
``eor rd, rn, rm``: Bitwise XOR
``bic rd, rn, rm``: Bit clear

Comparison
==========

``cmp rn, operand``: Compare (sets N, Z, C, V flags)
``cmn rn, operand``: Compare negative
``tst rn, operand``: Test bits (N, Z flags)
``teq rn, operand``: Test equivalence (N, Z flags)

Memory
======

``ldr rd, =label``: Load label address
``ldr rd, [rn]``: Load word from memory
``str rd, [rn]``: Store word to memory
``ldrb``, ``strb``: Byte variants

Branching
=========

``b label``: Branch
``bl label``: Branch with link (save return address)
``bx rn``: Branch to address in register

Condition Codes
===============

Execute instruction only if condition is true:

=== =============
EQ  Equal (Z=1)
NE  Not Equal (Z=0)
GT  Greater Than
LT  Less Than
GE  Greater/Equal
LE  Less/Equal
CS  Carry Set
CC  Carry Clear
=== =============


.. code-block:: varm

   cmp r0, r1
   bgt greater   ; Branch if r0 > r1

System
======

``swi``: Syscall (syscall number in r7)
``halt``: Stop execution

See :doc:`../reference/instruction-set` for complete reference.
