Internal Architecture Documentation
===================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   vm-architecture
   instruction-pipeline
   memory-subsystem
   register-file
   condition-codes

.. warning::
   **varm is not stable.** This documentation describes the current
   implementation and is subject to change without notice. The VM,
   instruction set, and APIs are experimental. Do not use for
   production workloads.

Overview
--------

This section provides detailed internal architecture documentation for
varm, a von Neumann-style virtual machine with a 32-bit instruction set.
The documentation is intended for developers who wish to:

- Understand how varm works internally
- Contribute to the varm project
- Build languages or tools on top of varm

.. important::
   All information in this section reflects the current implementation.
   Future versions may introduce breaking changes without warning.
   APIs, instruction encodings, and internal structures are not stable.

Conventions Used
----------------

- Register names: ``r0`` through ``r15`` (with ``sp``, ``lr``, ``pc`` aliases)
- Memory addresses: hexadecimal notation (e.g., ``0x10000``)
- Instructions: shown in hexadecimal with breakdown
- Complexity: Big-O notation for time/space analysis

Source Code References
----------------------

Code snippets in this documentation reference the actual varm source
code. The repository structure is as follows:

::

   varm/
   ├── src/
   │   ├── vm/              # VM core implementation
   │   │   ├── cpu.c        # CPU execution logic
   │   │   ├── decode.c     # Instruction decoding
   │   │   ├── execute.c    # Instruction execution
   │   │   └── pipeline.c   # Pipeline orchestration
   │   ├── memory/          # Memory subsystem
   │   │   ├── ram.c        # RAM implementation
   │   │   └── mmu.c        # Memory management unit
   │   ├── registers/       # Register file
   │   │   └── regfile.c    # Register operations
   │   └── common/          # Shared utilities
   │       └── flags.c      # Condition code handling

See :doc:`vm-architecture` to begin learning about the system.
