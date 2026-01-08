/*
 * varm Assembler - Symbol Table
 * Hash table for O(1) label lookup using FNV-1a hash
 */

#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "../../include/vm.h"

#define SYMBOL_TABLE_DEFAULT_SIZE 256

typedef struct {
    char name[64];
    u32 address;
    int defined;
} symbol_entry_t;

typedef struct {
    symbol_entry_t* entries;
    u32 size;
    u32 mask;
    u32 count;
} symbol_table_t;

static inline u32
fnv1a_hash(const char* str)
{
    u32 hash = 0x811c9dc5;
    while (*str) {
	hash ^= (u8)*str++;
	hash *= 0x01000193;
    }
    return hash;
}

void
symbol_init(symbol_table_t* tbl, u32 size);

void
symbol_destroy(symbol_table_t* tbl);

int
symbol_insert(symbol_table_t* tbl, const char* name, u32 addr);

int
symbol_lookup(symbol_table_t* tbl, const char* name, u32* out_addr);

int
symbol_define(symbol_table_t* tbl, const char* name, u32 addr);

u32
symbol_count(symbol_table_t* tbl);

void
symbol_clear(symbol_table_t* tbl);

#endif
