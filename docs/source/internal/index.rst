Developer Guide
===============

This section provides detailed technical documentation for developers
who want to understand how varm works internally. This includes:

* How the virtual machine executes instructions
* How the assembler translates source code to bytecode
* The data structures and algorithms used
* The design rationale behind key decisions

.. warning::

   **This section contains implementation details that may change.**

   The internal API is not stable. Code that relies on internal
   implementation details may break between versions.

Who Should Read This
--------------------

You should read this documentation if you:

* Want to **contribute to varm** development
* Are **building a language** on top of the varm runtime
* Need to **debug or extend** the VM or assembler
* Are **studying the implementation** for a computer architecture course
* Want to understand the **design trade-offs** made

This documentation assumes familiarity with:

* C programming (the implementation language)
* Computer organization concepts (registers, memory, instruction execution)
* Data structures (hash tables, arrays)
* Algorithm analysis (time and space complexity)

If you are looking for documentation on how to *use* varm (not modify it),
please see the :doc:`../user/index` section instead.

Structure
---------

The internal documentation is organized into several sections:

.. toctree::
   :maxdepth: 2
   :caption: Developer Guide Contents

   architecture/index
   implementation/index
   assembler/index
   data-structures/index
   design/index

Architecture
~~~~~~~~~~~~

The :doc:`architecture/index` section describes how the virtual machine
works at a high level:

* The fetch-decode-execute cycle
* Memory layout and addressing
* Register file organization
* Condition code evaluation

Implementation
~~~~~~~~~~~~~~

The :doc:`implementation/index` section provides detailed explanations
of how specific operations work:

* How opcodes are decoded from instruction bytes
* How immediate values are encoded and decoded
* The syscall dispatch mechanism
* The literal pool for position-independent code

Assembler
~~~~~~~~~

The :doc:`assembler/index` section covers the vasm assembler:

* How the lexer tokenizes source code
* The parser architecture and handler functions
* The symbol table implementation
* Code generation and relocation handling

Data Structures
~~~~~~~~~~~~~~~

The :doc:`data-structures/index` section documents the key data
structures used:

* The hash table for label lookup
* The literal pool mechanism
* The relocation fixup system

Design
~~~~~~

The :doc:`design/index` section explains the rationale behind key
design decisions:

* Why certain encodings were chosen
* Trade-offs considered
* Alternatives that were rejected

Code Documentation
------------------

For API documentation of the C source code, see the
`Doxygen documentation <doxygen/index.html>`_ generated from the source.

For the actual source code, the main files are:

+---------------------------+----------------------------------------+
| File                      | Description                            |
+---------------------------+----------------------------------------+
| ``src/vm/core.c``         | Main VM execution loop                 |
| ``src/vm/instruction.c``  | Instruction implementations            |
| ``src/vm/memory.c``       | Memory management                      |
| ``src/asm/parser.c``      | Main parser implementation             |
| ``src/asm/lexer.c``       | Tokenization                           |
| ``src/asm/symbol_table.c``| Label lookup table                     |
+---------------------------+----------------------------------------+

Getting Started
---------------

If you want to contribute to varm:

1. Read the :doc:`architecture/index` to understand the big picture
2. Look at the source code structure in ``src/``
3. Check the GitHub issues for tasks
4. Read the :doc:`design/index` to understand the philosophy
5. Start with a small, well-defined task

For language implementers building on varm:

1. Read :doc:`architecture/memory-subsystem` to understand memory layout
2. Read :doc:`architecture/condition-codes` to understand flags
3. Review the :doc:`../user/reference/syscalls` for I/O operations
4. Study the literal pool mechanism in :doc:`implementation/literal-pool`
