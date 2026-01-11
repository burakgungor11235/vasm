Exercise 3: Data and I/O Operations
====================================

Learning Objectives
-------------------
- Declare and use data in the .data section
- Use LDR and STR instructions for memory access
- Use the =label pseudo-instruction for loading addresses
- Perform syscalls for WRITE operations
- Understand memory addressing

Task Description
----------------
In this exercise, you will create a program that uses data sections and
memory access instructions to display a message.

Working Solution
----------------

.. code-with-test:: varm
   :expected_stdout: Hello, varm!

   .data
   msg: .byte 'H', 'e', 'l', 'l', 'o', ',', ' ', 'v', 'a', 'r', 'm', '!', 10

   .text
   mov r0, #1          ; stdout
   ldr r1, =msg        ; address of msg
   mov r2, #13         ; length = 13
   mov r7, #3          ; syscall = WRITE
   swi                 ; write to stdout

   mov r0, #0          ; exit code = 0
   mov r7, #1          ; syscall = EXIT
   swi                 ; exit

Explanation
-----------
- ``.byte 'H', 'e', 'l', 'l', 'o'`` declares a string as individual bytes
- ``ldr r1, =msg`` loads the address of msg (pseudo-instruction)
- ``mov r2, #13`` sets the number of bytes to write
- The WRITE syscall (3) outputs data to a file descriptor

Notes
-----
- ``ldrb rd, [rn, #offset]`` loads a single byte, zero-extended
- ``strb rd, [rn, #offset]`` stores a single byte
- The READ syscall (2) returns the number of bytes read in r0
- Character literals like #'a' are converted to their ASCII values at assembly time
