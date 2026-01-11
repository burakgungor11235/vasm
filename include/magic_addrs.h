#ifndef MAGIC_ADDRS_H
#define MAGIC_ADDRS_H

/*
 * varm - Magic Numbers and Bit Masks
 *
 * This file documents magic numbers used throughout the codebase with
 * explanations of their purpose and bit positions.
 *
 * INSTRUCTION FORMAT (32-bit):
 *   31:24 opcode  |  23:20 cond  |  19:16 rn  |  15:12 rd  |  11:0 operand
 *
 * OPERAND2 FIELD (12 bits):
 *   - Immediate form (bit 11 = 1):  11:imm  |  10:8 rotate  |  7:0 imm8
 *   - Register form (bit 11 = 0):   11:reg  |  10:9 unused  |  8:5 shift_imm  |  4:0 rm
 */

/* ============================================================
 * MEMORY LAYOUT
 * ============================================================ */

/* Total memory size: 1MB */
#define MEMORY_SIZE (1024 * 1024)

/* Text section starts at address 0 */
#define TEXT_OFFSET 0x00000000

/* Data section starts at address 0x10000 */
#define DATA_OFFSET 0x00010000

/* ============================================================
 * INSTRUCTION FIELD SHIFTS
 * ============================================================ */

/* Opcode field position (bits 24-31) */
#define OPCODE_SHIFT 24

/* Condition field position (bits 20-23) */
#define COND_SHIFT 20

/* First source register position (bits 16-19) */
#define RN_SHIFT 16

/* Destination register position (bits 12-15) */
#define RD_SHIFT 12

/* ============================================================
 * BIT MASKS FOR INSTRUCTION FIELD EXTRACTION
 * ============================================================ */

/* Mask for 12-bit operand field (bits 0-11) */
#define OPERAND_MASK 0xFFF

/* Sign bit for 12-bit offset (bit 11) - indicates negative offset when set */
#define OFFSET_SIGN_BIT 0x800

/* Sign bit for 24-bit branch offset (bit 23) */
#define BRANCH_SIGN_BIT 0x800000

/* Mask for 24-bit branch offset (bits 0-23) - for legacy/ARM compatibility */
#define BRANCH_OFFSET_MASK 0xFFFFFF

/* Sign bit for 20-bit branch offset (bit 19) */
#define BRANCH20_SIGN_BIT 0x80000

/* Mask for 20-bit branch offset (bits 0-19) - for varm instruction format */
#define BRANCH20_OFFSET_MASK 0xFFFFF

/* Mask for 12-bit operand/offset field (bits 0-11) */
#define OFFSET_MASK 0xFFF

/* Sign extension mask for 12-bit to 32-bit: 0xFFFFF000
 * Applied when OFFSET_SIGN_BIT is set to extend sign */
#define SIGN_EXTEND_12 0xFFFFF000

/* Sign extension mask for 24-bit to 32-bit: 0xFF000000
 * Applied when BRANCH_SIGN_BIT is set to extend sign */
#define SIGN_EXTEND_24 0xFF000000

/* ============================================================
 * OPERAND2 FIELD MASKS
 * ============================================================ */

/* Immediate mode flag - bit 11 of operand field
 * Set to 1 for immediate operands, 0 for register operands */
#define OPERAND_IMM_BIT (1 << 11)

/* Immediate flag for operand2 in ALU instructions (bit 19 of instruction)
 * When set, operand2 is in immediate form, else register form
 * Note: This is different from OPERAND_IMM_BIT which is bit 11 of operand2 field */
#define OPERAND_IMM_FLAG (1 << 19)

/* Rotate field in immediate operand (bits 8-10, 3 bits) */
#define IMM_ROTATE_MASK 0x700
#define IMM_ROTATE_SHIFT 8

/* Immediate value in immediate operand (bits 0-7, 8 bits) */
#define IMM_VALUE_MASK 0xFF

/* Register operand (bits 0-3, 4 bits) - selects source register */
#define REG_MASK 0xF

/* Shift type field (bits 5-6, 2 bits): LSL=0, LSR=1, ASR=2, ROR=3 */
#define SHIFT_TYPE_MASK 0x60
#define SHIFT_TYPE_SHIFT 5

/* 5-bit shift amount field for register-form ALU instructions (bits 7-11)
 * Extracted with: (instr >> 7) & 0x1F, giving values 0-31
 * 
 * Bit 11 is shared: it serves as the immediate/register form flag for the operand,
 * and when in register form (bit 11=0), it becomes the MSB of shift_imm (bit 4). */
#define SHIFT_IMM_MASK 0xF80
#define SHIFT_IMM_SHIFT 7

/* ============================================================
 * SIGN AND CARRY FLAG MASKS
 * ============================================================ */

/* CPSR flag bit positions (not masks - use with set_flag/get_flag functions) */
#define CPSR_N_POS 31
#define CPSR_Z_POS 30
#define CPSR_C_POS 29
#define CPSR_V_POS 28

/* CPSR sign bit (bit 31 of CPSR register) */
#define CPSR_N_BIT (1 << 31)

/* CPSR zero bit (bit 30 of CPSR register) */
#define CPSR_Z_BIT (1 << 30)

/* CPSR carry bit (bit 29 of CPSR register) */
#define CPSR_C_BIT (1 << 29)

/* CPSR overflow bit (bit 28 of CPSR register) */
#define CPSR_V_BIT (1 << 28)

/* Mask to extract sign bit from 32-bit value */
#define VALUE_SIGN_BIT 0x80000000

/* ============================================================
 * BYTE AND WORD ACCESS MASKS
 * ============================================================ */

/* Mask for single byte (bits 0-7) */
#define BYTE_MASK 0xFF

/* ============================================================
 * CONDITION CODE FLAGS
 * ============================================================ */

/* Condition codes are in bits 20-23 of instruction.
 * These are defined in opcode.h as COND_* enum values.
 * See opcode.h for specific values (COND_EQ=0x0, COND_NE=0x1, etc.)
 */

#endif /* MAGIC_ADDRS_H */
