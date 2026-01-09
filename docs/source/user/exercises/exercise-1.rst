Exercise 1: Basic Operations
============================

Learning Objectives
-------------------
- Understand immediate values and register operations
- Use MOV, ADD, and SUB instructions for arithmetic
- Comprehend instruction operand formats (register and immediate)
- Calculate arithmetic expressions using assembly instructions
- Use syscalls to exit with a specific code

Time Estimate
-------------
10-15 minutes

Starter Code
------------

.. code-block:: vasm

   ; calculator.vasm - Basic arithmetic operations
   ; Task: Compute (42 + 58) * 2 and exit with the result

   .data
   ; No data needed for this exercise

   .text
   ; TODO: Complete this program
   ; Step 1: Load 42 into a register
   ; mov r0, #42

   ; Step 2: Load 58 into another register
   ; mov r1, #58

   ; Step 3: Add them together (42 + 58 = 100)
   ; add r2, r0, r1

   ; Step 4: Multiply by 2 using addition
   ; add r3, r2, r2

   ; Step 5: Exit with the result in r0
   ; mov r0, r3
   ; mov r7, #1
   ; swi

Task Description
----------------
In this exercise, you will write a program that performs basic arithmetic
operations and exits with the computed result.

The program should:
1. Load the values 42 and 58 into registers
2. Add them together (42 + 58 = 100)
3. Multiply the result by 2 (100 * 2 = 200)
4. Exit with the final result (200)

The varm instruction set does not have a MUL instruction for immediate
multiplication, so you will use ADD to multiply by 2 (adding a number to itself).

Expected Output
---------------
The program will exit silently with code 200. To verify:

.. code-block:: bash

   ./qol.sh asmrun calculator.vasm
   echo $?    ; Should print 200

Challenge/Extension (Optional)
------------------------------
For extra practice, try these variations:

1. **Calculate a different expression**: Compute (10 + 20) * 3 - 5 = 85
2. **Use more registers**: Rewrite to use registers r4-r7 for intermediate values
3. **Chain operations**: Compute ((10 + 20) * 2) / 5 and verify you get 12

Solution
--------

One possible solution:

.. code-block:: vasm

   ; calculator.vasm - Complete solution
   ; Compute (42 + 58) * 2 = 200

   .data
   ; No data needed

   .text
           mov r0, #42      ; r0 = 42
           mov r1, #58      ; r1 = 58
           add r2, r0, r1   ; r2 = r0 + r1 = 100
           add r3, r2, r2   ; r3 = r2 + r2 = 200 (multiply by 2)
           mov r0, r3       ; exit code = result
           mov r7, #1       ; syscall = EXIT
           swi              ; exit with code 200

Notes
-----
- The ``mov r0, #42`` syntax uses ``#`` to indicate an immediate value
- ``add rd, rn, rm`` adds rn and rm, storing the result in rd
- The exit code is passed in r0, and the EXIT syscall number (1) is in r7
- ``swi`` triggers the software interrupt for syscalls
