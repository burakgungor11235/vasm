#include "../include/vm.h"

u32
get_flag(vm_state_t* vm, int flag)
{
    return (vm->regs.cpsr >> flag) & 1;
}

void
set_flag(vm_state_t* vm, int flag, u32 value)
{
    if (value) {
	vm->regs.cpsr |= (1 << flag);
    } else {
	vm->regs.cpsr &= ~(1 << flag);
    }
}

void
set_nz(vm_state_t* vm, u32 result)
{
    set_flag(vm, CPSR_N_POS, (result >> CPSR_N_POS) & 1);
    set_flag(vm, CPSR_Z_POS, result == 0);
}

void
set_nzcv(vm_state_t* vm, u32 result)
{
    set_flag(vm, CPSR_N_POS, (result >> CPSR_N_POS) & 1);
    set_flag(vm, CPSR_Z_POS, result == 0);
    set_flag(vm, CPSR_C_POS, 0);
    set_flag(vm, CPSR_V_POS, 0);
}

u32
check_condition(vm_state_t* vm, u8 cond)
{
    if (cond == COND_AL) {
	return 1;
    }

    u32 n = get_flag(vm, CPSR_N_POS);
    u32 z = get_flag(vm, CPSR_Z_POS);
    u32 c = get_flag(vm, CPSR_C_POS);
    u32 v = get_flag(vm, CPSR_V_POS);

    switch (cond) {
    case COND_EQ:
	return z;
    case COND_NE:
	return !z;
    case COND_CS:
	return c;
    case COND_CC:
	return !c;
    case COND_MI:
	return n;
    case COND_PL:
	return !n;
    case COND_VS:
	return v;
    case COND_VC:
	return !v;
    case COND_HI:
	return c && !z;
    case COND_LS:
	return !c || z;
    case COND_GE:
	return n == v;
    case COND_LT:
	return n != v;
    case COND_GT:
	return !z && (n == v);
    case COND_LE:
	return z || (n != v);
    default:
	return 0;
    }
}
