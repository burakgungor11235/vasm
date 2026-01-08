/*
 * varm Assembler - Symbol Table Implementation
 * Hash table for O(1) label lookup using FNV-1a hash
 */

#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

void
symbol_init(symbol_table_t* tbl, u32 size)
{
    if (size == 0) {
	size = SYMBOL_TABLE_DEFAULT_SIZE;
    }

    tbl->size = 1;
    while (tbl->size < size) {
	tbl->size *= 2;
    }

    tbl->mask = tbl->size - 1;
    tbl->count = 0;
    tbl->entries = calloc(tbl->size, sizeof(symbol_entry_t));
}

void
symbol_destroy(symbol_table_t* tbl)
{
    free(tbl->entries);
    tbl->entries = NULL;
    tbl->size = 0;
    tbl->count = 0;
}

int
symbol_insert(symbol_table_t* tbl, const char* name, u32 addr)
{
    if (tbl->entries == NULL || tbl->count >= tbl->size * 7 / 10) {
	return -1;
    }

    u32 hash = fnv1a_hash(name) & tbl->mask;
    u32 start = hash;

    while (tbl->entries[hash].name[0] != '\0') {
	if (strcmp(tbl->entries[hash].name, name) == 0) {
	    tbl->entries[hash].address = addr;
	    return 0;
	}
	hash = (hash + 1) & tbl->mask;
	if (hash == start) {
	    return -1;
	}
    }

    strncpy(tbl->entries[hash].name, name, 63);
    tbl->entries[hash].address = addr;
    tbl->entries[hash].defined = 1;
    tbl->count++;

    return 0;
}

int
symbol_lookup(symbol_table_t* tbl, const char* name, u32* out_addr)
{
    if (tbl->entries == NULL) {
	return -1;
    }

    u32 hash = fnv1a_hash(name) & tbl->mask;
    u32 start = hash;

    while (tbl->entries[hash].name[0] != '\0') {
	if (strcmp(tbl->entries[hash].name, name) == 0) {
	    *out_addr = tbl->entries[hash].address;
	    return 0;
	}
	hash = (hash + 1) & tbl->mask;
	if (hash == start) {
	    break;
	}
    }

    return -1;
}

int
symbol_define(symbol_table_t* tbl, const char* name, u32 addr)
{
    return symbol_insert(tbl, name, addr);
}

u32
symbol_count(symbol_table_t* tbl)
{
    return tbl->count;
}

void
symbol_clear(symbol_table_t* tbl)
{
    if (tbl->entries) {
	memset(tbl->entries, 0, tbl->size * sizeof(symbol_entry_t));
    }
    tbl->count = 0;
}
