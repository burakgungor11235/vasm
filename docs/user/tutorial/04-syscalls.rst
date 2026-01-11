========
Syscalls
========

Syscalls provide I/O via the ``SWI`` instruction. Syscall number in r7.

Convention
==========

====== ======================================
r7     Syscall number
r0-r2  Arguments
r0     Return value
====== ======================================

EXIT (1): Exit Program
======================

.. code-block:: varm

   mov r0, #0
   mov r7, #1
   swi

WRITE (3): Write to File Descriptor
===================================

.. code-block:: varm

   .data
   msg: .byte 'H', 'e', 'l', 'l', 'o', '!'

   .text
   mov r0, #1           ; stdout
   ldr r1, =msg
   mov r2, #6
   mov r7, #3
   swi

READ (2): Read from File Descriptor
===================================

.. code-block:: varm

   mov r0, #0           ; stdin
   ldr r1, =buffer
   mov r2, #16
   mov r7, #2
   swi                 ; r0 = bytes read

See :doc:`../reference/syscalls` for complete reference.
