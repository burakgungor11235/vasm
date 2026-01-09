#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "../../include/vm.h"

#define MAX_LABELS 256
#define MAX_INSTRUCTIONS 4096
#define MAX_LITERAL_POOL_ENTRIES 256
#define MAX_LITERAL_POOL_REFS 256

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
    TOKEN_EQUAL,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_EXCLAM,
    TOKEN_HASH,
    TOKEN_NEWLINE
} token_type_t;

typedef struct {
    token_type_t type;
    char* value;
    int line;
    int column;
} token_t;

typedef struct {
    char name[64];
    u32 address;
} label_t;

typedef struct {
    u32 address;
    char name[64];
    int is_branch;
} reloc_t;

typedef struct {
    u32 value;
    u32 offset;
} literal_pool_entry_t;

typedef struct {
    u32 instr_addr;
    int pool_index;
} literal_pool_ref_t;

typedef struct {
    u32 text[MAX_INSTRUCTIONS];
    u32 text_size;
    u8 data[65536];
    u32 data_size;
    label_t labels[MAX_LABELS];
    int label_count;
    reloc_t relocs[MAX_LABELS];
    int reloc_count;
    literal_pool_entry_t literal_pool[MAX_LITERAL_POOL_ENTRIES];
    int literal_pool_count;
    literal_pool_ref_t literal_pool_refs[MAX_LITERAL_POOL_REFS];
    int literal_pool_ref_count;
    u32 current_addr;
    int in_text_section;
} program_state_t;

int
tokenize(const char* input, token_t* tokens, int max_tokens);
void
free_tokens(token_t* tokens, int count);
int
parse(token_t* tokens, int token_count, program_state_t* prog);
int
write_vm_file(program_state_t* prog, const char* filename);
int
assemble(const char* input_file, const char* output_file);

extern int asm_debug;

#endif
