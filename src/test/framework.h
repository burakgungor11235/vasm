#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/vm.h"

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define COLOR_RESET "\033[0m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"

typedef void (*test_fn)(void);

static void
assert_eq(int line, u32 actual, u32 expected)
{
    g_tests_run++;
    if (actual != expected) {
	g_tests_failed++;
	fprintf(stderr, COLOR_RED "  FAIL at line %d:" COLOR_RESET " expected 0x%08X, got 0x%08X\n",
	        line, expected, actual);
    } else {
	g_tests_passed++;
    }
}

static void
assert_true(int line, int condition)
{
    g_tests_run++;
    if (!condition) {
	g_tests_failed++;
	fprintf(stderr, COLOR_RED "  FAIL at line %d:" COLOR_RESET " expected TRUE, got FALSE\n",
	        line);
    } else {
	g_tests_passed++;
    }
}

static void
assert_false(int line, int condition)
{
    g_tests_run++;
    if (condition) {
	g_tests_failed++;
	fprintf(stderr, COLOR_RED "  FAIL at line %d:" COLOR_RESET " expected FALSE, got TRUE\n",
	        line);
    } else {
	g_tests_passed++;
    }
}

static void
assert_str_eq(int line, const char* actual, const char* expected)
{
    g_tests_run++;
    if (strcmp(actual, expected) != 0) {
	g_tests_failed++;
	fprintf(stderr, COLOR_RED "  FAIL at line %d:" COLOR_RESET " expected \"%s\", got \"%s\"\n",
	        line, expected, actual);
    } else {
	g_tests_passed++;
    }
}

static void
print_header(const char* name)
{
    printf("  " COLOR_YELLOW "[%s]" COLOR_RESET "\n", name);
}

static void
run_tests(const char* group_name, test_fn tests[])
{
    printf(COLOR_YELLOW "=== %s ===" COLOR_RESET "\n", group_name);
    for (int i = 0; tests[i] != NULL; i++) {
	tests[i]();
    }
}

static void
print_summary(void)
{
    printf("\n");
    printf(COLOR_YELLOW "==================" COLOR_RESET "\n");
    printf(COLOR_YELLOW "  TEST SUMMARY" COLOR_RESET "\n");
    printf(COLOR_YELLOW "==================" COLOR_RESET "\n");
    printf("  " COLOR_GREEN "PASSED: %d" COLOR_RESET "\n", g_tests_passed);
    printf("  " COLOR_RED "FAILED: %d" COLOR_RESET "\n", g_tests_failed);
    printf("  TOTAL: %d\n", g_tests_run);
    printf(COLOR_YELLOW "==================" COLOR_RESET "\n");
    if (g_tests_failed == 0) {
	printf(COLOR_GREEN "\n  ALL TESTS PASSED!\n" COLOR_RESET);
    } else {
	printf(COLOR_RED "\n  SOME TESTS FAILED!\n" COLOR_RESET);
    }
}

#endif
