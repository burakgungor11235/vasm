Reference
=========

This reference section provides complete documentation for the varm
instruction set, assembler directives, system calls, and file format.

.. toctree::
   :maxdepth: 2
   :caption: Reference Contents

   instruction-set
   directives
   syscalls
   file-format

Quick Reference Tables
----------------------

### Condition Codes

.. list-table::
   :header-rows: 1
   :widths: 15 15 40

   * - Code
     - Name
     - Meaning
   * - EQ
     - Equal
     - Z = 1
   * - NE
     - Not Equal
     - Z = 0
   * - CS/HS
     - Carry Set / Higher or Same
     - C = 1
   * - CC/LO
     - Carry Clear / Lower
     - C = 0
   * - MI
     - Minus
     - N = 1
   * - PL
     - Plus
     - N = 0
   * - VS
     - Overflow Set
     - V = 1
   * - VC
     - Overflow Clear
     - V = 0
   * - HI
     - Higher
     - C = 1 and Z = 0
   * - LS
     - Lower or Same
     - C = 0 or Z = 1
   * - GE
     - Greater or Equal
     - N = V
   * - LT
     - Less Than
     - N != V
   * - GT
     - Greater Than
     - Z = 0 and N = V
   * - LE
     - Less or Equal
     - Z = 1 or N != V
   * - AL
     - Always
     - Unconditional
   * - NV
     - Never
     - Reserved (do not use)

### Registers

.. list-table::
   :header-rows: 1
   :widths: 15 25 40

   * - Register
     - Alias
     - Purpose
   * - r0
     -
     - General purpose / return value
   * - r1-r12
     -
     - General purpose
   * - r13
     - sp
     - Stack pointer
   * - r14
     - lr
     - Link register
   * - r15
     - pc
     - Program counter

### Opcodes (Partial)

.. list-table::
   :header-rows: 1
   :widths: 15 15 40

   * - Opcode
     - Instruction
     - Syntax
   * - 0x00
     - MOV
     - MOV Rd, #<imm>
   * - 0x01
     - MVN
     - MVN Rd, #<imm>
   * - 0x02
     - ADD
     - ADD Rd, Rn, #<imm>
   * - 0x03
     - ADC
     - ADC Rd, Rn, #<imm>
   * - 0x04
     - SUB
     - SUB Rd, Rn, #<imm>
   * - ...
     - ...
     - ...

For the complete opcode table, see :doc:`instruction-set`.

### Syscalls

.. list-table::
   :header-rows: 1
   :widths: 15 15 40

   * - Number
     - Name
     - Purpose
   * - 1
     - EXIT
     - Terminate program
   * - 2
     - READ
     - Read from file descriptor
   * - 3
     - WRITE
     - Write to file descriptor

For details, see :doc:`syscalls`.

Related Documentation
--------------------

* :doc:`../tutorial/index` - Tutorial introduction
* :doc:`../../internal/architecture/index` - VM internals
* :doc:`../../internal/implementation/index` - Implementation details
