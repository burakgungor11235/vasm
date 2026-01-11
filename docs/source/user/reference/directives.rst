.. _directives:

==========
Directives
==========

Assembler directives control assembly and define data.

Section Control
===============

``.text``: Switch to code section (instructions at 0x20)

``.data``: Switch to data section (variables at 0x10000)

Data Definition
===============

``.byte val, ...``: 8-bit values

.. code-block:: varm

   .byte 65, 'A', 0x41

``.word val, ...``: 32-bit values

.. code-block:: varm

   .word 12345, 0xDEADBEEF

``.space n``: Allocate n zeroed bytes

.. code-block:: varm

   .space 16    ; 16 bytes of zeros

``.ascii "str"``: ASCII string (no null)

``.asciz "str"``: ASCII string with null terminator

Symbol Control
==============

``.global label``: Export label (makes it visible)

``label:``: Define label (named address)

Constant Definition
===================

``.equ name, value``: Define constant

.. code-block:: varm

   .equ BUFFER_SIZE, 256
   mov r0, #BUFFER_SIZE   ; becomes mov r0, #256
