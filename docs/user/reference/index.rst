=========
Reference
=========

Complete reference for varm instruction set, directives, syscalls, and file format.

.. toctree::
   :maxdepth: 2

   instruction-set
   directives
   syscalls
   file-format

Quick Reference
===============

Registers: r0-r12 (general), r13=sp (stack), r14=lr (link), r15=pc (counter)

Condition Codes: EQ, NE, GT, LT, GE, LE, CS, CC, MI, PL, VS, VC

Syscalls: 1=EXIT, 2=READ, 3=WRITE

See :doc:`instruction-set` for complete opcode table.
