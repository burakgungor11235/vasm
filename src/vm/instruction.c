#include "../include/vm.h"

void exec_mov(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_add(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_sub(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_and(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_orr(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_b(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_bl(vm_state_t *vm, u32 instr) {
  (void)vm;
  (void)instr;
}

void exec_halt(vm_state_t *vm) {
  vm->running = 0;
}
