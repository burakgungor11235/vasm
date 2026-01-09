/**
 * @file parser.c
 * @brief Main parser for the varm assembler - converts tokens to machine code instructions.
 *
 * @details This module implements the core parsing logic for the varm assembler. It takes
 * tokenized input from the lexer and produces encoded machine code instructions. The parser
 * handles instruction encoding, label resolution, literal pool management, and relocation.
 *
 * The parser implements a multi-pass approach:
 * - Pass 1: Token classification and instruction dispatch
 * - Pass 2: Label resolution and relocation fixups
 * - Pass 3: Literal pool emission
 *
 * @author varm Development Team
 * @version 0.1.0
 *
 * @warning This API is not stable. Function signatures and behavior may change.
 * @note Instruction encoding is little-endian. Maximum instruction count is 4096.
 *
 * @see lexer.c Tokenization
 * @see symbol_table.c Label management
 * @see assembler.h Public API
 *
 * INSTRUCTION FORMAT (32 bits, little-endian):
 * ============================================
 *   31:24       23:20      19:16      15:12      11:0
 *  ┌─────────┬─────────┬─────────┬─────────┬─────────────┐
 *  │ Opcode  │  Cond   │   Rn    │   Rd    │   Operand   │
 *  │  8 bit  │  4 bit  │  4 bit  │  4 bit  │   12 bit    │
 *  └─────────┴─────────┴─────────┴─────────┴─────────────┘
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

/**
 * @brief Converts a register name string to its numeric index.
 *
 * @details Parses register names in the following formats:
 *   - Single-digit: r0-r9
 *   - Double-digit: r10-r15
 *   - Special names: sp (r13), lr (r14), pc (r15)
 *
 * @param name The register name string to parse
 * @return int The register index (0-15) on success, -1 on failure
 *
 * @note Case-insensitive comparison for special register names
 * @warning Invalid register names silently return -1
 */
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

/**
 * @brief Parses an immediate value from a string literal.
 *
 * @details Supports multiple numeric formats:
 *   - Decimal: "42"
 *   - Hexadecimal: "0x2A"
 *   - Binary: "0b101010"
 *
 * After parsing, the value is rotated right by 2 bits until it fits in 8 bits.
 * The rotation count is encoded in bits 8-15 of the return value.
 *
 * @param value The string representation of the immediate value
 * @return u32 The encoded immediate value (8-bit value in bits 0-7, rotation in bits 8-15)
 *
 * @note Complexity: O(n) where n is the number of rotation steps (max 16)
 * @warning Large values that cannot be rotated into 8 bits are truncated
 */
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

/**
 * @brief Emits an encoded instruction to the text section.
 *
 * @details Adds the instruction to the text buffer and increments the current address
 * by 4 bytes. The instruction is only stored if we are in the .text section and
 * there is space remaining in the instruction buffer.
 *
 * @param ctx Pointer to the parser context
 * @param instr The 32-bit encoded instruction to emit
 *
 * @note Complexity: O(1)
 * @warning Silently drops instruction if text section is full or we are in .data section
 */
static void
emit_instr(parser_ctx_t* ctx, u32 instr)
{
    ctx->current_addr += 4;
    if (ctx->in_text_section && ctx->text_size < MAX_INSTRUCTIONS) {
	ctx->text[ctx->text_size++] = instr;
    }
}

/**
 * @brief Adds a label to the symbol table with the current address.
 *
 * @details Inserts a label definition into the parser's symbol table. Labels
 * are used to mark positions in the code for branch targets and data references.
 *
 * @param ctx Pointer to the parser context
 * @param name The label name to insert
 * @param addr The address to associate with the label
 * @return int 0 on success, -1 on failure (table full or duplicate)
 *
 * @see symbol_insert() Underlying symbol table insertion
 * @note Complexity: O(1) average case hash table lookup
 */
static int
add_label(parser_ctx_t* ctx, const char* name, u32 addr)
{
    return symbol_insert(&ctx->labels, name, addr);
}

/**
 * @brief Resolves a label name to its associated address.
 *
 * @details Performs a symbol table lookup to find the address of a label.
 * Used for branch targets and data references.
 *
 * @param ctx Pointer to the parser context
 * @param name The label name to look up
 * @return int The resolved address on success, -1 if label not found
 *
 * @see symbol_lookup() Underlying symbol table lookup
 * @note Complexity: O(1) average case hash table lookup
 * @warning Returns -1 for undefined labels without error
 */
static int
lookup_label(parser_ctx_t* ctx, const char* name)
{
    u32 addr;
    if (symbol_lookup(&ctx->labels, name, &addr)) {
	return (int)addr;
    }
    return -1;
}

/**
 * @brief Records a relocation entry for a label reference.
 *
 * @details Adds a relocation record for forward references or external labels.
 * Relocations are resolved after the first pass when all labels are defined.
 *
 * @param ctx Pointer to the parser context
 * @param name The label name to be relocated
 * @param addr The address in the instruction stream requiring fixup
 * @param is_branch Set to 1 for branch relocations, 0 for data relocations
 *
 * @note Complexity: O(1)
 * @warning Silently drops relocation if relocation table is full
 */
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

/**
 * @brief Adds a value to the literal pool or returns existing entry index.
 *
 * @details The literal pool stores 32-bit constants that can be loaded via
 * PC-relative LDR instructions. Duplicate values share a single pool entry.
 *
 * @param ctx Pointer to the parser context
 * @param value The 32-bit value to store in the pool
 * @return u32 The pool entry index, or 0 if pool is full
 *
 * @note Complexity: O(n) where n is the current literal pool count
 * @warning Returns 0 if pool exceeds maximum size (MAX_LITERAL_POOL_ENTRIES)
 */
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

/**
 * @brief Emits the literal pool and fixes up LDR instructions.
 *
 * @details Writes all literal pool entries to the end of the text section,
 * then updates the LDR instructions that reference the pool with the correct
 * byte offsets. This function is called after the first parsing pass completes.
 *
 * @param ctx Pointer to the parser context containing pool entries and references
 *
 * @note Complexity: O(n + m) where n is pool count and m is reference count
 * @warning Assumes all pool references are already recorded
 */
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

/**
 * @brief Parses assembler directives.
 *
 * @details Handles the following directives:
 *   - .text: Switch to code section, reset address to 0
 *   - .data: Switch to data section, set address to 0x10000
 *   - .word: Emit 32-bit values (little-endian)
 *   - .byte: Emit 8-bit values
 *   - .equ/.set: Define label-value associations
 *
 * @param ctx Pointer to the parser context
 * @param dir The directive name string
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @return int 0 on success, -1 on failure
 *
 * @note Complexity: O(k) where k is the number of values in directive
 * @warning Silently ignores malformed directives
 */
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

/**
 * @brief Parses an operand (immediate or register).
 *
 * @details Extracts either an immediate value or register from the token stream.
 * Accepts operands in the following forms:
 *   - #immediate or immediate (parsed via parse_immediate)
 *   - register name (parsed via get_register)
 *
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param imm_value Pointer to store parsed immediate value
 * @param reg_value Pointer to store parsed register index
 * @return int 1 if immediate parsed, 0 if register parsed, -1 on failure
 *
 * @note Complexity: O(1) amortized
 * @warning Returns -1 for malformed operands without error
 */
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

/**
 * @brief Parses MOV and MVN (move and move not) instructions.
 *
 * @details Encodes move instructions with the following format:
 *   MOV/MVN Rd, #imm  or  MOV/MVN Rd, Rn
 *
 * The operand field indicates immediate mode with bit 11 set.
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value (OP_MOV or OP_MVN)
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for AL)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 */
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

/**
 * @brief Parses B and BL (branch and branch with link) instructions.
 *
 * @details Encodes branch instructions with 24-bit signed relative offset.
 * Forward references are recorded as relocations for later resolution.
 *
 * Branch offset calculation: (target_addr - current_addr - 4) / 4
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value (OP_B or OP_BL)
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 * @warning Labels are resolved in second pass
 */
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

/**
 * @brief Parses system instructions (HALT, NOP, SWI).
 *
 * @details Encodes system-level instructions:
 *   - HALT: Stop execution, optional status code
 *   - NOP: No operation
 *   - SWI: Software interrupt with optional syscall number
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value (OP_HALT, OP_NOP, or OP_SWI)
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure
 */
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

/**
 * @brief Parses ALU instructions (ADD, SUB, AND, ORR, CMP, CMN, TST, TEQ).
 *
 * @details Encodes arithmetic and logical operations with the format:
 *   OP Rd, Rn, #imm  or  OP Rd, Rn, Rm
 *
 * Comparison instructions (CMP, CMN, TST, TEQ) ignore the destination register
 * and set condition flags instead.
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value for the ALU operation
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 */
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

/**
 * @brief Parses multiplication instructions (MUL, MLA).
 *
 * @details Encodes multiplication operations:
 *   - MUL Rd, Rm, Rs: Rd = Rm * Rs
 *   - MLA Rd, Rm, Rs, Rn: Rd = Rm * Rs + Rn
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value (OP_MUL or OP_MLA)
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 */
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

/**
 * @brief Parses load/store instructions (LDR, LDRB, STR, STRB).
 *
 * @details Encodes memory access with base-plus-offset addressing:
 *   LDR/STR Rt, [Rn, #offset]
 *
 * Supports byte (B suffix) and word (default) transfers.
 *
 * @param ctx Pointer to the parser context
 * @param opcode The opcode value (OP_LDR, OP_LDRB, OP_STR, or OP_STRB)
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1)
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 */
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

/**
 * @brief Parses the pseudo-instruction ldr rd, =label.
 *
 * @details Implements the ldr pseudo-instruction for loading label addresses.
 * Generates a LDR instruction with PC-relative addressing plus a literal pool
 * entry containing the label's address.
 *
 * Example: ldr r0, =my_label
 *   - Emits: LDR r0, [pc, #offset] where offset points to literal pool
 *   - Pool entry contains: address of my_label
 *
 * @param ctx Pointer to the parser context
 * @param tokens The token array
 * @param i Pointer to current token index (modified by function)
 * @param token_count Total number of tokens
 * @param condition The condition code suffix (NULL for default)
 * @return int 0 on success, -1 on parse failure
 *
 * @note Complexity: O(1) for parsing, O(n) for literal pool emission
 * @retval 0 Success
 * @retval -1 Parse failure (invalid syntax)
 * @see emit_literal_pool() Pool emission after parsing
 */
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

/**
 * @brief Main parsing entry point - converts tokens to machine code.
 *
 * @details This is the primary parsing function that orchestrates the entire
 * assembly process. It performs a single-pass parsing algorithm that:
 *
 * 1. Initializes the parser context (symbol table, empty sections)
 * 2. Iterates through all tokens:
 *    - NEWLINE: Skip (whitespace)
 *    - LABEL: Add to symbol table with current address
 *    - DIRECTIVE: Pass to parse_directive()
 *    - INSTRUCTION: Dispatch to appropriate handler:
 *        ├─ B/BL → parse_branch()
 *        ├─ HALT/NOP/SWI → parse_system()
 *        ├─ MOV/MVN → parse_move()
 *        ├─ CMP/CMN/TST/TEQ → parse_alu()
 *        ├─ MUL/MLA → parse_mult()
 *        ├─ LDR (=label) → parse_pseudo_ldr()
 *        ├─ LDR/STR variants → parse_load_store()
 *        └─ Other → parse_alu() (ADD, SUB, etc.)
 * 3. After all tokens: Resolve relocations (fix up addresses)
 * 4. Emit literal pool entries
 * 5. Copy results to program_state_t output
 *
 * @param tokens The array of tokens from the lexer
 * @param token_count The number of tokens in the array
 * @param prog Pointer to the output program_state_t structure
 * @return int 0 on success, -1 on failure
 *
 * @note Time complexity: O(n) where n is the number of tokens
 * @note Space complexity: O(k) where k is the number of labels and instructions
 * @warning This function modifies the prog structure
 *
 * @see tokenize() Tokenization
 * @see write_vm_file() Output file generation
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

/**
 * @brief Writes the assembled program to a .varm file.
 *
 * @details Creates a binary file with the varm executable format:
 *   - 32-byte header containing metadata
 *   - Text section (encoded instructions, little-endian)
 *   - Data section (initialized data)
 *
 * The header structure:
 *   Offset  Size  Description
 *   0       4     Magic: "VARM"
 *   4       4     text_offset (always 32)
 *   8       4     text_size (in bytes)
 *   12      4     data_offset (32 + text_size)
 *   16      4     data_size (in bytes)
 *   20      4     entry point (always text_offset)
 *   24      4     Reserved (0)
 *   28      4     Reserved (0)
 *
 * @param prog Pointer to the assembled program_state_t
 * @param filename The output file path
 * @return int 0 on success, -1 on failure (file open error)
 *
 * @note Complexity: O(n) where n is text_size + data_size
 * @retval 0 Success
 * @retval -1 File open failed
 */
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

/**
 * @brief High-level assembly function - reads source file and produces output.
 *
 * @details This is the main entry point for the assembler. It orchestrates
 * the complete assembly pipeline:
 *
 * 1. Read entire input file into memory
 * 2. Tokenize the source code (via tokenize())
 * 3. Parse tokens into encoded instructions (via parse())
 * 4. Write output .varm file (via write_vm_file())
 *
 * @param input_file Path to the assembly source file (.s or .asm)
 * @param output_file Path for the output .varm binary file
 * @return int 0 on success, -1 on failure (file error or allocation failure)
 *
 * @note Time complexity: O(n + m) where n is source size, m is instruction count
 * @note Allocates temporary buffer for file contents
 * @retval 0 Success
 * @retval -1 File open failed or memory allocation failed
 *
 * @see tokenize() Lexical analysis
 * @see parse() Syntax analysis
 * @see write_vm_file() Output generation
 */
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
