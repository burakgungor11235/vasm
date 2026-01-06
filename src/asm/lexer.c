#include <stdlib.h>
#include <string.h>
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
  TOKEN_NEWLINE
} token_type_t;

typedef struct {
  token_type_t type;
  char *value;
  int line;
  int column;
} token_t;

typedef struct {
  u32 *text;
  u32 text_size;
  u32 text_capacity;
  u32 data;
  u32 data_size;
  u32 data_capacity;
  u32 entry;
} program_t;

int tokenize(const char *input, token_t *tokens, int max_tokens) {
  (void)input;
  (void)tokens;
  (void)max_tokens;
  return 0;
}

int parse(const char *input, program_t *prog) {
  (void)input;
  (void)prog;
  return 0;
}

int assemble(const char *input_file, const char *output_file) {
  (void)input_file;
  (void)output_file;
  return 0;
}
