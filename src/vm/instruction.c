#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../include/vm.h"
#include "../include/debug.h"
#include "../include/magic_addrs.h"

static u32
decode_operand2(vm_state_t* vm, u32 instr)
{
    u32 operand = instr & OPERAND_MASK;
    u8 is_immediate = (operand >> 11) & 1;
    u8 rotate = (operand >> IMM_ROTATE_SHIFT) & 0x7;
    u8 imm8 = operand & IMM_VALUE_MASK;
    u32 value = imm8;
    if (rotate > 0) {
	value = (imm8 >> (rotate * 2)) | (imm8 << (32 - rotate * 2));
    }
    if (is_immediate) {
	return value;
    } else {
	u8 rm = instr & REG_MASK;
	u8 shift_type = (instr >> SHIFT_TYPE_SHIFT) & 0x3;
	u8 shift_imm = (instr >> SHIFT_IMM_SHIFT) & 0x1F;
	u32 reg_value = vm_get_reg(vm, rm);
	switch (shift_type) {
	case 0:
	    return reg_value << shift_imm;
	case 1:
	    return reg_value >> shift_imm;
	case 2:
	    return (reg_value >> shift_imm) | ((reg_value & VALUE_SIGN_BIT) >> (32 - shift_imm));
	case 3:
	    return (reg_value >> shift_imm) | (reg_value << (32 - shift_imm));
	default:
	    return reg_value;
	}
    }
}

void
exec_mov(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    vm_set_reg(vm, rd, operand);
    set_nzcv(vm, operand);
}

void
exec_mvn(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    vm_set_reg(vm, rd, ~operand);
    set_nzcv(vm, ~operand);
}

void
exec_add(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) + operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_adc(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 carry = get_flag(vm, 29);
    u32 result = vm_get_reg(vm, rn) + operand + carry;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_sub(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) - operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_sbc(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 carry = get_flag(vm, 29);
    u32 result = vm_get_reg(vm, rn) - operand - (1 - carry);
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_rsb(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = operand - vm_get_reg(vm, rn);
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_rsc(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 carry = get_flag(vm, 29);
    u32 result = operand - vm_get_reg(vm, rn) - (1 - carry);
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_and(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) & operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_eor(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) ^ operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_orr(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) | operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_bic(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) & ~operand;
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_cmp(vm_state_t* vm, u32 instr)
{
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) - operand;
    set_nzcv(vm, result);
}

void
exec_cmn(vm_state_t* vm, u32 instr)
{
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) + operand;
    set_nzcv(vm, result);
}

void
exec_tst(vm_state_t* vm, u32 instr)
{
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) & operand;
    set_nzcv(vm, result);
}

void
exec_teq(vm_state_t* vm, u32 instr)
{
    u8 rn = (instr >> 16) & 0xF;
    u32 operand = decode_operand2(vm, instr);
    u32 result = vm_get_reg(vm, rn) ^ operand;
    set_nzcv(vm, result);
}

void
exec_mul(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rm = (instr >> 8) & 0xF;
    u8 rs = (instr >> 4) & 0xF;
    u32 result = vm_get_reg(vm, rm) * vm_get_reg(vm, rs);
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_mla(vm_state_t* vm, u32 instr)
{
    u8 rd = (instr >> 12) & 0xF;
    u8 rm = (instr >> 8) & 0xF;
    u8 rs = (instr >> 4) & 0xF;
    u8 rn = (instr >> 16) & 0xF;
    u32 result = vm_get_reg(vm, rm) * vm_get_reg(vm, rs) + vm_get_reg(vm, rn);
    vm_set_reg(vm, rd, result);
    set_nzcv(vm, result);
}

void
exec_ldr(vm_state_t* vm, u32 instr)
{
    u8 rt = (instr >> RD_SHIFT) & REG_MASK;
    u8 rn = (instr >> RN_SHIFT) & REG_MASK;
    u32 offset = instr & OPERAND_MASK;
    if (offset & OFFSET_SIGN_BIT) {
	offset |= SIGN_EXTEND_12;
    }
    u32 addr = vm_get_reg(vm, rn) + offset;
    u32 value = vm_mem_read32(vm, addr);
    vm_set_reg(vm, rt, value);
}

void
exec_ldrb(vm_state_t* vm, u32 instr)
{
    u8 rt = (instr >> RD_SHIFT) & REG_MASK;
    u8 rn = (instr >> RN_SHIFT) & REG_MASK;
    u32 offset = instr & OPERAND_MASK;
    if (offset & OFFSET_SIGN_BIT) {
	offset |= SIGN_EXTEND_12;
    }
    u32 addr = vm_get_reg(vm, rn) + offset;
    u8 value = vm_mem_read8(vm, addr);
    vm_set_reg(vm, rt, value);
}

void
exec_str(vm_state_t* vm, u32 instr)
{
    u8 rt = (instr >> RD_SHIFT) & REG_MASK;
    u8 rn = (instr >> RN_SHIFT) & REG_MASK;
    u32 offset = instr & OPERAND_MASK;
    if (offset & OFFSET_SIGN_BIT) {
	offset |= SIGN_EXTEND_12;
    }
    u32 addr = vm_get_reg(vm, rn) + offset;
    u32 value = vm_get_reg(vm, rt);
    vm_mem_write32(vm, addr, value);
}

void
exec_strb(vm_state_t* vm, u32 instr)
{
    u8 rt = (instr >> RD_SHIFT) & REG_MASK;
    u8 rn = (instr >> RN_SHIFT) & REG_MASK;
    u32 offset = instr & OPERAND_MASK;
    if (offset & OFFSET_SIGN_BIT) {
	offset |= SIGN_EXTEND_12;
    }
    u32 addr = vm_get_reg(vm, rn) + offset;
    u8 value = vm_get_reg(vm, rt) & BYTE_MASK;
    vm_mem_write8(vm, addr, value);
}

void
exec_b(vm_state_t* vm, u32 instr)
{
    u32 offset = instr & BRANCH20_OFFSET_MASK;
    if (offset & BRANCH20_SIGN_BIT) {
	offset |= 0xFFF00000;
    }
    vm->regs.pc = (vm->regs.pc - 4) + 8 + (offset << 2);
}

void
exec_bl(vm_state_t* vm, u32 instr)
{
    vm->regs.lr = vm->regs.pc;
    u32 offset = instr & BRANCH20_OFFSET_MASK;
    if (offset & BRANCH20_SIGN_BIT) {
	offset |= 0xFFF00000;
    }
    vm->regs.pc = (vm->regs.pc - 4) + 8 + (offset << 2);
}

void
exec_bx(vm_state_t* vm, u32 instr)
{
    u8 rn = instr & REG_MASK;
    vm->regs.pc = vm_get_reg(vm, rn);
}

void
exec_halt(vm_state_t* vm, u32 instr)
{
    (void)instr;
    vm->running = 0;
}

void
exec_nop(vm_state_t* vm, u32 instr)
{
    (void)vm;
    (void)instr;
}

static int
syscall_validate_buffer(vm_state_t* vm, u32 addr, u32 size)
{
    if (size == 0) {
	return 0;
    }
    if (addr >= MEMORY_SIZE) {
	return -1;
    }
    if (addr + size > MEMORY_SIZE) {
	return -1;
    }
    return 0;
}

static void
syscall_handler(vm_state_t* vm)
{
    u32 syscall = vm->regs.r[7];
    u32 r0 = vm->regs.r[0];
    u32 r1 = vm->regs.r[1];
    u32 r2 = vm->regs.r[2];
    u32 result = 0;

    debug_syscall(vm->debug_config, vm, syscall);

    switch (syscall) {
    case SYSCALL_EXIT:
	vm->exit_code = r0 & BYTE_MASK;
	vm->running = 0;
	break;

    case SYSCALL_READ:
	if (r0 == 0) {
	    if (syscall_validate_buffer(vm, r1, r2) != 0) {
		vm->regs.r[0] = (u32)-1;
	    } else {
		result = read(STDIN_FILENO, (void*)(vm->mem.memory + r1), r2);
		vm->regs.r[0] = result;
	    }
	} else {
	    vm->regs.r[0] = (u32)-1;
	}
	break;

    case SYSCALL_WRITE:
	if (r0 == 1 || r0 == 2) {
	    if (syscall_validate_buffer(vm, r1, r2) != 0) {
		vm->regs.r[0] = (u32)-1;
	    } else {
		result = write(STDOUT_FILENO, (void*)(vm->mem.memory + r1), r2);
		vm->regs.r[0] = result;
		fflush(stdout);
	    }
	} else {
	    vm->regs.r[0] = (u32)-1;
	}
	break;

    default:
	printf("Unknown syscall: %u\n", syscall);
	vm->running = 0;
	break;
    }
}

void
exec_swi(vm_state_t* vm, u32 instr)
{
    (void)instr;
    syscall_handler(vm);
}
