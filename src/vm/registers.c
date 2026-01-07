#include "../include/vm.h"

u32
vm_get_reg(vm_state_t* vm, int reg)
{
    if (reg >= 0 && reg <= 7) {
	return vm->regs.r[reg];
    }
    switch (reg) {
    case 13:
	return vm->regs.sp;
    case 14:
	return vm->regs.lr;
    case 15:
	return vm->regs.pc;
    default:
	return 0;
    }
}

void
vm_set_reg(vm_state_t* vm, int reg, u32 value)
{
    if (reg >= 0 && reg <= 7) {
	vm->regs.r[reg] = value;
    } else {
	switch (reg) {
	case 13:
	    vm->regs.sp = value;
	    break;
	case 14:
	    vm->regs.lr = value;
	    break;
	case 15:
	    vm->regs.pc = value;
	    break;
	}
    }
}
