Register File
=============

.. warning::
   **varm is not stable.** The register file implementation, register
   count, and special register behaviors are subject to change.

1. Register File Overview
-------------------------

The register file provides fast storage for the CPU. It consists of
16 general-purpose 32-bit registers, accessible in a single cycle.

1.1 Register File Properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+------------------+-------------------+
| Property         | Value             |
+==================+===================+
| Register count   | 16                |
| Register width   | 32 bits (4 bytes) |
| Total size       | 64 bytes          |
| Read ports       | 3 (simultaneous)  |
| Write ports      | 1                 |
| Access time      | O(1)              |
+------------------+-------------------+

1.2 Register Naming
~~~~~~~~~~~~~~~~~~~

+----------+------------------+----------------------------------------+
| Name     | Number           | Purpose                                |
+==========+==================+========================================+
| r0       | 0                | General purpose / return value         |
| r1       | 1                | General purpose / argument 1           |
| r2       | 2                | General purpose / argument 2           |
| r3       | 3                | General purpose / argument 3           |
| r4       | 4                | General purpose                        |
| r5       | 5                | General purpose                        |
| r6       | 6                | General purpose                        |
| r7       | 7                | General purpose / syscall number       |
| r8       | 8                | General purpose                        |
| r9       | 9                | General purpose                        |
| r10      | 10               | General purpose                        |
| r11      | 11               | General purpose                        |
| r12      | 12               | General purpose                        |
| sp (r13) | 13               | Stack pointer                          |
| lr (r14) | 14               | Link register                          |
| pc (r15) | 15               | Program counter                        |
+----------+------------------+----------------------------------------+

2. Register Roles
-----------------

2.1 General-Purpose Registers (r0-r12)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These registers are available for general use without special semantics:

- r0-r3: Function arguments and return values
- r4-r11: Callee-saved registers
- r12: Intra-procedure call scratch register

2.2 Special-Purpose Registers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Stack Pointer (sp / r13)**

Points to the current top of stack. The stack grows downward:

::

   sp -->  +--------+
           | param3 |
           +--------+
           | param2 |
           +--------+
           | param1 |
           +--------+

.. code-block:: c

   // Source: src/registers/regfile.c
   #define REG_SP   13

   void push(vm_t *vm, uint32_t value) {
       vm->regs.sp -= 4;           // Decrement sp
       memory_write_word(vm->memory, vm->regs.sp, value);
   }

   uint32_t pop(vm_t *vm) {
       uint32_t value = memory_read_word(vm->memory, vm->regs.sp);
       vm->regs.sp += 4;           // Increment sp
       return value;
   }

**Link Register (lr / r14)**

Holds the return address for function calls:

.. code-block:: c

   void handle_bl(vm_t *vm, decoded_instr_t *dec) {
       vm->regs.lr = vm->pc;       // Save return address
       vm->pc = dec->target;       // Branch to target
   }

**Program Counter (pc / r15)**

Holds the address of the next instruction to execute. See special
behavior section below.

3. Register Access
------------------

3.1 Direct Read/Write
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/registers/regfile.h
   uint32_t register_read(register_file_t *regs, uint8_t reg_num) {
       return regs->r[reg_num];
   }

   void register_write(register_file_t *regs, uint8_t reg_num, uint32_t value) {
       if (reg_num != 0) {  // Optional: r0 is always 0
           regs->r[reg_num] = value;
       }
   }

3.2 Register File Implementation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/registers/regfile.c
   typedef struct {
       uint32_t r[16];    // General-purpose registers
   } register_file_t;

   // Initialize to zero
   void register_file_init(register_file_t *regs) {
       for (int i = 0; i < 16; i++) {
           regs->r[i] = 0;
       }
   }

3.3 Multi-Port Access
~~~~~~~~~~~~~~~~~~~~~

The register file supports:

- 3 simultaneous reads (for ALU operations with 2 operands + destination)
- 1 write per cycle

.. code-block:: c

   // Read two source operands and destination
   uint32_t op1 = regs->r[rn];
   uint32_t op2 = regs->r[rm];
   uint32_t op3 = regs->r[rd];  // For operations that read Rd (e.g., MUL)

   // Write result (in writeback stage)
   regs->r[rd] = result;

4. Register Usage Conventions
-----------------------------

4.1 Procedure Call Standard
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+------------------+------------------+--------------------------------+
| Register         | Name             | Preserved across calls         |
+==================+==================+================================+
| r0               | arg0 / ret       | No (caller-saved)              |
| r1               | arg1             | No (caller-saved)              |
| r2               | arg2             | No (caller-saved)              |
| r3               | arg3             | No (caller-saved)              |
| r4               |                  | Yes (callee-saved)             |
| r5               |                  | Yes (callee-saved)             |
| r6               |                  | Yes (callee-saved)             |
| r7               | syscall num      | Yes (callee-saved)             |
| r8               |                  | Yes (callee-saved)             |
| r9               |                  | Yes (callee-saved)             |
| r10              |                  | Yes (callee-saved)             |
| r11              | fp               | Yes (callee-saved)             |
| r12              | ip               | No (caller-saved)              |
| sp (r13)         | stack pointer    | Yes (must be restored)         |
| lr (r14)         | return address   | -                              |
| pc (r15)         | program counter  | -                              |
+------------------+------------------+--------------------------------+

4.2 Caller-Saved Registers
~~~~~~~~~~~~~~~~~~~~~~~~~~

Registers r0-r3, r12 must be saved by the caller if their values
are needed after the call:

.. code-block:: c

   // Caller-save example
   push r0              ; Save r0 before call
   push r1              ; Save r1 before call
   bl callee            ; Call function
   pop r1               ; Restore r1
   pop r0               ; Restore r0

4.3 Callee-Saved Registers
~~~~~~~~~~~~~~~~~~~~~~~~~~

Registers r4-r11, sp must be preserved by the callee:

.. code-block:: c

   // Callee-save example
   push {r4, r5, r6, r7}  ; Save registers
   ; ... do work ...
   pop {r4, r5, r6, r7}   ; Restore registers
   bx lr                  ; Return

4.4 Syscall Convention
~~~~~~~~~~~~~~~~~~~~~~

System calls use r7 for the syscall number:

+------------------+------------------+
| Register         | Purpose          |
+==================+==================+
| r7               | Syscall number   |
| r0               | Arg 0 / return   |
| r1               | Arg 1            |
| r2               | Arg 2            |
| r3               | Arg 3            |
+------------------+------------------+

.. code-block:: c

   // Syscall example: exit with code
   mov r7, #1       ; Syscall number 1 = exit
   mov r0, #0       ; Exit code 0
   svc 0            ; Supervisor call

5. Special Register Behavior
----------------------------

5.1 PC Read Semantics
~~~~~~~~~~~~~~~~~~~~~

Reading the program counter (pc / r15) returns a value that depends
on the current pipeline stage:

+--------------------------+----------------------------------+
| Stage                    | Value returned                   |
+==========================+==================================+
| Fetch (reading ir)       | pc (current instruction)        |
| Decode (reading decoded) | pc + 4 (next instruction)       |
| Execute                  | pc + 4                          |
| Writeback                | pc + 8 (two instructions ahead) |
+--------------------------+----------------------------------+

This behavior mimics ARM processors, where PC reads return PC+8 in
ARM state.

.. code-block:: c

   // Source: src/registers/regfile.c
   uint32_t register_read(register_file_t *regs, uint8_t reg_num) {
       if (reg_num == REG_PC) {
           // PC read semantics depend on pipeline stage
           // In decode stage: return PC + 4
           // In execute stage: return PC + 4
           return regs->pc + 4;
       }
       return regs->r[reg_num];
   }

5.2 PC Write Semantics
~~~~~~~~~~~~~~~~~~~~~~

Writing to pc (r15) sets the next instruction to execute:

.. code-block:: c

   void register_write(register_file_t *regs, uint8_t reg_num, uint32_t value) {
       if (reg_num == REG_PC) {
           // Writing to PC updates next instruction address
           // Value should be word-aligned
           regs->pc = value & ~0x3;  // Clear bits 0-1
       } else {
           regs->r[reg_num] = value;
       }
   }

5.3 Branch Mechanics
~~~~~~~~~~~~~~~~~~~~

Branches work by modifying the pc register:

.. code-block:: c

   void handle_branch(vm_t *vm, decoded_instr_t *dec) {
       uint32_t offset = dec->imm_value << 2;  // Word offset
       uint32_t target = vm->pc + offset;

       // Update pc to branch target
       vm->regs.pc = target;

       // Pipeline will fetch from new pc next cycle
   }

5.4 Condition Code Register Access
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The CPSR (Current Program Status Register) can be read/written
using special instructions:

.. code-block:: c

   // Read CPSR (future)
   mrs r0, cpsr

   // Write CPSR (future)
   msr cpsr, r0

See :doc:`condition-codes` for full CPSR details.

6. Complexity Analysis
----------------------

6.1 Register Operations
~~~~~~~~~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Operation | Time             | Space            |
+==========+==================+==================+
| Read one  | O(1)             | O(1)             |
| Read three| O(1)             | O(1)             |
| Write one | O(1)             | O(1)             |
| Init      | O(16)            | O(64 bytes)      |
+----------+------------------+------------------+

6.2 Implementation Details
~~~~~~~~~~~~~~~~~~~~~~~~~~

The O(1) complexity comes from:

- Direct array indexing (no address computation)
- No structural hazards in basic implementation
- No register renaming

.. code-block:: c

   // Register read: single array index
   return regs->r[reg_num];  // O(1)

   // Register write: single array index
   regs->r[reg_num] = value;  // O(1)

6.3 Pipeline Interactions
~~~~~~~~~~~~~~~~~~~~~~~~~

The register file interacts with the pipeline as follows:

::

   Pipeline Stage    Register Access
   --------------    ---------------
   Fetch             Read PC (to get address)
   Decode            Read r[n], r[m] (operands)
   Execute           Read r[n], r[m] (if not already read)
   Writeback         Write r[d] (result)

6.4 Future Extensions
~~~~~~~~~~~~~~~~~~~~~

Planned register file improvements:

- **Register renaming**: For out-of-order execution
- **Larger register file**: For compilers needing more temporaries
- **Register windows**: For efficient procedure calls (ARM style)

.. warning::
   These features are not implemented. The current register file
   has fixed 16 registers with no renaming.
