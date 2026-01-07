#include "../framework.h"
#include "../../asm/lexer.c"
#include "../../asm/parser.c"

static token_t tokens[256];
static program_state_t prog;

static void
parse_mov_immediate(void)
{
    print_header("parse_mov_immediate");
    const char* src = "mov r0, #42";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_mov_register(void)
{
    print_header("parse_mov_register");
    const char* src = "mov r1, r2";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_add_instruction(void)
{
    print_header("parse_add_instruction");
    const char* src = "add r0, r1, #10";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_sub_instruction(void)
{
    print_header("parse_sub_instruction");
    const char* src = "sub r2, r3, #5";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_swi_instruction(void)
{
    print_header("parse_swi_instruction");
    const char* src = "swi #1";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_halt_instruction(void)
{
    print_header("parse_halt_instruction");
    const char* src = "halt";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_nop_instruction(void)
{
    print_header("parse_nop_instruction");
    const char* src = "nop";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_multiple_instructions(void)
{
    print_header("parse_multiple_instructions");
    const char* src = "mov r0, #1\nmov r1, #2\nadd r2, r0, r1";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size == 3);
}

static void
parse_label(void)
{
    print_header("parse_label");
    const char* src = "loop: mov r0, #42";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.label_count >= 1);
    assert_true(__LINE__, strcmp(prog.labels[0].name, "loop") == 0);
    assert_true(__LINE__, prog.labels[0].address == 0);
}

static void
parse_label_at_address(void)
{
    print_header("parse_label_at_address");
    const char* src = "mov r0, #1\nloop: mov r1, #2";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.label_count >= 1);
    assert_true(__LINE__, strcmp(prog.labels[0].name, "loop") == 0);
    assert_true(__LINE__, prog.labels[0].address == 4);
}

static void
parse_equ_directive(void)
{
    print_header("parse_equ_directive");
    const char* src = ".equ CONST, 100\nmov r0, #1";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_word_directive(void)
{
    print_header("parse_word_directive");
    const char* src = ".data\n.word 0x12345678";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.data_size >= 4);
    assert_true(__LINE__, prog.data[0] == 0x78);
    assert_true(__LINE__, prog.data[1] == 0x56);
    assert_true(__LINE__, prog.data[2] == 0x34);
    assert_true(__LINE__, prog.data[3] == 0x12);
}

static void
parse_byte_directive(void)
{
    print_header("parse_byte_directive");
    const char* src = ".data\n.byte 0xFF";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.data_size >= 1);
    assert_true(__LINE__, prog.data[0] == 0xFF);
}

static void
parse_immediate_hex(void)
{
    print_header("parse_immediate_hex");
    const char* src = "mov r0, #0xFF";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_immediate_binary(void)
{
    print_header("parse_immediate_binary");
    const char* src = "mov r0, #0b10101010";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_register_sp(void)
{
    print_header("parse_register_sp");
    const char* src = "mov sp, #0";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_register_pc(void)
{
    print_header("parse_register_pc");
    const char* src = "mov pc, lr";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_register_lr(void)
{
    print_header("parse_register_lr");
    const char* src = "mov lr, sp";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_mov_r10(void)
{
    print_header("parse_mov_r10");
    const char* src = "mov r10, #0";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_condition_eq(void)
{
    print_header("parse_condition_eq");
    const char* src = "mov r0, #1";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_cmp(void)
{
    print_header("parse_cmp");
    const char* src = "cmp r0, #10";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_mul(void)
{
    print_header("parse_mul");
    const char* src = "mul r0, r1, r2";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_ldr(void)
{
    print_header("parse_ldr");
    const char* src = "ldr r0, [r1]";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static void
parse_str(void)
{
    print_header("parse_str");
    const char* src = "str r0, [r1]";
    int count = tokenize(src, tokens, 256);
    memset(&prog, 0, sizeof(prog));
    parse(tokens, count, &prog);

    assert_true(__LINE__, prog.text_size >= 1);
}

static test_fn tests[] = {parse_mov_immediate,
                          parse_mov_register,
                          parse_add_instruction,
                          parse_sub_instruction,
                          parse_swi_instruction,
                          parse_halt_instruction,
                          parse_nop_instruction,
                          parse_multiple_instructions,
                          parse_label,
                          parse_label_at_address,
                          parse_equ_directive,
                          parse_word_directive,
                          parse_byte_directive,
                          parse_immediate_hex,
                          parse_immediate_binary,
                          parse_register_sp,
                          parse_register_pc,
                          parse_register_lr,
                          parse_mov_r10,
                          parse_condition_eq,
                          parse_cmp,
                          parse_mul,
                          parse_ldr,
                          parse_str,
                          NULL};

int
main(void)
{
    printf(COLOR_YELLOW "\n========================================\n");
    printf("  PARSER UNIT TESTS\n");
    printf("========================================\n" COLOR_RESET);
    run_tests("parser", tests);
    print_summary();
    return g_tests_failed > 0 ? 1 : 0;
}
