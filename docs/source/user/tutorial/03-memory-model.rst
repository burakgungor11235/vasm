============
Memory Model
============

varm has 1 MB addressable memory.

========== =============
Section    Start Address
========== =============
.text      0x20
.data      0x10000
Stack      0x100000 (grows down)
========== =============

.text (Code)
============

Executable instructions. Each instruction is 4 bytes.

.. code-block:: varm

   .text
   mov r0, #42

.data (Data)
============

Initialized variables.

.. code-block:: varm

   .data
   msg:    .byte 'H', 'e', 'l', 'l', 'o'
   value:  .word 42
   buffer: .space 16

Directives: ``.byte``, ``.word``, ``.space``

Memory Access
=============

Load/Store instructions:

.. code-block:: varm

   ldr r0, [r1]           ; r0 = *r1
   ldr r2, [r3, #8]       ; r2 = *(r3 + 8)
   ldr r4, =label         ; r4 = address of label
   str r0, [r1]           ; *r1 = r0

Array indexing:

.. code-block:: varm

   ldr r0, [r2, r3, LSL #2]  ; r0 = *(r2 + r3*4)

Stack
=====

Grows downward from 0x100000.

.. code-block:: varm

   push {r0}   ; sp -= 4, *sp = r0
   pop {r1}    ; r1 = *sp, sp += 4
