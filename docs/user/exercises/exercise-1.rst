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

Task Description
----------------
In this exercise, you will write a program that performs basic arithmetic
operations and exits with the computed result.

The program should compute (42 + 58) * 2 and exit with the result 200.

Working Solution
----------------

.. code-with-test:: varm
   :expected_exit: 200

   mov r0, #42      ; r0 = 42
   mov r1, #58      ; r1 = 58
   add r2, r0, r1   ; r2 = r0 + r1 = 100
   add r3, r2, r2   ; r3 = r2 + r2 = 200 (multiply by 2)
   mov r0, r3       ; exit code = result
   mov r7, #1       ; syscall = EXIT
   swi              ; exit with code 200

Explanation
-----------
- ``mov r0, #42`` loads immediate value 42 into register r0
- ``add rd, rn, rm`` adds rn and rm, storing the result in rd
- The exit code is passed in r0, syscall number 1 (EXIT) is in r7
- ``swi`` triggers the software interrupt for syscalls

Notes
-----
- The ``mov r0, #42`` syntax uses ``#`` to indicate an immediate value
- The varm instruction set does not have a MUL instruction, so we use ADD to multiply by 2
