Implementation Documentation
============================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   opcode-decoding
   operand-decoding
   syscall-interface
   literal-pool

.. warning::
   **varm is not stable.** This documentation describes the current
   implementation and is subject to change without notice. The VM,
   instruction set, and APIs are experimental. Do not use for
   production workloads.

Overview
--------

This section provides detailed implementation documentation for varm,
covering the internal mechanisms of instruction decoding, operand
processing, system calls, and the literal pool.

This documentation is intended for developers who wish to:

- Understand how varm decodes and executes instructions
- Contribute to the varm project
- Build tools or debuggers on top of varm

.. important::
   All information in this section reflects the current implementation.
   Future versions may introduce breaking changes without warning.
   APIs, instruction encodings, and internal structures are not stable.

Related Documentation
---------------------

See :doc:`../architecture/index` for higher-level architecture overview
including the instruction pipeline, memory subsystem, and register file.

Source Code References
----------------------

Key source files referenced in this documentation:

- ``src/vm/core.c`` - Execution engine and opcode dispatch
- ``src/vm/instruction.c`` - Instruction execution handlers
- ``scripts/gen_lookup_tables.py`` - Build-time table generation
- ``src/asm/parser.c`` - Assembler and literal pool handling
- ``src/asm/generated/`` - Generated lookup tables

Conventions Used
----------------

- Instruction bit positions use ARM-style notation (bit 31 is MSB)
- Opcode values are shown in hexadecimal (e.g., ``0x00``)
- Complexity analysis uses Big-O notation
- Source code references include line numbers for key functions
