#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include "../../include/vm.h"
#include "../../include/assembler.h"

static int
is_whitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\r';
}

static int
is_identifier_start(char c)
{
    return isalpha(c) || c == '_' || c == '.';
}

static int
is_identifier_char(char c)
{
    return isalnum(c) || c == '_' || c == '.';
}

static int
is_number_start(char c)
{
    return isdigit(c);
}

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
		        "mov", "mvn",  "add", "adc",  "sub", "sbc", "rsb", "rsc",  "and",
		        "eor", "orr",  "bic", "cmp",  "cmn", "tst", "teq", "mul",  "mla",
		        "ldr", "ldrb", "str", "strb", "b",   "bl",  "bx",  "halt", "swi",
		        "nop", "push", "pop", "call", "ret", NULL};

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

void
free_tokens(token_t* tokens, int count)
{
    for (int i = 0; i < count; i++) {
	if (tokens[i].value) {
	    free(tokens[i].value);
	}
    }
}
