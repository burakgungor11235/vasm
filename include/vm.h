#ifndef VM_H
#define VM_H

#include "opcode.h"
#include "magic_addrs.h"

vm_state_t*
vm_create(void);
void
vm_destroy(vm_state_t* vm);
int
vm_load(vm_state_t* vm, const char* filename);
void
vm_run(vm_state_t* vm);
void
vm_step(vm_state_t* vm);
void
vm_reset(vm_state_t* vm);

u32
vm_get_reg(vm_state_t* vm, int reg);
void
vm_set_reg(vm_state_t* vm, int reg, u32 value);
u32
vm_mem_read32(vm_state_t* vm, u32 addr);
u16
vm_mem_read16(vm_state_t* vm, u32 addr);
u8
vm_mem_read8(vm_state_t* vm, u32 addr);
void
vm_mem_write32(vm_state_t* vm, u32 addr, u32 value);
void
vm_mem_write16(vm_state_t* vm, u32 addr, u16 value);
void
vm_mem_write8(vm_state_t* vm, u32 addr, u8 value);

#endif
