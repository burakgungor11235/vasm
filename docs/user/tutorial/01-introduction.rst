============
Introduction
============

varm is an **educational ARM-like virtual machine** for learning assembly programming.

Why varm?
=========

* **Safe**: Bugs crash the VM, not your system
* **Simple**: ~20 instructions vs. hundreds on real ARM
* **Portable**: Runs anywhere with a C compiler

Registers
=========

varm has 16 registers (r0-r15):

* **r0-r12**: General purpose
* **r13 (sp)**: Stack pointer
* **r14 (lr)**: Link register
* **r15 (pc)**: Program counter

Memory Layout
=============

* **Text** (0x20): Executable code
* **Data** (0x10000): Variables

First Program
=============

.. code-block:: varm

   .text
   mov r0, #42
   mov r7, #1
   swi

Build and run:

.. code-block:: bash

   vasm program.vasm -o program.vm
   varm program.vm

Key Instructions
================

* ``mov rd, #imm``: Move value to register
* ``add rd, rn, rm``: Addition
* ``sub rd, rn, rm``: Subtraction
* ``mul rd, rn, rm``: Multiplication
* ``swi``: Syscall (I/O, exit)
* ``ldr rd, =label``: Load label address
* ``ldr rd, [rn]``: Load from memory

See :doc:`02-instruction-set` for the full instruction set.
