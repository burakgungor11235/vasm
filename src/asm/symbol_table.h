/**
 * @file symbol_table.h
 * @brief Symbol table interface for label management.
 *
 * @details Implements a hash table for O(1) average-case label lookup using
 * the FNV-1a hash algorithm. The symbol table stores label names and their
 * associated addresses for branch targets and data references.
 *
 * @author varm Development Team
 * @version 0.1.0
 *
 * @warning This API is not stable. Function signatures may change.
 * @note Uses open addressing with linear probing.
 *
 * @see symbol_table.c Implementation
 *
 * HASH TABLE PROPERTIES:
 * ======================
 *   - Algorithm: FNV-1a 32-bit
 *   - Collision resolution: Linear probing
 *   - Load factor threshold: 0.7 (70%)
 *   - Default size: 256 entries (power of 2)
 *   - Maximum label length: 63 characters
 *
 * TIME COMPLEXITY:
 * ================
 *   symbol_init()     - O(n) where n is the table size
 *   symbol_insert()   - O(1) average, O(n) worst case
 *   symbol_lookup()   - O(1) average, O(n) worst case
 *   symbol_destroy()  - O(1)
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

/**
 * @brief Computes FNV-1a 32-bit hash of a string.
 *
 * @details The FNV-1a hash algorithm produces well-distributed hash values
 * for string keys. It is faster and more uniform than simple polynomial hashes.
 *
 * Algorithm:
 *   hash = FNV_offset_basis (0x811c9dc5)
 *   for each byte:
 *     hash ^= byte
 *     hash *= FNV_prime (0x01000193)
 *
 * @param str Null-terminated string to hash
 * @return u32 The 32-bit hash value
 *
 * @note Complexity: O(n) where n is string length
 */
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

/**
 * @brief Initializes a symbol table with specified capacity.
 *
 * @details Allocates and zero-initializes the hash table. The requested size
 * is rounded up to the next power of 2 for efficient modulo operations.
 *
 * @param tbl Pointer to the symbol table structure
 * @param size Requested number of entries (0 for default size)
 *
 * @note Default size is 256 entries when size is 0
 * @warning Call symbol_destroy() to free allocated memory
 */
void
symbol_init(symbol_table_t* tbl, u32 size);

/**
 * @brief Destroys a symbol table and frees all memory.
 *
 * @details Frees the entries array and resets all fields to zero.
 *
 * @param tbl Pointer to the symbol table to destroy
 *
 * @note Safe to call on already-destroyed tables
 */
void
symbol_destroy(symbol_table_t* tbl);

/**
 * @brief Inserts or updates a symbol in the table.
 *
 * @details If the symbol already exists, its address is updated.
 * If the table is full (load factor >= 0.7), insertion fails.
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name (max 63 characters)
 * @param addr The address to associate with the label
 * @return int 0 on success, -1 on failure (full table or duplicate)
 *
 * @retval 0 Success
 * @retval -1 Table full or allocation failed
 */
int
symbol_insert(symbol_table_t* tbl, const char* name, u32 addr);

/**
 * @brief Looks up a symbol by name.
 *
 * @details Performs hash table lookup to find the address of a label.
 * Returns -1 if the label is not found.
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name to look up
 * @param out_addr Pointer to store the found address
 * @return int 0 on success, -1 if label not found
 *
 * @retval 0 Found, address stored in *out_addr
 * @retval -1 Label not found
 */
int
symbol_lookup(symbol_table_t* tbl, const char* name, u32* out_addr);

/**
 * @brief Defines a symbol (alias for symbol_insert).
 *
 * @details Convenience function equivalent to symbol_insert().
 *
 * @param tbl Pointer to the symbol table
 * @param name The label name
 * @param addr The address to associate
 * @return int 0 on success, -1 on failure
 *
 * @see symbol_insert()
 */
int
symbol_define(symbol_table_t* tbl, const char* name, u32 addr);

/**
 * @brief Returns the number of symbols in the table.
 *
 * @param tbl Pointer to the symbol table
 * @return u32 The count of defined symbols
 */
u32
symbol_count(symbol_table_t* tbl);

/**
 * @brief Clears all symbols from the table.
 *
 * @details Zeroes the entries array and resets the count.
 * Does not free the underlying memory.
 *
 * @param tbl Pointer to the symbol table to clear
 */
void
symbol_clear(symbol_table_t* tbl);

#endif
