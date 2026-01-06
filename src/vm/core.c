#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/vm.h"

static void exec_mov(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u32 operand = 0;
  (void)operand;
  vm_set_reg(vm, rd, 0);
  set_nzcv(vm, 0);
}

static void exec_add(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_sub(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_and(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_orr(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_b(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_bl(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

static void exec_halt(vm_state_t *vm, u32 instr) {
  (void)instr;
  vm->running = 0;
}

typedef void (*exec_fn)(vm_state_t *, u32);

static exec_fn exec_table[256] = {0};

static void init_exec_table(void) {
  exec_table[OP_MOV] = exec_mov;
  exec_table[OP_ADD] = exec_add;
  exec_table[OP_SUB] = exec_sub;
  exec_table[OP_AND] = exec_and;
  exec_table[OP_ORR] = exec_orr;
  exec_table[OP_B] = exec_b;
  exec_table[OP_BL] = exec_bl;
  exec_table[OP_HALT] = exec_halt;
}

void vm_step(vm_state_t *vm) {
  static int initialized = 0;
  if (!initialized) {
    init_exec_table();
    initialized = 1;
  }

  u32 instr = vm_mem_read32(vm, vm->regs.pc);
  u8 opcode = (instr >> 24) & 0xFF;
  u8 cond = (instr >> 20) & 0xF;

  if (check_condition(vm, cond)) {
    if (exec_table[opcode] != NULL) {
      exec_table[opcode](vm, instr);
    } else {
      printf("Unknown opcode: 0x%02X\n", opcode);
      vm->running = 0;
    }
  }
}

void vm_run(vm_state_t *vm) {
  while (vm->running) {
    vm_step(vm);
  }
}

static int check_header(const char *data) {
  return data[0] == 'A' && data[1] == 'R' && data[2] == 'M' && data[3] == 'V';
}

int vm_load(vm_state_t *vm, const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    return -1;
  }

  char header[32];
  if (fread(header, 1, 32, f) != 32) {
    fclose(f);
    return -1;
  }

  if (!check_header(header)) {
    fclose(f);
    return -1;
  }

  u32 text_offset, text_size, data_offset, data_size, entry;
  text_offset = *(u32 *)&header[4];
  text_size = *(u32 *)&header[8];
  data_offset = *(u32 *)&header[12];
  data_size = *(u32 *)&header[16];
  entry = *(u32 *)&header[20];

  vm->mem.text_offset = text_offset;
  vm->mem.text_size = text_size;
  vm->mem.data_offset = data_offset;
  vm->mem.data_size = data_size;

  if (fread(vm->mem.memory + text_offset, 1, text_size, f) != (size_t)text_size) {
    fclose(f);
    return -1;
  }

  if (data_size > 0) {
    if (fread(vm->mem.memory + data_offset, 1, data_size, f) != (size_t)data_size) {
      fclose(f);
      return -1;
    }
  }

  vm->regs.pc = entry;
  fclose(f);
  return 0;
}
