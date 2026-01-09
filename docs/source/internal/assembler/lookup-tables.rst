Lookup Tables
=============

.. warning::

   **varm is NOT stable.** The lookup tables are auto-generated at build
   time. Their contents, organization, and lookup algorithms may change
   when opcodes are modified. Do not manually edit generated files.

1. Lookup Tables Overview
-------------------------

Lookup tables provide fast O(1) conversion from text representations
to numeric codes:

- **Instruction name lookup**: ``"mov"`` → ``0x00``
- **Condition code lookup**: ``"eq"`` → ``0x0``

These tables are generated at build time from ``include/opcode.h``
by the ``scripts/gen_lookup_tables.py`` script.

Build Pipeline
~~~~~~~~~~~~~~

::

    ┌────────────────┐    ┌────────────────┐    ┌────────────────┐
    │ include/opcode.h│───→│gen_lookup_tables│───→│ src/asm/generated/
    │                │    │   (Python)     │    │  *.c, *.h
    └────────────────┘    └────────────────┘    └────────────────┘
           │                      │                       │
           │                      ▼                       ▼
           │              ┌────────────────┐    ┌────────────────┐
           │              │  Parses enum   │    │  Compiled into  │
           │              │  definitions   │    │  vasm executable│
           │              └────────────────┘    └────────────────┘

2. gen_lookup_tables.py
------------------------

Location: ``scripts/gen_lookup_tables.py``

This Python script:
1. Parses ``include/opcode.h`` to extract opcodes and conditions
2. Generates C lookup tables with optimal hash functions
3. Outputs three files:
   - ``instr_name_lookup.c``
   - ``cond_code_lookup.c``
   - ``lookup_tables.h``

Usage
~~~~~

::

    python3 gen_lookup_tables.py              # Generate files
    python3 gen_lookup_tables.py --stdout     # Output to stdout
    python3 gen_lookup_tables.py --cond       # Output condition table
    python3 gen_lookup_tables.py --header     # Output header

Parsing Opcodes
~~~~~~~~~~~~~~~

::

    def parse_opcode_enum():
        """Parse opcode.h and extract instruction definitions."""
        with open(OPCODES_HEADER, "r") as f:
            content = f.read()

        opcodes = {}

        # Parse #define patterns
        pattern = r"#define\s+(OP_\w+)\s+(0x[0-9A-Fa-f]+)"
        for name, value in re.findall(pattern, content):
            if name.startswith("OP_") and name not in ("OP_MASK",):
                opcodes[name] = int(value, 16)

        # Parse enum patterns
        pattern_enum = r"OP_(\w+)\s*=\s*(0x[0-9A-Fa-f]+)"
        # ... extract from enum body

        return opcodes

3. Instruction Name Lookup
--------------------------

The instruction name lookup uses a **perfect hash** with FNV-1a:

::

    #define INSTR_NAME_TABLE_SIZE 256

    typedef struct {
        const char* name;
        uint8_t opcode;
    } instr_name_entry_t;

    static const instr_name_entry_t INSTR_NAME_TABLE[INSTR_NAME_TABLE_SIZE] = {
        { NULL, 0 },
        { NULL, 0 },
        { NULL, 0 },
        { "bic", 11 },
        // ... 256 entries (sparse table)
    };

Hash Function
~~~~~~~~~~~~~

::

    uint8_t lookup_opcode(const char* name) {
        if (!name || !name[0]) return 0;
        uint32_t h = 0x811c9dc5;           // FNV offset basis
        for (const char* p = name; *p; p++) {
            h ^= (uint8_t)(*p | 0x20);     // XOR with lowercase byte
            h *= 0x01000193;               // FNV prime
        }
        const instr_name_entry_t* entry = &INSTR_NAME_TABLE[h & 255];
        if (entry->name && strcasecmp(entry->name, name) == 0) {
            return entry->opcode;
        }
        return 0;                          // Not found
    }

Key Properties
~~~~~~~~~~~~~~

- **Case folding**: ``*p | 0x20`` converts to lowercase
- **Modulo**: ``h & 255`` maps to 0-255 range (table size = 256)
- **Sparse table**: Most entries are NULL (empty)
- **Case-insensitive**: ``"MOV"``, ``"mov"``, ``"Mov"`` all work

Table Generation Algorithm
~~~~~~~~~~~~~~~~~~~~~~~~~~

::

    INSTR_NAME_TABLE_SIZE = 256
    items = [("mov", 0), ("add", 2), ("ldr", 32), ...]

    table = [(None, 0) for _ in range(256)]

    for name, opcode in items:
        h = fnv1a_hash(name)           // Hash name
        idx = h & 255                   // Modulo 256

        while table[idx][0] is not None:  // Collision
            idx = (idx + 1) & 255        // Linear probe

        table[idx] = (name, opcode)     // Insert

4. Condition Code Lookup
------------------------

Condition codes use a simple array lookup:

::

    typedef struct {
        const char* suffix;
        uint8_t code;
    } cond_code_entry_t;

    static const cond_code_entry_t COND_CODE_TABLE[] = {
        { "eq", 0x0 }, { "ne", 0x1 },
        { "cs", 0x2 }, { "hs", 0x2 },
        { "cc", 0x3 }, { "lo", 0x3 },
        { "mi", 0x4 }, { "pl", 0x5 },
        { "vs", 0x6 }, { "vc", 0x7 },
        { "hi", 0x8 }, { "ls", 0x9 },
        { "ge", 0xA }, { "lt", 0xB },
        { "gt", 0xC }, { "le", 0xD },
        { "al", 0xE }, { "nv", 0xF },
        { "", 0xE },                      // Default: always
    };

Condition codes include aliases:

- ``cs`` and ``hs`` both map to 0x2 (Carry Set / Higher or Same)
- ``cc`` and ``lo`` both map to 0x3 (Carry Clear / Lower)

Lookup Function
~~~~~~~~~~~~~~~

::

    uint8_t parse_condition(const char* suffix) {
        if (!suffix) return 0xE;          // NULL → AL (always)
        for (size_t i = 0; i < sizeof(COND_CODE_TABLE)/sizeof(COND_CODE_TABLE[0]); i++) {
            if (COND_CODE_TABLE[i].suffix[0] == '\0') {
                if (suffix[0] == '\0') return COND_CODE_TABLE[i].code;
            } else if (strcasecmp(COND_CODE_TABLE[i].suffix, suffix) == 0) {
                return COND_CODE_TABLE[i].code;
            }
        }
        return 0xE;                       // Default: AL (always)
    }

Condition Code Table
~~~~~~~~~~~~~~~~~~~~

======= ===== =============
Suffix  Code  Meaning
======= ===== =============
eq      0x0   Equal
ne      0x1   Not equal
cs/hs   0x2   Carry set / Higher or same
cc/lo   0x3   Carry clear / Lower
mi      0x4   Minus / Negative
pl      0x5   Plus / Non-negative
vs      0x6   Overflow set
vc      0x7   Overflow clear
hi      0x8   Higher
ls      0x9   Lower or same
ge      0xA   Greater or equal
lt      0xB   Less than
gt      0xC   Greater than
le      0xD   Less or equal
al/nv   0xE   Always / Never execute
======= ===== =============

5. Meson Integration
--------------------

Location: ``src/meson.build``

The build system uses Meson's ``custom_target`` to generate lookup tables:

::

    gen_script = files('../scripts/gen_lookup_tables.py')
    python = find_program('python3')

    gen_lookup = custom_target(
        'gen_lookup',
        input: gen_script,
        output: 'instr_name_lookup.c',
        command: [python, '@INPUT@', '--stdout'],
        capture: true,
    )

    gen_cond = custom_target(
        'gen_cond',
        input: gen_script,
        output: 'cond_code_lookup.c',
        command: [python, '@INPUT@', '--cond'],
        capture: true,
    )

    gen_header = custom_target(
        'gen_header',
        input: gen_script,
        output: 'lookup_tables.h',
        command: [python, '@INPUT@', '--header'],
        capture: true,
    )

Build Process
~~~~~~~~~~~~~

::

    1. Meson reads src/meson.build
    2. Identifies custom_target rules
    3. Runs Python script, captures output
    4. Writes generated .c files to build directory
    5. Compiles generated files with project

::

    build/
    ├── src/
    │   ├── asm/
    │   │   └── generated/
    │   │       ├── instr_name_lookup.c    ← Generated
    │   │       ├── cond_code_lookup.c     ← Generated
    │   │       └── lookup_tables.h        ← Generated
    │   └── ...

6. Generated Files
------------------

lookup_tables.h
~~~~~~~~~~~~~~~

::

    /* AUTO-GENERATED - DO NOT EDIT */
    #ifndef LOOKUP_TABLES_H
    #define LOOKUP_TABLES_H

    #include <stdint.h>

    uint8_t lookup_opcode(const char* name);
    uint8_t parse_condition(const char* suffix);

    #endif

instr_name_lookup.c
~~~~~~~~~~~~~~~~~~~

::

    /* AUTO-GENERATED - DO NOT EDIT */
    /* Generated by: scripts/gen_lookup_tables.py */

    #include <stdint.h>
    #include <stddef.h>
    #include <strings.h>
    #include "lookup_tables.h"

    // ... INSTR_NAME_TABLE definition ...
    // ... lookup_opcode() function ...

cond_code_lookup.c
~~~~~~~~~~~~~~~~~~

::

    /* AUTO-GENERATED - DO NOT EDIT */

    #include <stdint.h>
    #include <stddef.h>
    #include <strings.h>
    #include "lookup_tables.h"

    // ... COND_CODE_TABLE definition ...
    // ... parse_condition() function ...

7. Build Process Summary
------------------------

::

    meson setup build/
          │
          ▼
    Parse src/meson.build
          │
          ▼
    Run custom_target: python3 scripts/gen_lookup_tables.py --stdout
          │
          ▼
    Write: build/src/asm/generated/instr_name_lookup.c
    Write: build/src/asm/generated/cond_code_lookup.c
    Write: build/src/asm/generated/lookup_tables.h
          │
          ▼
    Compile:
    - src/main_asm.c
    - src/asm/lexer.c
    - src/asm/parser.c
    - src/asm/symbol_table.c
    - src/asm/directives.c
    - build/src/asm/generated/*.c
          │
          ▼
    Link → vasm executable

Regenerating Tables
~~~~~~~~~~~~~~~~~~~

If ``include/opcode.h`` changes, regenerate lookup tables:

::

    rm -rf build/
    meson setup build/
    ninja -C build/

Or regenerate manually:

::

    python3 scripts/gen_lookup_tables.py

.. warning::

   The ``AUTO-GENERATED - DO NOT EDIT`` comments are not just warnings.
   Any manual changes will be overwritten on the next build. Modify
   ``scripts/gen_lookup_tables.py`` or ``include/opcode.h`` instead.
