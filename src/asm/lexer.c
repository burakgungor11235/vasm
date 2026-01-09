/**
 * @file lexer.c
 * @brief Lexer for the varm assembler - converts source code to tokens.
 *
 * @details This module implements the lexical analysis phase of the assembler.
 * It reads source code character-by-character and produces a stream of tokens
 * for the parser. The lexer handles:
 *
 * - Whitespace and comment stripping
 * - Number parsing (decimal, hexadecimal, binary, character literals)
 * - Identifier and label recognition
 * - Keyword and instruction classification
 * - Punctuation and operator tokenization
 *
 * @author varm Development Team
 * @version 0.1.0
 *
 * @warning This API is not stable. Function signatures and behavior may change.
 * @note Maximum token count is 4096.
 *
 * @see parser.c Parsing
 * @see assembler.h Public API
 *
 * TOKEN TYPES:
 * ============
 *   TOKEN_NEWLINE     - Line terminator
 *   TOKEN_EOF         - End of file marker
 *   TOKEN_COMMA       - Comma operator
 *   TOKEN_HASH        - Immediate prefix (#)
 *   TOKEN_EQUAL       - Assignment (=)
 *   TOKEN_LBRACKET    - Left bracket ([)
 *   TOKEN_RBRACKET    - Right bracket (])
 *   TOKEN_EXCLAM      - Exclamation mark (!)
 *   TOKEN_STRING      - Quoted string literal
 *   TOKEN_IMMEDIATE   - Numeric constant
 *   TOKEN_IDENTIFIER  - User-defined identifier
 *   TOKEN_LABEL       - Label definition (identifier with trailing :)
 *   TOKEN_INSTRUCTION - Assembly instruction mnemonic
 *   TOKEN_DIRECTIVE   - Assembler directive (.text, .data, etc.)
 *
 * LEXING EXAMPLE:
 * ===============
 * Input: "loop: mov r0, #42"
 *
 * Tokens:
 *   TOKEN_LABEL,    value="loop"
 *   TOKEN_INSTRUCTION, value="mov"
 *   TOKEN_IDENTIFIER,  value="r0"
 *   TOKEN_COMMA
 *   TOKEN_HASH
 *   TOKEN_IMMEDIATE, value="42"
 *   TOKEN_NEWLINE
 *   TOKEN_EOF
 */
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>

#include "../../include/assembler.h"

/**
 * @brief Checks if a character is whitespace.
 *
 * @details Whitespace characters: space (' '), tab ('\t'), carriage return ('\r').
 * Newline ('\n') is handled separately as a TOKEN_NEWLINE.
 *
 * @param c The character to check
 * @return int Non-zero if whitespace, zero otherwise
 */
static int
is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

/**
 * @brief Checks if a character can start an identifier.
 *
 * @details Valid identifier starts: letters (a-z, A-Z), underscore (_), dot (.).
 * The dot allows for directive names like .text, .data, etc.
 *
 * @param c The character to check
 * @return int Non-zero if valid start character, zero otherwise
 */
static int
is_identifier_start(char c)
{
    return isalpha(c) || c == '_' || c == '.';
}

/**
 * @brief Checks if a character is valid in an identifier.
 *
 * @details Valid identifier characters: alphanumeric (a-z, A-Z, 0-9),
 * underscore (_), and dot (.).
 *
 * @param c The character to check
 * @return int Non-zero if valid identifier character, zero otherwise
 */
static int
is_identifier_char(char c)
{
    return isalnum(c) || c == '_' || c == '.';
}

/**
 * @brief Checks if a character starts a number.
 *
 * @details Numbers start with decimal digits (0-9).
 * The base (decimal, hex, binary) is determined by prefix.
 *
 * @param c The character to check
 * @return int Non-zero if digit, zero otherwise
 */
static int
is_number_start(char c)
{
    return isdigit(c);
}

/**
 * @brief Creates a heap-allocated copy of a string with specified length.
 *
 * @details Allocates a new buffer of size (len + 1), copies len bytes from
 * the source, and appends a null terminator. Used for token values.
 *
 * @param str The source string to copy
 * @param len The number of bytes to copy
 * @return char* Newly allocated string, or NULL on allocation failure
 *
 * @note Caller is responsible for freeing the returned pointer
 * @warning Returns NULL if malloc fails
 */
static char*
strdup_len(const char* str, int len)
{
    char* result = malloc(len + 1);
    if (result) {
	memcpy(result, str, len);
	result[len] = '\0';
    }
    return result;
}

/**
 * @brief Main lexing function - converts source code to tokens.
 *
 * @details Performs lexical analysis on assembly source code, producing a
 * token stream for the parser. The lexer implements a simple state machine
 * that recognizes:
 *
 * - Comments: From ';' to end of line
 * - Strings: Double-quoted with escape handling
 * - Character literals: Single-quoted single characters
 * - Numbers: Decimal, hex (0x), binary (0b), and negative
 * - Identifiers: Letters, digits, underscore, dot
 * - Labels: Identifier followed by ':'
 * - Keywords: Instruction mnemonics and assembler directives
 * - Punctuation: Comma, brackets, hash, equals, etc.
 *
 * @param input The null-terminated source code string
 * @param tokens Pre-allocated array to store tokens
 * @param max_tokens Maximum number of tokens to produce
 * @return int The number of tokens produced, including TOKEN_EOF
 *
 * @note Time complexity: O(n) where n is the input length
 * @note Space complexity: O(k) where k is the number of tokens
 * @warning Truncates output if max_tokens is exceeded
 * @warning Token values are heap-allocated and must be freed via free_tokens()
 *
 * @see free_tokens() Memory cleanup
 * @see parser.c Token consumption
 */
int
tokenize(const char* input, token_t* tokens, int max_tokens)
{
    int token_count = 0;
    int line = 1;
    const char* p = input;

    while (*p && token_count < max_tokens) {
	if (is_whitespace(*p)) {
	    p++;
	    continue;
	}

	if (*p == '\n') {
	    tokens[token_count].type = TOKEN_NEWLINE;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    line++;
	    p++;
	    continue;
	}

	if (*p == ';') {
	    while (*p && *p != '\n')
		p++;
	    continue;
	}

	if (*p == '#') {
	    tokens[token_count].type = TOKEN_HASH;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == ',') {
	    tokens[token_count].type = TOKEN_COMMA;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == '=') {
	    tokens[token_count].type = TOKEN_EQUAL;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == '[') {
	    tokens[token_count].type = TOKEN_LBRACKET;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == ']') {
	    tokens[token_count].type = TOKEN_RBRACKET;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == '!') {
	    tokens[token_count].type = TOKEN_EXCLAM;
	    tokens[token_count].value = NULL;
	    tokens[token_count].line = line;
	    tokens[token_count].column = p - input;
	    token_count++;
	    p++;
	    continue;
	}

	if (*p == '"') {
	    const char* start = p;
	    p++;
	    while (*p && *p != '"')
		p++;
	    int len = p - start;
	    if (*p == '"')
		p++;
	    tokens[token_count].type = TOKEN_STRING;
	    tokens[token_count].value = strdup_len(start + 1, len - 1);
	    tokens[token_count].line = line;
	    token_count++;
	    continue;
	}

	if (*p == '\'') {
	    char ch = *(p + 1);
	    if (ch && *(p + 2) == '\'') {
		char value[8];
		snprintf(value, sizeof(value), "%d", (int)(unsigned char)ch);
		tokens[token_count].type = TOKEN_IMMEDIATE;
		tokens[token_count].value = strdup_len(value, strlen(value));
		tokens[token_count].line = line;
		token_count++;
		p += 3;
		continue;
	    }
	}

	if (*p == '-' && is_number_start(*(p + 1))) {
	    const char* start = p;
	    p++;
	    while (is_number_start(*p))
		p++;
	    int len = p - start;
	    tokens[token_count].type = TOKEN_IMMEDIATE;
	    tokens[token_count].value = strdup_len(start, len);
	    tokens[token_count].line = line;
	    token_count++;
	    continue;
	}

	if (is_number_start(*p)) {
	    const char* start = p;
	    while (is_number_start(*p) || *p == 'x' || *p == 'X' || (*p >= 'a' && *p <= 'f') ||
	           (*p >= 'A' && *p <= 'F'))
		p++;
	    int len = p - start;
	    tokens[token_count].type = TOKEN_IMMEDIATE;
	    tokens[token_count].value = strdup_len(start, len);
	    tokens[token_count].line = line;
	    token_count++;
	    continue;
	}

	if (is_identifier_start(*p)) {
	    const char* start = p;
	    while (is_identifier_char(*p))
		p++;
	    int len = p - start;

	    if (*p == ':') {
		tokens[token_count].type = TOKEN_LABEL;
		tokens[token_count].value = strdup_len(start, len);
		tokens[token_count].line = line;
		token_count++;
		p++;
	    } else {
		char* name = strdup_len(start, len);

		static const char* instructions[] = {
		        "mov", "mvn", "add", "adc", "sub", "sbc", "rsb", "rsc", "and", "eor", "orr",
		        "bic", "cmp", "cmn", "tst", "teq", "mul", "mla", "ldr", "ldrb", "str",
		        "strb", "b", "bl", "bx", "halt", "swi", "nop", "push", "pop", "call", "ret",
		        /* Conditional branches */
		        "beq", "bne", "bcs", "bhs", "bcc", "blo", "bmi", "bpl", "bvs", "bvc", "bhi",
		        "bls", "bge", "blt", "bgt", "ble", NULL};

		static const char* directives[] = {".text",  ".data",  ".word",   ".byte",
		                                   ".ascii", ".asciz", ".space",  ".align",
		                                   ".equ",   ".set",   ".global", NULL};

		int is_instruction = 0;
		for (int i = 0; instructions[i]; i++) {
		    if (strcasecmp(name, instructions[i]) == 0) {
			is_instruction = 1;
			break;
		    }
		}

		if (is_instruction) {
		    tokens[token_count].type = TOKEN_INSTRUCTION;
		} else {
		    int is_directive = 0;
		    for (int i = 0; directives[i]; i++) {
			if (strcasecmp(name, directives[i]) == 0) {
			    is_directive = 1;
			    break;
			}
		    }
		    tokens[token_count].type = is_directive ? TOKEN_DIRECTIVE : TOKEN_IDENTIFIER;
		}

		tokens[token_count].value = name;
		tokens[token_count].line = line;
		token_count++;
	    }
	    continue;
	}

	p++;
    }

    tokens[token_count].type = TOKEN_EOF;
    tokens[token_count].value = NULL;
    tokens[token_count].line = line;
    token_count++;

    return token_count;
}

/**
 * @brief Frees memory allocated for token values.
 *
 * @details Iterates through the token array and frees any heap-allocated
 * value strings. Token structures themselves are not freed as they are
 * typically stack-allocated.
 *
 * @param tokens The token array to free
 * @param count The number of tokens in the array
 *
 * @note This function is safe to call with NULL values (no-op)
 * @warning After calling, token value pointers are invalid
 */
void
free_tokens(token_t* tokens, int count)
{
    for (int i = 0; i < count; i++) {
	if (tokens[i].value) {
	    free(tokens[i].value);
	}
    }
}
