#include <stdlib.h>
#include <string.h>
#include "../include/vm.h"

vm_state_t*
vm_create(void)
{
    vm_state_t* vm = malloc(sizeof(vm_state_t));
    if (vm == NULL) {
	return NULL;
    }

    vm->mem.memory = malloc(MEMORY_SIZE);
    if (vm->mem.memory == NULL) {
	free(vm);
	return NULL;
    }

    memset(vm->mem.memory, 0, MEMORY_SIZE);
    vm_reset(vm);

    return vm;
}

void
vm_destroy(vm_state_t* vm)
{
    if (vm != NULL) {
	if (vm->mem.memory != NULL) {
	    free(vm->mem.memory);
	}
	free(vm);
    }
}

void
vm_reset(vm_state_t* vm)
{
    memset(&vm->regs, 0, sizeof(vm->regs));
    vm->mem.text_offset = TEXT_OFFSET;
    vm->mem.data_offset = DATA_OFFSET;
    vm->mem.text_size = 0;
    vm->mem.data_size = 0;
    vm->running = 1;
    vm->debug = 0;
    vm->exit_code = 0;
}

u32
vm_mem_read32(vm_state_t* vm, u32 addr)
{
    if (addr >= MEMORY_SIZE - 3) {
	return 0;
    }
    return (u32)vm->mem.memory[addr] | ((u32)vm->mem.memory[addr + 1] << 8) |
           ((u32)vm->mem.memory[addr + 2] << 16) | ((u32)vm->mem.memory[addr + 3] << 24);
}

uint16_t
vm_mem_read16(vm_state_t* vm, u32 addr)
{
    if (addr >= MEMORY_SIZE - 1) {
	return 0;
    }
    return (uint16_t)vm->mem.memory[addr] | ((uint16_t)vm->mem.memory[addr + 1] << 8);
}

uint8_t
vm_mem_read8(vm_state_t* vm, u32 addr)
{
    if (addr >= MEMORY_SIZE) {
	return 0;
    }
    return vm->mem.memory[addr];
}

void
vm_mem_write32(vm_state_t* vm, u32 addr, u32 value)
{
    if (addr >= MEMORY_SIZE - 3) {
	return;
    }
    vm->mem.memory[addr] = value & 0xFF;
    vm->mem.memory[addr + 1] = (value >> 8) & 0xFF;
    vm->mem.memory[addr + 2] = (value >> 16) & 0xFF;
    vm->mem.memory[addr + 3] = (value >> 24) & 0xFF;
}

void
vm_mem_write16(vm_state_t* vm, u32 addr, uint16_t value)
{
    if (addr >= MEMORY_SIZE - 1) {
	return;
    }
    vm->mem.memory[addr] = value & 0xFF;
    vm->mem.memory[addr + 1] = (value >> 8) & 0xFF;
}

void
vm_mem_write8(vm_state_t* vm, u32 addr, uint8_t value)
{
    if (addr >= MEMORY_SIZE) {
	return;
    }
    vm->mem.memory[addr] = value;
}
