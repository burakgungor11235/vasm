Alternatives
=============

.. warning::

   **varm is NOT stable.** The alternatives documented here were
   considered during initial design but rejected for specific reasons.
   These reasons may change as requirements evolve.

1. Introduction
---------------

This document describes design alternatives that were considered but
rejected. Understanding why alternatives were not chosen is as important
as understanding why the chosen approach was selected.

Each alternative is presented with:
* Why it was considered
* Why it was ultimately rejected
* The tradeoffs involved

2. Alternative Instruction Formats
----------------------------------

2.1 16-bit Thumb-Like Encoding
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What was considered**: Adding a Thumb-like 16-bit instruction mode
for improved code density, similar to ARM's Thumb instruction set.

**Why considered**:

* **Smaller code size**: 16-bit instructions would halve text section size
* **Industry relevance**: Thumb is widely used in ARM systems
* **Educational value**: Comparing 16-bit vs 32-bit modes would be instructive

**Why rejected**:

* **Complexity**: Decode becomes conditional on mode bit
* **Opcode space**: 16 bits provides only 256 opcodes, less than 32-bit
* **Limited operands**: Not enough bits for register + immediate encoding
* **Scope creep**: Beyond current educational goals

**Tradeoff analysis**:

+---------------------------+------------------+---------------------+
| Factor                    | 32-bit (chosen)  | 16-bit (rejected)   |
+===========================+==================+=====================+
| Decode complexity         | Low              | High (mode-switch)  |
| Code density              | 4 bytes/instr    | 2 bytes/instr       |
| Opcode space              | 256 opcodes      | 256 opcodes         |
| Register encoding         | 4 bits (16 reg)  | 3 bits (8 reg)      |
| Immediate size            | 12 bits          | 5-8 bits            |
| Educational clarity       | High             | Medium              |
+---------------------------+------------------+---------------------+

2.2 Variable-Length Encoding
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**What was considered**: Variable-length instructions like x86, where
instructions range from 1 to 15 bytes.

**Why considered**:

* **Optimal density**: Short instructions for common cases
* **Real-world example**: x86 uses this approach
* **Opcode flexibility**: More opcode space with escape sequences

**Why rejected**:

* **Complex fetch**: Don't know how many bytes to read until decoding
* **Complex decode**: Prefix bytes, opcode extensions, ModR/M bytes
* **Pipeline complications**: Variable instruction boundaries
* **Implementation size**: Would add 1000+ lines of code

**Tradeoff analysis**:

+---------------------------+------------------+---------------------+
| Factor                    | Fixed (chosen)   | Variable (rejected) |
+===========================+==================+=====================+
| Fetch complexity          | O(1)             | O(depth)            |
| Decode complexity         | O(1)             | O(length)           |
| Code density              | Lower            | Higher              |
| Implementation size       | ~2000 lines      | ~4000+ lines        |
| Educational clarity       | High             | Low                 |
+---------------------------+------------------+---------------------+

2.3 64-bit Instructions
~~~~~~~~~~~~~~~~~~~~~~~

**What was considered**: Using 64-bit instructions for more opcode
space and larger immediate values.

**Why considered**:

* **More opcode space**: 16 bits for opcode instead of 8
* **Larger immediates**: Could encode 32-bit constants directly
* **Future-proof**: Aligns with 64-bit architecture trends

**Why rejected**:

* **Memory waste**: Doubles text section size
* **Over-engineered**: Complexity not justified for educational use
* **Larger registers**: Would require 64-bit register file
* **Misalignment**: Not a learning goal

3. Alternative Hash Functions
-----------------------------

3.1 CRC32
~~~~~~~~~

**What was considered**: Using CRC32 for label hashing.

**Why considered**:

* **Good distribution**: Designed for error detection
* **Hardware support**: Many systems have CRC instructions
* **Well-studied**: Collision properties well understood

**Why rejected**:

* **Slower software implementation**: Bit-by-bit processing
* **Requires lookup table**: 1KB table for byte-wise CRC
* **Overkill for labels**: No security or error detection requirement
* **Endian-dependent**: Result varies by platform endianness

**Performance comparison** (software implementation):

+---------------------------+------------------+---------------------+
| Operation                 | FNV-1a (chosen)  | CRC32 (rejected)    |
+===========================+==================+====================-+
| Per-character ops         | 2 (xor, mul)     | 8-16 (shift, xor)   |
| Lookup table              | None             | 1KB                 |
| Cache-friendly            | High             | Low (table thrash)  |
| Collision quality         | Good             | Good                |
+---------------------------+------------------+---------------------+

3.2 MurmurHash3
~~~~~~~~~~~~~~~

**What was considered**: Using MurmurHash3 for label hashing.

**Why considered**:

* **Excellent distribution**: Very low collision rate
* **Avalanche effect**: Small changes cause large hash differences
* **Speed**: Well-optimized implementation available

**Why rejected**:

* **More complex**: Multiple mixing passes, endian handling
* **Code size**: Implementation is ~50 lines vs 10 for FNV-1a
* **Overkill**: Labels are short, distribution quality less critical
* **Little-endian assumption**: Requires special handling for other platforms

3.3 SHA-256
~~~~~~~~~~~

**What was considered**: Using SHA-256 for cryptographic label hashing.

**Why considered**:

* **Collision-resistant**: Theoretically impossible to find collisions
* **Security**: Would prevent hash-based attacks (not a real threat)
* **Industry standard**: Widely used and trusted

**Why rejected**:

* **Extremely slow**: Orders of magnitude slower than FNV-1a
* **Overkill**: No security requirements for label lookup
* **Code complexity**: Full implementation is ~200 lines
* **Memory**: 64-word working array required

**Speed comparison** (approximate, per KB of input):

+---------------------------+------------------+---------------------+
| Hash Function             | Cycles/byte      | Relative speed      |
+===========================+==================+====================-+
| FNV-1a (chosen)           | ~5               | 1x (baseline)       |
| CRC32                     | ~10              | 0.5x                |
| MurmurHash3               | ~3               | 1.7x                |
| SHA-256                   | ~30              | 0.17x               |
+---------------------------+------------------+---------------------+

4. Alternative Hash Table Sizes
-------------------------------

4.1 128 Slots
~~~~~~~~~~~~~

**What was considered**: Using a 128-slot hash table instead of 256.

**Why considered**:

* **Less memory**: 9KB vs 18KB
* **Still power of 2**: Fast modulo with bitmask
* **Adequate for most programs**: 64 symbols at 50% load

**Why rejected**:

* **Higher load factor**: Reaches 50% load with only 64 symbols
* **More collisions**: Average probe count increases
* **Limited headroom**: Tight constraint for larger programs
* **L1 cache is large enough**: 256 slots fit easily

4.2 512 Slots
~~~~~~~~~~~~~

**What was considered**: Using a 512-slot hash table for more capacity.

**Why considered**:

* **Lower load factor**: 256 symbols at 50% load
* **Fewer collisions**: Better average probe count
* **More headroom**: Room for larger programs

**Why rejected**:

* **May exceed L1 cache**: 36KB might not fit all L1 caches
* **Memory waste**: Most programs use far fewer labels
* **Diminishing returns**: 256 slots already provides good performance
* **Fixed allocation**: Larger table means more waste for small programs

4.3 1024 Slots
~~~~~~~~~~~~~~

**What was considered**: Using a 1024-slot hash table.

**Why considered**:

* **Minimal collisions**: Almost never probe more than once
* **Maximum capacity**: 512 symbols at 50% load

**Why rejected**:

* **L1 cache miss**: 72KB definitely exceeds L1 cache
* **Memory waste**: Significant unused space for typical programs
* **No benefit**: 256 slots already provides <2 probes average

**Size comparison**:

+---------------------------+-----------+-----------+-----------+
| Table Size                | Memory    | L1 Fit?   | Load (50%)|
+===========================+===========+===========+===========+
| 128 slots                 | 9 KB      | Yes       | 64 sym    |
| 256 slots (chosen)        | 18 KB     | Yes       | 128 sym   |
| 512 slots                 | 36 KB     | Maybe     | 256 sym   |
| 1024 slots                | 72 KB     | No        | 512 sym   |
+---------------------------+-----------+-----------+-----------+

5. Alternative Lookup Mechanisms
-------------------------------

5.1 Linear Search
~~~~~~~~~~~~~~~~~

**What was considered**: Using simple linear search through an array
of labels.

**Why considered**:

* **Extremely simple**: No hash function, no collision handling
* **No resize logic**: Fixed array, no rehashing
* **Good for small n**: 5 labels in 5 comparisons is fine

**Why rejected**:

* **O(n) lookup**: Too slow for large programs
* **Worst case**: Every lookup scans entire array
* **Poor scaling**: Performance degrades linearly with label count

**Performance comparison** (label count vs operations):

+---------------------------+------------------+---------------------+
| Method                    | 10 labels        | 100 labels          |
+===========================+==================+====================-+
| Linear search             | 5 ops avg        | 50 ops avg          |
| Hash table (chosen)       | 1.2 ops avg      | 1.5 ops avg         |
| Balanced tree             | 3 ops avg        | 7 ops avg           |
+---------------------------+------------------+---------------------+

5.2 Balanced Tree
~~~~~~~~~~~~~~~~~

**What was considered**: Using a balanced binary search tree (AVL or
red-black tree) for label storage.

**Why considered**:

* **O(log n) lookup**: Better than linear, worse than hash
* **Ordered traversal**: Can iterate in sorted order
* **Predictable performance**: Worst case O(log n), not O(n)

**Why rejected**:

* **More complex**: Tree rotations, balancing logic
* **Worse cache locality**: Pointer chasing through tree nodes
* **Higher constant factors**: More operations per lookup
* **No deletion needed**: varm only inserts, never deletes

5.3 Perfect Hashing
~~~~~~~~~~~~~~~~~~~

**What was considered**: Using perfect hashing (e.g., CHM hash) for
collision-free lookup.

**Why considered**:

* **No collisions**: Guaranteed O(1) lookup
* **Optimal space**: Minimal table size for known set
* **Academic interest**: Interesting algorithm

**Why rejected**:

* **Two-phase build**: Requires analyzing all labels first
* **Complex implementation**: Requires sophisticated algorithms
* **Static only**: Can't handle dynamic label insertion
* **Overkill**: Standard hashing is already fast enough

6. What We Might Change
-----------------------

Design decisions are not permanent. Here are circumstances that might
lead to reconsidering current choices:

6.1 If We Needed More Labels
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Current**: 256-slot table, 128 symbols at 50% load

**If we needed 500+ symbols**: We'd consider:

* **Resizable hash table**: Grow when load factor exceeds threshold
* **Tiered symbol tables**: Separate tables for different scopes
* **Different collision resolution**: Robin Hood hashing for better
  worst-case performance

6.2 If We Needed More Speed
~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Current**: ~1.5 probes per lookup at 50% load

**If we needed faster assembly**: We'd consider:

* **SIMD operations**: Process multiple comparisons simultaneously
* **Perfect hashing**: Eliminate probes entirely
* **Prefetching**: Fetch next probe location in advance
* **Cache blocking**: Process labels in cache-friendly chunks

6.3 If We Needed Better Code Density
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Current**: 4 bytes per instruction

**If we needed smaller binaries**: We'd consider:

* **Thumb mode**: Add 16-bit instruction encoding
* **LZW compression**: Compress text section in .varm file
* **Common subexpression elimination**: Reuse identical instruction
  sequences

7. Conclusion
-------------

varm's design choices reflect deliberate tradeoffs between competing
concerns: simplicity vs performance, memory vs speed, current needs
vs future extensibility.

The current choices are reasonable for an educational ARM-like virtual
machine. They prioritize understandability and implementation clarity
over raw performance or code density.

Future requirements might justify revisiting these decisions. The key
is understanding the tradeoffs involved and making changes deliberately,
not by accident or oversight.

When considering changes:

1. Understand the current tradeoff rationale
2. Evaluate alternatives against the same criteria
3. Consider how changes affect the educational mission
4. Document any new tradeoffs explicitly

The goal is not to create the "best" system in any absolute sense, but
to create a system that serves its educational purpose well while
remaining comprehensible to its target audience.
