Instruction Pipeline
====================

.. warning::
   **varm is not stable.** The pipeline implementation described here
   is subject to change. Future versions may introduce pipelining,
   superscalar execution, or other microarchitectural improvements.

1. Instruction Pipeline Overview
--------------------------------

varm implements a simplified four-stage instruction pipeline. Each
stage performs a specific task in the instruction execution process.

1.1 Pipeline Stages
~~~~~~~~~~~~~~~~~~~

::

   +----------+   +----------+   +----------+   +------------+
   |  Fetch   |-->|  Decode  |-->| Execute  |-->| Writeback  |
   +----------+   +----------+   +----------+   +------------+

+----------+-------------------------------------------------------+
| Stage    | Function                                              |
+==========+=======================================================+
| Fetch    | Read instruction from memory at PC address            |
| Decode   | Extract opcode, registers, operands from instruction  |
| Execute  | Perform operation (ALU, memory, branch)               |
| Writeback| Update registers, flags, PC                           |
+----------+-------------------------------------------------------+

.. code-block:: c

   // Source: src/vm/pipeline.c
   void pipeline_cycle(vm_t *vm) {
       stage_fetch(vm);
       stage_decode(vm);
       stage_execute(vm);
       stage_writeback(vm);
   }

1.2 Stage Isolation
~~~~~~~~~~~~~~~~~~~

Each stage operates on its own data structures:

.. code-block:: c

   typedef struct {
       uint32_t pc;
       uint32_t instruction;
   } fetch_stage_t;

   typedef struct {
       uint8_t opcode;
       uint8_t cond;
       uint8_t rd, rn, rm;
       uint32_t imm_value;
   } decode_stage_t;

   typedef struct {
       uint8_t opcode;
       uint32_t operands[3];
       uint32_t result;
       bool take_branch;
   } execute_stage_t;

2. Fetch Stage
--------------

2.1 Fetch Process
~~~~~~~~~~~~~~~~~

The fetch stage reads the instruction at the current PC address
from memory.

::

   PC ----> Memory[PC] ----> Instruction Register (IR)
                 |
                 +---> PC = PC + 4

2.2 Algorithm
~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/vm/pipeline.c
   void stage_fetch(vm_t *vm) {
       // Read instruction from memory at PC
       vm->ir = memory_read_word(vm->memory, vm->pc);

       // Increment PC for sequential execution
       vm->pc += 4;
   }

2.3 PC Increment Details
~~~~~~~~~~~~~~~~~~~~~~~~

The PC is incremented by 4 (one word) after each fetch:

- Instructions are 4 bytes (32 bits) each
- Word-aligned addresses simplify memory access
- Branch instructions override PC after fetch

.. important::
   When reading the PC register directly, the value returned is
   PC + 8 (two instructions ahead). See :doc:`register-file` for details.

2.4 Complexity
~~~~~~~~~~~~~~

+----------+----------------+
| Metric   | Value          |
+==========+================+
| Time     | O(1)           |
| Memory   | 1 word (4 B)   |
+----------+----------------+

3. Decode Stage
---------------

3.1 Decode Process
~~~~~~~~~~~~~~~~~~

The decode stage extracts fields from the 32-bit instruction word:

::

   +--------+--------+--------+--------+
   |  opcode   | cond |  rd  |  rn  |  rm  |
   +--------+--------+--------+--------+
     ^         ^      ^      ^      ^
     |         |      |      |      |
     +---------+------+------+------+
               Extract each field

3.2 Field Extraction
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/vm/decode.c
   void stage_decode(vm_t *vm) {
       decoded_instr_t *dec = &vm->decoded;

       // Extract fields using bit masking and shifting
       dec->opcode = (vm->ir >> 24) & 0xFF;    // Bits 31-24
       dec->cond   = (vm->ir >> 20) & 0x0F;    // Bits 23-20
       dec->rd     = (vm->ir >> 16) & 0x0F;    // Bits 19-16
       dec->rn     = (vm->ir >> 12) & 0x0F;    // Bits 15-12
       dec->rm     = (vm->ir >> 8)  & 0x0F;    // Bits 11-8
   }

3.3 Opcode Handler Lookup
~~~~~~~~~~~~~~~~~~~~~~~~~

Each opcode maps to a handler function via a jump table:

.. code-block:: c

   // Source: src/vm/decode.c
   typedef void (*opcode_handler_t)(vm_t *vm, decoded_instr_t *dec);

   static opcode_handler_t opcode_table[256] = {
       [OP_ADD] = handle_add,
       [OP_SUB] = handle_sub,
       [OP_MOV] = handle_mov,
       [OP_LDR] = handle_ldr,
       [OP_STR] = handle_str,
       [OP_B]   = handle_branch,
       // ... other opcodes
   };

   void stage_decode(vm_t *vm) {
       // ... field extraction ...

       // Look up handler
       vm->current_handler = opcode_table[dec->opcode];
   }

3.4 Register Read
~~~~~~~~~~~~~~~~~

Source registers are read during decode:

.. code-block:: c

   void stage_decode(vm_t *vm) {
       // ... extract fields ...

       // Read source operands
       dec->op1 = register_read(vm->regs, dec->rn);
       dec->op2 = register_read(vm->regs, dec->rm);

       // Handle immediate values if present
       if (dec->has_immediate) {
           dec->op2 = dec->imm_value;
       }
   }

3.5 Complexity
~~~~~~~~~~~~~~

+----------+----------------+
| Metric   | Value          |
+==========+================+
| Time     | O(1)           |
| Operations | 5 shifts,    |
|           | 5 masks       |
+----------+----------------+

4. Execute Stage
----------------

4.1 Execute Process
~~~~~~~~~~~~~~~~~~~

The execute stage performs the actual operation specified by the opcode.

::

   +------------------+
   | Execute Stage    |
   +------------------+
   |                  |
   | +----+ +-------+ |
   | |ALU | | Memory| |
   | +----+ +-------+ |
   |      |           |
   |    Branch       |
   +------------------+

4.2 ALU Operations
~~~~~~~~~~~~~~~~~~

Arithmetic and logic operations use the ALU:

.. code-block:: c

   // Source: src/vm/execute.c
   void handle_add(vm_t *vm, decoded_instr_t *dec) {
       uint32_t result = dec->op1 + dec->op2;

       // Update condition codes
       flags_update(vm, result, dec->op1, dec->op2, OP_ADD);

       // Store result for writeback
       vm->exec_result = result;
   }

4.3 Memory Access
~~~~~~~~~~~~~~~~~

Load and store operations access memory:

.. code-block:: c

   void handle_ldr(vm_t *vm, decoded_instr_t *dec) {
       uint32_t address = dec->op1 + dec->op2;
       vm->exec_result = memory_read_word(vm->memory, address);
   }

   void handle_str(vm_t *vm, decoded_instr_t *dec) {
       uint32_t address = dec->op1 + dec->op2;
       memory_write_word(vm->memory, address, dec->op2);
   }

4.4 Branch Operations
~~~~~~~~~~~~~~~~~~~~~

Branches modify the PC directly:

.. code-block:: c

   void handle_branch(vm_t *vm, decoded_instr_t *dec) {
       uint32_t offset = dec->imm_value << 2;  // Word offset
       vm->pc = vm->pc + offset;

       // Mark that PC was updated
       vm->pc_updated = true;
   }

4.5 Execute Flow
~~~~~~~~~~~~~~~~

::

   Decode ----> Check Condition ----> Execute Handler ----> Next Stage
                  |                                      |
                  +-- (false) ---> Skip to Writeback ----+

4.6 Complexity
~~~~~~~~~~~~~~

+----------+----------------+
| Metric   | Value          |
+==========+================+
| ALU op   | O(1)           |
| Load     | O(1)           |
| Store    | O(1)           |
| Branch   | O(1)           |
+----------+----------------+

5. Writeback Stage
------------------

5.1 Writeback Process
~~~~~~~~~~~~~~~~~~~~~

The writeback stage commits results to the architectural state:

.. code-block:: c

   // Source: src/vm/pipeline.c
   void stage_writeback(vm_t *vm) {
       decoded_instr_t *dec = &vm->decoded;

       // Write result to destination register
       if (dec->writes_result) {
           register_write(vm->regs, dec->rd, vm->exec_result);
       }

       // Update condition codes if instruction modifies flags
       if (dec->updates_flags) {
           // Flags already updated in execute stage
       }

       // PC update for branches handled in execute
       // No action needed here
   }

5.2 Register Write
~~~~~~~~~~~~~~~~~~

.. code-block:: c

   void register_write(register_file_t *regs, uint8_t rd, uint32_t value) {
       if (rd != 0) {  // r0 is always 0 (optional convention)
           regs->r[rd] = value;
       }
   }

5.3 Flag Update
~~~~~~~~~~~~~~~

Condition codes are updated based on the result:

.. code-block:: c

   void flags_update(vm_t *vm, uint32_t result,
                     uint32_t op1, uint32_t op2, uint8_t opcode) {
       cpsr_t *cpsr = &vm->cpsr;

       // Negative flag: result[31]
       cpsr->N = (result >> 31) & 1;

       // Zero flag: result == 0
       cpsr->Z = (result == 0);

       // Carry and overflow depend on operation
       switch (opcode) {
           case OP_ADD:
               cpsr->C = (uint64_t)op1 + (uint64_t)op2 > UINT32_MAX;
               cpsr->V = ((op1 ^ result) & (op2 ^ result)) >> 31;
               break;
           case OP_SUB:
               // ... similar logic
               break;
       }
   }

See :doc:`condition-codes` for full flag semantics.

5.4 Complexity
~~~~~~~~~~~~~~

+----------+----------------+
| Metric   | Value          |
+==========+================+
| Register write | O(1)      |
| Flag update    | O(1)      |
| Total stage    | O(1)      |
+----------+----------------+

6. Pipeline Diagram
-------------------

6.1 Sequential Execution (Basic Implementation)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

   Time ->
  Instr  +------------------------------------------------------------>
          | Fetch | Decode | Execute | Writeback |
   -------+--------+--------+---------+-----------+
   I1     | F1    | D1     | E1      | W1        |
   I2     |        | F2     | D2      | E2        | W2
   I3     |        |        | F3      | D3        | E3       | W3
   I4     |        |        |         | F4        | D4       | E4 | W4

   Total cycles for N instructions: N + 3

6.2 Branch Execution
~~~~~~~~~~~~~~~~~~~~

When a branch is taken, the pipeline is flushed:

::

   Instr  +------------------------------------------------------------>
          | Fetch | Decode | Execute | Writeback |
   -------+--------+--------+---------+-----------+
   I1     | F1    | D1     | E1      | W1        |
   I2     | F2    | D2     | E2(B)   |           |
   I3     |       | F3     | D3      | E3        | W3  <-- Target

   Cycles 1-2: Fetch/decode of I2 (wasted)
   Cycle 3: Branch resolved in execute
   Cycle 4: Fetch of I3 (branch target)

7. Timing Analysis
------------------

7.1 Instructions Per Cycle
~~~~~~~~~~~~~~~~~~~~~~~~~~

In the basic implementation:

- **IPC (Instructions Per Cycle)**: 0.25 (1 instruction per 4 cycles)
- **CPI (Cycles Per Instruction)**: 4.0

7.2 Pipeline Throughput
~~~~~~~~~~~~~~~~~~~~~~~

For N instructions (no branches):

- Total cycles: N + 3 (pipeline fill + drain)
- Throughput: 1 instruction every 4 cycles

7.3 Hazards
~~~~~~~~~~~

The basic implementation has no hazard detection or resolution:

+----------+----------+----------------------------------------+
| Hazard   | Present  | Resolution                             |
+==========+==========+========================================+
| Data     | No       | N/A (in-order execution)               |
| Control  | Yes      | Pipeline flush on taken branches       |
| Structural| No      | N/A (single-stage functional units)    |
+----------+----------+----------------------------------------+

7.4 Future Improvements
~~~~~~~~~~~~~~~~~~~~~~~

Future versions may introduce:

- **Pipelining**: Multiple instructions in flight
- **Superscalar**: Multiple instructions per cycle
- **Branch prediction**: Reduce branch penalties
- **Out-of-order execution**: Improve ILP

.. warning::
   These are planned features. The current implementation does not
   include any of the above optimizations.
