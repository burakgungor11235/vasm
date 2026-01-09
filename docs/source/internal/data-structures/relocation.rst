Relocation
==========

.. warning::
   The relocation implementation described here is specific to varm |version|.
   This internal data structure is NOT stable and may change in any release.

1. Overview
-----------

Relocations are data structures that record information about label
references that cannot be resolved during the first pass of assembly.
When a label is used before it is defined (forward reference), the
assembler must defer resolution until the label's address is known.

**Common forward reference scenarios:**

.. code-block:: asm

   b .L_loop          ; Branch to label defined later
   ldr r0, =.L_data   ; Load address of label defined later
   add r1, r2, .L_sym ; Use symbol in expression

The relocation system ensures these references are correctly resolved
once all label definitions are known.

2. Data Structure
-----------------

Relocations are represented by the ``reloc_t`` structure defined in
``src/reloc.h``:

.. code-block:: c
   :linenos:

   #define RELOC_NAME_MAX 64

   typedef struct {
       uint32_t address;           // Address needing fixup
       char name[RELOC_NAME_MAX];  // Label name to resolve
       int is_branch;              // True if branch instruction
       int conditional;            // True if conditional branch
   } reloc_t;

   typedef struct {
       reloc_t *relocs;
       size_t count;
       size_t capacity;
   } reloc_list_t;

**Field Descriptions:**

``reloc_t`` represents a single unresolved reference:

- ``address``: Memory address of the instruction or data needing fixup
- ``name``: Label name to look up in the symbol table
- ``is_branch``: Boolean indicating branch vs. data load
- ``conditional``: Boolean for conditional branch variants

``reloc_list_t`` manages a collection of relocations:

- ``relocs``: Dynamic array of ``reloc_t`` entries
- ``count``: Current number of relocations
- ``capacity``: Allocated capacity (grows as needed)

3. Two-Pass Approach
--------------------

varm uses a classic two-pass algorithm to handle forward references:

**Pass 1: Collect relocations and build symbol table**

.. code-block:: c
   :linenos:

   int pass1_collect(assembler_t *as, section_t *section) {
       uint32_t addr = section->base;
       
       for (instr = section->head; instr != NULL; instr = instr->next) {
           switch (instr->type) {
               case INSTR_B:
               case INSTR_BL:
                   if (!symbol_defined(as, instr->operands[0].label)) {
                       // Forward reference - record relocation
                       reloc_list_add(&section->relocs, addr, 
                                     instr->operands[0].label,
                                     1, 0);  // is_branch = true
                   }
                   addr += 4;
                   break;
                   
               case INSTR_LDR:
                   if (instr->operands[1].is_literal) {
                       reloc_list_add(&section->relocs, addr,
                                     instr->operands[1].label,
                                     0, 0);  // is_branch = false
                   }
                   addr += 4;
                   break;
           }
       }
       
       return 0;
   }

**Pass 2: Resolve relocations after symbol table is complete**

.. code-block:: c
   :linenos:

   int pass2_resolve(assembler_t *as, section_t *section) {
       for (size_t i = 0; i < section->relocs.count; i++) {
           reloc_t *reloc = &section->relocs.relocs[i];
           uint32_t symbol_addr;
           
           if (symbol_lookup(&as->symbols, reloc->name, &symbol_addr) < 0) {
               error("Undefined symbol: %s", reloc->name);
               return -1;
           }
           
           if (reloc->is_branch) {
               resolve_branch_relocation(reloc, symbol_addr, section->base);
           } else {
               resolve_data_relocation(reloc, symbol_addr, section->base);
           }
       }
       
       return 0;
   }

4. Branch Relocation
--------------------

Branch instructions use a PC-relative offset encoded in the instruction.
The offset calculation accounts for the ARM pipeline (PC is 8 bytes ahead
during execution).

**ARM Branch Encoding:**

::

   Bits 23-0: Signed immediate (in units of 4 bytes)
   Effective offset: sign_extend(imm24) * 4
   
   Instruction at address A:
   - CPU fetches at A, A+4, A+8 (pipeline)
   - PC during execution = A + 8
   - Branch target T = PC + offset

**Offset Formula:**

.. code-block:: c
   :linenos:

   void resolve_branch_relocation(reloc_t *reloc, 
                                  uint32_t target,
                                  uint32_t section_base) {
       uint32_t pc = reloc->address + 8;  // ARM pipeline offset
       int32_t offset = target - pc;
       
       // Offset must be divisible by 4
       assert((offset & 3) == 0);
       
       // Convert to instruction encoding (in units of 4)
       int32_t encoded = offset >> 2;
       
       // Check range: ±32MB (24-bit signed = ±8M instructions)
       assert(encoded >= -0x800000 && encoded <= 0x7FFFFF);
       
       // Read instruction, patch, write back
       uint32_t instr = read_instr(reloc->address);
       instr = (instr & 0xFF000000) | (encoded & 0x00FFFFFF);
       write_instr(reloc->address, instr);
   }

**Branch Range Calculation:**

::

   24-bit signed immediate: -2^23 to +2^23 - 1
   Multiplied by 4: -32MB to +32MB
   
   Example:
   - Current: 0x1000
   - Target: 0x2004
   - PC during execution: 0x1008
   - Raw offset: 0x2004 - 0x1008 = 0xFFC
   - Encoded: 0xFFC / 4 = 0x3FF

5. Diagram: Relocation List and Resolution
------------------------------------------

**Collection Phase (Pass 1):**

::

   Assembly source:
   ┌─────────────────────────────┐
   │ b .L_loop      ; forward!   │
   │ mov r0, #0                  │
   │ ...                         │
   │ .L_loop:       ; defined    │
   │ add r1, r2, r3              │
   └─────────────────────────────┘

   Symbol table (partial):
   ┌──────────┬──────────┐
   │ .L_loop  │ ???      │  ← Not yet known
   └──────────┴──────────┘

   Relocation list:
   ┌──────────┬──────────┬───────┬───────┐
   │ address  │ name     │ branch│ cond  │
   ├──────────┼──────────┼───────┼───────┤
   │ 0x1000   │ .L_loop  │   1   │   0   │
   └──────────┴──────────┴───────┴───────┘

**Resolution Phase (Pass 2):**

::

   Symbol table (after all labels processed):
   ┌──────────┬──────────┐
   │ .L_loop  │ 0x2000   │  ← Now known!
   └──────────┴──────────┘

   Relocation resolution:
   ┌──────────┬──────────────────────────────────┐
   │ address  │ 0x1000                           │
   │ name     │ .L_loop                          │
   │ target   │ 0x2000                           │
   │ PC       │ 0x1000 + 8 = 0x1008              │
   │ offset   │ 0x2000 - 0x1008 = 0xFF8          │
   │ encoded  │ 0xFF8 / 4 = 0x3FE                │
   │ instr    │ b000000 00 1111111100 0000000000 │
   └──────────┴──────────────────────────────────┘

   Result:
   ┌──────────┬──────────────────┐
   │ 0x1000   │ EA0003FE         │  ; b .L_loop encoded
   └──────────┴──────────────────┘

6. Complexity Analysis
----------------------

**Space Complexity:**

::

   O(r) where r = number of relocations
   
   Each reloc_t: 4 bytes (address) + 64 bytes (name) + 8 bytes (flags) = 76 bytes
   Typical alignment: 80 bytes per relocation

**Time Complexity:**

+--------------------+------------------+------------------------+
| Operation          | Naive            | With Hash Table        |
+====================+==================+========================+
| Collect relocs     | O(n)             | O(n)                   |
| Resolve relocs     | O(r × s)         | O(r)                   |
+--------------------+------------------+------------------------+

Where:
- n = total instructions
- r = number of relocations
- s = number of symbols

**Naive Resolution (O(r × s)):**

For each relocation, scan all symbols to find the target:

::

   for each reloc in relocs:
       for each symbol in symbols:
           if reloc.name == symbol.name:
               resolve(reloc, symbol.address)

This is O(r × s) in the worst case.

**Hash Table Resolution (O(r)):**

Using the symbol table's hash lookup:

::

   for each reloc in relocs:
       symbol_addr = hash_lookup(symbol_table, reloc.name)
       resolve(reloc, symbol_addr)

This is O(r) since each lookup is O(1) average case.

**Branch vs. Data Relocations:**

+--------------------+----------------+----------------+
| Type               | Complexity     | Additional     |
+====================+================+================+
| Branch             | O(r)           | Range check    |
| Data (LDR)         | O(r)           | Offset calc    |
| Absolute           | O(r)           | Sign extend    |
+--------------------+----------------+----------------+

**Worst-Case Considerations:**

The worst case for hash table lookup is O(s) when all symbols hash to the
same bucket. This is mitigated by:

- FNV-1a's good distribution
- Maintaining low load factor in symbol table
- Practical symbol names are varied

**Optimization Opportunity:**

Current implementation collects all relocations, then resolves. An
alternative is to resolve immediately when the symbol becomes known
(using a dependency graph). This would reduce memory usage but add
complexity.
