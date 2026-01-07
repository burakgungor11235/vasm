/*
 * varm Assembler - Parser
 * 
 * Converts tokens to machine code instructions following the varm instruction format.
 * 
 * INSTRUCTION FORMAT (REFERENCE.md):
 * ================================
 * All instructions are 32 bits:
 * 
 *   31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:0 operand
 *   +----------+  +--------+  +-------+  +--------+  +-------------+
 *   | 8 bits   |  | 4 bit  |  | 4 bit |  | 4 bit  |  | 12 bits     |
 *   +----------+  +--------+  +-------+  +--------+  +-------------+
 *   
 * Field descriptions:
 * - opcode: Operation code (0x00-0xFF). See opcode.h for values.
 * - cond:   Condition code (see COND_* enums in opcode.h)
 * - rn:     First source register (for ALU operations)
 * - rd:     Destination register
 * - operand: Second operand (immediate or register reference)
 * 
 * VM DECODER EXPECTATIONS (vm/core.c):
 * ====================================
 * The VM decodes instructions using:
 *   u8 opcode = (instr >> 24) & 0xFF;   // bits 24-31
 *   u8 cond   = (instr >> 20) & 0xF;    // bits 20-23
 *   u8 rn     = (instr >> 16) & 0xF;    // bits 16-19
 *   u8 rd     = (instr >> 12) & 0xF;    // bits 12-15
 *   u32 operand = instr & 0xFFF;        // bits 0-11
 * 
 * ENCODING RULES:
 * ===============
 * 1. opcode goes at bits 24-31: use (opcode << 24)
 * 2. cond goes at bits 20-23:  use (cond << 20)
 * 3. rn goes at bits 16-19:    use (rn << 16)
 * 4. rd goes at bits 12-15:    use (rd << 12)
 * 5. operand is 12 bits:       mask with & 0xFFF
 * 6. Combine with: opcode | cond | rn | rd | operand
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "../../include/vm.h"
#include "../../include/assembler.h"

#define MAX_LABELS 256
#define MAX_INSTRUCTIONS 4096

/*
 * Forward declarations for program state struct members.
 * These must be defined before program_state_t.
 */
typedef struct {
    char name[64];
    u32 address;
} label_t;

typedef struct {
    u32 address;
    char name[64];
    int is_branch;
} reloc_t;

/*
 * Literal pool entry - stores 32-bit values for PC-relative loading.
 */
typedef struct {
    u32 value;  /* The value to store */
    u32 offset; /* Offset from text section start where this entry will be placed */
} literal_pool_entry_t;

/*
 * Literal pool reference - tracks LDR instructions that need to be fixed up.
 */
typedef struct {
    u32 instr_addr; /* Address of the LDR instruction (text offset) */
    int pool_index; /* Which literal pool entry this instruction references */
} literal_pool_ref_t;

#define MAX_LITERAL_POOL_ENTRIES 256
#define MAX_LITERAL_POOL_REFS 256

/*
 * Program state holds the assembled output and symbol table.
 * 
 * The text section contains encoded instructions (4 bytes each).
 * The data section contains initial data values.
 * Labels are collected during first pass and resolved in second pass.
 */
typedef struct {
    u32 text[MAX_INSTRUCTIONS]; /* Encoded instructions */
    u32 text_size;              /* Number of instructions */
    u8 data[65536];             /* Data section */
    u32 data_size;              /* Data size in bytes */
    label_t labels[MAX_LABELS]; /* Symbol table */
    int label_count;
    reloc_t relocs[MAX_LABELS]; /* Unresolved references */
    int reloc_count;
    literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES]; /* Literal pool entries */
    int literal_pool_count;
    literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS]; /* References to fix up */
    int literal_pool_ref_count;
    u32 current_addr;    /* Assembly address counter */
    int in_text_section; /* 1 = text, 0 = data */
} program_state_t;

/*
 * Look up a label in the symbol table.
 * Returns the address if found, -1 if not found.
 */
static int
lookup_label(program_state_t* prog, const char* name)
{
    for (int i = 0; i < prog->label_count; i++) {
	if (strcmp(prog->labels[i].name, name) == 0) {
	    return prog->labels[i].address;
	}
    }
    return -1;
}

/*
 * Add a value to the literal pool.
 * Returns the offset from the text section start where the value will be stored.
 *
 * The literal pool is placed AFTER all text instructions. Since we don't know
 * the final text size at this point, we reserve space and recalculate offsets
 * at the end when emit_literal_pool() is called.
 */
static u32
add_literal_pool_entry(program_state_t* prog, u32 value)
{
    /* Check if value already exists in literal pool */
    for (int i = 0; i < prog->literal_pool_count; i++) {
	if (prog->literal_pool[i].value == value) {
	    return prog->literal_pool[i].offset;
	}
    }

    /* Add new entry if space available */
    if (prog->literal_pool_count < MAX_LITERAL_POOL_ENTRIES) {
	prog->literal_pool[prog->literal_pool_count].value = value;
	prog->literal_pool[prog->literal_pool_count].offset =
	        0; /* Will be set by emit_literal_pool */
	prog->literal_pool_count++;
	/* Return a placeholder - actual offset will be calculated in emit_literal_pool */
	return prog->literal_pool_count - 1; /* Return index for later resolution */
    }

    return 0;
}

/*
 * Emit literal pool entries at the end of the text section.
 * This is called after all instructions have been parsed.
 * It also fixes up the LDR instructions that reference the literal pool.
 */
static void
emit_literal_pool(program_state_t* prog)
{
    /* Calculate the offset where the literal pool starts.
   * The literal pool is placed after all text instructions.
   */
    u32 pool_start = prog->text_size * 4;

    fprintf(stderr, "DEBUG: emit_literal_pool: text_size=%u, pool_start=%u, count=%d\n",
            prog->text_size, pool_start, prog->literal_pool_count);

    /* Calculate offsets for each literal pool entry and emit them */
    for (int i = 0; i < prog->literal_pool_count; i++) {
	prog->literal_pool[i].offset = pool_start + i * 4;
	prog->text[prog->text_size++] = prog->literal_pool[i].value;

	fprintf(stderr, "DEBUG:   pool[%d]: value=0x%08X, offset=%u\n", i,
	        prog->literal_pool[i].value, prog->literal_pool[i].offset);
    }

    /* Fix up LDR instructions that reference the literal pool */
    for (int j = 0; j < prog->literal_pool_ref_count; j++) {
	u32 instr_addr = prog->literal_pool_refs[j].instr_addr / 4;
	int pool_idx = prog->literal_pool_refs[j].pool_index;

	if (pool_idx >= 0 && pool_idx < prog->literal_pool_count) {
	    u32 pool_offset = prog->literal_pool[pool_idx].offset;

	    /* Recalculate PC-relative offset
        * The offset field in the instruction is a BYTE offset.
        * VM calculates: addr = PC + offset (where offset is in bytes).
        *
        * At exec time: PC = entry + instr_byte_offset + 4 (VM increments after fetch)
        * We want: addr = entry + pool_byte_offset
        * So: offset = (entry + pool_byte_offset) - (entry + instr_byte_offset + 4)
        *    offset = pool_byte_offset - instr_byte_offset - 4
        *
        * instr_addr is word index (bytes / 4), so instr_byte_offset = instr_addr * 4
        */
	    int byte_offset = (int)pool_offset - (int)(instr_addr * 4 + 4);
	    u32 offset_val = byte_offset;

	    u32 old_instr = prog->text[instr_addr];
	    u32 new_instr = (prog->text[instr_addr] & 0xFFFFF000) | (offset_val & 0xFFF);
	    prog->text[instr_addr] = new_instr;

	    fprintf(stderr, "DEBUG:   fixup LDR at %u: pool_offset=%u, byte_offset=%d, offset=%u\n",
	            instr_addr * 4, pool_offset, byte_offset, offset_val);
	    fprintf(stderr,
	            "DEBUG:   old_instr=0x%08X (opcode=0x%02X, cond=0x%X, rn=0x%X, rd=0x%X, "
	            "off=0x%03X)\n",
	            old_instr, (old_instr >> 24) & 0xFF, (old_instr >> 20) & 0xF,
	            (old_instr >> 16) & 0xF, (old_instr >> 12) & 0xF, old_instr & 0xFFF);
	    fprintf(stderr,
	            "DEBUG:   new_instr=0x%08X (opcode=0x%02X, cond=0x%X, rn=0x%X, rd=0x%X, "
	            "off=0x%03X)\n",
	            new_instr, (new_instr >> 24) & 0xFF, (new_instr >> 20) & 0xF,
	            (new_instr >> 16) & 0xF, (new_instr >> 12) & 0xF, new_instr & 0xFFF);
	}
    }
}

/*
 * Add a label to the symbol table.
 */
static void
add_label(program_state_t* prog, const char* name, u32 addr)
{
    if (prog->label_count < MAX_LABELS) {
	strncpy(prog->labels[prog->label_count].name, name, 63);
	prog->labels[prog->label_count].address = addr;
	prog->label_count++;
    }
}

/*
 * Add a relocation entry for label resolution.
 * is_branch = 1 for branch instructions, 0 for data references.
 */
static void
add_reloc(program_state_t* prog, const char* name, u32 addr, int branch)
{
    if (prog->reloc_count < MAX_LABELS) {
	strncpy(prog->relocs[prog->reloc_count].name, name, 63);
	prog->relocs[prog->reloc_count].address = addr;
	prog->relocs[prog->reloc_count].is_branch = branch;
	prog->reloc_count++;
    }
}

/*
 * Parse a register name to its number.
 * Supports: r0-r12, sp (13), lr (14), pc (15)
 * Returns -1 if not a valid register name.
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

/*
 * Parse condition suffix to condition code.
 * Examples: "eq" -> COND_EQ (0x0), "" or NULL -> COND_AL (0xE)
 * See REFERENCE.md for condition code table.
 */
static int
parse_condition(const char* cond)
{
    if (cond == NULL || cond[0] == '\0')
	return COND_AL;

    if (strcasecmp(cond, "eq") == 0)
	return COND_EQ;
    if (strcasecmp(cond, "ne") == 0)
	return COND_NE;
    if (strcasecmp(cond, "cs") == 0 || strcasecmp(cond, "hs") == 0)
	return COND_CS;
    if (strcasecmp(cond, "cc") == 0 || strcasecmp(cond, "lo") == 0)
	return COND_CC;
    if (strcasecmp(cond, "mi") == 0)
	return COND_MI;
    if (strcasecmp(cond, "pl") == 0)
	return COND_PL;
    if (strcasecmp(cond, "vs") == 0)
	return COND_VS;
    if (strcasecmp(cond, "vc") == 0)
	return COND_VC;
    if (strcasecmp(cond, "hi") == 0)
	return COND_HI;
    if (strcasecmp(cond, "ls") == 0)
	return COND_LS;
    if (strcasecmp(cond, "ge") == 0)
	return COND_GE;
    if (strcasecmp(cond, "lt") == 0)
	return COND_LT;
    if (strcasecmp(cond, "gt") == 0)
	return COND_GT;
    if (strcasecmp(cond, "le") == 0)
	return COND_LE;
    return COND_AL;
}

/*
 * Get opcode number from instruction mnemonic.
 * Returns -1 if not a recognized instruction.
 * See opcode.h for OP_* constants.
 */
static int
get_opcode(const char* name)
{
    if (strcasecmp(name, "mov") == 0)
	return OP_MOV;
    if (strcasecmp(name, "mvn") == 0)
	return OP_MVN;
    if (strcasecmp(name, "add") == 0)
	return OP_ADD;
    if (strcasecmp(name, "adc") == 0)
	return OP_ADC;
    if (strcasecmp(name, "sub") == 0)
	return OP_SUB;
    if (strcasecmp(name, "sbc") == 0)
	return OP_SBC;
    if (strcasecmp(name, "rsb") == 0)
	return OP_RSB;
    if (strcasecmp(name, "rsc") == 0)
	return OP_RSC;
    if (strcasecmp(name, "and") == 0)
	return OP_AND;
    if (strcasecmp(name, "eor") == 0)
	return OP_EOR;
    if (strcasecmp(name, "orr") == 0)
	return OP_ORR;
    if (strcasecmp(name, "bic") == 0)
	return OP_BIC;
    if (strcasecmp(name, "cmp") == 0)
	return OP_CMP;
    if (strcasecmp(name, "cmn") == 0)
	return OP_CMN;
    if (strcasecmp(name, "tst") == 0)
	return OP_TST;
    if (strcasecmp(name, "teq") == 0)
	return OP_TEQ;
    if (strcasecmp(name, "mul") == 0)
	return OP_MUL;
    if (strcasecmp(name, "mla") == 0)
	return OP_MLA;
    if (strcasecmp(name, "ldr") == 0)
	return OP_LDR;
    if (strcasecmp(name, "ldrb") == 0)
	return OP_LDRB;
    if (strcasecmp(name, "str") == 0)
	return OP_STR;
    if (strcasecmp(name, "strb") == 0)
	return OP_STRB;
    if (strcasecmp(name, "b") == 0)
	return OP_B;
    if (strcasecmp(name, "bl") == 0)
	return OP_BL;
    if (strcasecmp(name, "bx") == 0)
	return OP_BX;
    if (strcasecmp(name, "halt") == 0)
	return OP_HALT;
    if (strcasecmp(name, "swi") == 0)
	return OP_SWI;
    if (strcasecmp(name, "nop") == 0)
	return OP_NOP;
    return -1;
}

/*
 * Parse immediate value with ARM-style rotate encoding.
 * 
 * ARM immediates are 8 bits rotated right by an even amount.
 * This allows representing values like 0xFF000000 efficiently.
 * 
 * parse_immediate("42")     -> 0x2A (no rotation needed)
 * parse_immediate("0xFF")   -> 0xFF (rotate=0, imm8=0xFF)
 * parse_immediate("0xFF000000") -> 0xFF (rotate=0xC, imm8=0xFF)
 *   because 0xFF000000 rotated right by 24 = 0xFF
 * 
 * Returns: 12-bit encoded immediate: bits 11:8 = rotate, bits 7:0 = imm8
 */
static u32
parse_immediate(const char* value)
{
    u32 result = 0;

    /* Parse the numeric value (supports decimal, hex, binary) */
    if (strncasecmp(value, "0x", 2) == 0) {
	sscanf(value + 2, "%x", &result);
    } else if (strncasecmp(value, "0b", 2) == 0) {
	result = strtoul(value + 2, NULL, 2);
    } else {
	result = strtoul(value, NULL, 10);
    }

    /*
   * Rotate the value right by 2 bits until it fits in 8 bits.
   * Count how many rotations were needed (rotate field).
   * This is the ARM "rotate right 2" encoding.
   */
    u8 rotate = 0;
    while (rotate < 16 && (result & 0xFF000000)) {
	result = (result >> 2) | ((result & 0x3) << 30);
	rotate++;
    }

    /* Encode as: (rotate << 8) | imm8 */
    return (rotate << 8) | (result & 0xFF);
}

/*
 * Encode a raw u32 value as an ARM immediate.
 * 
 * ARM immediates are 8 bits rotated right by an even amount (0, 2, 4, ... 30).
 * This allows representing values like 0xFF000000 efficiently.
 * 
 * For example:
 *   0x00000042 -> rotate=0, imm8=0x42 -> encoding=0x42
 *   0xFF000000 -> rotate=12, imm8=0xFF -> encoding=0xFF0C
 *   0x0F0F0F0F -> rotate=8, imm8=0x0F -> encoding=0x0F08
 * 
 * Returns: 12-bit encoded immediate: bits 11:8 = rotate, bits 7:0 = imm8
 */
static u32
encode_immediate(u32 value)
{
    u8 rotate = 0;
    while (rotate < 16 && (value & 0xFF000000)) {
	value = (value >> 2) | ((value & 0x3) << 30);
	rotate++;
    }
    return (rotate << 8) | (value & 0xFF);
}

/*
 * Emit a single instruction to the text section.
 * Each instruction is 4 bytes (32 bits).
 */
static void
emit_instr(program_state_t* prog, u32 instr)
{
    if (prog->in_text_section && prog->text_size < MAX_INSTRUCTIONS) {
	prog->text[prog->text_size++] = instr;
    }
    prog->current_addr += 4; /* Each instruction is 4 bytes */
}

/*
 * Main parsing function - converts tokens to instructions.
 * 
 * Two-pass assembly:
 * 1. First pass: tokenize and collect labels
 * 2. Second pass: encode instructions and resolve labels
 * 
 * Actually implemented as single pass with forward references
 * stored in relocs for later resolution.
 */
int
parse(token_t* tokens, int token_count, program_state_t* prog)
{
    int i = 0;
    prog->text_size = 0;
    prog->data_size = 0;
    prog->label_count = 0;
    prog->reloc_count = 0;
    prog->literal_pool_count = 0;
    prog->literal_pool_ref_count = 0;
    prog->current_addr = 0;
    prog->in_text_section = 1;

    for (i = 0; i < token_count && tokens[i].type != TOKEN_EOF; i++) {
	/* Skip empty lines */
	if (tokens[i].type == TOKEN_NEWLINE) {
	    continue;
	}

	/* Handle labels: label: instruction */
	if (tokens[i].type == TOKEN_LABEL) {
	    add_label(prog, tokens[i].value, prog->current_addr);
	    continue;
	}

	/* Handle assembler directives (.data, .word, .equ, etc.) */
	if (tokens[i].type == TOKEN_DIRECTIVE) {
	    char* dir = tokens[i].value;
	    i++;

	    if (strcasecmp(dir, ".text") == 0) {
		prog->in_text_section = 1;
		continue;
	    }

	    if (strcasecmp(dir, ".data") == 0) {
		prog->in_text_section = 0;
		prog->current_addr = 0x10000;
		continue;
	    }

	    if (strcasecmp(dir, ".word") == 0) {
		while (i < token_count && tokens[i].type != TOKEN_NEWLINE &&
		       tokens[i].type != TOKEN_EOF) {
		    if (tokens[i].type == TOKEN_IMMEDIATE) {
			u32 val = strtoul(tokens[i].value, NULL, 0);
			if (prog->data_size < 65536) {
			    prog->data[prog->data_size++] = val & 0xFF;
			    prog->data[prog->data_size++] = (val >> 8) & 0xFF;
			    prog->data[prog->data_size++] = (val >> 16) & 0xFF;
			    prog->data[prog->data_size++] = (val >> 24) & 0xFF;
			    prog->current_addr += 4;
			}
		    }
		    i++;
		}
		continue;
	    }

	    if (strcasecmp(dir, ".byte") == 0) {
		while (i < token_count && tokens[i].type != TOKEN_NEWLINE &&
		       tokens[i].type != TOKEN_EOF) {
		    if (tokens[i].type == TOKEN_IMMEDIATE) {
			u32 val = strtoul(tokens[i].value, NULL, 0);
			if (prog->data_size < 65536) {
			    prog->data[prog->data_size++] = val & 0xFF;
			    prog->current_addr++;
			}
		    }
		    i++;
		}
		continue;
	    }

	    if (strcasecmp(dir, ".equ") == 0 || strcasecmp(dir, ".set") == 0) {
		if (i + 2 < token_count && tokens[i + 1].type == TOKEN_IDENTIFIER) {
		    u32 val = 0;
		    if (i + 3 < token_count && tokens[i + 2].type == TOKEN_COMMA) {
			if (i + 4 < token_count && tokens[i + 3].type == TOKEN_IMMEDIATE) {
			    val = strtoul(tokens[i + 3].value, NULL, 0);
			}
		    } else if (i + 3 < token_count && tokens[i + 2].type == TOKEN_IMMEDIATE) {
			val = strtoul(tokens[i + 2].value, NULL, 0);
		    }
		    add_label(prog, tokens[i + 1].value, val);
		}
		while (i < token_count && tokens[i].type != TOKEN_NEWLINE &&
		       tokens[i].type != TOKEN_EOF)
		    i++;
		continue;
	    }

	    continue;
	}

	/* Process actual instructions */
	if (tokens[i].type == TOKEN_INSTRUCTION) {
	    char* instr_name = tokens[i].value;
	    char* condition = NULL;

	    /* Extract condition suffix (e.g., "eq" from "moveq") */
	    char* dot = strchr(instr_name, '.');
	    if (dot != NULL) {
		*dot = '\0';
		condition = dot + 1;
	    }

	    int opcode = get_opcode(instr_name);
	    i++;

	    if (opcode < 0) {
		continue;
	    }

	    u32 instr = 0;
	    u8 rd = 0, rn = 0;
	    u32 operand = 0;
	    u32 offset = 0;

	    /*
       * =====================================================
       * BRANCH INSTRUCTIONS: B, BL
       * Format: opcode | cond | 24-bit offset
       * Offset is signed, applied as (offset << 2) to PC
       * =====================================================
       */
	    if (opcode == OP_B || opcode == OP_BL) {
		if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
		    add_reloc(prog, tokens[i].value, prog->current_addr,
		              opcode == OP_BL || opcode == OP_B);
		    offset = 0;
		    i++;
		} else if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
		    offset = strtoul(tokens[i].value, NULL, 0);
		    i++;
		}
		/*
         * Branch format per REFERENCE.md:
         *   31:24 opcode | 23:20 cond | 19:0 offset (signed, << 2)
         */
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (offset & 0xFFFFF);
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * SYSTEM INSTRUCTIONS: HALT, NOP
       * Format: opcode | cond
       * No other fields used
       * =====================================================
       */
	    if (opcode == OP_HALT || opcode == OP_NOP) {
		instr = (opcode << 24) | (parse_condition(condition) << 20);
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * SYSTEM CALL: SWI (software interrupt)
       * Format: opcode | cond | syscall_number (in operand field)
       * =====================================================
       */
	    if (opcode == OP_SWI) {
		if (i < token_count &&
		    (tokens[i].type == TOKEN_HASH || tokens[i].type == TOKEN_IMMEDIATE)) {
		    if (tokens[i].type == TOKEN_HASH) {
			i++;
			if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
			    offset = strtoul(tokens[i].value, NULL, 0);
			    i++;
			}
		    } else {
			offset = strtoul(tokens[i].value, NULL, 0);
			i++;
		    }
		}
		/* SWI syscall number goes in operand field (bits 0-11) */
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (offset & 0xFFF);
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * MOVE INSTRUCTIONS: MOV, MVN
       * Format: opcode | cond | 0 | rd | operand
       * rn is unused (set to 0)
       * =====================================================
       */
	    if (opcode == OP_MOV || opcode == OP_MVN) {
		if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
		    rd = get_register(tokens[i].value);
		    i++;
		}
		if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		    i++;
		}
		u32 operand_immed = 0;
		u32 operand_reg = 0;
		if (i < token_count) {
		    if (tokens[i].type == TOKEN_HASH) {
			i++;
			if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
			    operand_immed = parse_immediate(tokens[i].value);
			    i++;
			}
		    } else if (tokens[i].type == TOKEN_IMMEDIATE) {
			operand_immed = parse_immediate(tokens[i].value);
			i++;
		    } else if (tokens[i].type == TOKEN_IDENTIFIER) {
			int rm = get_register(tokens[i].value);
			if (rm >= 0) {
			    operand_reg = rm;
			} else {
			    operand_immed = parse_immediate(tokens[i].value);
			}
			i++;
		    }
		}
		/*
         * Encode: opcode<<24 | cond<<20 | rn(=0)<<16 | rd<<12 | operand
         * Operand is 12 bits: combine immediate (with rotate encoding) and register
         */
		operand = (operand_immed | operand_reg) & 0xFFF;
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) |
		        (rd << 12) | operand;
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * COMPARE INSTRUCTIONS: CMP, CMN, TST, TEQ
       * Format: opcode | cond | rn | 0 | operand
       * rd is unused (set to 0), updates CPSR flags
       * =====================================================
       */
	    if (opcode == OP_CMP || opcode == OP_CMN || opcode == OP_TST || opcode == OP_TEQ) {
		if (i < token_count) {
		    rn = get_register(tokens[i].value);
		    i++;
		}
		if (i < token_count && (tokens[i].type == TOKEN_COMMA)) {
		    i++;
		    if (i < token_count) {
			if (tokens[i].type == TOKEN_HASH) {
			    i++;
			    if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
				operand = parse_immediate(tokens[i].value);
				i++;
			    }
			} else if (tokens[i].type == TOKEN_IMMEDIATE) {
			    operand = parse_immediate(tokens[i].value);
			    i++;
			} else if (tokens[i].type == TOKEN_IDENTIFIER) {
			    operand = parse_immediate(tokens[i].value);
			    i++;
			}
		    }
		}
		operand = operand & 0xFFF;
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) |
		        (rd << 12) | operand;
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * MULTIPLY INSTRUCTIONS: MUL, MLA
       * Format: opcode | cond | rn | rd | rm | rs | 0
       * MUL: rd = rm * rs
       * MLA: rd = rm * rs + rn
       * =====================================================
       */
	    if (opcode == OP_MUL || opcode == OP_MLA) {
		u8 rd_mul = 0, rm = 0, rs = 0, rn_mul = 0;
		if (i < token_count) {
		    rd_mul = get_register(tokens[i].value);
		    i++;
		}
		if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		    i++;
		}
		if (i < token_count) {
		    rm = get_register(tokens[i].value);
		    i++;
		}
		if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		    i++;
		}
		if (i < token_count) {
		    rs = get_register(tokens[i].value);
		    i++;
		}
		if (opcode == OP_MLA) {
		    if (i < token_count && tokens[i].type == TOKEN_COMMA) {
			i++;
		    }
		    if (i < token_count) {
			rn_mul = get_register(tokens[i].value);
			i++;
		    }
		}
		/*
         * Multiply format per REFERENCE.md:
         *   31:24 opcode | 23:20 cond | 19:16 rn | 15:12 rd | 11:8 rm | 7:4 rs | 3:0 -
         */
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn_mul << 16) |
		        (rd_mul << 12) | (rm << 8) | (rs << 4);
		emit_instr(prog, instr);
		continue;
	    }

	    /*
        * =====================================================
        * PSEUDO-INSTRUCTION: LDR Rd, =label
        *
        * Load a 32-bit label address into a register using PC-relative
        * load from a literal pool.
        *
        * Syntax: ldr rd, =label
        * Example: ldr r1, =msg  ; r1 = address of msg label
        *
        * Implementation:
        *   1. Add label address to literal pool (emitted after all code)
        *   2. Emit: LDR rd, [pc, #offset] where offset points to pool entry
        *
        * The literal pool stores 32-bit values after the text section.
        * PC-relative offset formula: (pool_offset - (current_addr + 8)) / 4
        * =====================================================
        */
	    if (opcode == OP_LDR && i + 3 < token_count && tokens[i + 1].type == TOKEN_COMMA &&
	        tokens[i + 2].type == TOKEN_EQUAL && tokens[i + 3].type == TOKEN_IDENTIFIER) {
		/* Get destination register - it's the token at current position */
		u8 rd = get_register(tokens[i].value);

		/* Look up label address */
		const char* label_name = tokens[i + 3].value;
		int label_addr = lookup_label(prog, label_name);

		fprintf(stderr, "DEBUG: =label pseudo-instr: label='%s' addr=0x%X rd=%d\n",
		        label_name, label_addr, rd);

		if (label_addr >= 0) {
		    /* Add value to literal pool */
		    u32 pool_index = add_literal_pool_entry(prog, (u32)label_addr);

		    /* Record this instruction for later fixup */
		    if (prog->literal_pool_ref_count < MAX_LITERAL_POOL_REFS) {
			prog->literal_pool_refs[prog->literal_pool_ref_count].instr_addr =
			        prog->text_size * 4;
			prog->literal_pool_refs[prog->literal_pool_ref_count].pool_index =
			        pool_index;
			prog->literal_pool_ref_count++;
		    }

		    /* Emit placeholder LDR instruction - will be fixed up after literal pool is placed */
		    /* LDR rd, [pc, #0] - placeholder offset of 0 */
		    instr = (OP_LDR << 24) | (parse_condition(condition) << 20) | (15 << 16) |
		            (rd << 12);
		    fprintf(stderr,
		            "DEBUG: =label emit: instr=0x%08X (opcode=0x%02X, cond=0x%X, rn=0x%X, "
		            "rd=0x%X)\n",
		            instr, (instr >> 24) & 0xFF, (instr >> 20) & 0xF, (instr >> 16) & 0xF,
		            (instr >> 12) & 0xF);
		    emit_instr(prog, instr);

		    fprintf(stderr, "DEBUG: =label: pool_index=%u, instr_addr=%u\n", pool_index,
		            prog->text_size * 4 - 4);
		} else {
		    /* Label not found - emit NOP */
		    fprintf(stderr, "Warning: label '%s' not found\n", label_name);
		    instr = (OP_NOP << 24) | (parse_condition(condition) << 20);
		    emit_instr(prog, instr);
		}

		i += 4; /* Skip rd, comma, =, and label */
		continue;
	    }

	    /*
       * =====================================================
       * MEMORY INSTRUCTIONS: LDR, LDRB, STR, STRB
       * Format: opcode | cond | rn | rt | 12-bit offset
       * [rn, #offset] - base register with byte offset
       * =====================================================
       */
	    if (opcode == OP_LDR || opcode == OP_LDRB || opcode == OP_STR || opcode == OP_STRB) {
		u8 rt = 0, rn_ldr = 0;
		u32 offset_ldr = 0;

		if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
		    rt = get_register(tokens[i].value);
		    i++;
		}
		if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		    i++;
		}
		if (i < token_count && tokens[i].type == TOKEN_LBRACKET) {
		    i++;
		    if (i < token_count && tokens[i].type == TOKEN_IDENTIFIER) {
			rn_ldr = get_register(tokens[i].value);
			i++;
		    }
		    if (i < token_count && tokens[i].type == TOKEN_COMMA) {
			i++;
			if (i < token_count && tokens[i].type == TOKEN_HASH) {
			    i++;
			    if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
				offset_ldr = strtoul(tokens[i].value, NULL, 0);
				i++;
			    }
			}
		    }
		    if (i < token_count && tokens[i].type == TOKEN_RBRACKET) {
			i++;
		    }
		}
		offset_ldr = offset_ldr & 0xFFF;
		instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn_ldr << 16) |
		        (rt << 12) | offset_ldr;
		emit_instr(prog, instr);
		continue;
	    }

	    /*
       * =====================================================
       * ALU INSTRUCTIONS: ADD, SUB, AND, ORR, etc.
       * Format: opcode | cond | rn | rd | operand
       * rd = rn OP operand
       * =====================================================
       */
	    if (i < token_count) {
		rd = get_register(tokens[i].value);
		i++;
	    }

	    if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		i++;
	    }

	    if (i < token_count) {
		rn = get_register(tokens[i].value);
		i++;
	    }

	    if (i < token_count && tokens[i].type == TOKEN_COMMA) {
		i++;
	    }

	    if (i < token_count) {
		if (tokens[i].type == TOKEN_HASH) {
		    i++;
		    if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
			operand = parse_immediate(tokens[i].value);
			i++;
		    }
		} else if (tokens[i].type == TOKEN_IMMEDIATE) {
		    operand = parse_immediate(tokens[i].value);
		    i++;
		} else if (tokens[i].type == TOKEN_IDENTIFIER) {
		    operand = parse_immediate(tokens[i].value);
		    i++;
		} else if (tokens[i].type == TOKEN_LBRACKET) {
		    u32 base = rn;
		    u32 off = 0;
		    i++;
		    if (i < token_count && tokens[i].type == TOKEN_HASH) {
			i++;
			if (i < token_count && tokens[i].type == TOKEN_IMMEDIATE) {
			    off = strtoul(tokens[i].value, NULL, 0);
			    i++;
			}
		    }
		    if (i < token_count && tokens[i].type == TOKEN_RBRACKET) {
			i++;
		    }
		    offset = off & 0xFFF;
		    rn = base;
		    operand = offset;
		    instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) |
		            (rd << 12) | operand;
		    emit_instr(prog, instr);
		    continue;
		}
	    }

	    operand = operand & 0xFFF;
	    instr = (opcode << 24) | (parse_condition(condition) << 20) | (rn << 16) | (rd << 12) |
	            operand;
	    emit_instr(prog, instr);
	}
    }

    /*
   * Second pass: resolve label references
   * Branches need offset calculation, other refs are direct addresses
   */
    for (int j = 0; j < prog->reloc_count; j++) {
	int addr = lookup_label(prog, prog->relocs[j].name);
	if (addr >= 0) {
	    u32* patch_addr = &prog->text[prog->relocs[j].address / 4];
	    if (prog->relocs[j].is_branch) {
		/* Branch offset: (target - current - 8) / 4 */
		u32 offset = (addr - prog->relocs[j].address - 8) / 4;
		*patch_addr = (*patch_addr & 0xFF000000) | (offset & 0xFFFFF);
	    } else {
		*patch_addr = addr;
	    }
	}
    }

    /* Emit literal pool entries at the end of text section */
    emit_literal_pool(prog);

    return 0;
}

/*
 * Write the assembled program to a .varm file.
 * 
 * File format:
 *   - 32-byte header (see REFERENCE.md)
 *   - Text section (instructions)
 *   - Data section (initialized data)
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
    u32 text_total_size = prog->text_size * 4; /* Includes literal pool */
    u32 data_offset = text_offset + text_total_size;
    u32 entry = text_offset; /* Entry point is at text section start */

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

/*
 * Main assembler entry point.
 * Reads .vasm file, assembles to .varm file.
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
    printf("Labels: %d\n", prog.label_count);

    return 0;
}
