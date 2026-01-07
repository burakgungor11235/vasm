#ifndef OPCODE_H
#define OPCODE_H

#include <stdint.h>

#define MEMORY_SIZE (1024 * 1024)
#define TEXT_OFFSET 0x00000000
#define DATA_OFFSET 0x00010000

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef enum {
  COND_EQ = 0x0,
  COND_NE = 0x1,
  COND_CS = 0x2,
  COND_CC = 0x3,
  COND_MI = 0x4,
  COND_PL = 0x5,
  COND_VS = 0x6,
  COND_VC = 0x7,
  COND_HI = 0x8,
  COND_LS = 0x9,
  COND_GE = 0xA,
  COND_LT = 0xB,
  COND_GT = 0xC,
  COND_LE = 0xD,
  COND_AL = 0xE,
} condition_t;

typedef enum {
  OP_MOV = 0x00,
  OP_MVN = 0x01,
  OP_ADD = 0x02,
  OP_ADC = 0x03,
  OP_SUB = 0x04,
  OP_SBC = 0x05,
  OP_RSB = 0x06,
  OP_RSC = 0x07,
  OP_AND = 0x08,
  OP_EOR = 0x09,
  OP_ORR = 0x0A,
  OP_BIC = 0x0B,
  OP_CMP = 0x0C,
  OP_CMN = 0x0D,
  OP_TST = 0x0E,
  OP_TEQ = 0x0F,
  OP_MUL = 0x10,
  OP_MLA = 0x11,
  OP_LDR = 0x20,
  OP_LDRB = 0x21,
  OP_STR = 0x22,
  OP_STRB = 0x23,
  OP_LDM = 0x24,
  OP_STM = 0x25,
  OP_B = 0x30,
  OP_BL = 0x31,
  OP_BX = 0x32,
  OP_HALT = 0x40,
  OP_SWI = 0x41,
  OP_NOP = 0x42,
} opcode_t;

typedef enum {
  SHIFT_LSL = 0,
  SHIFT_LSR = 1,
  SHIFT_ASR = 2,
  SHIFT_ROR = 3,
} shift_type_t;

typedef struct {
  u8 opcode;
  u8 cond;
  u8 rd;
  u8 rn;
  u32 operand;
} instruction_t;

typedef struct {
  u8 is_immediate;
  u32 value;
  u8 rm;
  u8 shift_type;
  u8 shift_imm;
  u8 shift_reg;
} operand2_t;

typedef struct {
  u32 r[8];
  u32 sp;
  u32 lr;
  u32 pc;
  u32 cpsr;
} registers_t;

typedef struct {
  u8 *memory;
  u32 text_offset;
  u32 text_size;
  u32 data_offset;
  u32 data_size;
} memory_t;

typedef struct {
  registers_t regs;
  memory_t mem;
  int running;
  int debug;
  int exit_code;
} vm_state_t;

typedef enum {
  SYSCALL_EXIT = 1,
  SYSCALL_READ = 2,
  SYSCALL_WRITE = 3,
} syscall_num_t;

u32 check_condition(vm_state_t *vm, u8 cond);
void set_nzcv(vm_state_t *vm, u32 result);
u32 get_flag(vm_state_t *vm, int flag);

#endif
