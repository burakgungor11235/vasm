#include "../framework.h"
#include "../../asm/error.h"

static void
test_basic_error(void)
{
    print_header("test_basic_error");
    error_context_t ctx;
    error_init(&ctx, "test.vasm");

    assert_true(__LINE__, !error_has_errors(&ctx));
    assert_eq(__LINE__, error_count(&ctx), 0);

    error_add(&ctx, 10, 5, ERR_UNKNOWN_INSTRUCTION, "unknown op: %s", "foo");

    assert_true(__LINE__, error_has_errors(&ctx));
    assert_eq(__LINE__, error_count(&ctx), 1);

    error_destroy(&ctx);
}

static void
test_multiple_errors(void)
{
    print_header("test_multiple_errors");
    error_context_t ctx;
    error_init(&ctx, "test.vasm");

    error_add(&ctx, 1, 1, ERR_INVALID_REGISTER, "invalid reg: %s", "r99");
    error_add(&ctx, 2, 1, ERR_INVALID_IMMEDIATE, "invalid imm: %s", "0xGG");
    error_add(&ctx, 3, 1, ERR_LABEL_NOT_FOUND, "label not found: %s", "foo");

    assert_eq(__LINE__, error_count(&ctx), 3);
    assert_eq(__LINE__, error_warning_count(&ctx), 0);

    error_destroy(&ctx);
}

static void
test_warnings(void)
{
    print_header("test_warnings");
    error_context_t ctx;
    error_init(&ctx, "test.vasm");

    error_add(&ctx, 1, 1, ERR_INVALID_REGISTER, "invalid reg");
    error_warning(&ctx, 2, 1, "deprecated syntax");

    assert_eq(__LINE__, error_count(&ctx), 2);
    assert_eq(__LINE__, error_warning_count(&ctx), 1);

    error_destroy(&ctx);
}

static void
test_syntax_error(void)
{
    print_header("test_syntax_error");
    error_context_t ctx;
    error_init(&ctx, "test.vasm");

    error_syntax(&ctx, 5, 10, "register", "immediate");

    assert_true(__LINE__, error_has_errors(&ctx));
    assert_true(__LINE__, ctx.errors[0].type == ERR_SYNTAX);

    error_destroy(&ctx);
}

static void
test_error_print(void)
{
    print_header("test_error_print");
    error_context_t ctx;
    error_init(&ctx, "test.vasm");

    error_add(&ctx, 42, 10, ERR_UNKNOWN_INSTRUCTION, "unknown op: %s", "xyz");

    FILE* tmp = fopen("/tmp/test_error_output.txt", "w");
    error_print_all(&ctx, tmp);
    fclose(tmp);

    FILE* check = fopen("/tmp/test_error_output.txt", "r");
    char line[256];
    fgets(line, sizeof(line), check);
    fclose(check);

    assert_true(__LINE__, strstr(line, "test.vasm:42:10") != NULL);
    assert_true(__LINE__, strstr(line, "unknown instruction") != NULL);

    error_destroy(&ctx);
}

static void
test_error_type_name(void)
{
    print_header("test_error_type_name");

    assert_str_eq(__LINE__, error_type_name(ERR_NONE), "none");
    assert_str_eq(__LINE__, error_type_name(ERR_UNKNOWN_INSTRUCTION), "unknown instruction");
    assert_str_eq(__LINE__, error_type_name(ERR_INVALID_REGISTER), "invalid register");
    assert_str_eq(__LINE__, error_type_name(ERR_SYNTAX), "syntax error");

    assert_str_eq(__LINE__, error_type_name(ERR_MAX), "unknown");
    assert_str_eq(__LINE__, error_type_name(-1), "unknown");
}

static void
test_filename(void)
{
    print_header("test_filename");
    error_context_t ctx;
    error_init(&ctx, "myfile.vasm");

    error_add(&ctx, 1, 1, ERR_SYNTAX, "test error");

    FILE* tmp = fopen("/tmp/test_filename.txt", "w");
    error_print_all(&ctx, tmp);
    fclose(tmp);

    FILE* check = fopen("/tmp/test_filename.txt", "r");
    char line[256];
    fgets(line, sizeof(line), check);
    fclose(check);

    assert_true(__LINE__, strstr(line, "myfile.vasm") != NULL);

    error_destroy(&ctx);
}

static void
test_null_filename(void)
{
    print_header("test_null_filename");
    error_context_t ctx;
    error_init(&ctx, NULL);

    error_add(&ctx, 1, 1, ERR_SYNTAX, "test error");

    FILE* tmp = fopen("/tmp/test_null_filename.txt", "w");
    error_print_all(&ctx, tmp);
    fclose(tmp);

    FILE* check = fopen("/tmp/test_null_filename.txt", "r");
    char line[256];
    fgets(line, sizeof(line), check);
    fclose(check);

    assert_true(__LINE__, strstr(line, "<input>") != NULL);

    error_destroy(&ctx);
}

static void (*tests[])(void) = {test_basic_error,  test_multiple_errors, test_warnings,
                                test_syntax_error, test_error_print,     test_error_type_name,
                                test_filename,     test_null_filename,   NULL};

int
main(void)
{
    run_tests("error", tests);
    print_summary();
    return g_tests_failed > 0 ? 1 : 0;
}
