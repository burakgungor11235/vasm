Literal Pool
============

.. warning::
   **varm is not stable.** This implementation may change without notice.

1. Literal Pool Overview
------------------------

A **literal pool** is a data section containing constant values that are
referenced by code. In varm, the literal pool solves the problem of
loading label addresses into registers.

The pseudo-instruction:

::

   ldr rd, =label

loads the address of ``label`` into register ``rd``. Since labels may be
defined after their use (forward references), the assembler cannot know
the address at encode time.

Problem: Loading Label Addresses
--------------------------------

Consider this code:

::

   b main      ; Branch to main (forward reference)
   ldr r0, =msg ; Load address of msg

   msg: .word 0x12345678

When encoding ``ldr r0, =msg``, the assembler doesn't yet know the
address of ``msg``. The instruction format only allows a 12-bit offset.

Solution: Literal Pool
~~~~~~~~~~~~~~~~~~~~~~

The assembler:

1. Creates an LDR instruction with PC-relative addressing
2. Reserves space in the literal pool for the label's address
3. Emits the pool at a known location (end of text section)
4. Fixes up the LDR offset after all labels are known

2. Implementation
-----------------

The literal pool is managed by the assembler in ``src/asm/parser.c``.

Pool entry structure:

.. code-block:: c

   // src/asm/parser_internal.h
   #define MAX_LITERAL_POOL_ENTRIES 256
   #define MAX_LITERAL_POOL_REFS 256

   typedef struct {
       u32 value;   // The constant value
       u32 offset;  // Offset in text section (filled later)
   } literal_pool_entry_t;

   typedef struct {
       u32 instr_addr;   // Address of referencing LDR
       int pool_index;   // Index in literal pool
   } literal_pool_ref_t;

Parser context includes:

.. code-block:: c

   // src/asm/parser.c: parser_ctx_t
   literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES];
   int literal_pool_count;

   literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS];
   int literal_pool_ref_count;

During parsing - add pool entry:

.. code-block:: c

   // src/asm/parser.c:159-176
   static u32
   add_literal_pool_entry(parser_ctx_t* ctx, u32 value)
   {
       // Check for duplicate
       for (int i = 0; i < ctx->literal_pool_count; i++) {
           if (ctx->literal_pool[i].value == value) {
               return ctx->literal_pool[i].offset;
           }
       }

       // Add new entry (offset set to 0, filled later)
       if (ctx->literal_pool_count < MAX_LITERAL_POOL_ENTRIES) {
           ctx->literal_pool[ctx->literal_pool_count].value = value;
           ctx->literal_pool[ctx->literal_pool_count].offset = 0;
           ctx->literal_pool_count++;
           return ctx->literal_pool_count - 1;
       }

       return 0;
   }

Record reference:

.. code-block:: c

   // src/asm/parser.c:607-611
   if (ctx->literal_pool_ref_count < MAX_LITERAL_POOL_REFS) {
       ctx->literal_pool_refs[ctx->literal_pool_ref_count].instr_addr =
           ctx->text_size * 4;
       ctx->literal_pool_refs[ctx->literal_pool_ref_count].pool_index =
           pool_index;
       ctx->literal_pool_ref_count++;
   }

3. Two-Pass Approach
--------------------

varm uses a two-pass approach to resolve literal pool entries:

Pass 1: Collect labels and create references
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   for each token:
       if instruction references label via =label:
           add_literal_pool_entry(ctx, 0)   // placeholder value
           record reference (instr_addr, pool_index)

       if label definition:
           add_label(ctx, label_name, current_addr)

Pass 2: Emit pool and fix up
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After all parsing, relocations are resolved and the pool is emitted:

.. code-block:: c

   // src/asm/parser.c:765-777
   // First: resolve relocations (fill in actual values)
   for (int j = 0; j < ctx->reloc_count; j++) {
       int addr = lookup_label(&ctx, ctx.relocs[j].name);
       if (addr >= 0) {
           if (!ctx.relocs[j].is_branch) {
               // Literal pool entry
               ctx.literal_pool[ctx.relocs[j].address].value = (u32)addr;
           }
           // ... branch fixup
       }
   }

   // Then: emit pool and fix up LDR offsets
   emit_literal_pool(&ctx);

4. PC-Relative Addressing
-------------------------

The LDR instruction uses PC-relative addressing to load from the pool:

::

   ldr rd, [pc, #offset]

The offset is calculated as:

::

   offset = pool_addr - instr_addr - 4

The ``-4`` accounts for pipeline behavior (PC is 2 instructions ahead
during execution).

Pool emission and fixup:

.. code-block:: c

   // src/asm/parser.c:178-211
   static void
   emit_literal_pool(parser_ctx_t* ctx)
   {
       u32 pool_start = ctx->text_size * 4;

       // Emit pool entries
       for (int i = 0; i < ctx->literal_pool_count; i++) {
           ctx->literal_pool[i].offset = pool_start + i * 4;
           ctx->text[ctx->text_size++] = ctx->literal_pool[i].value;
       }

       // Fix up LDR instructions with correct offsets
       for (int j = 0; j < ctx->literal_pool_ref_count; j++) {
           u32 instr_addr = ctx->literal_pool_refs[j].instr_addr / 4;
           int pool_idx = ctx->literal_pool_refs[j].pool_index;

           u32 pool_offset = ctx->literal_pool[pool_idx].offset;
           int byte_offset = (int)pool_offset - (int)(instr_addr * 4 + 4);

           // Patch the offset into the instruction
           u32 new_instr = (ctx->text[instr_addr] & 0xFFFFF000) |
                          (offset_val & 0xFFF);
           ctx->text[instr_addr] = new_instr;
       }
   }

5. ASCII Diagram: Pool Flow
---------------------------

::

   Assembly Source          Text Section               Literal Pool
   ────────────────────────────────────────────────────────────────────

   .text                   ┌─────────────────────┐
   start:                  │ 0x00000000: LDR     │─┐
       ldr r0, =msg        │ 0x00000004: ...     │ │
   loop:                   │ 0x00000008: ...     │ │
       b loop              └─────────────────────┘ │
                           pool_start = 0x0C         │
                                                   │ offset
   msg: .word 0x1234       ┌─────────────────────┐◄─┘
                           │ 0x0000000C: 0x1234  │  ← pool entry
                           └─────────────────────┘

   During emit_literal_pool():
   1. Calculate pool_start = 0x0C (after all instructions)
   2. Emit 0x1234 at address 0x0C
   3. Calculate offset for LDR: 0x0C - 0x00 - 4 = 0x08
   4. Patch LDR instruction with offset 0x08

6. Complexity Analysis
----------------------

+-------------------+------------+----------------------------------+
| Operation         | Complexity | Notes                            |
+-------------------+------------+----------------------------------+
| Add pool entry    | O(1)       | Deduplication is O(n) worst case |
| Record reference  | O(1)       | Simple array append              |
| Pool emission     | O(n)       | n = literal_pool_count           |
| Fixup             | O(m)       | m = literal_pool_ref_count       |
| Lookup            | O(1)       | Array index                      |
+-------------------+------------+----------------------------------+

The deduplication in ``add_literal_pool_entry()`` is O(n) in the worst
case where all values are unique. This is acceptable since literal pools
are typically small (< 256 entries).

7. Code References
------------------

+------------------+--------------------------------------------+
| Component        | Location                                   |
+------------------+--------------------------------------------+
| Pool emission    | ``parser.c:178-211`` (``emit_literal_pool``) |
| Add entry        | ``parser.c:159-176`` (``add_literal_pool_entry``) |
| Pseudo-LDR parse | ``parser.c:573-620`` (``parse_pseudo_ldr``) |
| Relocation fixup | ``parser.c:765-777``                       |
+------------------+--------------------------------------------+

See Also
--------

- :doc:`operand-decoding` - LDR operand encoding
- ``src/asm/parser.c`` - Full literal pool implementation
- ``docs/user/reference/directives.rst`` - Assembler directives
- :doc:`../architecture/memory-subsystem` - Memory layout
