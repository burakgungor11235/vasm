Design Rationale
================

.. warning::

   **varm is NOT stable.** The design decisions documented here reflect
   the current implementation only. These choices may be reconsidered
   and changed in future versions.

1. Introduction
---------------

This document explains the reasoning behind key design decisions in varm.
Understanding why decisions were made helps developers comprehend the
system's philosophy, recognize constraints, and make informed changes.

1.1 Why This Documentation Exists
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Every non-trivial system involves design decisions that trade one benefit
for another. This documentation makes those trade-offs explicit so that:

* Contributors understand the guiding principles
* Changes can be evaluated against the design philosophy
* The rationale is preserved for future maintainers
* Educational value is maximized for students

1.2 Design Goals Stated Upfront
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before diving into specific decisions, here are the explicit goals that
guided the design, in priority order:

1. **Educational value**: The system should be easy to understand and study
2. **Small implementation**: The codebase should be comprehensible in one sitting
3. **ARM-like**: Familiar instruction set semantics for industry relevance
4. **Fast compile times**: Quick iteration for development and testing

These goals sometimes conflict. When they do, educational value and
implementation simplicity take precedence over performance optimization.

2. Design Goals
---------------

2.1 Educational Value (Simple to Understand)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The primary goal is educational clarity. This means:

* **Straightforward algorithms**: Prefer O(n) and O(1) algorithms that
  are easy to trace over complex but theoretically faster approaches
* **Minimal magic**: Avoid clever tricks that obscure behavior
* **Clear naming**: Variable and function names should describe intent
* **Single responsibility**: Each component does one thing well

The target audience is developers learning about virtual machines,
compilers, or computer architecture—not expert systems programmers.

2.2 Small Implementation (Easy to Study)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

varm aims to be study-able, meaning:

* **Single-file components**: Related logic grouped together
* **No external dependencies**: Self-contained implementation
* **Minimal build configuration**: Simple meson.build
* **Under 5000 lines of code**: The entire VM and assembler should fit
  in a programmer's mental model

This constraint shapes technology choices. For example, the hash table
is hand-written rather than using a library, because seeing the
implementation teaches more than hiding it behind an abstraction.

2.3 ARM-Like (Industry Relevance)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

ARM is one of the most widely deployed ISAs in the world. By mimicking
ARM semantics, varm provides:

* **Transferable knowledge**: Skills learned transfer to real ARM development
* **Familiar concepts**: Condition codes, register naming, instruction syntax
* **Practical examples**: Students can compare varm to real ARM code
* **Future extensibility**: Real ARM features can be added incrementally

This doesn't mean varm is ARM-compatible—just ARM-inspired. The encoding
is simplified for educational purposes.

2.4 Fast Compile Times
~~~~~~~~~~~~~~~~~~~~~~

Quick iteration cycles improve development velocity and student
productivity. This means:

* **No heavy preprocessing**: Direct compilation, no code generation steps
* **Simple dependencies**: Standard C library only
* **Incremental builds**: Meson supports this well
* **Fast test execution**: Tests complete in milliseconds

3. Key Decisions Explained
--------------------------

3.1 32-bit Instruction Format
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: All instructions are fixed 32 bits (4 bytes) in length.

**Why 32 bits?**

The 32-bit instruction width was chosen for several reasons:

* **Sufficient opcode space**: 8 bits for opcode provides 256 instruction
  types, more than adequate for the educational instruction set
* **Simple decode**: Fixed position fields mean no complex parsing
* **Word alignment**: Instructions fall on word boundaries, simplifying
  memory access
* **Balance**: Not so narrow that we lack encoding space, not so wide
  that it wastes memory

**Why fixed length?**

Fixed-length instructions provide:

* **Predictable fetch**: PC always increments by 4, no calculation needed
* **Simple decode**: Opcode position is known (bits 31-24)
* **Pipeline simplicity**: No variable-length decoding stage
* **Cache efficiency**: Instructions pack predictably in memory

**Alternative considered**: Variable-length encoding (like x86) would
provide better code density but adds significant decode complexity.
For educational purposes, the simplicity of fixed length outweighs
the memory savings.

3.2 Little-Endian Byte Order
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: All multi-byte values are stored in little-endian format.

**Why little-endian?**

* **x86 compatibility**: Most common desktop architecture uses little-endian
* **ARM convention**: ARM processors use little-endian by default
* **Intuitive incrementing**: When iterating through bytes of a word,
  the least significant byte changes first

**Example**: The value 0x12345678 stored at address 0x1000:

::

   Address  Content
   0x1000   0x78  (least significant byte)
   0x1001   0x56
   0x1002   0x34
   0x1003   0x12  (most significant byte)

**Could have been big-endian**: Big-endian has advantages for string
comparison (alphabetical order matches memory order), but little-endian
is more common in modern systems.

3.3 FNV-1a Hash Function
~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: Labels are hashed using the FNV-1a algorithm.

**Why FNV-1a?**

* **Speed**: Minimal operations per character (XOR then multiply)
* **Good distribution**: Proven avalanche effect, minimal collisions
* **Simple implementation**: No lookup tables, few lines of code
* **Endian-neutral**: Works correctly regardless of platform byte order

**Implementation** (from symbol_table.h):

.. code-block:: c

   u32 fnv1a_hash(const char* str) {
       u32 hash = 0x811c9dc5;  // FNV offset basis
       while (*str) {
           hash ^= (u8)*str++;
           hash *= 0x01000193;  // FNV prime
       }
       return hash;
   }

**Alternatives considered and rejected:**

* **CRC32**: Good distribution but slower (requires table or bit-by-bit)
* **SHA-256**: Cryptographically secure but overkill and slow
* **DJB2**: Fast but slightly worse distribution than FNV-1a
* **MurmurHash3**: Better distribution but more complex implementation

For label hashing (short strings, no security requirements), FNV-1a
provides the best balance of simplicity and quality.

3.4 Linear Probing for Collision Resolution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: When hash collisions occur, the symbol table uses linear
probing to find the next available slot.

**Why linear probing?**

* **Cache-friendly**: Sequential memory access patterns
* **Simpler than chaining**: No linked list management, no extra allocations
* **Good locality**: Probe sequences access contiguous memory
* **No tombstones needed**: We don't delete entries from the symbol table

**How it works**:

.. code-block:: c

   // Pseudo-code for lookup with linear probing
   idx = hash(name) & mask;

   while (entries[idx].defined) {
       if (strcmp(entries[idx].name, name) == 0) {
           return entries[idx].address;  // Found
       }
       idx = (idx + 1) & mask;  // Probe next slot
   }

   return NOT_FOUND;  // Empty slot means not present

**Alternative**: Separate chaining (like Java's HashMap) uses linked
lists for collisions. While theoretically better at high load factors,
chaining wastes memory on next pointers and hurts cache locality.
For the typical load factor of ~50%, linear probing performs better.

3.5 256-Slot Hash Table
~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: The symbol table uses exactly 256 slots.

**Why power of 2?**

* **Fast modulo**: `index & (size - 1)` instead of `index % size`
* **Simple mask**: The table mask is always `size - 1`
* **Predictable behavior**: Hash directly maps to slot index

**Why 256 specifically?**

* **L1 cache fit**: 256 entries × ~72 bytes = ~18KB, fits in L1 cache
* **Adequate capacity**: 128 symbols at 50% load is plenty for assembly
* **Simple constants**: 256 = 2^8, easy to work with

**Why not 128 or 512?**

* **128**: Would be tighter in cache but risk higher load factor
* **512**: Would provide more room but might exceed L1 cache on some systems

The 256 size was chosen as the sweet spot for typical educational
programs while remaining cache-friendly.

3.6 PC-Relative Literal Loading
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: The `ldr rd, =label` pseudo-instruction uses PC-relative
addressing to load label addresses.

**Why PC-relative?**

* **Position-independent**: Code works regardless of load address
* **No relocation**: The offset is calculated at assembly time
* **Simple fixup**: The assembler computes the offset once

**How it works**:

1. Assembler adds label value to literal pool at end of text section
2. Emits LDR instruction with PC-relative offset to pool entry
3. Pool entry contains the 32-bit label address
4. At runtime, LDR loads the address from the pool

**Example**:

.. code-block:: asm

   .data
   msg: .byte 'H', 'e', 'l', 'l', 'o'

   .text
   ldr r1, =msg    ; Load address of msg into r1

**Generated instructions**:

::

   0x20: LDR r1, [pc, #20]    ; Load from literal pool at 0x34
   0x28: ...                  ; (next instruction)
   0x2C: ...                  ; (next instruction)
   0x30: literal pool entry   ; 0x00010000 (msg address)

**Alternative**: Absolute addressing would require runtime relocation,
making the bytecode non-portable. PC-relative is the right choice for
position-independent code.

4. What Was Not Considered
--------------------------

varm intentionally omits several features found in real processors:

4.1 Out-of-Order Execution
~~~~~~~~~~~~~~~~~~~~~~~~~~

Not considered because:

* **Complexity**: Would multiply code complexity by 10x
* **Educational clarity**: Harder to trace execution flow
* **Single-issue**: We execute one instruction at a time anyway

4.2 Speculative Execution
~~~~~~~~~~~~~~~~~~~~~~~~~

Not considered because:

* **Complexity**: Branch prediction state would obscure logic
* **Security**: Spectre/Meltdown-type vulnerabilities are beyond scope
* **Single-threaded**: No benefit for single instruction stream

4.3 Virtual Memory
~~~~~~~~~~~~~~~~~~

Not considered because:

* **Complexity**: Page tables, TLB would add thousands of lines
* **Flat address space**: 64KB is small enough to fit in physical memory
* **Educational focus**: Memory management is a separate topic

4.4 Memory Protection
~~~~~~~~~~~~~~~~~~~~~

Not considered because:

* **No user/kernel split**: Single-privilege environment
* **Simple trust model**: Assembled bytecode is trusted
* **Educational scope**: Protection is a systems programming topic

5. Sources of Inspiration
-------------------------

5.1 ARM Thumb-2
~~~~~~~~~~~~~~~

Thumb-2 introduced 16-bit and 32-bit mixed-length instructions. varm
borrows:

* **Condition codes**: Same semantics as ARM (EQ, NE, GT, etc.)
* **Register naming**: r0-r12, sp, lr, pc
* **Instruction syntax**: `mov r0, #42` style

5.2 RISC-V
~~~~~~~~~~

RISC-V emphasizes simplicity and modularity. varm borrows:

* **Load-store architecture**: Separate memory and ALU instructions
* **Fixed opcode positions**: Predictable decode
* **Minimal encoding**: No unnecessary instruction fields

5.3 x86
~~~~~~~

Despite varm being RISC-like, x86 influenced:

* **Little-endian**: Default byte order on x86 and ARM
* **Syscall convention**: R7 for syscall number, inspired by ARM EABI
* **Segmented memory model**: Text/data separation

6. Summary
----------

Every design decision in varm reflects its educational goals. The
32-bit fixed instruction format prioritizes decode simplicity over
code density. The FNV-1a hash function provides good-enough performance
with minimal implementation complexity. Linear probing with 256 slots
balances cache efficiency against table size.

Understanding these trade-offs helps developers make informed changes.
When extending varm, ask: "Does this align with our goals of simplicity,
educational value, and ARM-likeness?"

If the answer is no, reconsider the change or accept that you're
diverging from the core design philosophy.
