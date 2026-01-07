#include "../framework.h"
#include "../../asm/lexer.c"

static token_t tokens[256];

static void
tokenize_mov_instruction(void)
{
    print_header("tokenize_mov_instruction");
    const char* src = "mov r0, #42";
    int count = tokenize(src, tokens, 256);

    assert_true(__LINE__, count >= 3);
    assert_true(__LINE__, tokens[0].type == TOKEN_INSTRUCTION);
    assert_str_eq(__LINE__, tokens[0].value, "mov");
}

static void
tokenize_register_r0(void)
{
    print_header("tokenize_register_r0");
    const char* src = "r0";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IDENTIFIER);
    assert_str_eq(__LINE__, tokens[0].value, "r0");
}

static void
tokenize_register_r10(void)
{
    print_header("tokenize_register_r10");
    const char* src = "r10";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IDENTIFIER);
    assert_str_eq(__LINE__, tokens[0].value, "r10");
}

static void
tokenize_hash(void)
{
    print_header("tokenize_hash");
    const char* src = "#";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_HASH);
}

static void
tokenize_immediate_decimal(void)
{
    print_header("tokenize_immediate_decimal");
    const char* src = "42";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IMMEDIATE);
    assert_str_eq(__LINE__, tokens[0].value, "42");
}

static void
tokenize_immediate_hex(void)
{
    print_header("tokenize_immediate_hex");
    const char* src = "0x2A";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IMMEDIATE);
    assert_str_eq(__LINE__, tokens[0].value, "0x2A");
}

static void
tokenize_immediate_binary(void)
{
    print_header("tokenize_immediate_binary");
    const char* src = "0b101010";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IMMEDIATE);
    assert_str_eq(__LINE__, tokens[0].value, "0b101010");
}

static void
tokenize_comment(void)
{
    print_header("tokenize_comment");
    const char* src = "mov r0, #42 ; this is a comment";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_INSTRUCTION);
    assert_true(__LINE__, tokens[1].type == TOKEN_IDENTIFIER);
    assert_true(__LINE__, tokens[2].type == TOKEN_COMMA);
    assert_true(__LINE__, tokens[3].type == TOKEN_HASH);
    assert_true(__LINE__, tokens[4].type == TOKEN_IMMEDIATE);
}

static void
tokenize_label(void)
{
    print_header("tokenize_label");
    const char* src = "loop: mov r0, #42";
    tokenize(src, tokens, 256);

    printf("    DEBUG: token[0].type=%d TOKEN_LABEL=%d token[1].type=%d TOKEN_INSTRUCTION=%d\n",
           tokens[0].type, TOKEN_LABEL, tokens[1].type, TOKEN_INSTRUCTION);
    assert_true(__LINE__, tokens[0].type == TOKEN_LABEL);
    assert_str_eq(__LINE__, tokens[0].value, "loop");
    assert_true(__LINE__, tokens[1].type == TOKEN_INSTRUCTION);
}

static void
tokenize_comma(void)
{
    print_header("tokenize_comma");
    const char* src = ",";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_COMMA);
}

static void
tokenize_lbracket(void)
{
    print_header("tokenize_lbracket");
    const char* src = "[";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_LBRACKET);
}

static void
tokenize_rbracket(void)
{
    print_header("tokenize_rbracket");
    const char* src = "]";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_RBRACKET);
}

static void
tokenize_newline(void)
{
    print_header("tokenize_newline");
    const char* src = "mov r0, #42\nadd r1, r2, r3";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_INSTRUCTION);
    assert_true(__LINE__, tokens[5].type == TOKEN_NEWLINE);
    assert_true(__LINE__, tokens[6].type == TOKEN_INSTRUCTION);
}

static void
tokenize_eof(void)
{
    print_header("tokenize_eof");
    const char* src = "";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_EOF);
}

static void
tokenize_swi(void)
{
    print_header("tokenize_swi");
    const char* src = "swi";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_INSTRUCTION);
    assert_str_eq(__LINE__, tokens[0].value, "swi");
}

static void
tokenize_sp(void)
{
    print_header("tokenize_sp");
    const char* src = "sp";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IDENTIFIER);
    assert_str_eq(__LINE__, tokens[0].value, "sp");
}

static void
tokenize_pc(void)
{
    print_header("tokenize_pc");
    const char* src = "pc";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IDENTIFIER);
    assert_str_eq(__LINE__, tokens[0].value, "pc");
}

static void
tokenize_lr(void)
{
    print_header("tokenize_lr");
    const char* src = "lr";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_IDENTIFIER);
    assert_str_eq(__LINE__, tokens[0].value, "lr");
}

static void
tokenize_directive_text(void)
{
    print_header("tokenize_directive_text");
    const char* src = ".text";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_DIRECTIVE);
    assert_str_eq(__LINE__, tokens[0].value, ".text");
}

static void
tokenize_directive_data(void)
{
    print_header("tokenize_directive_data");
    const char* src = ".data";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_DIRECTIVE);
    assert_str_eq(__LINE__, tokens[0].value, ".data");
}

static void
tokenize_directive_word(void)
{
    print_header("tokenize_directive_word");
    const char* src = ".word";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_DIRECTIVE);
    assert_str_eq(__LINE__, tokens[0].value, ".word");
}

static void
tokenize_whitespace(void)
{
    print_header("tokenize_whitespace");
    const char* src = "   mov   r0  ,  #42   ";
    tokenize(src, tokens, 256);

    assert_true(__LINE__, tokens[0].type == TOKEN_INSTRUCTION);
    assert_true(__LINE__, tokens[1].type == TOKEN_IDENTIFIER);
    assert_true(__LINE__, tokens[2].type == TOKEN_COMMA);
    assert_true(__LINE__, tokens[3].type == TOKEN_HASH);
    assert_true(__LINE__, tokens[4].type == TOKEN_IMMEDIATE);
}

static test_fn tests[] = {tokenize_mov_instruction,
                          tokenize_register_r0,
                          tokenize_register_r10,
                          tokenize_hash,
                          tokenize_immediate_decimal,
                          tokenize_immediate_hex,
                          tokenize_immediate_binary,
                          tokenize_comment,
                          tokenize_label,
                          tokenize_comma,
                          tokenize_lbracket,
                          tokenize_rbracket,
                          tokenize_newline,
                          tokenize_eof,
                          tokenize_swi,
                          tokenize_sp,
                          tokenize_pc,
                          tokenize_lr,
                          tokenize_directive_text,
                          tokenize_directive_data,
                          tokenize_directive_word,
                          tokenize_whitespace,
                          NULL};

int
main(void)
{
    printf(COLOR_YELLOW "\n========================================\n");
    printf("  LEXER UNIT TESTS\n");
    printf("========================================\n" COLOR_RESET);
    run_tests("lexer", tests);
    print_summary();
    return g_tests_failed > 0 ? 1 : 0;
}
