========
Examples
========

Working programs demonstrating varm concepts.

Basic Arithmetic
================

.. code-with-test:: varm
   :expected_exit: 42

   mov r0, #25
   mov r1, #17
   add r2, r0, r1
   mov r0, r2
   mov r7, #1
   swi

Data Handling
=============

.. code-with-test:: varm
   :expected_exit: 150

   .data
   value1: .byte 100
   value2: .byte 50

   .text
   ldr r0, =value1
   ldrb r1, [r0]
   ldr r0, =value2
   ldrb r2, [r0]
   add r3, r1, r2
   mov r0, r3
   mov r7, #1
   swi

Count to 5
==========

.. code-with-test:: varm
   :expected_exit: 5

   mov r0, #0
   mov r1, #5
   loop:
       add r0, r0, #1
       cmp r0, r1
       blt loop
   mov r7, #1
   swi

See :doc:`../exercises/index` for practice problems.
