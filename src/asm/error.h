/**
 * @file error.h
 * @brief Error handling interface for the assembler.
 *
 * @details Provides structured error reporting with source location tracking.
 * Errors are collected during parsing and can be printed at the end or
 * during compilation. Supports both fatal errors and warnings.
 *
 * @author varm Development Team
 * @version 0.1.0
 *
 * @warning This API is not stable. Error codes may change.
 *
 * @see error.c Implementation
 *
 * ERROR TYPES:
 * ============
 *   ERR_NONE               - No error / Warning
 *   ERR_UNKNOWN_INSTRUCTION - Unrecognized instruction mnemonic
 *   ERR_INVALID_REGISTER   - Invalid register name or number
 *   ERR_INVALID_IMMEDIATE  - Malformed immediate value
 *   ERR_LABEL_NOT_FOUND    - Reference to undefined label
 *   ERR_LABEL_REDEFINED    - Duplicate label definition
 *   ERR_OVERFLOW           - Numeric overflow
 *   ERR_SYNTAX             - General syntax error
 *   ERR_UNEXPECTED_TOKEN   - Token type mismatch
 *
 * USAGE PATTERN:
 * ==============
 *   error_context_t ctx;
 *   error_init(&ctx, "test.s");
 *   ...
 *   if (error_has_errors(&ctx)) {
 *       error_print_all(&ctx, stderr);
 *       return 1;
 *   }
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

/**
 * @brief Initializes an error context.
 *
 * @details Resets the error count and warnings, and sets the filename
 * for use in error messages.
 *
 * @param ctx Pointer to the error context
 * @param filename The source filename (can be NULL)
 */
void
error_init(error_context_t* ctx, const char* filename);

/**
 * @brief Destroys an error context.
 *
 * @details Currently a no-op but provided for symmetry with init.
 *
 * @param ctx Pointer to the error context to destroy
 */
void
error_destroy(error_context_t* ctx);

/**
 * @brief Adds an error to the context.
 *
 * @details Records an error with location information and formatted
 * message. Stops collecting if maximum errors reached.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number (1-based)
 * @param col Source column number (1-based)
 * @param type The error type
 * @param fmt Printf-style format string
 * @param ... Format arguments
 *
 * @note Maximum message length is 255 characters
 * @warning Silently drops errors after MAX_ERRORS
 */
void
error_add(error_context_t* ctx, int line, int col, error_type_t type, const char* fmt, ...);

/**
 * @brief Adds a warning to the context.
 *
 * @details Records a non-fatal warning. Warnings are counted separately
 * but stored in the same error array.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param fmt Printf-style format string
 * @param ... Format arguments
 *
 * @warning Silently drops warnings after MAX_ERRORS
 */
void
error_warning(error_context_t* ctx, int line, int col, const char* fmt, ...);

/**
 * @brief Reports a syntax error with expected/found tokens.
 *
 * @details Convenience function for token mismatch errors.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param expected What was expected
 * @param found What was actually found
 */
void
error_syntax(error_context_t* ctx, int line, int col, const char* expected, const char* found);

/**
 * @brief Reports an unexpected token error.
 *
 * @details Alias for error_syntax() for semantic clarity.
 *
 * @param ctx Pointer to the error context
 * @param line Source line number
 * @param col Source column number
 * @param expected What was expected
 * @param found What was actually found
 *
 * @see error_syntax()
 */
void
error_unexpected(error_context_t* ctx, int line, int col, const char* expected, const char* found);

/**
 * @brief Checks if any errors were recorded.
 *
 * @param ctx Pointer to the error context
 * @return int Non-zero if errors exist, zero otherwise
 */
int
error_has_errors(error_context_t* ctx);

/**
 * @brief Returns the total error count.
 *
 * @param ctx Pointer to the error context
 * @return int Number of errors (not including warnings)
 */
int
error_count(error_context_t* ctx);

/**
 * @brief Returns the warning count.
 *
 * @param ctx Pointer to the error context
 * @return int Number of warnings
 */
int
error_warning_count(error_context_t* ctx);

/**
 * @brief Prints all collected errors and warnings.
 *
 * @details Outputs formatted messages to the specified file stream.
 * Format: "filename:line:col: type: message"
 *
 * @param ctx Pointer to the error context
 * @param out Output file stream (e.g., stderr)
 */
void
error_print_all(error_context_t* ctx, FILE* out);

/**
 * @brief Gets the string name of an error type.
 *
 * @param type The error type code
 * @return const char* Human-readable error name
 */
const char*
error_type_name(error_type_t type);

#endif
