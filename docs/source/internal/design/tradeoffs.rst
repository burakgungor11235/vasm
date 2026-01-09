Tradeoffs
=========

.. warning::

   **varm is NOT stable.** The tradeoffs documented here reflect current
   implementation decisions. These may change as the system evolves.

1. Introduction
---------------

Every design choice involves tradeoffs. This document makes those
tradeoffs explicit, documenting the costs and benefits of key decisions
so developers understand the implicit choices made during design.

A tradeoff isn't a mistake or compromise—it's a deliberate decision to
favor one quality at the expense of another. Understanding tradeoffs
helps developers make informed changes and recognize when a decision
should be revisited.

2. Performance Tradeoffs
------------------------

2.1 Time vs Space
~~~~~~~~~~~~~~~~~

varm consistently favors simplicity over optimal performance:

* **Hash table**: O(1) average case but with linear probing clustering
* **Instruction decode**: Fixed position but requires 3 shifts and 3 masks
* **Literal pool**: Position-independent but requires extra memory

These choices prioritize:

1. **Understandability** over raw speed
2. **Cache efficiency** over theoretical throughput
3. **Predictable performance** over best-case optimization

2.2 Simplicity vs Optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The instruction decoder could be optimized with lookup tables:

.. code-block:: c

   // Simpler current approach
   opcode = (instr >> 24) & 0xFF;
   cond = (instr >> 20) & 0xF;
   rd = (instr >> 12) & 0xF;

   // Faster approach with lookup tables
   opcode = opcode_table[instr >> 24];

The simpler approach is:
* Easier to read and understand
* Easier to modify and extend
* Fast enough for educational use

The optimized approach would require:
* Pre-computed tables (build-time code generation)
* Complex validation logic
* More memory for lookup tables

For varm's purpose, simplicity wins.

3. Specific Tradeoffs
---------------------

3.1 Hash Table Size (256 Slots)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: Fixed 256-slot hash table for labels.

**Pros**:

* **Fits in L1 cache**: 256 × 72 bytes = 18KB, fits in most L1 caches (32-48KB)
* **Very fast**: Average 1.5 probes at 50% load
* **Predictable memory**: Fixed allocation, no resize overhead
* **Fast modulo**: Power of 2 enables bitmask indexing

**Cons**:

* **Wasted memory if few labels**: Empty slots still consume memory
* **Fixed capacity**: Cannot grow beyond 128 symbols at 50% load
* **Load factor ceiling**: Must reject assembly if too many labels

**Decision rationale**: Cache efficiency is prioritized for assembly
speed. Most assembly files have far fewer than 128 labels, so the
memory overhead is acceptable for the performance benefit.

**Quantified**: At 50% load, average successful lookup requires 1.5
probes, unsuccessful requires 2 probes. At 25% load, these drop to
1.17 and 1.33 probes respectively.

3.2 Linear Probing vs Chaining
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: Open addressing with linear probing for collision resolution.

**Pros**:

* **Cache locality**: Probe sequences access contiguous memory
* **Simpler implementation**: No linked list management
* **No allocation overhead**: All entries in one array
* **Better for small tables**: Clustering is minimal at low load factors

**Cons**:

* **Primary clustering**: Keys with similar hashes cluster together
* **Deletion complexity**: Requires tombstones (not needed for varm)
* **Worst-case O(n)**: Pathological hash functions degrade to linear search

**Decision rationale**: For typical assembly files with 10-50 labels,
clustering is minimal. The cache locality benefit outweighs the
theoretical worst case. Varm doesn't need deletion (symbol table is
built once), so tombstones aren't required.

**Quantified**: At 50% load factor, average probe count is 1.5 for
successful lookup. At 75% load, it rises to 2.5 probes.

3.3 Fixed Instruction Length
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: All instructions are exactly 32 bits.

**Pros**:

* **Simple fetch**: PC always increments by 4
* **Simple decode**: Opcode position is always bits 31-24
* **Word alignment**: Memory access is always aligned
* **Pipeline friendly**: No variable-length decode stage

**Cons**:

* **Wasted space**: Small immediates occupy full instruction
* **Limited immediate range**: 12-bit operand field limits immediate values
* **No compression**: No Thumb-like 16-bit instruction variant

**Decision rationale**: For educational purposes, decode simplicity
outweighs code density. The 12-bit immediate (with 8-bit value and
4-bit rotate) is sufficient for most educational programs.

**Quantified**: Maximum immediate value with rotation is 32 bits, but
requiring power-of-2 rotation. For example, 0xFF can be rotated to
any even rotation, giving 0xFF, 0xFF00, 0xFF0000, 0xFF000000.

3.4 No Pipelining
~~~~~~~~~~~~~~~~~

**Decision**: The basic implementation executes one instruction at a time.

**Pros**:

* **Simple to understand**: Fetch-decode-execute is obvious
* **Easy to debug**: Single-step is straightforward
* **No hazards**: No structural, data, or control hazards to manage
* **Clear state transitions**: State is always consistent

**Cons**:

* **Slower execution**: Each instruction must complete before next starts
* **Underutilized CPU**: No instruction-level parallelism
* **Longer execution time**: More cycles per program

**Decision rationale**: Educational clarity is paramount. Understanding
pipelining requires understanding hazards, forwarding, and branch
prediction—topics beyond varm's scope. The performance cost is acceptable
for a pedagogical tool.

**Quantified**: A pipelined implementation would require 3-5x more code
and significantly more complex state management.

3.5 Build-Time Code Generation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Decision**: Opcode and instruction encoding tables are generated at
build time rather than parsed from configuration at runtime.

**Pros**:

* **Type safety**: Generated code is compile-time checked
* **No runtime parsing**: No startup cost for reading configuration
* **Optimizable**: Compiler can optimize generated tables
* **Self-contained**: No external data files needed

**Cons**:

* **Longer build time**: Generation step adds to compile time
* **Complexified build**: Meson must handle code generation
* **Harder to modify**: Changing encodings requires rebuild

**Decision rationale**: The build time impact is minimal (milliseconds),
and the type safety benefits outweigh the flexibility cost. For an
educational project, compile-time correctness is more important than
runtime configurability.

4. Quantified Tradeoffs
-----------------------

4.1 Hash Table Performance
~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+----------------+----------------+----------------+
| Load %   | Successful     | Unsuccessful   | Memory         |
+==========+================+================+================+
| 25%      | 1.17 probes    | 1.33 probes    | 18KB           |
| 50%      | 1.50 probes    | 2.00 probes    | 18KB           |
| 75%      | 2.50 probes    | 4.00 probes    | 18KB           |
+----------+----------------+----------------+----------------+

Average probes formula for linear probing:

* Successful lookup: (1 + 1/(1-α)) / 2
* Unsuccessful lookup: 1/(1-α)

Where α = load factor (count/size).

4.2 Cache Considerations
~~~~~~~~~~~~~~~~~~~~~~~~

+---------------------------+-----------+------------------------+
| Component                 | Size      | Cache Level            |
+===========================+===========+========================+
| Hash table (256 slots)    | 18 KB     | L1 (typically 32-48KB) |
| Instruction buffer        | 16 KB     | L1                     |
| Register file             | 64 bytes  | L1 (in CPU)            |
| Literal pool (typical)    | 256 bytes | L1                     |
+---------------------------+-----------+------------------------+

4.3 Instruction Decode Cost
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+---------------------------+----------------+---------------------+
| Operation                 | Operations     | Time Complexity     |
+===========================+================+====================-+
| Extract opcode            | 1 shift, 1 mask| O(1)                |
| Extract condition         | 1 shift, 1 mask| O(1)                |
| Extract registers         | 2 shifts, 2 masks| O(1)               |
| Determine operand type    | 1 shift, 1 mask| O(1)                |
| **Total per instruction** | **3 shifts, 3 masks**| **O(1)**        |
+---------------------------+----------------+---------------------+

4.4 Memory Tradeoffs
~~~~~~~~~~~~~~~~~~~~

+---------------------------+-----------+------------------------+
| Feature                   | Memory    | Tradeoff               |
+===========================+===========+========================+
| Fixed instruction size    | 4 bytes   | Simplicity vs density  |
| PC-relative literal pool  | +4 bytes/ | Position-independence  |
|                           | reference | vs memory              |
| Hash table (256 slots)    | 18 KB     | Lookup speed vs memory |
| Unified address space     | 64 KB     | Simplicity vs isolation|
+---------------------------+-----------+------------------------+

5. Summary
----------

The tradeoffs in varm consistently favor:

1. **Simplicity** over optimization
2. **Cache efficiency** over memory savings
3. **Educational clarity** over performance
4. **Predictable behavior** over best-case scenarios

These choices make varm effective for its educational purpose. When
considering changes, evaluate whether they align with these priorities.
If you need maximum performance, varm may not be the right tool—but
for learning about virtual machines, these tradeoffs are appropriate.
