Symbol Table
============

.. warning::
   The symbol table implementation described here is specific to varm |version|.
   This internal data structure is NOT stable and may change in any release.

1. Overview
-----------

The symbol table is a fundamental data structure that stores mappings from
label names to their corresponding memory addresses. During assembly, labels
are encountered in two contexts:

- **Definition**: A label appears at a specific address (e.g., ``.L1:`` at address 0x1000)
- **Reference**: A label is referenced before it is defined (forward reference)

The symbol table enables the assembler to resolve both backward and forward
references by maintaining a centralized mapping of all known labels.

**Why a hash table?**

The symbol table uses a hash table because the primary operations are:

- **Insert**: Add a new label → address mapping (O(1) average case)
- **Lookup**: Find the address for a given label (O(1) average case)

These operations dominate the assembler's runtime, making a hash table the
optimal choice. A balanced tree would give O(log n) operations, and a linear
search would give O(n) operations—neither is acceptable for an assembler.

2. Data Structure Definition
----------------------------

The symbol table consists of two core structures defined in ``src/symbol.h``:

.. code-block:: c
   :linenos:

   #define SYMBOL_NAME_MAX 64

   typedef struct {
       char name[SYMBOL_NAME_MAX];
       uint32_t address;
       int defined;
   } symbol_entry_t;

   typedef struct {
       symbol_entry_t *entries;
       size_t size;
       size_t mask;
       size_t count;
   } symbol_table_t;

**Field Descriptions:**

``symbol_entry_t`` represents a single label mapping:

- ``name``: Fixed-size buffer for label (64 bytes, null-terminated)
- ``address``: 32-bit word address (ARM instructions are 4-byte aligned)
- ``defined``: Boolean flag indicating if address is known

``symbol_table_t`` represents the hash table itself:

- ``entries``: Pointer to allocated array of ``symbol_entry_t``
- ``size``: Total number of slots in the table (power of 2)
- ``mask``: Bitmask for fast modulo operation (size - 1)
- ``count``: Current number of entries (used for load factor calculation)

3. Hash Function
----------------

varm uses the **FNV-1a** hash function for its simplicity, speed, and
excellent distribution characteristics for short strings like labels.

**Why FNV-1a?**

- **Fast**: Minimal operations per character (XOR then multiply)
- **Good distribution**: Well-studied, proven avalanche effect
- **Simple**: No lookup tables required
- **Endian-neutral**: Works correctly regardless of platform byte order

**Implementation (src/symbol.c:45-58):**

.. code-block:: c
   :linenos:

   static uint32_t hash_function(const char *name) {
       uint32_t hash = 2166136261u;
       for (const char *p = name; *p; p++) {
           hash ^= (uint8_t)*p;
           hash *= 16777619u;
       }
       return hash;
   }

The constant 2166136261u is the FNV offset basis, and 16777619u is the
FNV prime. These magic numbers are part of the FNV-1a specification.

**Hash computation example:**

::

   Label: ".L1"
   
   Step 1: hash = 2166136261
   Step 2: hash ^= '.' (0x2E)  → 2166136261 XOR 0x2E
   Step 3: hash *= 16777619
   Step 4: hash ^= 'L' (0x4C)
   Step 5: hash *= 16777619
   Step 6: hash ^= '1' (0x31)
   Step 7: hash *= 16777619
   Result: Final hash value, then masked with table->mask

4. Hash Table Operations
------------------------

4.1 Initialization
~~~~~~~~~~~~~~~~~~

.. code-block:: c
   :linenos:

   int symbol_table_init(symbol_table_t *table, size_t size) {
       table->size = size;
       table->mask = size - 1;
       table->count = 0;
       
       table->entries = calloc(size, sizeof(symbol_entry_t));
       if (!table->entries) {
           return -1;
       }
       
       return 0;
   }

The table is allocated with ``calloc`` to zero-initialize all entries,
ensuring the ``defined`` flag is correctly set to 0 (false) for all slots.

4.2 Insertion
~~~~~~~~~~~~~

.. code-block:: c
   :linenos:

   int symbol_insert(symbol_table_t *table, const char *name, 
                     uint32_t address) {
       uint32_t h = hash_function(name) & table->mask;
       size_t idx = h;
       
       // Linear probing to find empty slot or existing entry
       while (1) {
           if (!table->entries[idx].defined) {
               // Empty slot - insert new entry
               strncpy(table->entries[idx].name, name, SYMBOL_NAME_MAX - 1);
               table->entries[idx].name[SYMBOL_NAME_MAX - 1] = '\0';
               table->entries[idx].address = address;
               table->entries[idx].defined = 1;
               table->count++;
               return 0;
           }
           
           // Slot occupied - check if same symbol
           if (strcmp(table->entries[idx].name, name) == 0) {
               // Update existing entry
               table->entries[idx].address = address;
               return 0;
           }
           
           // Collision - probe next slot
           idx = (idx + 1) & table->mask;
       }
   }

4.3 Lookup
~~~~~~~~~~

.. code-block:: c
   :linenos:

   int symbol_lookup(symbol_table_t *table, const char *name, 
                     uint32_t *out_address) {
       uint32_t h = hash_function(name) & table->mask;
       size_t idx = h;
       
       // Linear probing until found or empty slot
       while (table->entries[idx].defined) {
           if (strcmp(table->entries[idx].name, name) == 0) {
               *out_address = table->entries[idx].address;
               return 0;  // Found
           }
           idx = (idx + 1) & table->mask;
       }
       
       return -1;  // Not found
   }

4.4 Destruction
~~~~~~~~~~~~~~~

.. code-block:: c
   :linenos:

   void symbol_table_destroy(symbol_table_t *table) {
       if (table->entries) {
           free(table->entries);
           table->entries = NULL;
       }
       table->size = 0;
       table->mask = 0;
       table->count = 0;
   }

5. Collision Resolution
-----------------------

varm uses **open addressing with linear probing** for collision resolution.

**Open Addressing:**

All entries are stored directly in the table array. Unlike chaining, there
are no linked lists or secondary structures. This provides excellent cache
locality since entries are stored contiguously.

**Linear Probing:**

When a collision occurs, the algorithm checks the next sequential slot:

::

   index = (original_hash + probe_count) & mask

For linear probing, probe_count increments by 1 each iteration.

**Probe Sequence Example:**

::

   Table size: 8 (mask = 0b111)
   Hash(".foo") = 5
   Hash(".bar") = 5  (collision!)
   
   Slot 5: ".foo"  ← first insertion
   Slot 6: ".bar"  ← collision, probe next (5 + 1) & 7 = 6
   Slot 7: empty
   
   Lookup ".bar":
   - Hash = 5, check slot 5 → ".foo" (no match)
   - Probe to slot 6 → ".bar" (match!)

**Load Factor Considerations:**

The maximum load factor is kept at **50%** (or lower). This ensures:

- Short probe chains (average < 2 probes)
- Worst-case behavior remains acceptable
- Cache efficiency stays high

When the load factor approaches 50%, the table should be resized. Current
implementation uses a fixed size of 256 slots.

6. Hash Table Diagram
---------------------

**Empty table (size=8 for illustration, actual=256):**

::

   +-------+-------+-------+-------+-------+-------+-------+-------+
   |       |       |       |       |       |       |       |       |
   | empty | empty | empty | empty | empty | empty | empty | empty |
   |       |       |       |       |       |       |       |       |
   +-------+-------+-------+-------+-------+-------+-------+-------+
     0       1       2       3       4       5       6       7
   
   count = 0
   mask = 7

**After inserting ".L1" at 0x1000, ".L2" at 0x1004:**

::

   +--------+--------+--------+--------+--------+--------+--------+--------+
   |        |        |        |        |        | .L1    |        | .L2    |
   | empty  | empty  | empty  | empty  | empty  | →0x1000| empty  | →0x1004|
   |        |        |        |        |        |        |        |        |
   +--------+--------+--------+--------+--------+--------+--------+--------+
     0        1        2        3        4        5        6        7
   
   count = 2

**Collision demonstration (hash(".data") == hash(".L2")):**

::

   +--------+--------+--------+--------+--------+--------+--------+--------+
   |        |        |        |        |        | .L1    | .data  | .L2    |
   | empty  | empty  | empty  | empty  | empty  | →0x1000| →0x2000| →0x1004|
   |        |        |        |        |        |        |        |        |
   +--------+--------+--------+--------+--------+--------+--------+--------+
     0        1        2        3        4        5        6        7
   
   ".data" collided with ".L2" (hash = 7), probed to slot 6.

**Lookup ".data" - probe chain traversal:**

::

   lookup(".data"):
   ├── Hash = 7
   ├── Check slot 7: ".L2" ≠ ".data" → continue
   ├── Probe to slot 0: (7 + 1) & 7 = 0
   ├── Check slot 0: empty → NOT FOUND
   
   (This shows why deleted entries require tombstones)

7. Complexity Analysis
----------------------

**Space Complexity:**

::

   O(n) where n = table size (fixed at 256 slots)
   
   Each slot: 64 bytes (name) + 4 bytes (address) + 4 bytes (flags) = 72 bytes
   Total: 256 * 72 = 18,432 bytes ≈ 18 KB

**Time Complexity:**

+----------+------------------+-------------------+
| Operation | Average Case     | Worst Case        |
+===========+==================+===================+
| Insert    | O(1)             | O(n)              |
| Lookup    | O(1)             | O(n)              |
| Destroy   | O(1)             | O(1)              |
+----------+------------------+-------------------+

**Average Case Analysis:**

With load factor α = count/size:

- Successful lookup: (1 + 1/(1-α)) / 2 probes
- Unsuccessful lookup: 1/(1-α) probes

At 50% load factor:
- Successful: ~1.5 probes
- Unsuccessful: ~2 probes

**Worst Case Analysis:**

Worst case occurs when all entries hash to the same slot, creating a probe
chain of length n. This requires examining every slot.

**Load Factor Impact:**

+----------+----------------+----------------+
| Load %   | Avg Probes     | Performance    |
+==========+================+================+
| 25%      | 1.17           | Excellent      |
| 50%      | 1.50           | Good           |
| 75%      | 2.50           | Degraded       |
| 90%      | 5.50           | Poor           |
| 99%      | 50.50          | Unacceptable   |
+----------+----------------+----------------+

8. Design Rationale
-------------------

**Why 256 slots?**

- **L1 cache fit**: 256 entries × 72 bytes ≈ 18 KB fits in typical L1 cache (32-48 KB)
- **Power of 2**: Enables fast modulo via bitwise AND (mask)
- **Fixed size**: Eliminates resize complexity and rehashing overhead
- **Adequate capacity**: 256 slots at 50% load = 128 symbols, sufficient for most assembly units

The fixed size is a deliberate trade-off:
- Pros: No resize logic, no rehash cost, predictable memory
- Cons: Cannot grow beyond 128 symbols

**Why FNV-1a over other hash functions?**

+----------+-----------+-------------+-----------+
| Function | Speed     | Quality     | Complexity|
+===========+===========+=============+===========
| FNV-1a   | Fast      | Excellent   | Simple    |
| SHA-256  | Slow      | Excellent   | Complex   |
| DJB2     | Fast      | Good        | Simple    |
| CRC32    | Fast      | Good        | Medium    |
+----------+-----------+-------------+-----------+

FNV-1a provides the best balance for label hashing:
- Labels are short (typically < 20 characters)
- No cryptographic security requirements
- Simplicity means fewer bugs

**Trade-offs not made:**

- **Dynamic resizing**: Would improve unbounded growth but adds complexity
- **Separate chaining**: Would eliminate clustering but wastes memory and hurts cache
- **Robin Hood hashing**: Would reduce variance but adds implementation complexity
- **Quadratic probing**: Would spread collisions better but breaks cache locality

The current implementation prioritizes simplicity, cache efficiency, and
predictable performance over theoretical optimality.
