Symbol Table
============

.. warning::

   **varm is NOT stable.** The symbol table implementation, hash function,
   and collision resolution strategy may change. Do not depend on internal
   hash values or table layout.

1. Symbol Table Overview
------------------------

The symbol table stores mappings from label names to their addresses:

::

    label_name → address (u32)

Labels are defined in assembly source code:

::

    my_label:         ; my_label = current address
        mov r0, r1    ; instruction at address 0
    other_label:      ; other_label = address 4

Why a Symbol Table is Needed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Assembly code can reference labels that are defined before or after the
reference (forward references):

::

    start:
        b loop      ; forward reference to 'loop'

    loop:
        mov r0, r1  ; 'loop' defined here

The symbol table allows the assembler to:
1. Record label addresses as they're encountered
2. Resolve label references to addresses
3. Handle forward references via relocation

2. Implementation Details
-------------------------

Location: ``src/asm/symbol_table.c``, ``src/asm/symbol_table.h``

The symbol table is implemented as a **hash table** with:
- **Hash function**: FNV-1a (Fowler-Noll-Vo)
- **Collision resolution**: Open addressing with linear probing
- **Load factor threshold**: 70% (triggers error if exceeded)

Hash Table Structure
~~~~~~~~~~~~~~~~~~~~

::

    ┌─────────────────────────────────────────────────────────┐
    │                   SYMBOL TABLE                          │
    ├─────────────────────────────────────────────────────────┤
    │  Size: power of 2 (default 256)                         │
    │  Mask: size - 1                                         │
    │  Count: number of entries                               │
    ├─────────────────────────────────────────────────────────┤
    │                                                         │
    │  Index  Entry                                           │
    │  ┌──────┴──────────────────────────────────────────┐   │
    │  │                                                  │   │
    │  │  [0]  name="start", addr=0x0000, defined=1      │   │
    │  │  [1]  name="loop",  addr=0x0004, defined=1      │   │
    │  │  [2]  (empty)                                   │   │
    │  │  [3]  name="data_ptr", addr=0x10000, defined=1  │   │
    │  │  ...                                            │   │
    │  │  [255](empty)                                   │   │
    │  │                                                  │   │
    │  └──────────────────────────────────────────────────┘   │
    └─────────────────────────────────────────────────────────┘

3. Hash Function
----------------

The FNV-1a hash function is used for its good distribution properties
and simplicity:

::

    u32 fnv1a_hash(const char* str) {
        u32 hash = 0x811c9dc5;        // FNV offset basis
        while (*str) {
            hash ^= (u8)*str++;       // XOR with byte
            hash *= 0x01000193;       // FNV prime
        }
        return hash;
    }

FNV-1a Properties
~~~~~~~~~~~~~~~~~

- **Initial value**: 0x811C9DC5 (32-bit FNV offset basis)
- **FNV prime**: 0x01000193
- **Final hash**: ANDed with table mask (size - 1)
- **Case sensitivity**: Case-sensitive (lowercase recommended)

Example
~~~~~~~

::

    hash("loop") = 0x811c9dc5
                 ^ 0x6c ( 'l' )
                 = 0x811c9da9
                 * 0x01000193
                 = 0x811c9da9
                 ... (continues for each character)

4. Data Structure
-----------------

symbol_entry_t
~~~~~~~~~~~~~~

::

    typedef struct {
        char name[64];      // Label name (max 63 chars + null)
        u32 address;        // Resolved address
        int defined;        // 1 if defined, 0 if only declared
    } symbol_entry_t;

symbol_table_t
~~~~~~~~~~~~~~

::

    typedef struct {
        symbol_entry_t* entries;   // Array of entries (dynamically allocated)
        u32 size;                   // Table size (power of 2)
        u32 mask;                   // size - 1 (for fast modulo)
        u32 count;                  // Number of entries
    } symbol_table_t;

Table Sizing
~~~~~~~~~~~~

::

    #define SYMBOL_TABLE_DEFAULT_SIZE 256

The table size is always a power of 2 for efficient masking:

::

    void symbol_init(symbol_table_t* tbl, u32 size) {
        if (size == 0) {
            size = SYMBOL_TABLE_DEFAULT_SIZE;
        }

        tbl->size = 1;
        while (tbl->size < size) {    // Round up to power of 2
            tbl->size *= 2;
        }

        tbl->mask = tbl->size - 1;     // For: hash & mask == hash % size
        tbl->count = 0;
        tbl->entries = calloc(tbl->size, sizeof(symbol_entry_t));
    }

5. Operations
-------------

symbol_init()
~~~~~~~~~~~~~

::

    void symbol_init(symbol_table_t* tbl, u32 size)

Initializes an empty symbol table with the specified (or default) size.

symbol_insert()
~~~~~~~~~~~~~~~

::

    int symbol_insert(symbol_table_t* tbl, const char* name, u32 addr)

Inserts a label into the table. Returns 0 on success, -1 on failure.

Algorithm:

::

    1. Check if table is too full (count >= size * 7/10)
    2. Compute hash = fnv1a_hash(name) & mask
    3. While slot is occupied:
       a. If name matches: update address, return success
       b. Else: linear probe to next slot (hash + 1) & mask
    4. If empty slot found: insert entry, count++, return success
    5. If all slots checked: return failure (table full)

symbol_lookup()
~~~~~~~~~~~~~~~

::

    int symbol_lookup(symbol_table_t* tbl, const char* name, u32* out_addr)

Looks up a label name. Returns 0 on success, -1 if not found.

Algorithm:

::

    1. Compute hash = fnv1a_hash(name) & mask
    2. While slot is not empty:
       a. If name matches: set *out_addr, return success
       b. Else: linear probe to next slot (hash + 1) & mask
    3. If empty slot reached: return not found

symbol_destroy()
~~~~~~~~~~~~~~~~

::

    void symbol_destroy(symbol_table_t* tbl)

Frees allocated memory and clears the table structure.

6. Complexity Analysis
----------------------

Time Complexity
~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Operation | Average Case     | Worst Case       |
+===========+==================+==================+
| Insert    | O(1)             | O(n)             |
| Lookup    | O(1)             | O(n)             |
| Destroy   | O(1)             | O(1)             |
+----------+------------------+------------------+

Worst case occurs when all slots are occupied (load factor = 100%)
and the probed item is at the end or not present.

Space Complexity
~~~~~~~~~~~~~~~~

::

    O(s) where s = table size (default 256 entries)

    Memory usage:
    - entries: size * sizeof(symbol_entry_t)
    - Default: 256 * 72 bytes = 18,432 bytes (~18KB)

Load Factor Impact
~~~~~~~~~~~~~~~~~~

The implementation rejects inserts when load factor exceeds 70%:

::

    if (tbl->count >= tbl->size * 7 / 10) {
        return -1;  // Table too full
    }

This ensures:
- Amortized O(1) operations
- Bounded probe sequences
- Predictable memory usage

Collision Resolution
~~~~~~~~~~~~~~~~~~~~

Linear probing provides cache-friendly access patterns:

::

    ┌──────────────────────────────────────────────────────────┐
    │  Hash Collision Example                                   │
    ├──────────────────────────────────────────────────────────┤
    │                                                           │
    │  "label_a" hashes to index 5                              │
    │  "label_b" also hashes to index 5 (collision)             │
    │                                                           │
    │  [0]  ...                                                 │
    │  ...                                                      │
    │  [5]  "label_a" → addr=0x10                              │
    │  [6]  "label_b" → addr=0x20  ← placed here (collision)    │
    │  [7]  ...                                                 │
    │                                                           │
    │  Lookup "label_b":                                        │
    │  1. Hash → index 5                                        │
    │  2. "label_a" != "label_b" → probe index 6                │
    │  3. Found! Return 0x20                                    │
    │                                                           │
    └──────────────────────────────────────────────────────────┘

.. warning::

   The symbol table is case-sensitive. ``MyLabel`` and ``mylabel`` are
   different symbols. The FNV-1a hash does not perform case folding.
