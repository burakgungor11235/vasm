Assembler Internals
===================

.. warning::

   **varm is NOT stable.** This documentation describes the current implementation
   as it exists NOW. APIs, data structures, and algorithms may change without notice.
   Do not rely on any internal details for external tools or scripts.

Overview
--------

The vasm assembler converts human-readable assembly source code into binary
instructions for the varm virtual machine. The assembler is implemented as a
traditional pipeline:

.. mermaid::
   :caption: Assembler Pipeline

   flowchart LR
       SOURCE["Source\n.s file"] --> LEX["Lexer\ntokenize()"]
       LEX --> TOKENS["Tokens"]
       TOKENS --> PARSER["Parser\nparse()"]
       PARSER --> SYM["Symbol Table"]
       PARSER --> CODE["Machine Code"]
       CODE --> OUTPUT[".varm\nFile"]

This section documents the internal architecture of the assembler components:

.. toctree::
   :maxdepth: 2

   lexer-design
   parser-architecture
   symbol-table
   code-generation
   lookup-tables

Architecture Overview
---------------------

The assembler consists of several modular components, each responsible for
a specific phase of compilation:

1. **Lexer** (``src/asm/lexer.c``): Converts raw source text into tokens
2. **Symbol Table** (``src/asm/symbol_table.c``): Stores and resolves labels
3. **Parser** (``src/asm/parser.c``): Converts tokens into encoded instructions
4. **Lookup Tables** (``src/asm/generated/*``): Fast instruction/condition lookup

Instruction Format
------------------

All varm instructions are encoded as 32-bit values in little-endian byte order:

.. mermaid::
   :caption: Instruction Bit Layout

   flowchart LR
       direction TB
       OPCODE["Opcode\n31-24\n8 bits"] --> COND["Cond\n23-20\n4 bits"]
       COND --> RN["Rn\n19-16\n4 bits"]
       RN --> RD["Rd\n15-12\n4 bits"]
       RD --> OPERAND["Operand2\n11-0\n12 bits"]

Byte layout in memory (little-endian at address A):

::

    A+0:  operand[7:0]
    A+1:  operand[11:8] | rd[3:0]
    A+2:  rd[7:4] | rn[3:0]
    A+3:  cond[3:0] | opcode[7:0]

Component Relationships
-----------------------

.. mermaid::
   :caption: Assembler Architecture

   graph TB
       subgraph ASSEMBLER["Assembler"]
           LEX["Lexer\ntokenize()"] --> TOKENS["Token Stream"]
           TOKENS --> PAR["Parser\nparse_directive()\nparse_alu()\nparse_move()\nparse_branch()\nparse_load_store()"]
           PAR --> SYM["Symbol Table\nFNV-1a Hash\nLinear Probing"]
           PAR --> CODE["Machine Code\n.text + .data"]
           SYM -.->|"Lookup"| PAR
       end

       SOURCE["Source .s"] --> LEX
       CODE --> OUTPUT[".varm File"]

Key Files
---------

==================  ==========================================================
File                Description
==================  ==========================================================
src/asm/lexer.c     Tokenization implementation
src/asm/parser.c    Main parsing and code generation
src/asm/symbol_table.c  Hash table for label lookup
src/asm/directives.c    Assembly directive handlers
src/asm/error.c     Error collection and reporting
include/assembler.h     Public assembler API and types
include/opcode.h    Instruction encoding definitions
==================  ==========================================================

.. warning::

   The maximum instruction count is 4096 (``MAX_INSTRUCTIONS``). Programs
   exceeding this limit will not assemble correctly.
