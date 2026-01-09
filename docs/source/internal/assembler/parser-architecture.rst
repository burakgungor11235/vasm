Parser Architecture
===================

.. warning::

   **varm is NOT stable.** The parser implementation, handler functions, and
   internal data structures may change. The parsing algorithm and token
   handling described here reflect the current implementation only.

1. Parser Overview
------------------

The parser converts the token stream produced by the lexer into encoded
machine instructions. It is implemented as a **recursive descent parser**
with dispatch to specialized handler functions:

::

    Tokens → Parser → Encoded Instructions → .varm File

The parser handles:
- Assembly directives (``.text``, ``.data``, ``.word``, etc.)
- All instruction types (ALU, multiply, load/store, branch, system)
- Label definitions and forward references
- Literal pool management for pseudo-instructions

2. Parser Structure
-------------------

The parser consists of a main entry point and specialized handler functions
for each instruction category.

Main Entry Point
~~~~~~~~~~~~~~~~

``int parse(token_t* tokens, int token_count, program_state_t* prog)``
    Parses the token stream and populates the program state.

Handler Functions
~~~~~~~~~~~~~~~~~

==================  ==========================================================
Handler            Description
==================  ==========================================================
parse_directive()  Processes assembly directives
parse_move()       MOV, MVN instructions
parse_alu()        ADD, SUB, AND, ORR, CMP, CMN, TST, TEQ
parse_mult()       MUL, MLA instructions
parse_load_store() LDR, LDRB, STR, STRB instructions
parse_branch()     B, BL instructions
parse_system()     HALT, NOP, SWI instructions
parse_pseudo_ldr() ldr rd, =label pseudo-instruction
==================  ==========================================================

3. Token Stream Processing
--------------------------

The parser iterates through the token stream sequentially, using the
token type to dispatch to appropriate handlers:

::

    for (int i = 0; i < token_count && tokens[i].type != TOKEN_EOF; i++) {
        switch (tokens[i].type) {
            case TOKEN_NEWLINE:
                continue;  // Skip

            case TOKEN_LABEL:
                add_label(&ctx, tokens[i].value, ctx.current_addr);
                continue;

            case TOKEN_DIRECTIVE:
                parse_directive(&ctx, tokens[i].value, tokens, &i, token_count);
                continue;

            case TOKEN_INSTRUCTION:
                // Extract instruction name and optional condition
                // Dispatch to appropriate handler
                parse_XXX(&ctx, opcode, tokens, &i, token_count, condition);
                continue;
        }
    }

Lookahead for Disambiguation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The parser uses limited lookahead to disambiguate certain constructs:

**Pseudo-instruction detection:**

::

    if (opcode == OP_LDR &&
        i + 3 < token_count &&
        tokens[i + 1].type == TOKEN_COMMA &&
        tokens[i + 2].type == TOKEN_EQUAL &&
        tokens[i + 3].type == TOKEN_IDENTIFIER) {
        parse_pseudo_ldr(&ctx, tokens, &i, token_count, condition);
        continue;
    }

**Branch condition suffixes:**

::

    if (strlen(instr_name) > 2 && instr_name[0] == 'b') {
        // Check if this is "beq", "bne", etc. (short form)
        for (int c = 0; c < 16; c++) {
            if (strcasecmp(instr_name + 1, suffixes[c]) == 0) {
                instr_name = "b";
                condition = suffixes[c];
                break;
            }
        }
    }

4. Parsing Strategy
-------------------

The parsing strategy follows the pattern:

1. **Identify token type** - Newline, label, directive, or instruction
2. **Extract instruction name** - Handle condition suffix (e.g., ``mov.eq``)
3. **Look up opcode** - Using lookup table
4. **Dispatch to handler** - Based on opcode category
5. **Handler consumes tokens** - Each handler advances the token index

Dispatch Logic
~~~~~~~~~~~~~~

::

    int opcode = lookup_opcode(instr_name);

    if (opcode == OP_B || opcode == OP_BL) {
        parse_branch(&ctx, opcode, tokens, &i, token_count, condition);
    } else if (opcode == OP_HALT || opcode == OP_NOP || opcode == OP_SWI) {
        parse_system(&ctx, opcode, tokens, &i, token_count, condition);
    } else if (opcode == OP_MOV || opcode == OP_MVN) {
        parse_move(&ctx, opcode, tokens, &i, token_count, condition);
    } else if (opcode == OP_CMP || opcode == OP_CMN || opcode == OP_TST || opcode == OP_TEQ) {
        parse_alu(&ctx, opcode, tokens, &i, token_count, condition);
    } else if (opcode == OP_MUL || opcode == OP_MLA) {
        parse_mult(&ctx, opcode, tokens, &i, token_count, condition);
    } else if (opcode == OP_LDR || opcode == OP_LDRB || opcode == OP_STR || opcode == OP_STRB) {
        parse_load_store(&ctx, opcode, tokens, &i, token_count, condition);
    } else {
        parse_alu(&ctx, opcode, tokens, &i, token_count, condition);  // ADD, SUB, etc.
    }

5. Parser Context (parser_ctx_t)
--------------------------------

The parser maintains state in a ``parser_ctx_t`` structure (defined in
``src/asm/parser_internal.h``):

::

    typedef struct {
        u32 text[MAX_INSTRUCTIONS];              // Encoded instructions
        u32 text_size;                           // Instruction count
        u8 data[65536];                          // Initialized data
        u32 data_size;                           // Data byte count
        symbol_table_t labels;                   // Symbol table
        reloc_t relocs[MAX_INSTRUCTIONS];        // Pending relocations
        int reloc_count;
        literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES];
        int literal_pool_count;
        literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS];
        int literal_pool_ref_count;
        u32 current_addr;                        // Assembly location counter
        int in_text_section;                     // .text vs .data section
        error_context_t* errors;                 // Error collection
    } parser_ctx_t;

Context Fields Explained
~~~~~~~~~~~~~~~~~~~~~~~~

=============== ============================================================
Field           Description
=============== ============================================================
text[]          Array of encoded 32-bit instructions
text_size       Number of instructions in text[]
data[]          Raw bytes for .data section
data_size       Number of bytes in data[]
labels          Hash table of label → address mappings
relocs[]        Forward references needing resolution
current_addr    Assembly location counter (in bytes)
in_text_section Boolean: 1 = .text, 0 = .data
literal_pool    Pool of 32-bit values for ldr rd, =label
literal_pool_refs References from LDR instructions to pool entries
=============== ============================================================

6. Error Handling
-----------------

The parser collects errors during parsing and reports them at the end:

::

    ctx.errors = NULL;

    // During parsing, errors are appended:
    error_add(&ctx.errors, "message", line, column);

    // At the end, all errors are reported:
    error_report(ctx.errors);

Error Types Handled
~~~~~~~~~~~~~~~~~~~

- Missing operands
- Invalid register names
- Invalid immediates
- Undefined labels
- Section overflow
- Too many labels/instructions

Error Collection
~~~~~~~~~~~~~~~~

Errors are stored in a linked list structure and printed at the end of
parsing, allowing all errors to be reported rather than failing fast:

::

    if (asm_error_count > 0) {
        fprintf(stderr, "Assembly failed with %d error(s)\n", asm_error_count);
        return -1;
    }

7. Complexity Analysis
----------------------

Time Complexity: **O(n)** where n = token count

- Each token is processed exactly once
- Handler functions perform O(1) work per instruction
- Symbol table operations are O(1) average case
- Relocation resolution is O(r) where r = number of relocations

::

    token_count = t
    t_token_processing: O(t)
    symbol_lookups: O(t) average case
    relocation_resolution: O(r) where r <= t
    total: O(t + r) = O(t)

Space Complexity: **O(i + d + s)** where:
- i = instruction count (MAX_INSTRUCTIONS = 4096)
- d = data size (MAX_DATA = 65536 bytes)
- s = symbol count (MAX_LABELS = 256)

::

    text[]:    4096 * 4 bytes = 16KB
    data[]:    65536 bytes    = 64KB
    symbols:   256 * ~72 bytes = 18KB
    relocs:    4096 * ~72 bytes = 288KB
    literals:  256 * 8 bytes = 2KB
    total:     ~388KB maximum

Handler Function Complexity
~~~~~~~~~~~~~~~~~~~~~~~~~~~

All handler functions are **O(1)** except for:

- ``parse_directive()``: O(k) where k = number of operands for .word/.byte
- ``emit_literal_pool()``: O(p + r) where p = pool entries, r = pool references

.. warning::

   The parser does not perform extensive semantic validation. Some invalid
   programs may assemble without errors but behave incorrectly at runtime.
