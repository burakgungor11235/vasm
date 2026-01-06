#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/vm.h"

extern void exec_mov(vm_state_t *vm, u32 instr);
extern void exec_mvn(vm_state_t *vm, u32 instr);
extern void exec_add(vm_state_t *vm, u32 instr);
extern void exec_adc(vm_state_t *vm, u32 instr);
extern void exec_sub(vm_state_t *vm, u32 instr);
extern void exec_sbc(vm_state_t *vm, u32 instr);
extern void exec_rsb(vm_state_t *vm, u32 instr);
extern void exec_rsc(vm_state_t *vm, u32 instr);
extern void exec_and(vm_state_t *vm, u32 instr);
extern void exec_eor(vm_state_t *vm, u32 instr);
extern void exec_orr(vm_state_t *vm, u32 instr);
extern void exec_bic(vm_state_t *vm, u32 instr);
extern void exec_cmp(vm_state_t *vm, u32 instr);
extern void exec_cmn(vm_state_t *vm, u32 instr);
extern void exec_tst(vm_state_t *vm, u32 instr);
extern void exec_teq(vm_state_t *vm, u32 instr);
extern void exec_mul(vm_state_t *vm, u32 instr);
extern void exec_mla(vm_state_t *vm, u32 instr);
extern void exec_ldr(vm_state_t *vm, u32 instr);
extern void exec_ldrb(vm_state_t *vm, u32 instr);
extern void exec_str(vm_state_t *vm, u32 instr);
extern void exec_strb(vm_state_t *vm, u32 instr);
extern void exec_b(vm_state_t *vm, u32 instr);
extern void exec_bl(vm_state_t *vm, u32 instr);
extern void exec_bx(vm_state_t *vm, u32 instr);
extern void exec_halt(vm_state_t *vm, u32 instr);
extern void exec_swi(vm_state_t *vm, u32 instr);
extern void exec_nop(vm_state_t *vm, u32 instr);

typedef void (*exec_fn)(vm_state_t *, u32);

static exec_fn exec_table[256] = {0};

static void init_exec_table(void) {
  exec_table[OP_MOV] = exec_mov;
  exec_table[OP_MVN] = exec_mvn;
  exec_table[OP_ADD] = exec_add;
  exec_table[OP_ADC] = exec_adc;
  exec_table[OP_SUB] = exec_sub;
  exec_table[OP_SBC] = exec_sbc;
  exec_table[OP_RSB] = exec_rsb;
  exec_table[OP_RSC] = exec_rsc;
  exec_table[OP_AND] = exec_and;
  exec_table[OP_EOR] = exec_eor;
  exec_table[OP_ORR] = exec_orr;
  exec_table[OP_BIC] = exec_bic;
  exec_table[OP_CMP] = exec_cmp;
  exec_table[OP_CMN] = exec_cmn;
  exec_table[OP_TST] = exec_tst;
  exec_table[OP_TEQ] = exec_teq;
  exec_table[OP_MUL] = exec_mul;
  exec_table[OP_MLA] = exec_mla;
  exec_table[OP_LDR] = exec_ldr;
  exec_table[OP_LDRB] = exec_ldrb;
  exec_table[OP_STR] = exec_str;
  exec_table[OP_STRB] = exec_strb;
  exec_table[OP_B] = exec_b;
  exec_table[OP_BL] = exec_bl;
  exec_table[OP_BX] = exec_bx;
  exec_table[OP_HALT] = exec_halt;
  exec_table[OP_SWI] = exec_swi;
  exec_table[OP_NOP] = exec_nop;
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
