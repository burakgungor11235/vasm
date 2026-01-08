#include "../framework.h"
#include "../../asm/symbol_table.h"

static void
test_basic_insert_lookup(void)
{
    print_header("test_basic_insert_lookup");
    symbol_table_t tbl;
    symbol_init(&tbl, 256);

    u32 addr = 0;
    assert_true(__LINE__, symbol_lookup(&tbl, "foo", &addr) != 0);

    symbol_insert(&tbl, "foo", 0x1000);
    assert_eq(__LINE__, symbol_lookup(&tbl, "foo", &addr), 0);
    assert_eq(__LINE__, addr, 0x1000);

    symbol_destroy(&tbl);
}

static void
test_hash_distribution(void)
{
    print_header("test_hash_distribution");
    symbol_table_t tbl;
    symbol_init(&tbl, 256);

    for (int i = 0; i < 100; i++) {
	char name[16];
	snprintf(name, sizeof(name), "label_%d", i);
	symbol_insert(&tbl, name, i * 4);
    }

    assert_true(__LINE__, symbol_count(&tbl) == 100);

    symbol_destroy(&tbl);
}

static void
test_redefine_label(void)
{
    print_header("test_redefine_label");
    symbol_table_t tbl;
    symbol_init(&tbl, 256);

    symbol_insert(&tbl, "foo", 0x1000);
    u32 addr = 0;
    symbol_lookup(&tbl, "foo", &addr);
    assert_eq(__LINE__, addr, 0x1000);

    symbol_insert(&tbl, "foo", 0x2000);
    symbol_lookup(&tbl, "foo", &addr);
    assert_eq(__LINE__, addr, 0x2000);

    symbol_destroy(&tbl);
}

static void
test_not_found(void)
{
    print_header("test_not_found");
    symbol_table_t tbl;
    symbol_init(&tbl, 256);

    u32 addr = 0;
    assert_true(__LINE__, symbol_lookup(&tbl, "nonexistent", &addr) != 0);

    symbol_destroy(&tbl);
}

static void
test_clear(void)
{
    print_header("test_clear");
    symbol_table_t tbl;
    symbol_init(&tbl, 256);

    symbol_insert(&tbl, "foo", 0x1000);
    symbol_insert(&tbl, "bar", 0x2000);

    symbol_clear(&tbl);

    assert_eq(__LINE__, symbol_count(&tbl), 0);

    u32 addr = 0;
    assert_true(__LINE__, symbol_lookup(&tbl, "foo", &addr) != 0);

    symbol_destroy(&tbl);
}

static void
test_default_size(void)
{
    print_header("test_default_size");
    symbol_table_t tbl;
    symbol_init(&tbl, 0);

    assert_true(__LINE__, tbl.size >= 256);
    assert_true(__LINE__, tbl.mask == tbl.size - 1);

    symbol_destroy(&tbl);
}

static void (*tests[])(void) = {test_basic_insert_lookup,
                                test_hash_distribution,
                                test_redefine_label,
                                test_not_found,
                                test_clear,
                                test_default_size,
                                NULL};

int
main(void)
{
    run_tests("symbol_table", tests);
    print_summary();
    return g_tests_failed > 0 ? 1 : 0;
}
