#include <stdio.h>
#include "../include/vm.h"

static u32 decode_operand2(vm_state_t *vm, u32 instr) {
  u8 is_immediate = (instr >> 25) & 1;
  if (is_immediate) {
    u8 rotate = (instr >> 8) & 0xF;
    u8 imm8 = instr & 0xFF;
    u32 value = imm8;
    if (rotate > 0) {
      value = (imm8 >> (rotate * 2)) | (imm8 << (32 - rotate * 2));
    }
    return value;
  } else {
    u8 rm = instr & 0xF;
    u8 shift_type = (instr >> 5) & 0x3;
    u8 shift_imm = (instr >> 7) & 0x1F;
    u32 value = vm_get_reg(vm, rm);
    switch (shift_type) {
      case 0: return value << shift_imm;
      case 1: return value >> shift_imm;
      case 2: return (value >> shift_imm) | ((value & 0x80000000) >> (32 - shift_imm));
      case 3: return (value >> shift_imm) | (value << (32 - shift_imm));
      default: return value;
    }
  }
}

void exec_mov(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  vm_set_reg(vm, rd, operand);
  set_nzcv(vm, operand);
}

void exec_mvn(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  vm_set_reg(vm, rd, ~operand);
  set_nzcv(vm, ~operand);
}

void exec_add(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) + operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_adc(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 carry = get_flag(vm, 29);
  u32 result = vm_get_reg(vm, rn) + operand + carry;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_sub(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) - operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_sbc(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 carry = get_flag(vm, 29);
  u32 result = vm_get_reg(vm, rn) - operand - (1 - carry);
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_rsb(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = operand - vm_get_reg(vm, rn);
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_rsc(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 carry = get_flag(vm, 29);
  u32 result = operand - vm_get_reg(vm, rn) - (1 - carry);
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_and(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) & operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_eor(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) ^ operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_orr(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) | operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_bic(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) & ~operand;
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_cmp(vm_state_t *vm, u32 instr) {
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) - operand;
  set_nzcv(vm, result);
}

void exec_cmn(vm_state_t *vm, u32 instr) {
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) + operand;
  set_nzcv(vm, result);
}

void exec_tst(vm_state_t *vm, u32 instr) {
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) & operand;
  set_nzcv(vm, result);
}

void exec_teq(vm_state_t *vm, u32 instr) {
  u8 rn = (instr >> 16) & 0xF;
  u32 operand = decode_operand2(vm, instr);
  u32 result = vm_get_reg(vm, rn) ^ operand;
  set_nzcv(vm, result);
}

void exec_mul(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rm = (instr >> 8) & 0xF;
  u8 rs = (instr >> 4) & 0xF;
  u32 result = vm_get_reg(vm, rm) * vm_get_reg(vm, rs);
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_mla(vm_state_t *vm, u32 instr) {
  u8 rd = (instr >> 12) & 0xF;
  u8 rm = (instr >> 8) & 0xF;
  u8 rs = (instr >> 4) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 result = vm_get_reg(vm, rm) * vm_get_reg(vm, rs) + vm_get_reg(vm, rn);
  vm_set_reg(vm, rd, result);
  set_nzcv(vm, result);
}

void exec_ldr(vm_state_t *vm, u32 instr) {
  u8 rt = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 offset = instr & 0xFFF;
  if (offset & 0x800) {
    offset |= 0xFFFFF000;
  }
  u32 addr = vm_get_reg(vm, rn) + offset;
  u32 value = vm_mem_read32(vm, addr);
  vm_set_reg(vm, rt, value);
}

void exec_ldrb(vm_state_t *vm, u32 instr) {
  u8 rt = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 offset = instr & 0xFFF;
  if (offset & 0x800) {
    offset |= 0xFFFFF000;
  }
  u32 addr = vm_get_reg(vm, rn) + offset;
  u8 value = vm_mem_read8(vm, addr);
  vm_set_reg(vm, rt, value);
}

void exec_str(vm_state_t *vm, u32 instr) {
  u8 rt = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 offset = instr & 0xFFF;
  if (offset & 0x800) {
    offset |= 0xFFFFF000;
  }
  u32 addr = vm_get_reg(vm, rn) + offset;
  u32 value = vm_get_reg(vm, rt);
  vm_mem_write32(vm, addr, value);
}

void exec_strb(vm_state_t *vm, u32 instr) {
  u8 rt = (instr >> 12) & 0xF;
  u8 rn = (instr >> 16) & 0xF;
  u32 offset = instr & 0xFFF;
  if (offset & 0x800) {
    offset |= 0xFFFFF000;
  }
  u32 addr = vm_get_reg(vm, rn) + offset;
  u8 value = vm_get_reg(vm, rt) & 0xFF;
  vm_mem_write8(vm, addr, value);
}

void exec_b(vm_state_t *vm, u32 instr) {
  u32 offset = instr & 0xFFFFFF;
  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }
  vm->regs.pc = (vm->regs.pc - 4) + 8 + (offset << 2);
}

void exec_bl(vm_state_t *vm, u32 instr) {
  vm->regs.lr = vm->regs.pc;
  u32 offset = instr & 0xFFFFFF;
  if (offset & 0x800000) {
    offset |= 0xFF000000;
  }
  vm->regs.pc = (vm->regs.pc - 4) + 8 + (offset << 2);
}

void exec_bx(vm_state_t *vm, u32 instr) {
  u8 rn = instr & 0xF;
  vm->regs.pc = vm_get_reg(vm, rn);
}

void exec_halt(vm_state_t *vm, u32 instr) {
  (void)instr;
  vm->running = 0;
}

void exec_swi(vm_state_t *vm, u32 instr) {
  u32 syscall = instr & 0xFFFFFF;
  (void)syscall;
  printf("SWI #%u\n", syscall);
  vm->running = 0;
}

void exec_nop(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}
