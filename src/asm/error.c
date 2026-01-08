/*
 * varm Assembler - Error Handling Implementation
 * Collect all errors, print at end, return error code
 */

#include <stdlib.h>
#include <string.h>
#include "error.h"

static const char* error_type_strings[ERR_MAX] = {"none",
                                                  "unknown instruction",
                                                  "invalid register",
                                                  "invalid immediate",
                                                  "label not found",
                                                  "label redefined",
                                                  "overflow",
                                                  "syntax error",
                                                  "unexpected token"};

void
error_init(error_context_t* ctx, const char* filename)
{
    ctx->count = 0;
    ctx->warnings = 0;
    ctx->filename = filename;
}

void
error_destroy(error_context_t* ctx)
{
    (void)ctx;
}

void
error_add(error_context_t* ctx, int line, int col, error_type_t type, const char* fmt, ...)
{
    if (ctx->count >= MAX_ERRORS) {
	return;
    }

    parser_error_t* err = &ctx->errors[ctx->count++];
    err->line = line;
    err->column = col;
    err->type = type;

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, MAX_ERROR_MSG, fmt, args);
    va_end(args);
}

void
error_warning(error_context_t* ctx, int line, int col, const char* fmt, ...)
{
    ctx->warnings++;

    if (ctx->count >= MAX_ERRORS) {
	return;
    }

    parser_error_t* err = &ctx->errors[ctx->count++];
    err->line = line;
    err->column = col;
    err->type = ERR_NONE;

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, MAX_ERROR_MSG, fmt, args);
    va_end(args);
}

void
error_syntax(error_context_t* ctx, int line, int col, const char* expected, const char* found)
{
    if (ctx->count >= MAX_ERRORS) {
	return;
    }

    parser_error_t* err = &ctx->errors[ctx->count++];
    err->line = line;
    err->column = col;
    err->type = ERR_SYNTAX;

    snprintf(err->message, MAX_ERROR_MSG, "syntax error: expected '%s', found '%s'", expected,
             found);
}

void
error_unexpected(error_context_t* ctx, int line, int col, const char* expected, const char* found)
{
    error_syntax(ctx, line, col, expected, found);
}

int
error_has_errors(error_context_t* ctx)
{
    return ctx->count > 0;
}

int
error_count(error_context_t* ctx)
{
    return ctx->count;
}

int
error_warning_count(error_context_t* ctx)
{
    return ctx->warnings;
}

void
error_print_all(error_context_t* ctx, FILE* out)
{
    for (int i = 0; i < ctx->count; i++) {
	parser_error_t* err = &ctx->errors[i];
	fprintf(out, "%s:%d:%d: ", ctx->filename ? ctx->filename : "<input>", err->line,
	        err->column);

	if (err->type != ERR_NONE) {
	    fprintf(out, "error (%s): ", error_type_strings[err->type]);
	} else {
	    fprintf(out, "warning: ");
	}

	fprintf(out, "%s\n", err->message);
    }
}

const char*
error_type_name(error_type_t type)
{
    if (type < 0 || type >= ERR_MAX) {
	return "unknown";
    }
    return error_type_strings[type];
}
