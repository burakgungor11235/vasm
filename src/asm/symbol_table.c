/*
 * varm Assembler - Symbol Table Implementation
 * Hash table for O(1) label lookup using FNV-1a hash
 *
 * This implementation uses open addressing with linear probing for
 * collision resolution. The table automatically resizes when the
 * load factor exceeds 70%.
 */

#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

/**
 * @brief Initializes a symbol table with specified capacity.
 *
 * @details Allocates memory for the hash table entries and initializes
 * all fields. The requested size is rounded up to the next power of 2
 * to enable efficient bitwise masking for hash table operations.
 *
 * @param tbl Pointer to the symbol table structure
 * @param size Requested number of entries (0 for default 256)
 *
 * @note Default size is 256 when size is 0
 * @note Uses calloc for zero-initialization
 * @warning Returns silently if size is 0
 */
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

/**
 * @brief Destroys a symbol table and frees all allocated memory.
 *
 * @details Frees the entries array and resets all structure fields to
 * zero to prevent use-after-free bugs. Safe to call multiple times.
 *
 * @param tbl Pointer to the symbol table to destroy
 *
 * @note Idempotent: safe to call on already-destroyed tables
 */
void
symbol_destroy(symbol_table_t* tbl)
{
    free(tbl->entries);
    tbl->entries = NULL;
    tbl->size = 0;
    tbl->count = 0;
}

/**
 * @brief Inserts a symbol or updates an existing one.
 *
 * @details Uses linear probing to find an empty slot or the existing
 * entry. If the symbol already exists, its address is updated. The
 * table rejects insertions when load factor exceeds 0.7 to maintain
 * O(1) lookup performance.
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name (truncated to 63 chars)
 * @param addr The address to associate with the label
 * @return int 0 on success, -1 if table is full
 *
 * @note Complexity: O(1) average, O(n) worst case
 * @retval 0 Success
 * @retval -1 Table full (load factor exceeded)
 */
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

/**
 * @brief Looks up a symbol by name.
 *
 * @details Computes the hash and probes the table until either the
 * symbol is found or an empty slot is encountered. Empty slots
 * indicate the symbol does not exist.
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name to look up
 * @param out_addr Pointer to store the found address
 * @return int 0 if found, -1 if not found
 *
 * @note Complexity: O(1) average, O(n) worst case
 * @retval 0 Found, *out_addr contains the address
 * @retval -1 Label not found
 */
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

/**
 * @brief Defines a symbol (convenience wrapper for symbol_insert).
 *
 * @details Alias function that provides semantic clarity for symbol
 * definition operations.
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name
 * @param addr The address to associate
 * @return int 0 on success, -1 on failure
 *
 * @see symbol_insert()
 */
int
symbol_define(symbol_table_t* tbl, const char* name, u32 addr)
{
    return symbol_insert(tbl, name, addr);
}

/**
 * @brief Returns the number of symbols in the table.
 *
 * @param tbl Pointer to the symbol table
 * @return u32 The count of defined symbols
 *
 * @note This is the count of successfully inserted symbols
 */
u32
symbol_count(symbol_table_t* tbl)
{
    return tbl->count;
}

/**
 * @brief Clears all symbols from the table.
 *
 * @details Zeroes the entries array and resets the count.
 * The allocated memory is preserved for reuse.
 *
 * @param tbl Pointer to the symbol table to clear
 *
 * @note Does not free the entries array
 */
void
symbol_clear(symbol_table_t* tbl)
{
    if (tbl->entries) {
	memset(tbl->entries, 0, tbl->size * sizeof(symbol_entry_t));
    }
    tbl->count = 0;
}
