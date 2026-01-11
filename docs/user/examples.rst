========
Examples
========

Executable examples demonstrating varm features.

Exit Code
=========

.. code-with-test:: varm
   :expected_exit: 42

   mov r0, #42
   mov r7, #1
   swi

Hello World
===========

.. code-with-test:: varm
   :expected_stdout: Hello!

   .data
   msg: .byte 'H', 'e', 'l', 'l', 'o', '!'

   .text
   mov r0, #1
   ldr r1, =msg
   mov r2, #6
   mov r7, #3
   swi
   mov r0, #0
   mov r7, #1
   swi

Arithmetic
==========

.. code-with-test:: varm
   :expected_exit: 52

   mov r0, #42
   mov r1, #10
   add r2, r0, r1
   mov r0, r2
   mov r7, #1
   swi

Load Data
=========

.. code-with-test:: varm
   :expected_exit: 42

   .data
   value: .byte 42

   .text
   ldr r0, =value
   ldrb r0, [r0]
   mov r7, #1
   swi

See :doc:`tutorial/index` for a comprehensive tutorial and :doc:`exercises/index` for practice problems.
