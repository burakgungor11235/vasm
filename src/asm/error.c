/*
 * varm Assembler - Error Handling Implementation
 * Collect all errors, print at end, return error code
 *
 * Errors are stored in a fixed-size array for fast, allocation-free
 * error collection. Messages use vsnprintf for safe formatting.
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

/**
 * @brief Initializes an error context.
 *
 * @details Resets error counters and stores the filename for use in
 * error messages. No memory allocation is performed.
 *
 * @param ctx Pointer to the error context
 * @param filename The source filename (can be NULL)
 */
void
error_init(error_context_t* ctx, const char* filename)
{
    ctx->count = 0;
    ctx->warnings = 0;
    ctx->filename = filename;
}

/**
 * @brief Destroys an error context.
 *
 * @details Currently a no-op as no dynamic allocation is performed.
 * Provided for API symmetry and potential future changes.
 *
 * @param ctx Pointer to the error context (unused)
 */
void
error_destroy(error_context_t* ctx)
{
    (void)ctx;
}

/**
 * @brief Adds an error to the context with formatted message.
 *
 * @details Records an error with source location, type, and message.
 * Uses vsnprintf for safe, bounds-checked formatting.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param type The error classification
 * @param fmt Printf-style format string
 * @param ... Variable arguments for formatting
 *
 * @note Stops collecting after MAX_ERRORS to prevent overflow
 */
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

/**
 * @brief Adds a warning to the context.
 *
 * @details Warnings are non-fatal issues that don't prevent assembly
 * but may indicate problematic code. They are counted separately but
 * stored in the same array as errors.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param fmt Printf-style format string
 * @param ... Format arguments
 */
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

/**
 * @brief Creates a syntax error with expected/found token info.
 *
 * @details Convenience function for reporting token mismatches.
 * Formats a message showing what was expected vs what was found.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param expected The expected token
 * @param found The actual token found
 */
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

/**
 * @brief Reports an unexpected token (alias for error_syntax).
 *
 * @details Provides semantic clarity for unexpected token errors.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param expected The expected token
 * @param found The unexpected token
 *
 * @see error_syntax()
 */
void
error_unexpected(error_context_t* ctx, int line, int col, const char* expected, const char* found)
{
    error_syntax(ctx, line, col, expected, found);
}

/**
 * @brief Checks if any errors were recorded.
 *
 * @param ctx Pointer to the error context
 * @return int Non-zero if errors exist, zero if clean
 */
int
error_has_errors(error_context_t* ctx)
{
    return ctx->count > 0;
}

/**
 * @brief Returns the number of errors.
 *
 * @param ctx Pointer to the error context
 * @return int Count of errors (warnings not included)
 */
int
error_count(error_context_t* ctx)
{
    return ctx->count;
}

/**
 * @brief Returns the number of warnings.
 *
 * @param ctx Pointer to the error context
 * @return int Count of warnings
 */
int
error_warning_count(error_context_t* ctx)
{
    return ctx->warnings;
}

/**
 * @brief Prints all collected errors and warnings.
 *
 * @details Outputs each error/warning in the format:
 *   "filename:line:col: [error (type): | warning:] message"
 *
 * @param ctx Pointer to the error context
 * @param out Output stream (e.g., stderr)
 */
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

/**
 * @brief Converts error type to human-readable string.
 *
 * @param type The error type code
 * @return const char* Error description or "unknown" if invalid
 */
const char*
error_type_name(error_type_t type)
{
    if (type < 0 || type >= ERR_MAX) {
	return "unknown";
    }
    return error_type_strings[type];
}
