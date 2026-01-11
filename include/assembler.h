/**
 * @file assembler.h
 * @brief Public assembler API for the varm virtual machine.
 *
 * @details This header defines the interfaces for the varm assembler,
 * including tokenization, parsing, and binary output generation.
 * The assembler converts human-readable assembly source code into
 * the varm executable format.
 *
 * @author varm Development Team
 * @version 0.1.0
 *
 * @warning This API is not stable. Backward compatibility is not guaranteed.
 *
 * @see parser.c Main parser implementation
 * @see lexer.c Tokenizer implementation
 *
 * ASSEMBLY PIPELINE:
 * ==================
 *
 *   [Source Code]
 *        |
 *        v
 *   [tokenize()]  - Lexical analysis (lexer.c)
 *        |
 *        v
 *   [parse()]     - Syntax analysis (parser.c)
 *        |
 *        v
 *   [write_vm_file()] - Binary output
 *        |
 *        v
 *   [.varm file]
 *
 * QUICK START:
 * ============
 *
 *   // Simple single-step assembly
 *   assemble("input.s", "output.varm");
 *
 *   // Or step-by-step for more control
 *   tokenize(source, tokens, MAX_TOKENS);
 *   parse(tokens, count, &program);
 *   write_vm_file(&program, "output.varm");
 *
 * LIMITS:
 * =======
 *   - Maximum labels: 256
 *   - Maximum instructions: 4096
 *   - Maximum data size: 65536 bytes
 *   - Maximum literal pool entries: 256
 */

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../../include/vm.h"

#define MAX_LABELS 256
#define MAX_INSTRUCTIONS 4096
#define MAX_LITERAL_POOL_ENTRIES 256
#define MAX_LITERAL_POOL_REFS 256

/**
 * @brief Token type enumeration.
 *
 * @details Enumerates all token types produced by the lexer.
 * Tokens represent the atomic units of the source language.
 */
typedef enum {
    TOKEN_EOF,         /**< End of file marker */
    TOKEN_INSTRUCTION, /**< Instruction mnemonic (mov, add, etc.) */
    TOKEN_DIRECTIVE,   /**< Assembler directive (.text, .data, etc.) */
    TOKEN_LABEL,       /**< Label definition (identifier with trailing :) */
    TOKEN_REGISTER,    /**< Register name (r0-r15, sp, lr, pc) */
    TOKEN_IMMEDIATE,   /**< Numeric constant */
    TOKEN_IDENTIFIER,  /**< User-defined identifier */
    TOKEN_STRING,      /**< Quoted string literal */
    TOKEN_COMMA,       /**< Comma separator */
    TOKEN_EQUAL,       /**< Assignment operator */
    TOKEN_LBRACKET,    /**< Left bracket [ */
    TOKEN_RBRACKET,    /**< Right bracket ] */
    TOKEN_EXCLAM,      /**< Exclamation mark */
    TOKEN_HASH,        /**< Immediate prefix # */
    TOKEN_NEWLINE      /**< Line terminator */
} token_type_t;

/**
 * @brief Token structure.
 *
 * @details Represents a single lexical token with its type,
 * associated value, and source location.
 */
typedef struct {
    token_type_t type; /**< Token classification */
    char* value;       /**< String value (heap-allocated, caller frees) */
    int line;          /**< Source line number (1-based) */
    int column;        /**< Source column number (1-based) */
} token_t;

/**
 * @brief Label structure.
 *
 * @details Represents a named position in the code or data section.
 */
typedef struct {
    char name[64]; /**< Label name (null-terminated, max 63 chars) */
    u32 address;   /**< Associated address */
} label_t;

/**
 * @brief Relocation entry.
 *
 * @details Records a location that requires address fixup after
 * label resolution.
 */
typedef struct {
    u32 address;   /**< Instruction address requiring fixup */
    char name[64]; /**< Label name to resolve */
    int is_branch; /**< Set to 1 for branch relocations */
} reloc_t;

/**
 * @brief Literal pool entry.
 *
 * @details Stores a 32-bit constant that can be loaded via
 * PC-relative LDR instructions.
 */
typedef struct {
    u32 value;  /**< The constant value */
    u32 offset; /**< Offset in text section */
} literal_pool_entry_t;

/**
 * @brief Literal pool reference.
 *
 * @details Links an LDR instruction to its literal pool entry.
 */
typedef struct {
    u32 instr_addr; /**< Address of the LDR instruction */
    int pool_index; /**< Index into literal_pool array */
} literal_pool_ref_t;

/**
 * @brief Complete program state.
 *
 * @details Contains all information about an assembled program,
 * including text (code), data sections, labels, and relocations.
 */
typedef struct {
    u32 text[MAX_INSTRUCTIONS];                                  /**< Encoded instructions */
    u32 text_size;                                               /**< Number of instructions */
    u8 data[65536];                                              /**< Data section bytes */
    u32 data_size;                                               /**< Data section size */
    label_t labels[MAX_LABELS];                                  /**< Defined labels */
    int label_count;                                             /**< Number of labels */
    reloc_t relocs[MAX_LABELS];                                  /**< Pending relocations */
    int reloc_count;                                             /**< Relocation count */
    literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES]; /**< Constants */
    int literal_pool_count;                                      /**< Pool entry count */
    literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS]; /**< Pool refs */
    int literal_pool_ref_count;                                  /**< Reference count */
    u32 current_addr;                                            /**< Current assembly address */
    int in_text_section;                                         /**< 1 if in .text, 0 if .data */
} program_state_t;

/**
 * @brief Tokenizes assembly source code.
 *
 * @details Performs lexical analysis on the input string, producing an
 * array of tokens for the parser. Token values are heap-allocated and
 * must be freed by calling free_tokens().
 *
 * @param input Null-terminated source code string
 * @param tokens Pre-allocated array to store tokens
 * @param max_tokens Maximum number of tokens to produce
 * @return int Number of tokens produced (including TOKEN_EOF)
 *
 * @see free_tokens() Memory cleanup
 * @see token_t Token structure
 *
 * @note Time complexity: O(n) where n is input length
 * @note Token values must be freed with free_tokens()
 */
int
tokenize(const char* input, token_t* tokens, int max_tokens);

/**
 * @brief Frees memory allocated for token values.
 *
 * @details Releases heap memory for all token value strings.
 * The token array itself is not freed.
 *
 * @param tokens The token array
 * @param count Number of tokens in the array
 *
 * @note Safe to call with NULL tokens (no-op)
 */
void
free_tokens(token_t* tokens, int count);

/**
 * @brief Parses tokens into machine code.
 *
 * @details Converts a token stream into encoded instructions.
 * Handles all instruction types, directives, labels, and relocations.
 *
 * @param tokens The token array from tokenize()
 * @param token_count Number of tokens in the array
 * @param prog Pointer to output program_state_t structure
 * @return int 0 on success, -1 on error
 *
 * @see tokenize() Tokenization
 * @see write_vm_file() Output generation
 *
 * @retval 0 Success
 * @retval -1 Parse error
 */
int
parse(token_t* tokens, int token_count, program_state_t* prog);

/**
 * @brief Writes assembled program to a .varm file.
 *
 * @details Creates a binary file with the varm executable format
 * containing header, text section, and data section.
 *
 * @param prog Pointer to the assembled program
 * @param filename Output file path
 * @return int 0 on success, -1 on file error
 *
 * @see parse() Assembly
 *
 * @retval 0 Success
 * @retval -1 File open/write error
 */
int
write_vm_file(program_state_t* prog, const char* filename);

/**
 * @brief Assembles a source file to a .varm file.
 *
 * @details High-level convenience function that performs the complete
 * assembly pipeline: read, tokenize, parse, write.
 *
 * @param input_file Path to assembly source file
 * @param output_file Path for output .varm file
 * @return int 0 on success, -1 on error
 *
 * @see tokenize() Tokenization
 * @see parse() Parsing
 * @see write_vm_file() Output
 *
 * @retval 0 Success
 * @retval -1 File error or assembly error
 */
int
assemble(const char* input_file, const char* output_file);

/**
 * @brief Global assembler debug flag (legacy, for backward compatibility).
 */
extern int asm_debug;

/**
 * @brief Enable a debug tag for the assembler.
 *
 * @param tag Tag name: LABEL, POOL, SYM, EMIT, INSTR, ALL
 * @return 0 on success, -1 if tag not found
 */
int
asm_debug_enable(const char* tag);

/**
 * @brief Disable all debug tags.
 */
void
asm_debug_disable_all(void);

/**
 * @brief List available debug tags.
 */
void
asm_debug_list_tags(void);

#endif
