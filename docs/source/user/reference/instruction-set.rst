.. _instruction-set:

===============
Instruction Set
===============

Complete instruction reference for varm.

Format
======

All instructions are 32-bit little-endian::

    31:24 opcode  │  23:20 cond  │  19:16 rn  │  15:12 rd  │  11:0 operand
    ┌──────────┬──────────┬──────────┬──────────┬────────────────────────┐
    │  Opcode  │  Cond    │    Rn    │    Rd    │        Operand         │
    │  8 bit   │  4 bit   │  4 bit   │  4 bit   │        12 bit          │
    └──────────┴──────────┴──────────┴──────────┴────────────────────────┘

Condition Codes
===============

======= ============ =====================
Code    Meaning      Flags
======= ============ =====================
EQ      Equal        Z=1
NE      Not Equal    Z=0
CS      Carry Set    C=1
CC      Carry Clear  C=0
MI      Minus        N=1
PL      Plus         N=0
VS      Overflow     V=1
VC      No Overflow  V=0
HI      Higher       C=1, Z=0
LS      Lower        C=0 or Z=1
GE      Greater/Eq   N=V
LT      Less Than    N≠V
GT      Greater      Z=0, N=V
LE      Less/Equal   Z=1 or N≠V
AL      Always       Unconditional
NV      Reserved     -
======= ============ =====================

CPSR Flags: N=Negative, Z=Zero, C=Carry, V=Overflow

Data Processing
===============

======= =================== ============================ =============
Opcode  Syntax              Description                  Flags
======= =================== ============================ =============
0x00    MOV  Rd, Op2        Move                         N, Z
0x01    MVN  Rd, Op2        Move NOT                     N, Z
0x02    ADD  Rd, Rn, Op2    Add                          N, Z, C, V
0x03    ADC  Rd, Rn, Op2    Add with Carry               N, Z, C, V
0x04    SUB  Rd, Rn, Op2    Subtract                     N, Z, C, V
0x05    SBC  Rd, Rn, Op2    Sub with Carry               N, Z, C, V
0x06    RSB  Rd, Rn, Op2    Reverse Subtract             N, Z, C, V
0x07    RSC  Rd, Rn, Op2    Rev Sub w/ Carry             N, Z, C, V
0x08    AND  Rd, Rn, Op2    Bitwise AND                  N, Z
0x09    EOR  Rd, Rn, Op2    Bitwise XOR                  N, Z
0x0A    ORR  Rd, Rn, Op2    Bitwise OR                   N, Z
0x0B    BIC  Rd, Rn, Op2    Bit Clear                    N, Z
======= =================== ============================ =============

Multiply
========

======= ====================== =============================
Opcode  Syntax                 Description
======= ====================== =============================
0x10    MUL  Rd, Rm, Rs        Multiply
0x11    MLA  Rd, Rm, Rs, Rn    Multiply-Accumulate
======= ====================== =============================

Memory
======

======= =================== =======================
Opcode  Syntax              Description
======= =================== =======================
0x20    LDR  Rd, [Rn]       Load Word
0x21    LDRB Rd, [Rn]       Load Byte
0x22    STR  Rd, [Rn]       Store Word
0x23    STRB Rd, [Rn]       Store Byte
0x24    LDM  Rn, {regs}     Load Multiple
0x25    STM  Rn, {regs}     Store Multiple
======= =================== =======================

Branching
=========

======= =================== =======================
Opcode  Syntax              Description
======= =================== =======================
0x30    B   offset          Branch
0x31    BL  offset          Branch with Link
0x32    BX  Rn              Branch and Exchange
======= =================== =======================

System
======

======= =================== =======================
Opcode  Syntax              Description
======= =================== =======================
0x40    HALT                Halt
0x41    SWI                 Software Interrupt
0x42    NOP                 No Operation
======= =================== =======================

Comparison (Set Flags Only)
===========================

======= =================== ============================ =============
Opcode  Syntax              Description                  Flags
======= =================== ============================ =============
0x0C    CMP  Rn, Op2        Compare                      N, Z, C, V
0x0D    CMN  Rn, Op2        Compare Negative             N, Z, C, V
0x0E    TST  Rn, Op2        Test Bits                    N, Z
0x0F    TEQ  Rn, Op2        Test Equivalence             N, Z
======= =================== ============================ =============

