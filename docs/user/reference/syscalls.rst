.. _syscalls:

========
Syscalls
========

Syscalls are invoked via ``SWI`` with syscall number in r7.

Convention
==========

====== ======================================
r7     Syscall number
r0-r2  Arguments
r0     Return value
====== ======================================

Reference
=========

EXIT (1): Terminate Program
---------------------------

Args: r0 = exit code

.. code-block:: varm

   mov r0, #0
   mov r7, #1
   swi

READ (2): Read from File Descriptor
-----------------------------------

Args: r0 = fd, r1 = buffer, r2 = count
Return: r0 = bytes read, or -1 on error

.. code-block:: varm

   mov r0, #0       ; stdin
   ldr r1, =buf
   mov r2, #16
   mov r7, #2
   swi

WRITE (3): Write to File Descriptor
-----------------------------------

Args: r0 = fd, r1 = buffer, r2 = count
Return: r0 = bytes written, or -1 on error

.. code-block:: varm

   mov r0, #1       ; stdout
   ldr r1, =msg
   mov r2, #6
   mov r7, #3
   swi

File Descriptors: 0=stdin, 1=stdout, 2=stderr
