/*
 * varm Assembler - Error Handling
 * Collect all errors, print at end, return error code
 */

#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>
#include <stdio.h>

#define MAX_ERRORS 64
#define MAX_ERROR_MSG 256

typedef enum {
    ERR_NONE = 0,
    ERR_UNKNOWN_INSTRUCTION,
    ERR_INVALID_REGISTER,
    ERR_INVALID_IMMEDIATE,
    ERR_LABEL_NOT_FOUND,
    ERR_LABEL_REDEFINED,
    ERR_OVERFLOW,
    ERR_SYNTAX,
    ERR_UNEXPECTED_TOKEN,
    ERR_MAX
} error_type_t;

typedef struct {
    int line;
    int column;
    error_type_t type;
    char message[MAX_ERROR_MSG];
} parser_error_t;

typedef struct {
    parser_error_t errors[MAX_ERRORS];
    int count;
    int warnings;
    const char* filename;
} error_context_t;

void
error_init(error_context_t* ctx, const char* filename);

void
error_destroy(error_context_t* ctx);

void
error_add(error_context_t* ctx, int line, int col, error_type_t type, const char* fmt, ...);

void
error_warning(error_context_t* ctx, int line, int col, const char* fmt, ...);

void
error_syntax(error_context_t* ctx, int line, int col, const char* expected, const char* found);

void
error_unexpected(error_context_t* ctx, int line, int col, const char* expected, const char* found);

int
error_has_errors(error_context_t* ctx);

int
error_count(error_context_t* ctx);

int
error_warning_count(error_context_t* ctx);

void
error_print_all(error_context_t* ctx, FILE* out);

const char*
error_type_name(error_type_t type);

#endif
