Code Generation
===============

.. warning::

   **varm is NOT stable.** The instruction encoding format, bit field
   layout, and output file format may change. The encoding described here
   reflects the current implementation only.

1. Code Generation Overview
---------------------------

Code generation is the final phase of assembly, converting parsed
instructions into their binary representation:

::

    Tokens → Parser → Instruction Fields → Encoded 32-bit Word

The assembler builds each instruction by combining fields:

::

    ┌─────────────────────────────────────────────────────────────┐
    │                     32-BIT INSTRUCTION                       │
    ├─────────────────────────────────────────────────────────────┤
    │  31:24       23:20      19:16      15:12      11:0          │
    │  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐   │
    │  │ Opcode  │  Cond   │   Rn    │   Rd    │   Operand   │   │
    │  │  8 bit  │  4 bit  │  4 bit  │  4 bit  │   12 bit    │   │
    │  └─────────┴─────────┴─────────┴─────────┴─────────────┘   │
    └─────────────────────────────────────────────────────────────┘

2. Instruction Encoding
-----------------------

Bit Field Assembly
~~~~~~~~~~~~~~~~~~

Each instruction field is shifted to its position and combined:

::

    #define OPCODE_SHIFT  24
    #define COND_SHIFT    20
    #define RN_SHIFT      16
    #define RD_SHIFT      12

    u32 instr = (opcode << OPCODE_SHIFT) |
                (cond   << COND_SHIFT)   |
                (rn     << RN_SHIFT)     |
                (rd     << RD_SHIFT)     |
                operand;

Example: mov r0, #42
~~~~~~~~~~~~~~~~~~~~

::

    opcode = OP_MOV = 0x00
    cond   = COND_AL = 0xE
    rd     = 0 (r0)
    operand = (1 << 11) | 42 = 0x82A

    instr = (0x00 << 24) | (0xE << 20) | (0 << 12) | 0x82A
          = 0x00E0082A

Little-Endian Byte Order
~~~~~~~~~~~~~~~~~~~~~~~~

The 32-bit instruction is stored in little-endian format:

::

    Address A:   0x2A  (operand[7:0])
    Address A+1: 0x08  (operand[11:8] | rd[3:0])
    Address A+2: 0xE0  (rd[7:4] | rn[3:0])
    Address A+3: 0x00  (cond[3:0] | opcode[7:0])

    Result bytes: 2A 08 E0 00

emit_instr() Function
~~~~~~~~~~~~~~~~~~~~~

::

    static void emit_instr(parser_ctx_t* ctx, u32 instr) {
        ctx->current_addr += 4;           // Advance location counter
        if (ctx->in_text_section && ctx->text_size < MAX_INSTRUCTIONS) {
            ctx->text[ctx->text_size++] = instr;  // Store instruction
        }
    }

3. Relocation Handling
----------------------

Forward References
~~~~~~~~~~~~~~~~~~

When a label is referenced before it's defined, the assembler:

1. Records a relocation entry
2. Emits a placeholder instruction
3. Fixes up the instruction after all labels are defined

::

    start:
        b loop      ; loop not defined yet!

    loop:           ; loop = 0x04
        mov r0, r1

Relocation Structure
~~~~~~~~~~~~~~~~~~~~

::

    typedef struct {
        u32 address;          // Address to fix up
        char name[64];        // Label name to resolve
        int is_branch;        // 1 for branch, 0 for literal pool
    } reloc_t;

Branch Relocation
~~~~~~~~~~~~~~~~~

Branch instructions use PC-relative offsets:

::

    signed_offset = (target_addr - current_addr - 4) / 4

    u32* patch_addr = &ctx->text[ctx.relocs[j].address / 4];
    signed int signed_offset = (addr - (int)ctx.relocs[j].address - 4) / 4;
    *patch_addr = (*patch_addr & 0xFFF0FFFF) | (offset & 0xFFFFF);

Two-Pass Resolution
~~~~~~~~~~~~~~~~~~~

::

    ┌─────────────────────────────────────────────────────────────┐
    │                      TWO-PASS ASSEMBLY                       │
    ├─────────────────────────────────────────────────────────────┤
    │                                                              │
    │  PASS 1:                                                     │
    │  ───────                                                     │
    │  1. Tokenize source                                          │
    │  2. Parse instructions                                       │
    │  3. Record label addresses                                   │
    │  4. Emit instructions with relocations for unknown labels    │
    │  5. Build literal pool                                       │
    │                                                              │
    │  PASS 2:                                                     │
    │  ───────                                                     │
    │  1. Resolve all relocations                                  │
    │  2. Fix up branch offsets                                    │
    │  3. Fix up literal pool references                           │
    │  4. Emit final binary                                        │
    │                                                              │
    └─────────────────────────────────────────────────────────────┘

4. Literal Pool Emission
------------------------

The literal pool stores 32-bit constants that cannot be encoded as
 immediates in the instruction itself:

::

    ldr r0, =0x12345678    ; 0x12345678 doesn't fit in 12-bit operand
                           ; Generated as:
                           ;   ldr r0, [pc, #offset]  ; load from pool
                           ;   pool: .word 0x12345678  ; constant in pool

When to Emit Pool
~~~~~~~~~~~~~~~~~

The literal pool is emitted after all parsing is complete:

::

    emit_literal_pool(&ctx);

Pool Structure
~~~~~~~~~~~~~~

::

    typedef struct {
        u32 value;           // The constant value
        u32 offset;          // Offset in text section (bytes)
    } literal_pool_entry_t;

    typedef struct {
        u32 instr_addr;      // Address of LDR instruction
        int pool_index;      // Index into literal_pool[]
    } literal_pool_ref_t;

Pool Emission Algorithm
~~~~~~~~~~~~~~~~~~~~~~~

::

    static void emit_literal_pool(parser_ctx_t* ctx) {
        u32 pool_start = ctx->text_size * 4;

        // Emit pool entries
        for (int i = 0; i < ctx->literal_pool_count; i++) {
            ctx->literal_pool[i].offset = pool_start + i * 4;
            ctx->text[ctx->text_size++] = ctx->literal_pool[i].value;
        }

        // Fix up LDR instructions with pool offsets
        for (int j = 0; j < ctx->literal_pool_ref_count; j++) {
            u32 instr_addr = ctx->literal_pool_refs[j].instr_addr / 4;
            int pool_idx = ctx->literal_pool_refs[j].pool_index;

            u32 pool_offset = ctx->literal_pool[pool_idx].offset;
            int byte_offset = (int)pool_offset - (int)(instr_addr * 4 + 4);
            u32 offset_val = byte_offset;

            u32 old_instr = ctx->text[instr_addr];
            u32 new_instr = (ctx->text[instr_addr] & 0xFFFFF000) | (offset_val & 0xFFF);
            ctx->text[instr_addr] = new_instr;
        }
    }

Pool Layout
~~~~~~~~~~~

::

    ┌─────────────────────────────────────────────────────────┐
    │                   LITERAL POOL                          │
    ├─────────────────────────────────────────────────────────┤
    │                                                          │
    │  Instructions...                                         │
    │  ┌──────────────────────────────────────────────────┐   │
    │  │  ldr r0, [pc, #12]      ; LDR with pool offset   │   │
    │  │  ldr r1, [pc, #16]      ; LDR with pool offset   │   │
    │  ├──────────────────────────────────────────────────┤   │
    │  │  POOL START                                     │   │
    │  │  ┌────────────┐                                 │   │
    │  │  │ 0x12345678 │ pool[0] offset=0x100            │   │
    │  │  │ 0x0000ABCD │ pool[1] offset=0x104            │   │
    │  │  │ 0xDEADBEEF │ pool[2] offset=0x108            │   │
    │  │  └────────────┘                                 │   │
    │  └──────────────────────────────────────────────────┘   │
    │                                                          │
    └─────────────────────────────────────────────────────────┘

5. Output Generation
--------------------

The assembler writes a ``.varm`` file containing:

1. 32-byte header
2. Text section (encoded instructions)
3. Data section (initialized data)

Varm File Format
~~~~~~~~~~~~~~~~

::

    ┌─────────────────────────────────────────────────────────────┐
    │                      .VARM FILE FORMAT                      │
    ├─────────────────────────────────────────────────────────────┤
    │  Header (32 bytes)                                          │
    │  ┌────────┬──────────────┬───────────────────────────────┐  │
    │  │ Offset │ Size         │ Description                   │  │
    │  ├────────┼──────────────┼───────────────────────────────┤  │
    │  │ 0x00   │ 4 bytes      │ Magic: 'V' 'A' 'R' 'M'        │  │
    │  │ 0x04   │ 4 bytes      │ text_offset (always 32)       │  │
    │  │ 0x08   │ 4 bytes      │ text_size (bytes)             │  │
    │  │ 0x0C   │ 4 bytes      │ data_offset                   │  │
    │  │ 0x10   │ 4 bytes      │ data_size (bytes)             │  │
    │  │ 0x14   │ 4 bytes      │ entry point                   │  │
    │  │ 0x18   │ 8 bytes      │ Reserved (0)                  │  │
    │  └────────┴──────────────┴───────────────────────────────┘  │
    │                                                              │
    │  Text Section (text_size bytes)                              │
    │  ┌──────────────────────────────────────────────────────┐   │
    │  │ Instruction 0 (4 bytes) little-endian                 │   │
    │  │ Instruction 1 (4 bytes) little-endian                 │   │
    │  │ ...                                                    │   │
    │  │ Instruction N-1 (4 bytes) little-endian               │   │
    │  └──────────────────────────────────────────────────────┘   │
    │                                                              │
    │  Data Section (data_size bytes)                              │
    │  ┌──────────────────────────────────────────────────────┐   │
    │  │ .word data (4 bytes each)                             │   │
    │  │ .byte data (1 byte each)                              │   │
    │  │ ...                                                    │   │
    │  └──────────────────────────────────────────────────────┘   │
    │                                                              │
    └─────────────────────────────────────────────────────────────┘

Header Writing
~~~~~~~~~~~~~~

::

    int write_vm_file(program_state_t* prog, const char* filename) {
        FILE* f = fopen(filename, "wb");

        char header[32] = {'V', 'A', 'R', 'M'};
        u32 text_offset = 32;
        u32 text_total_size = prog->text_size * 4;
        u32 data_offset = text_offset + text_total_size;
        u32 entry = text_offset;

        *(u32*)&header[4] = text_offset;
        *(u32*)&header[8] = text_total_size;
        *(u32*)&header[12] = data_offset;
        *(u32*)&header[16] = prog->data_size;
        *(u32*)&header[20] = entry;
        *(u32*)&header[24] = 0;
        *(u32*)&header[28] = 0;

        fwrite(header, 1, 32, f);
        fwrite(prog->text, 4, prog->text_size, f);
        fwrite(prog->data, 1, prog->data_size, f);

        fclose(f);
    }

6. Complexity Analysis
----------------------

Instruction Encoding
~~~~~~~~~~~~~~~~~~~~

**Time**: O(1) per instruction
- Each instruction encoding is a fixed sequence of shifts and ORs
- No loops or complex operations

**Space**: O(1) per instruction
- One 32-bit word per instruction
- Maximum: 4096 instructions * 4 bytes = 16KB

Output Generation
~~~~~~~~~~~~~~~~~

**Time**: O(n) where n = output size
- Each byte is written exactly once
- Linear scan of text[] and data[] arrays

**Space**: O(1) beyond output buffer
- No additional allocation during output

Relocation Resolution
~~~~~~~~~~~~~~~~~~~~

**Time**: O(r) where r = number of relocations
- Each relocation processed once
- Each involves a symbol lookup (O(1) average)

**Space**: O(r) for reloc array
- Maximum: 4096 relocations * ~72 bytes = 288KB

Literal Pool
~~~~~~~~~~~~

**Time**: O(p + r) where:
- p = pool entries
- r = pool references

**Space**: O(p) for pool entries
- Maximum: 256 pool entries * 8 bytes = 2KB

.. warning::

   The 12-bit offset field in load/store instructions limits the
   literal pool to a 4KB window (4096 bytes / 4 = 1024 pool entries).
   Pool entries outside this range will produce incorrect offsets.
