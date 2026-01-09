/*
 * varm Assembler - Parser
 *
 * Converts tokens to machine code instructions.
 *
 * INSTRUCTION FORMAT (32 bits, little-endian):
 * ============================================
 *   31:24       23:20      19:16      15:12      11:0
 *  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐
 *  │ Opcode  │  Cond   │   Rn    │   Rd    │   Operand   │
 *  │  8 bit  │  4 bit  │  4 bit  │  4 bit  │   12 bit    │
 *  └─────────┴─────────┴─────────┴─────────┴─────────────┘
 *
 * MEMORY LAYOUT (little-endian at address A):
 *   A+0:  operand[7:0]
 *   A+1:  operand[11:8] | rd[3:0]
 *   A+2:  rd[7:4] | rn[3:0]
 *   A+3:  cond[3:0] | opcode[7:0]
 *
 * PARSING FLOW:
 * =============
 *   1. Tokenize source code (lexer.c)
 *   2. Parse tokens into instruction binary
 *   3. Track labels in symbol table (O(1) hash lookup)
 *   4. Handle relocations for forward references
 *   5. Emit literal pool for ldr rd, =label
 *   6. Write .varm file (little-endian)
 *
 * EXAMPLE: mov r0, #42
 *   - opcode = 0x00 (OP_MOV)
 *   - cond = 0xE (COND_AL)
 *   - rd = 0 (r0)
 *   - operand = (1 << 11) | 42 = 0x82A
 *   - encoding = (0x00<<24) | (0xE<<20) | (0<<12) | 0x82A = 0x00E0082A
 *   - bytes: 2A 08 E0 00 (little-endian)
 *
 * HANDLER FUNCTIONS:
 * ==================
 *   parse_directive()   - .text, .data, .word, .byte, .equ
 *   parse_move()        - MOV, MVN
 *   parse_alu()         - ADD, SUB, AND, ORR, CMP, CMN, TST, TEQ
 *   parse_mult()        - MUL, MLA
 *   parse_load_store()  - LDR, LDRB, STR, STRB
 *   parse_branch()      - B, BL
 *   parse_system()      - HALT, NOP, SWI
 *   parse_pseudo_ldr()  - ldr rd, =label
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "../../include/vm.h"
#include "../../include/assembler.h"
#include "../../include/opcode.h"
#include "parser_internal.h"
#include "lookup_tables.h"

int asm_debug = 0;

#define ASM_DEBUG(fmt, ...)                                                                        \
    do {                                                                                           \
	if (asm_debug) {                                                                           \
	    fprintf(stderr, "[ASM] " fmt, ##__VA_ARGS__);                                          \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_LABEL(fmt, ...)                                                                  \
    do {                                                                                           \
	if (asm_debug) {                                                                           \
	    fprintf(stderr, "[LABEL] " fmt, ##__VA_ARGS__);                                        \
	}                                                                                          \
    } while (0)

#define ASM_DEBUG_POOL(fmt, ...)                                                                   \
    do {                                                                                           \
	if (asm_debug) {                                                                           \
	    fprintf(stderr, "[POOL] " fmt, ##__VA_ARGS__);                                         \
	}                                                                                          \
    } while (0)

static int
get_register(const char* name)
{
    if (strlen(name) == 2 && name[0] == 'r' && isdigit(name[1])) {
	return name[1] - '0';
    }
    if (strlen(name) == 3 && name[0] == 'r' && isdigit(name[1]) && isdigit(name[2])) {
	return (name[1] - '0') * 10 + (name[2] - '0');
    }
    if (strcasecmp(name, "sp") == 0)
	return 13;
    if (strcasecmp(name, "lr") == 0)
	return 14;
    if (strcasecmp(name, "pc") == 0)
	return 15;
    return -1;
}

static u32
parse_immediate(const char* value)
{
    u32 result = 0;

    if (strncasecmp(value, "0x", 2) == 0) {
	sscanf(value + 2, "%x", &result);
    } else if (strncasecmp(value, "0b", 2) == 0) {
	result = strtoul(value + 2, NULL, 2);
    } else {
	result = strtoul(value, NULL, 10);
    }

    u8 rotate = 0;
    while (rotate < 16 && (result & 0xFF000000)) {
	result = (result >> 2) | ((result & 0x3) << 30);
	rotate++;
    }

    return (rotate << 8) | (result & 0xFF);
}

static void
emit_instr(parser_ctx_t* ctx, u32 instr)
{
    ctx->current_addr += 4;
    if (ctx->in_text_section && ctx->text_size < MAX_INSTRUCTIONS) {
	ctx->text[ctx->text_size++] = instr;
    }
}

static int
add_label(parser_ctx_t* ctx, const char* name, u32 addr)
{
    return symbol_insert(&ctx->labels, name, addr);
}

static int
lookup_label(parser_ctx_t* ctx, const char* name)
{
    u32 addr;
    if (symbol_lookup(&ctx->labels, name, &addr)) {
	return (int)addr;
    }
    return -1;
}

static void
add_reloc(parser_ctx_t* ctx, const char* name, u32 addr, int is_branch)
{
    if (ctx->reloc_count < MAX_INSTRUCTIONS) {
	strncpy(ctx->relocs[ctx->reloc_count].name, name, 63);
	ctx->relocs[ctx->reloc_count].address = addr;
	ctx->relocs[ctx->reloc_count].is_branch = is_branch;
	ctx->reloc_count++;
    }
}

static u32
add_literal_pool_entry(parser_ctx_t* ctx, u32 value)
{
    for (int i = 0; i < ctx->literal_pool_count; i++) {
	if (ctx->literal_pool[i].value == value) {
	    return ctx->literal_pool[i].offset;
	}
    }

    if (ctx->literal_pool_count < MAX_LITERAL_POOL_ENTRIES) {
	ctx->literal_pool[ctx->literal_pool_count].value = value;
	ctx->literal_pool[ctx->literal_pool_count].offset = 0;
	ctx->literal_pool_count++;
	return ctx->literal_pool_count - 1;
    }

    return 0;
}

static void
emit_literal_pool(parser_ctx_t* ctx)
{
    u32 pool_start = ctx->text_size * 4;

    ASM_DEBUG_POOL("emit_literal_pool: text_size=%u, pool_start=%u, count=%d\n", ctx->text_size,
                   pool_start, ctx->literal_pool_count);

    for (int i = 0; i < ctx->literal_pool_count; i++) {
	ctx->literal_pool[i].offset = pool_start + i * 4;
	ctx->text[ctx->text_size++] = ctx->literal_pool[i].value;

	ASM_DEBUG_POOL("  pool[%d]: value=0x%08X, offset=%u\n", i, ctx->literal_pool[i].value,
	               ctx->literal_pool[i].offset);
    }

    for (int j = 0; j < ctx->literal_pool_ref_count; j++) {
	u32 instr_addr = ctx->literal_pool_refs[j].instr_addr / 4;
	int pool_idx = ctx->literal_pool_refs[j].pool_index;

	if (pool_idx >= 0 && pool_idx < ctx->literal_pool_count) {
	    u32 pool_offset = ctx->literal_pool[pool_idx].offset;
	    int byte_offset = (int)pool_offset - (int)(instr_addr * 4 + 4);
	    u32 offset_val = byte_offset;

	    u32 old_instr = ctx->text[instr_addr];
	    u32 new_instr = (ctx->text[instr_addr] & 0xFFFFF000) | (offset_val & 0xFFF);
	    ctx->text[instr_addr] = new_instr;

	    ASM_DEBUG_POOL("  fixup LDR at %u: pool_offset=%u, byte_offset=%d, offset=%u\n",
	                   instr_addr * 4, pool_offset, byte_offset, offset_val);
	}
    }
}

static int
parse_directive(parser_ctx_t* ctx, const char* dir, token_t* tokens, int* i, int token_count)
{
    if (strcasecmp(dir, ".text") == 0) {
	ctx->in_text_section = 1;
	ctx->current_addr = 0;
	return 0;
    }

    if (strcasecmp(dir, ".data") == 0) {
	ctx->in_text_section = 0;
	ctx->current_addr = 0x10000;
	return 0;
    }

    if (strcasecmp(dir, ".word") == 0) {
	while (*i < token_count && tokens[*i].type != TOKEN_NEWLINE &&
	       tokens[*i].type != TOKEN_EOF) {
	    if (tokens[*i].type == TOKEN_IMMEDIATE) {
		u32 val = strtoul(tokens[*i].value, NULL, 0);
		if (ctx->data_size < 65536) {
		    ctx->data[ctx->data_size++] = val & 0xFF;
		    ctx->data[ctx->data_size++] = (val >> 8) & 0xFF;
		    ctx->data[ctx->data_size++] = (val >> 16) & 0xFF;
		    ctx->data[ctx->data_size++] = (val >> 24) & 0xFF;
		    ctx->current_addr += 4;
		}
	    }
	    (*i)++;
	}
	return 0;
    }

    if (strcasecmp(dir, ".byte") == 0) {
	while (*i < token_count && tokens[*i].type != TOKEN_NEWLINE &&
	       tokens[*i].type != TOKEN_EOF) {
	    if (tokens[*i].type == TOKEN_IMMEDIATE) {
		u32 val = strtoul(tokens[*i].value, NULL, 0);
		if (ctx->data_size < 65536) {
		    ctx->data[ctx->data_size++] = val & 0xFF;
		    ctx->current_addr++;
		}
	    }
	    (*i)++;
	}
	return 0;
    }

    if (strcasecmp(dir, ".equ") == 0 || strcasecmp(dir, ".set") == 0) {
	if (*i + 2 < token_count && tokens[*i + 1].type == TOKEN_IDENTIFIER) {
	    u32 val = 0;
	    if (*i + 3 < token_count && tokens[*i + 2].type == TOKEN_COMMA) {
		if (*i + 4 < token_count && tokens[*i + 3].type == TOKEN_IMMEDIATE) {
		    val = strtoul(tokens[*i + 3].value, NULL, 0);
		}
	    } else if (*i + 3 < token_count && tokens[*i + 2].type == TOKEN_IMMEDIATE) {
		val = strtoul(tokens[*i + 2].value, NULL, 0);
	    }
	    add_label(ctx, tokens[*i + 1].value, val);
	}
	while (*i < token_count && tokens[*i].type != TOKEN_NEWLINE && tokens[*i].type != TOKEN_EOF)
	    (*i)++;
	return 0;
    }

    return 0;
}

static int
parse_operand(token_t* tokens, int* i, int token_count, u32* imm_value, int* reg_value)
{
    if (*i >= token_count)
	return -1;

    if (tokens[*i].type == TOKEN_HASH) {
	(*i)++;
	if (*i < token_count && tokens[*i].type == TOKEN_IMMEDIATE) {
	    *imm_value = parse_immediate(tokens[*i].value);
	    (*i)++;
	    return 1;
	}
    } else if (tokens[*i].type == TOKEN_IMMEDIATE) {
	*imm_value = parse_immediate(tokens[*i].value);
	(*i)++;
	return 1;
    } else if (tokens[*i].type == TOKEN_IDENTIFIER) {
	int reg = get_register(tokens[*i].value);
	if (reg >= 0) {
	    *reg_value = reg;
	    (*i)++;
	    return 0;
	}
	*imm_value = parse_immediate(tokens[*i].value);
	(*i)++;
	return 1;
    }

    return -1;
}

static int
parse_move(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
           const char* condition)
{
    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rd = get_register(tokens[*i].value);
    (*i)++;

    if (*i < token_count && tokens[*i].type == TOKEN_COMMA)
	(*i)++;

    u32 operand_immed = 0;
    int operand_reg = -1;
    int imm_type = parse_operand(tokens, i, token_count, &operand_immed, &operand_reg);
    if (imm_type < 0)
	return -1;

    u32 operand;
    u32 instr;
    u8 cond = parse_condition(condition);

    if (operand_reg >= 0) {
	operand = operand_reg;
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rd << RD_SHIFT) | operand;
    } else {
	operand = (1 << 11) | (operand_immed & 0xFFF);
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rd << RD_SHIFT) | operand;
    }

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_branch(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
             const char* condition)
{
    u32 branch_addr = ctx->current_addr;
    u32 offset = 0;

    if (*i < token_count && tokens[*i].type == TOKEN_IDENTIFIER) {
	ASM_DEBUG("BRANCH: Adding reloc for '%s' at addr 0x%X\n", tokens[*i].value, branch_addr);
	add_reloc(ctx, tokens[*i].value, branch_addr, opcode == OP_BL || opcode == OP_B);
	offset = 0;
	(*i)++;
    } else if (*i < token_count && tokens[*i].type == TOKEN_IMMEDIATE) {
	offset = strtoul(tokens[*i].value, NULL, 0);
	(*i)++;
    }

    u8 cond = parse_condition(condition);
    u32 instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (offset & 0xFFFFF);

    ASM_DEBUG("BRANCH: after encoding instr=0x%X opcode=%d cond=%d offset=%d\n", instr, opcode,
              cond, offset);

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_system(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
             const char* condition)
{
    u32 offset = 0;

    if (*i < token_count && (tokens[*i].type == TOKEN_HASH || tokens[*i].type == TOKEN_IMMEDIATE)) {
	if (tokens[*i].type == TOKEN_HASH) {
	    (*i)++;
	    if (*i < token_count && tokens[*i].type == TOKEN_IMMEDIATE) {
		offset = strtoul(tokens[*i].value, NULL, 0);
		(*i)++;
	    }
	} else {
	    offset = strtoul(tokens[*i].value, NULL, 0);
	    (*i)++;
	}
    }

    u8 cond = parse_condition(condition);
    u32 instr;
    if (opcode == OP_SWI) {
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (offset & OFFSET_MASK);
    } else {
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT);
    }

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_alu(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
          const char* condition)
{
    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER) {
	return -1;
    }

    u8 rd = get_register(tokens[*i].value);
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_COMMA) {
	return -1;
    }
    (*i)++;

    if (*i >= token_count) {
	return -1;
    }

    u8 rn = 0;
    int operand_reg = -1;
    u32 operand_immed = 0;

    if (tokens[*i].type == TOKEN_IDENTIFIER) {
	rn = get_register(tokens[*i].value);
	(*i)++;
    } else if (tokens[*i].type == TOKEN_HASH) {
	(*i)++;
	if (*i < token_count && tokens[*i].type == TOKEN_IMMEDIATE) {
	    operand_immed = parse_immediate(tokens[*i].value);
	    (*i)++;
	}
    } else if (tokens[*i].type == TOKEN_IMMEDIATE) {
	operand_immed = parse_immediate(tokens[*i].value);
	(*i)++;
    }

    if (*i < token_count && tokens[*i].type == TOKEN_COMMA) {
	(*i)++;
	int imm_type = parse_operand(tokens, i, token_count, &operand_immed, &operand_reg);
	if (imm_type < 0)
	    return -1;
    }

    u8 cond = parse_condition(condition);
    u32 operand;
    u32 instr;

    if (opcode == OP_CMP || opcode == OP_CMN || opcode == OP_TST || opcode == OP_TEQ) {
	rd = 0;
    }

    if (operand_reg >= 0) {
	operand = operand_reg;
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rn << RN_SHIFT) |
	        (rd << RD_SHIFT) | operand;
    } else {
	operand = (1 << 11) | (operand_immed & 0xFFF);
	instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rn << RN_SHIFT) |
	        (rd << RD_SHIFT) | operand;
    }

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_mult(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
           const char* condition)
{
    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rd = get_register(tokens[*i].value);
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_COMMA)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rm = get_register(tokens[*i].value);
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_COMMA)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rs = get_register(tokens[*i].value);
    (*i)++;

    u8 rn = 0;
    if (opcode == OP_MLA) {
	if (*i >= token_count || tokens[*i].type != TOKEN_COMMA)
	    return -1;
	(*i)++;

	if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	    return -1;

	rn = get_register(tokens[*i].value);
	(*i)++;
    }

    u8 cond = parse_condition(condition);
    u32 instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rn << RN_SHIFT) |
                (rd << RD_SHIFT) | (rm << 8) | (rs << 4);

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_load_store(parser_ctx_t* ctx, int opcode, token_t* tokens, int* i, int token_count,
                 const char* condition)
{
    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rt = get_register(tokens[*i].value);
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_COMMA)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_LBRACKET)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rn = get_register(tokens[*i].value);
    (*i)++;

    u32 offset = 0;
    if (*i < token_count && tokens[*i].type == TOKEN_COMMA) {
	(*i)++;
	if (*i < token_count && tokens[*i].type == TOKEN_HASH) {
	    (*i)++;
	    if (*i < token_count && tokens[*i].type == TOKEN_IMMEDIATE) {
		offset = strtoul(tokens[*i].value, NULL, 0);
		(*i)++;
	    }
	}
    }

    if (*i >= token_count || tokens[*i].type != TOKEN_RBRACKET)
	return -1;
    (*i)++;

    u8 cond = parse_condition(condition);
    offset = offset & 0xFFF;
    u32 instr = (opcode << OPCODE_SHIFT) | (cond << COND_SHIFT) | (rn << RN_SHIFT) |
                (rt << RD_SHIFT) | offset;

    emit_instr(ctx, instr);
    return 0;
}

static int
parse_pseudo_ldr(parser_ctx_t* ctx, token_t* tokens, int* i, int token_count, const char* condition)
{
    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    u8 rd = get_register(tokens[*i].value);
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_COMMA)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_EQUAL)
	return -1;
    (*i)++;

    if (*i >= token_count || tokens[*i].type != TOKEN_IDENTIFIER)
	return -1;

    const char* label_name = tokens[*i].value;
    int label_addr = lookup_label(ctx, label_name);

    ASM_DEBUG_LABEL("=label pseudo-instr: label='%s' addr=0x%X rd=%d\n", label_name, label_addr,
                    rd);

    u32 instr = 0;
    u8 cond = parse_condition(condition);
    u32 pool_index = add_literal_pool_entry(ctx, label_addr >= 0 ? (u32)label_addr : 0);

    if (label_addr < 0) {
	add_reloc(ctx, label_name, pool_index, 0);
    }

    if (ctx->literal_pool_ref_count < MAX_LITERAL_POOL_REFS) {
	ctx->literal_pool_refs[ctx->literal_pool_ref_count].instr_addr = ctx->text_size * 4;
	ctx->literal_pool_refs[ctx->literal_pool_ref_count].pool_index = pool_index;
	ctx->literal_pool_ref_count++;
    }

    instr = (OP_LDR << OPCODE_SHIFT) | (cond << COND_SHIFT) | (15 << RN_SHIFT) | (rd << RD_SHIFT);
    emit_instr(ctx, instr);

    ASM_DEBUG_LABEL("=label: pool_index=%u, instr_addr=%u\n", pool_index, ctx->text_size * 4 - 4);

    (*i)++;
    return 0;
}

/*
 * parse()
 * =======
 * Main parsing entry point.
 *
 * PARSING ALGORITHM:
 *   1. Initialize parser context (symbol table, empty sections)
 *   2. Iterate through tokens:
 *      - NEWLINE: Skip (whitespace)
 *      - LABEL: Add to symbol table with current address
 *      - DIRECTIVE: Pass to parse_directive()
 *      - INSTRUCTION: Dispatch to appropriate handler:
 *          ├─ B/BL → parse_branch()
 *          ├─ HALT/NOP/SWI → parse_system()
 *          ├─ MOV/MVN → parse_move()
 *          ├─ CMP/CMN/TST/TEQ → parse_alu()
 *          ├─ MUL/MLA → parse_mult()
 *          ├─ LDR (=label) → parse_pseudo_ldr()
 *          ├─ LDR/STR variants → parse_load_store()
 *          └─ Other → parse_alu() (ADD, SUB, etc.)
 *   3. After all tokens: Resolve relocations (fix up addresses)
 *   4. Emit literal pool entries
 *   5. Copy results to program_state_t output
 *
 * OUTPUT FORMAT (.varm file):
 *   ┌─────────────────────────────────────┐
 *   │ Header (32 bytes)                   │
 *   │   - Magic: "VARM"                   │
 *   │   - text_offset: 32                 │
 *   │   - text_size: text_size * 4        │
 *   │   - data_offset: 32 + text_size*4   │
 *   │   - data_size                       │
 *   ├─────────────────────────────────────┤
 *   │ Text Section (4 bytes per instr)    │
 *   │   - Encoded instructions            │
 *   │   - Little-endian byte order        │
 *   ├─────────────────────────────────────┤
 *   │ Data Section                        │
 *   │   - .word, .byte directives         │
 *   └─────────────────────────────────────┘
 */

int
parse(token_t* tokens, int token_count, program_state_t* prog)
{
    parser_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    symbol_init(&ctx.labels, 256);
    ctx.errors = NULL;
    ctx.text_size = 0;
    ctx.data_size = 0;
    ctx.reloc_count = 0;
    ctx.literal_pool_count = 0;
    ctx.literal_pool_ref_count = 0;
    ctx.current_addr = 0;
    ctx.in_text_section = 1;

    for (int i = 0; i < token_count && tokens[i].type != TOKEN_EOF; i++) {
	if (tokens[i].type == TOKEN_NEWLINE) {
	    continue;
	}

	if (tokens[i].type == TOKEN_LABEL) {
	    add_label(&ctx, tokens[i].value, ctx.current_addr);
	    continue;
	}

	if (tokens[i].type == TOKEN_DIRECTIVE) {
	    char* dir = tokens[i].value;
	    i++;
	    parse_directive(&ctx, dir, tokens, &i, token_count);
	    continue;
	}

	if (tokens[i].type == TOKEN_INSTRUCTION) {
	    char instr_name_buf[64];
	    strncpy(instr_name_buf, tokens[i].value, 63);
	    char* instr_name = instr_name_buf;
	    char* condition = NULL;

	    char* dot = strchr(instr_name, '.');
	    if (dot != NULL) {
		*dot = '\0';
		condition = dot + 1;
	    } else if (strlen(instr_name) > 2 && instr_name[0] == 'b') {
		static const char* suffixes[] = {"eq", "ne", "cs", "hs", "cc", "lo", "mi", "pl",
		                                 "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le"};
		for (int c = 0; c < 16; c++) {
		    if (strcasecmp(instr_name + 1, suffixes[c]) == 0) {
			instr_name = "b";
			condition = (char*)suffixes[c];
			break;
		    }
		}
	    }

	    int opcode = lookup_opcode(instr_name);
	    i++;

	    if (opcode < 0) {
		continue;
	    }

	    if (opcode == OP_B || opcode == OP_BL) {
		parse_branch(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_HALT || opcode == OP_NOP || opcode == OP_SWI) {
		parse_system(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_MOV || opcode == OP_MVN) {
		parse_move(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_CMP || opcode == OP_CMN || opcode == OP_TST || opcode == OP_TEQ) {
		parse_alu(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_MUL || opcode == OP_MLA) {
		parse_mult(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_LDR && i + 3 < token_count && tokens[i + 1].type == TOKEN_COMMA &&
	        tokens[i + 2].type == TOKEN_EQUAL && tokens[i + 3].type == TOKEN_IDENTIFIER) {
		parse_pseudo_ldr(&ctx, tokens, &i, token_count, condition);
		continue;
	    }

	    if (opcode == OP_LDR || opcode == OP_LDRB || opcode == OP_STR || opcode == OP_STRB) {
		parse_load_store(&ctx, opcode, tokens, &i, token_count, condition);
		continue;
	    }

	    parse_alu(&ctx, opcode, tokens, &i, token_count, condition);
	}
    }

    for (int j = 0; j < ctx.reloc_count; j++) {
	int addr = lookup_label(&ctx, ctx.relocs[j].name);
	if (addr >= 0) {
	    if (ctx.relocs[j].is_branch) {
		u32* patch_addr = &ctx.text[ctx.relocs[j].address / 4];
		signed int signed_offset = (addr - (int)ctx.relocs[j].address - 4) / 4;
		u32 offset = (u32)signed_offset;
		*patch_addr = (*patch_addr & 0xFFF0FFFF) | (offset & 0xFFFFF);
	    } else {
		ctx.literal_pool[ctx.relocs[j].address].value = (u32)addr;
	    }
	}
    }

    emit_literal_pool(&ctx);

    prog->text_size = ctx.text_size;
    prog->data_size = ctx.data_size;
    memcpy(prog->text, ctx.text, sizeof(ctx.text));
    memcpy(prog->data, ctx.data, sizeof(ctx.data));

    prog->label_count = 0;
    for (u32 i = 0; i < ctx.labels.size && prog->label_count < MAX_LABELS; i++) {
	if (ctx.labels.entries[i].defined) {
	    strncpy(prog->labels[prog->label_count].name, ctx.labels.entries[i].name, 63);
	    prog->labels[prog->label_count].address = ctx.labels.entries[i].address;
	    prog->label_count++;
	}
    }

    symbol_destroy(&ctx.labels);

    return 0;
}

int
write_vm_file(program_state_t* prog, const char* filename)
{
    FILE* f = fopen(filename, "wb");
    if (f == NULL) {
	return -1;
    }

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
    return 0;
}

int
assemble(const char* input_file, const char* output_file)
{
    FILE* f = fopen(input_file, "r");
    if (f == NULL) {
	fprintf(stderr, "Cannot open file: %s\n", input_file);
	return -1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = malloc(size + 1);
    if (buffer == NULL) {
	fclose(f);
	return -1;
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';
    fclose(f);

    token_t tokens[4096];
    int token_count = tokenize(buffer, tokens, 4096);
    free(buffer);

    program_state_t prog;
    memset(&prog, 0, sizeof(prog));

    parse(tokens, token_count, &prog);
    write_vm_file(&prog, output_file);

    printf("Assembled: %lu bytes of code, %lu bytes of data\n", (unsigned long)prog.text_size * 4,
           (unsigned long)prog.data_size);
    printf("Labels: %d\n", prog.text_size);

    return 0;
}
