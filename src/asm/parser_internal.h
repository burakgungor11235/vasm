/*
 * varm Assembler - Internal Parser Header
 *
 * Parser context and handler function declarations.
 *
 * ENDIANNESS:
 * ===========
 * All multi-byte values are stored in LITTLE-ENDIAN byte order.
 * This means the least significant byte is stored at the lowest address.
 *
 * INSTRUCTION FORMAT (32 bits):
 * =============================
 *   31:24       23:20      19:16      15:12      11:0
 *  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐
 *  │ Opcode  │  Cond   │   Rn    │   Rd    │   Operand   │
 *  │  8 bit  │  4 bit  │  4 bit  │  4 bit  │   12 bit    │
 *  └─────────┴─────────┴─────────┴─────────┴─────────────┘
 *
 * BYTE LAYOUT (little-endian):
 *   Address+0: Operand[7:0]  (bits 0-7)
 *   Address+1: Operand[11:8] + Rd[3:0] (bits 8-15)
 *   Address+2: Rd[7:4] + Rn[3:0] (bits 16-23)
 *   Address+3: Cond[3:0] + Opcode[7:0] (bits 24-31)
 *
 * PARSING ARCHITECTURE:
 * =====================
 *   Source → Tokenize → Parse → Emit Binary → Write .varm
 *                  │
 *                  └──→ Symbol Table (O(1) lookup via hash table)
 *                                      │
 *                                      └──→ Relocations (forward refs)
 */

#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "../../include/vm.h"
#include "../../include/assembler.h"
#include "symbol_table.h"
#include "error.h"
#include "lookup_tables.h"

typedef struct {
    u32 text[MAX_INSTRUCTIONS];
    u32 text_size;
    u8 data[65536];
    u32 data_size;
    symbol_table_t labels;
    reloc_t relocs[MAX_INSTRUCTIONS];
    int reloc_count;
    literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES];
    int literal_pool_count;
    literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS];
    int literal_pool_ref_count;
    u32 current_addr;
    int in_text_section;
    error_context_t* errors;
} parser_ctx_t;

#endif
