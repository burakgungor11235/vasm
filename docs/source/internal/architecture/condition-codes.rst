Condition Codes
===============

.. warning::
   **varm is not stable.** The condition code register format, flag
   semantics, and condition encoding are subject to change.

1. Condition Code Register (CPSR)
---------------------------------

The Current Program Status Register (CPSR) holds the condition flags
that control conditional execution of instructions.

1.1 CPSR Format
~~~~~~~~~~~~~~~

::

   +------+------+------+------+
   |  N   |  Z   |  C   |  V   |
   +------+------+------+------+
     31     30     29     28

+------------------+-------+---------------------------------------+
| Bit              | Name  | Purpose                               |
+==================+=======+=======================================+
| 31               | N     | Negative / Not equal                  |
| 30               | Z     | Zero                                  |
| 29               | C     | Carry                                 |
| 28               | V     | Overflow                              |
+------------------+-------+---------------------------------------+

1.2 CPSR Implementation
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/common/cpsr.h
   typedef struct {
       uint32_t N : 1;    // Negative flag
       uint32_t Z : 1;    // Zero flag
       uint32_t C : 1;    // Carry flag
       uint32_t V : 1;    // Overflow flag
       uint32_t _ : 28;   // Reserved
   } cpsr_t;

2. Flag Semantics
-----------------

2.1 Negative Flag (N)
~~~~~~~~~~~~~~~~~~~~

The N flag indicates the sign of the result:

.. math::

   N = \text{result}_{31}

The flag is set when the most significant bit (bit 31) of the result
is 1, indicating a negative number in two's complement representation.

2.2 Zero Flag (Z)
~~~~~~~~~~~~~~~~

The Z flag indicates that the result is zero:

.. math::

   Z = \begin{cases}
       1 & \text{if } \text{result} = 0 \\
       0 & \text{otherwise}
   \end{cases}

2.3 Carry Flag (C)
~~~~~~~~~~~~~~~~~~

The C flag indicates carry or borrow from arithmetic operations:

**Addition:**

.. math::

   C = \begin{cases}
       1 & \text{if } \text{unsigned\_overflow} \\
       0 & \text{otherwise}
   \end{cases}

Where unsigned overflow occurs when the sum of two unsigned numbers
exceeds the maximum representable value (2^32 - 1).

**Subtraction:**

.. math::

   C = \begin{cases}
       0 & \text{if } \text{unsigned\_underflow} \\
       1 & \text{otherwise}
   \end{cases}

For subtraction (a - b), carry is clear when b > a (unsigned).

**Shift Operations:**

For shift operations, C holds the last bit shifted out:

.. code-block:: c

   // For left shift: C = bit shifted out (bit 31)
   // For right shift: C = bit shifted out (bit 0)

2.4 Overflow Flag (V)
~~~~~~~~~~~~~~~~~~~~~

The V flag indicates signed overflow:

.. math::

   V = \begin{cases}
       1 & \text{if } \text{signed\_overflow} \\
       0 & \text{otherwise}
   \end{cases}

Signed overflow occurs when the result cannot be represented in the
signed range (i.e., the sign of the result differs from the expected
sign based on operand signs).

**Addition overflow detection:**

.. math::

   V = (A_{31} \oplus R_{31}) \land (B_{31} \oplus R_{31})

Where A and B are operands, R is the result, and \oplus is XOR.

**Subtraction overflow detection:**

.. math::

   V = (A_{31} \oplus B_{31}) \land (A_{31} \oplus R_{31})

3. Flag Evaluation Algorithm
----------------------------

3.1 Unified Flag Update Function
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/common/flags.c
   void flags_update(vm_t *vm, uint32_t result,
                     uint32_t op1, uint32_t op2,
                     uint8_t opcode) {
       cpsr_t *cpsr = &vm->cpsr;

       // N flag: result[31]
       cpsr->N = (result >> 31) & 1;

       // Z flag: result == 0
       cpsr->Z = (result == 0);

       // C and V flags depend on operation
       switch (opcode) {
           case OP_ADD:
           case OP_ADC:
               // Carry: unsigned overflow
               cpsr->C = (uint64_t)op1 + (uint64_t)op2 > UINT32_MAX;
               // Overflow: different signs, result sign differs
               cpsr->V = ((op1 ^ result) & (op2 ^ result)) >> 31;
               break;

           case OP_SUB:
           case OP_SBC:
           case OP_CMP:
               // Carry: no unsigned underflow (b <= a for a - b)
               cpsr->C = op2 <= op1;
               // Overflow: operand signs differ, result sign differs
               cpsr->V = ((op1 ^ op2) & (op1 ^ result)) >> 31;
               break;

           case OP_RSB:
               // Reverse subtraction: op2 - op1
               cpsr->C = op1 <= op2;
               cpsr->V = ((op2 ^ op1) & (op2 ^ result)) >> 31;
               break;

           case OP_MUL:
               // Carry for multiply (architectural dependent)
               // varm: set if high bits of result are non-zero
               cpsr->C = (result >> 32) != 0;
               // Overflow: not meaningful for multiply in varm
               cpsr->V = 0;
               break;

           case OP_LSL:
           case OP_LSR:
           case OP_ASR:
           case OP_ROR:
               // Carry is last bit shifted out
               cpsr->C = (op1 >> (op2 ? op2 - 1 : 31)) & 1;
               break;

           case OP_AND:
           case OP_ORR:
           case OP_EOR:
           case OP_BIC:
               // Logical operations: C unchanged, V unchanged
               break;

           default:
               // Default: no flags affected
               break;
       }
   }

3.2 Per-Operation Flag Behavior
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+-------------+--------+--------+--------+--------+
| Instruction | N      | Z      | C      | V      |
+=============+========+========+========+========+
| ADD         | Update | Update | Update | Update |
| SUB         | Update | Update | Update | Update |
| AND         | Update | Update | -      | -      |
| ORR         | Update | Update | -      | -      |
| MOV         | Update | Update | -      | -      |
| CMP         | Update | Update | Update | Update |
| B           | -      | -      | -      | -      |
| LDR         | -      | -      | -      | -      |
| STR         | -      | -      | -      | -      |
+-------------+--------+--------+--------+--------+

Note: '-' indicates flag is unchanged.

4. Condition Code Encoding
--------------------------

4.1 Condition Field
~~~~~~~~~~~~~~~~~~~

The 4-bit condition field in each instruction specifies which
condition must be satisfied for the instruction to execute.

+-------------+-------+---------------------------------------+
| Cond Value  | Mnem  | Meaning                               |
+=============+=======+=======================================+
| 0x0         | EQ    | Equal / Z set                         |
| 0x1         | NE    | Not equal / Z clear                   |
| 0x2         | CS/HS | Carry set / unsigned higher or same   |
| 0x3         | CC/LO | Carry clear / unsigned lower          |
| 0x4         | MI    | Minus / negative / N set              |
| 0x5         | PL    | Plus / positive / N clear             |
| 0x6         | VS    | Overflow / V set                      |
| 0x7         | VC    | No overflow / V clear                 |
| 0x8         | HI    | Unsigned higher                       |
| 0x9         | LS    | Unsigned lower or same                |
| 0xA         | GE    | Signed greater or equal               |
| 0xB         | LT    | Signed less than                      |
| 0xC         | GT    | Signed greater than                   |
| 0xD         | LE    | Signed less or equal                  |
| 0xE         | AL    | Always                                |
| 0xF         | NV    | Never (reserved)                      |
+-------------+-------+---------------------------------------+

4.2 Condition Code Table
~~~~~~~~~~~~~~~~~~~~~~~~

::

   +----------+--------+----------------------------------------+
   | Mnemonic | Flags  | Condition                              |
   +----------+--------+----------------------------------------+
   | EQ       | Z = 1  | Operands were equal                    |
   | NE       | Z = 0  | Operands were not equal                |
   | CS/HS    | C = 1  | Unsigned >=                            |
   | CC/LO    | C = 0  | Unsigned <                             |
   | MI       | N = 1  | Result was negative                    |
   | PL       | N = 0  | Result was non-negative                |
   | VS       | V = 1  | Signed overflow                        |
   | VC       | V = 0  | No signed overflow                     |
   | HI       | C=1,Z=0| Unsigned >                             |
   | LS       | C=0,Z=1| Unsigned <=                            |
   | GE       | N=V    | Signed >=                              |
   | LT       | N!=V   | Signed <                               |
   | GT       | Z=0,N=V| Signed >                               |
   | LE       | Z=1,N!=V| Signed <=                              |
   | AL       | -      | Always                                 |
   | NV       | -      | Never (do not execute)                 |
   +----------+--------+----------------------------------------+

5. Condition Code Testing
-------------------------

5.1 check_condition() Algorithm
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The CPU checks the condition field against the CPSR flags before
executing each instruction.

.. code-block:: c

   // Source: src/vm/cpu.c
   bool check_condition(vm_t *vm, uint8_t cond) {
       cpsr_t *cpsr = &vm->cpsr;

       switch (cond) {
           case COND_EQ:   return cpsr->Z == 1;
           case COND_NE:   return cpsr->Z == 0;
           case COND_CS:   return cpsr->C == 1;
           case COND_CC:   return cpsr->C == 0;
           case COND_MI:   return cpsr->N == 1;
           case COND_PL:   return cpsr->N == 0;
           case COND_VS:   return cpsr->V == 1;
           case COND_VC:   return cpsr->V == 0;
           case COND_HI:   return cpsr->C == 1 && cpsr->Z == 0;
           case COND_LS:   return cpsr->C == 0 || cpsr->Z == 1;
           case COND_GE:   return cpsr->N == cpsr->V;
           case COND_LT:   return cpsr->N != cpsr->V;
           case COND_GT:   return cpsr->Z == 0 && cpsr->N == cpsr->V;
           case COND_LE:   return cpsr->Z == 1 || cpsr->N != cpsr->V;
           case COND_AL:   return true;
           case COND_NV:   return false;
           default:        return true;  // Default to execute
       }
   }

5.2 Condition Decision Tree
~~~~~~~~~~~~~~~~~~~~~~~~~~~

::

                    Condition Field (4 bits)
                           |
            +-----+-----+--+--+----+----+-----+----+
            |     |     |  |  |    |    |     |    |
          0x0   0x1  0x2 0x3 0x4 0x5  0x6  0x7  ...
          EQ    NE   CS  CC  MI  PL   VS   VC   ...
            |     |     |  |  |    |    |     |
            +--+--+--+--+--+--+--+--+--+--+--+--+
               |  |  |  |  |  |  |  |  |  |  |
            Check Z, N, C, V flags accordingly

5.3 Execute Stage Condition Check
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/vm/execute.c
   void execute_instruction(vm_t *vm, decoded_instr_t *dec) {
       // Check condition before execution
       if (!check_condition(vm, dec->cond)) {
           // Condition failed: skip instruction
           // Effectively a NOP
           return;
       }

       // Condition passed: execute instruction
       vm->current_handler(vm, dec);
   }

5.4 Special Case: CMP Instruction
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The CMP (compare) instruction performs subtraction but only updates
flags, not the destination register:

.. code-block:: c

   void handle_cmp(vm_t *vm, decoded_instr_t *dec) {
       uint32_t result = dec->op1 - dec->op2;

       // Update flags only (no register write)
       flags_update(vm, result, dec->op1, dec->op2, OP_CMP);
   }

6. Complexity Analysis
----------------------

6.1 Condition Check Complexity
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Operation | Time             | Notes            |
+==========+==================+==================+
| Check cond| O(1)             | Single switch    |
| Update N  | O(1)             | Bit extraction   |
| Update Z  | O(1)             | Comparison       |
| Update C  | O(1)             | Arithmetic       |
| Update V  | O(1)             | XOR operations   |
+----------+------------------+------------------+

6.2 Flag Update Details
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Each flag update is O(1):
   cpsr->N = (result >> 31) & 1;           // 1 shift, 1 mask
   cpsr->Z = (result == 0);                // 1 comparison
   cpsr->C = (uint64_t)op1 + op2 > UINT32_MAX;  // 1 64-bit add
   cpsr->V = ((op1 ^ result) & (op2 ^ result)) >> 31;  // 3 XORs, 1 AND, 1 shift

6.3 Switch Statement Optimization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The condition check uses a switch statement which compiles to a
jump table, providing O(1) worst-case time:

- 16 cases (4-bit condition field)
- Direct jump to appropriate exit point

6.4 Pipeline Impact
~~~~~~~~~~~~~~~~~~~

Condition checking occurs in the execute stage:

::

   Cycle 1: Fetch
   Cycle 2: Decode (extract condition field)
   Cycle 3: Check condition (check CPSR) -> Skip if false
   Cycle 4: Execute (if condition passed)
   Cycle 5: Writeback (if condition passed)

Failed conditions result in a pipeline bubble (no work done).

6.5 Future Extensions
~~~~~~~~~~~~~~~~~~~~~

Planned improvements to condition code handling:

- **Conditional execution**: AL condition only (other conditions
  may be deprecated)
- **Flag speculation**: Predict flag values for better pipelining
- **Flag fusion**: Combine flag checks from multiple instructions

.. warning::
   These extensions are not implemented. The current implementation
   supports all 16 condition codes with full flag semantics.
