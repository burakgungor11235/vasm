#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../../include/vm.h"

typedef enum {
  TOKEN_EOF,
  TOKEN_INSTRUCTION,
  TOKEN_DIRECTIVE,
  TOKEN_LABEL,
  TOKEN_REGISTER,
  TOKEN_IMMEDIATE,
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_COMMA,
  TOKEN_LBRACKET,
  TOKEN_RBRACKET,
  TOKEN_EXCLAM,
  TOKEN_HASH,
  TOKEN_NEWLINE
} token_type_t;

typedef struct {
  token_type_t type;
  char *value;
  int line;
  int column;
} token_t;

int tokenize(const char *input, token_t *tokens, int max_tokens);
void free_tokens(token_t *tokens, int count);
int assemble(const char *input_file, const char *output_file);

#endif
