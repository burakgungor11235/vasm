Literal Pool
============

.. warning::
   The literal pool implementation described here is specific to varm |version|.
   This internal data structure is NOT stable and may change in any release.

1. Overview
-----------

The literal pool is a specialized data structure for storing constants that
need to be loaded via PC-relative instructions. When an ARM assembly program
uses a literal load like:

.. code-block:: asm

   ldr r0, =0x12345678    ; Load constant 0x12345678
   ldr r1, =my_label      ; Load address of my_label

The assembler cannot always encode the constant directly in the instruction.
Instead, the constant is placed in a nearby "literal pool" and loaded via
a PC-relative load.

**Why is a literal pool needed?**

- **Forward references**: Labels defined later in the program
- **Large constants**: Values too big for immediate encoding
- **Address loading**: ``ldr rd, =label`` requires address resolution

The literal pool serves as a "holding area" for constants that cannot be
resolved until the final layout is known.

2. Data Structure
-----------------

The literal pool consists of two structures defined in ``src/literal.h``:

.. code-block:: c
   :linenos:

   #define LITERAL_POOL_INITIAL_CAPACITY 16

   typedef struct {
       uint32_t value;
       uint32_t offset;
   } literal_pool_entry_t;

   typedef struct {
       literal_pool_entry_t *entries;
       size_t count;
       size_t capacity;
   } literal_pool_t;

   typedef struct {
       uint32_t instr_addr;
       size_t pool_index;
   } literal_pool_ref_t;

**Field Descriptions:**

``literal_pool_entry_t`` represents a constant stored in the pool:

- ``value``: The 32-bit constant value to be stored
- ``offset``: Word offset from pool base address (filled during emission)

``literal_pool_t`` manages the pool:

- ``entries``: Dynamic array of literal entries
- ``count``: Current number of entries
- ``capacity``: Allocated capacity (grows on demand)

``literal_pool_ref_t`` tracks an instruction that references the pool:

- ``instr_addr``: Address of the LDR instruction needing fixup
- ``pool_index``: Index into the literal pool (filled after pool emission)

**Pool Entry Growth Strategy:**

The pool uses exponential growth with an initial capacity of 16:

.. code-block:: c

   if (pool->count >= pool->capacity) {
       size_t new_capacity = pool->capacity * 2;
       literal_pool_entry_t *new_entries = realloc(pool->entries,
                                                    new_capacity * 
                                                    sizeof(literal_pool_entry_t));
       pool->capacity = new_capacity;
   }

This ensures O(1) amortized insertion time.

3. Pool Management
------------------

3.1 Adding Entries
~~~~~~~~~~~~~~~~~~

.. code-block:: c
   :linenos:

   int literal_pool_add(literal_pool_t *pool, uint32_t value) {
       // Check if value already exists (deduplication)
       for (size_t i = 0; i < pool->count; i++) {
           if (pool->entries[i].value == value) {
               return i;  // Return existing index
           }
       }
       
       // Add new entry
       if (pool->count >= pool->capacity) {
           size_t new_cap = pool->capacity * 2;
           literal_pool_entry_t *new_entries = realloc(
               pool->entries, 
               new_cap * sizeof(literal_pool_entry_t)
           );
           if (!new_entries) return -1;
           pool->entries = new_entries;
           pool->capacity = new_cap;
       }
       
       pool->entries[pool->count].value = value;
       pool->entries[pool->count].offset = 0;  // Set during emission
       return pool->count++;
   }

The deduplication check is O(n) where n = pool size, but typical pools are
small (< 50 entries), making this negligible.

3.2 Emitting the Pool
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c
   :linenos:

   uint32_t literal_pool_emit(literal_pool_t *pool, 
                              uint32_t base_address,
                              FILE *out) {
       // Calculate offsets for each entry
       for (size_t i = 0; i < pool->count; i++) {
           pool->entries[i].offset = i;  // Word offset from base
       }
       
       // Emit each constant as a 4-byte word
       for (size_t i = 0; i < pool->count; i++) {
           uint32_t word = pool->entries[i].value;
           fwrite(&word, sizeof(word), 1, out);
       }
       
       return base_address + pool->count * 4;  // Return next available address
   }

4. Fixup Mechanism
------------------

The fixup mechanism connects LDR instructions to their pool entries. This
is a two-stage process:

**Stage 1: Record the reference**

When the assembler encounters ``ldr rd, =value``:

.. code-block:: c
   :linenos:

   int record_literal_ref(literal_pool_t *pool,
                          literal_pool_refs_t *refs,
                          uint32_t instr_addr,
                          uint32_t value) {
       int pool_idx = literal_pool_add(pool, value);
       if (pool_idx < 0) return -1;
       
       // Record reference for later fixup
       if (refs->count >= refs->capacity) {
           // Grow refs array...
       }
       
       refs->refs[refs->count].instr_addr = instr_addr;
       refs->refs[refs->count].pool_index = pool_idx;
       refs->count++;
       
       return 0;
   }

**Stage 2: Fix up the instruction**

After the pool is emitted, each reference is resolved:

.. code-block:: c
   :linenos:

   void fixup_literal_refs(literal_pool_t *pool,
                           literal_pool_refs_t *refs,
                           uint32_t pool_address) {
       for (size_t i = 0; i < refs->count; i++) {
           uint32_t instr_addr = refs->refs[i].instr_addr;
           size_t pool_idx = refs->refs[i].pool_index;
           uint32_t entry_offset = pool->entries[pool_idx].offset;
           
           // Calculate PC-relative offset
           // PC at instruction + 8 ( ARM pipeline ), pool is at pool_address
           uint32_t pc_relative = pool_address + entry_offset * 4 
                                - instr_addr - 8;
           
           // Write fixup to instruction at instr_addr
           write_fixup(instr_addr, pc_relative);
       }
   }

5. Diagram: Code → Pool → Fixup Flow
------------------------------------

**Before Fixup (assembly time):**

::

   Address   Instruction          Pool Entry     Pool Ref
   +--------+-------------------+--------------+---------+
   0x1000   ldr r0, =0x12345678                {value: 0x12345678, idx: 0}
   0x1004   ldr r1, =0xDEADBEEF                {value: 0xDEADBEEF, idx: 1}
   0x1008   add r0, r1
   ...
   0x2000   (pool not yet emitted)

   Refs:
   +--------+--------+
   | instr  | pool   |
   | addr   | idx    |
   +--------+--------+
   | 0x1000 |   0    |
   | 0x1004 |   1    |
   +--------+--------+

**After Pool Emission:**

::

   Address   Instruction          Pool Entry     Pool Ref
   +--------+-------------------+--------------+---------+
   0x1000   ldr r0, [pc, #N]                   {value: 0x12345678, offset: 0}
   0x1004   ldr r1, [pc, #M]                   {value: 0xDEADBEEF, offset: 1}
   0x1008   add r0, r1
   ...
   0x2000   0x12345678         ← Pool emitted here
   0x2004   0xDEADBEEF
   0x2008   (next section)

**After Fixup:**

::

   Address   Instruction          Disassembly
   +--------+-------------------+------------------------+
   0x1000   ldr r0, [pc, #2048]  ; pc = 0x1008, pool = 0x2000
   0x1004   ldr r1, [pc, #2044]  ; Offset = 0x2004 - 0x1004 - 8
   0x1008   add r0, r1
   ...
   0x2000   0x12345678           ; Constant data
   0x2004   0xDEADBEEF           ; Constant data

6. Complexity Analysis
----------------------

**Space Complexity:**

::

   O(n + m) where:
   - n = number of unique literal values (pool entries)
   - m = number of LDR instructions referencing literals (refs)
   
   Pool: n × 8 bytes (value + offset) = 8n bytes
   Refs: m × 8 bytes (instr_addr + pool_index) = 8m bytes

**Time Complexity:**

+----------------------+------------------+----------------------+
| Operation            | Time             | Notes                |
+======================+==================+======================+
| Add unique entry     | O(n)             | Deduplication scan   |
| Add duplicate entry  | O(1)             | Early exit on match  |
| Emit pool            | O(n)             | Sequential write     |
| Fixup all refs       | O(m)             | Sequential fixup     |
+----------------------+------------------+----------------------+

**Amortized Analysis:**

For the add operation with deduplication:

- Each unique entry: O(n) for scan, O(1) for insert
- Each duplicate: O(k) where k is position in pool (early exit)

With n unique entries and exponential growth, the total cost of all
grows is O(n) amortized.

**Deduplication Benefit:**

Without deduplication, each LDR would get its own pool entry:

- With dedup: O(n) pool entries where n = unique values
- Without dedup: O(m) pool entries where m = references

This saves memory and instruction bytes in the pool.

**Fixup Offset Calculation:**

The fixup operation computes:

::

   offset = (pool_base + entry_offset) - (instr_addr + 8)
   
   Example:
   - pool_base = 0x2000
   - entry_offset = 0 words = 0 bytes
   - instr_addr = 0x1000
   - offset = 0x2000 - 0x1008 = 0xFF8 = 4080 bytes
   
   Instruction: ldr r0, [pc, #4080]
