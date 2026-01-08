#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "../../include/vm.h"
#include "../../include/debug.h"
#include "../../include/magic_addrs.h"

static const char* op_names[256] = {
        [0x00] = "MOV",  [0x01] = "MVN",  [0x02] = "ADD", [0x03] = "ADC", [0x04] = "SUB",
        [0x05] = "SBC",  [0x06] = "RSB",  [0x07] = "RSC", [0x08] = "AND", [0x09] = "EOR",
        [0x0A] = "ORR",  [0x0B] = "BIC",  [0x0C] = "CMP", [0x0D] = "CMN", [0x0E] = "TST",
        [0x0F] = "TEQ",  [0x10] = "MUL",  [0x11] = "MLA", [0x20] = "LDR", [0x21] = "LDRB",
        [0x22] = "STR",  [0x23] = "STRB", [0x30] = "B",   [0x31] = "BL",  [0x32] = "BX",
        [0x40] = "HALT", [0x41] = "SWI",  [0x42] = "NOP",
};

static void
register_tag(debug_config_t* config, const char* name, const char* abbr, const char* desc,
             int severity)
{
    if (config->tag_count >= MAX_TAGS)
	return;
    debug_tag_t* t = &config->tags[config->tag_count++];
    t->name = name;
    t->abbr = abbr;
    t->description = desc;
    t->enabled = 0;
    t->severity = severity;
}

void
debug_init(debug_config_t* config)
{
    memset(config, 0, sizeof(*config));
    config->output = stdout;
    config->verbosity = -1;

    register_tag(config, "error", "ERR", "Show errors", SEV_ERROR);
    register_tag(config, "warn", "WARN", "Show warnings", SEV_WARN);
    register_tag(config, "info", "INFO", "Show informational messages", SEV_INFO);
    register_tag(config, "debug", "DBG", "Show debug messages", SEV_DEBUG);
    register_tag(config, "trace", "TRACE", "Show trace messages", SEV_TRACE);
    register_tag(config, "instr", "INSTR", "Trace instruction execution", 0);
    register_tag(config, "regs", "REGS", "Dump registers", 0);
    register_tag(config, "mem", "MEM", "Log memory accesses", 0);
    register_tag(config, "syscall", "SYSCALL", "Trace system calls", 0);
    register_tag(config, "stats", "STATS", "Show execution statistics", 0);
    register_tag(config, "asm", "ASM", "Assembler diagnostics", 0);
    register_tag(config, "label", "LABEL", "Label resolution debug", 0);
    register_tag(config, "pool", "POOL", "Literal pool debug", 0);
}

void
debug_destroy(debug_config_t* config)
{
    (void)config;
}

static debug_tag_t*
find_tag(debug_config_t* config, const char* name)
{
    for (int i = 0; i < config->tag_count; i++) {
	if (strcasecmp(config->tags[i].name, name) == 0 ||
	    strcasecmp(config->tags[i].abbr, name) == 0) {
	    return &config->tags[i];
	}
    }
    return NULL;
}

int
debug_set_verbosity(debug_config_t* config, int level)
{
    if (level < 0 || level > 5)
	return -1;
    config->verbosity = level;
    for (int i = 0; i < config->tag_count; i++) {
	if (config->tags[i].severity > 0 && config->tags[i].severity <= level) {
	    config->tags[i].enabled = 1;
	}
    }
    return 0;
}

int
debug_enable_tag(debug_config_t* config, const char* tag)
{
    debug_tag_t* t = find_tag(config, tag);
    if (!t)
	return -1;
    t->enabled = 1;
    return 0;
}

int
debug_disable_tag(debug_config_t* config, const char* tag)
{
    debug_tag_t* t = find_tag(config, tag);
    if (!t)
	return -1;
    t->enabled = 0;
    return 0;
}

int
debug_is_enabled(debug_config_t* config, const char* tag)
{
    debug_tag_t* t = find_tag(config, tag);
    if (!t)
	return 0;
    return t->enabled;
}

void
debug_list_tags(debug_config_t* config)
{
    fprintf(config->output, "Available debug tags:\n");
    for (int i = 0; i < config->tag_count; i++) {
	debug_tag_t* t = &config->tags[i];
	fprintf(config->output, "  %-10s %-5s %s\n", t->name, t->abbr, t->description);
    }
}

void
debug_instr(debug_config_t* config, vm_state_t* vm, u32 instr, u32 pc)
{
    if (!debug_is_enabled(config, "instr") && !debug_is_enabled(config, "trace"))
	return;

    config->instr_count++;

    u8 opcode = (instr >> OPCODE_SHIFT) & 0xFF;
    u8 cond = (instr >> COND_SHIFT) & 0xF;
    u8 rd = (instr >> RD_SHIFT) & REG_MASK;
    u8 rn = (instr >> RN_SHIFT) & REG_MASK;
    u32 operand = instr & OPERAND_MASK;

    const char* op_name = op_names[opcode];
    if (!op_name)
	op_name = "???";

    static const char* cond_names[16] = {"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
                                         "HI", "LS", "GE", "LT", "GT", "LE", "AL", "??"};

    fprintf(config->output, "[INSTR] pc=0x%08x %02x%1x%11s ", pc, opcode, cond, cond_names[cond]);

    switch (opcode) {
    case OP_MOV:
    case OP_MVN:
	fprintf(config->output, "%-4s r%u, #0x%03x\n", op_name, rd, operand);
	break;
    case OP_ADD:
    case OP_ADC:
    case OP_SUB:
    case OP_SBC:
    case OP_RSB:
    case OP_RSC:
    case OP_AND:
    case OP_EOR:
	fprintf(config->output, "%-4s r%u, r%u, #0x%03x\n", op_name, rd, rn, operand);
	break;
    case OP_MUL:
	fprintf(config->output, "%-4s r%u, r%u, r%u\n", op_name, rd, (instr >> 8) & REG_MASK,
	        (instr >> 4) & REG_MASK);
	break;
    case OP_MLA:
	fprintf(config->output, "%-4s r%u, r%u, r%u, r%u\n", op_name, rd, (instr >> 8) & REG_MASK,
	        (instr >> 4) & REG_MASK, rn);
	break;
    case OP_LDR:
    case OP_LDRB:
    case OP_STR:
    case OP_STRB:
	fprintf(config->output, "%-4s r%u, [r%u, #%d]\n", op_name, rd, rn,
	        (operand & OFFSET_SIGN_BIT) ? (int)(operand | SIGN_EXTEND_12) : (int)operand);
	break;
    case OP_B:
    case OP_BL:
	fprintf(config->output, "%-4s 0x%07x\n", op_name, instr & 0xFFFFF);
	break;
    case OP_BX:
	fprintf(config->output, "%-4s r%u\n", op_name, instr & REG_MASK);
	break;
    case OP_HALT:
	fprintf(config->output, "%-4s\n", op_name);
	break;
    case OP_SWI:
	fprintf(config->output, "%-4s\n", op_name);
	break;
    case OP_NOP:
	fprintf(config->output, "%-4s\n", op_name);
	break;
    default:
	fprintf(config->output, "0x%02x\n", opcode);
	break;
    }
}

void
debug_regs(debug_config_t* config, vm_state_t* vm)
{
    if (!debug_is_enabled(config, "regs"))
	return;

    fprintf(config->output, "[REGS]\n");
    for (int i = 0; i < 8; i++) {
	fprintf(config->output, "  r%-2u: 0x%08x", i, vm_get_reg(vm, i));
	if (i == 3)
	    fprintf(config->output, "\n");
    }
    fprintf(config->output, "\n");
    fprintf(config->output, "  sp: 0x%08x  lr: 0x%08x  pc: 0x%08x\n", vm_get_reg(vm, 13),
            vm_get_reg(vm, 14), vm_get_reg(vm, 15));
    fprintf(config->output, "  cpsr: 0x%08x  N=%u Z=%u C=%u V=%u\n", vm->regs.cpsr,
            (vm->regs.cpsr >> 31) & 1, (vm->regs.cpsr >> 30) & 1, (vm->regs.cpsr >> 29) & 1,
            (vm->regs.cpsr >> 28) & 1);
}

void
debug_mem(debug_config_t* config, u32 addr, u32 value, int is_read)
{
    if (!debug_is_enabled(config, "mem"))
	return;
    config->mem_reads += is_read;
    config->mem_writes += !is_read;
    fprintf(config->output, "[MEM] %s [0x%08x] = 0x%08x\n", is_read ? "read" : "write", addr,
            value);
}

void
debug_syscall(debug_config_t* config, vm_state_t* vm, u32 syscall)
{
    if (!debug_is_enabled(config, "syscall"))
	return;
    config->syscalls++;

    static const char* names[] = {NULL, "EXIT", "READ", "WRITE"};

    fprintf(config->output, "[SYSCALL] %s", syscall <= 3 ? names[syscall] : "UNKNOWN");
    if (syscall == 1) {
	fprintf(config->output, " code=%u", vm_get_reg(vm, 0));
    } else if (syscall == 2) {
	fprintf(config->output, " fd=%u addr=0x%08x count=%u", vm_get_reg(vm, 0), vm_get_reg(vm, 1),
	        vm_get_reg(vm, 2));
    } else if (syscall == 3) {
	fprintf(config->output, " fd=%u addr=0x%08x count=%u", vm_get_reg(vm, 0), vm_get_reg(vm, 1),
	        vm_get_reg(vm, 2));
    }
    fprintf(config->output, "\n");
}

void
debug_stats(debug_config_t* config, vm_state_t* vm)
{
    if (!debug_is_enabled(config, "stats"))
	return;
    (void)vm;
    fprintf(config->output, "[STATS]\n");
    fprintf(config->output, "  instructions: %lu\n", (unsigned long)config->instr_count);
    fprintf(config->output, "  memory reads: %lu\n", (unsigned long)config->mem_reads);
    fprintf(config->output, "  memory writes: %lu\n", (unsigned long)config->mem_writes);
    fprintf(config->output, "  syscalls: %lu\n", (unsigned long)config->syscalls);
}
