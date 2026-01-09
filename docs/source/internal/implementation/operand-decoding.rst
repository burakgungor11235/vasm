Operand Decoding
================

.. warning::
   **varm is not stable.** This implementation may change without notice.

1. Operand Overview
-------------------

Operands in varm instructions specify the data that an operation acts upon.
Each instruction has a 12-bit operand field that can represent either:

- **Immediate value**: A constant encoded directly in the instruction
- **Register**: A reference to one of 16 general-purpose registers

The operand format is distinguished by bit 11:

::

   Bit 11 = 1: Immediate operand
   Bit 11 = 0: Register operand

2. Immediate Operands
---------------------

Immediate operands encode constant values using a rotate-encoding scheme
that allows efficient representation of common values.

Encoding format:

::

   11    10:8    7:0
   ┌────┬──────┬─────────┐
   │  1 │ rot  │ imm8    │
   └────┴──────┴─────────┘

- **Bit 11**: Must be 1 for immediate mode
- **Bits [10:8]**: Rotate amount (0-15), multiplied by 2 for actual rotation
- **Bits [7:0]**: 8-bit immediate value

Decode algorithm
~~~~~~~~~~~~~~~~

.. code-block:: c

   // src/vm/instruction.c:8-39
   static u32
   decode_operand2(vm_state_t* vm, u32 instr)
   {
       u32 operand = instr & OPERAND_MASK;
       u8 is_immediate = (operand >> 11) & 1;
       u8 rotate = (operand >> IMM_ROTATE_SHIFT) & 0x7;  // bits 10:8
       u8 imm8 = operand & IMM_VALUE_MASK;               // bits 7:0

       u32 value = imm8;
       if (rotate > 0) {
           // Rotate right by (rotate * 2) bits
           value = (imm8 >> (rotate * 2)) | (imm8 << (32 - rotate * 2));
       }

       if (is_immediate) {
           return value;
       }
       // ... register operand handling
   }

The rotate amount is multiplied by 2 because the ARM rotate encoding
uses even rotations only. This allows representing any 32-bit value
that can be formed by rotating an 8-bit value.

3. Register Operands
--------------------

Register operands specify a source register from which to read a value.

Encoding format:

::

   11    3:0
   ┌────┬─────┐
   │  0 │ rm  │
   └────┴─────┘

- **Bit 11**: Must be 0 for register mode
- **Bits [3:0]**: Register number (0-15)

Optional shift operations on register operands are supported:

::

   10:7    6:5    4:0
   ┌──────┬──────┬─────────┐
   │ type │  00  │ shift   │
   └──────┴──────┴─────────┘

- **Bits [10:7]**: Shift type (0=LSL, 1=LSR, 2=ASR, 3=ROR)
- **Bits [4:0]**: Shift amount (0-31)

Register operand decoding:

.. code-block:: c

   // From decode_operand2()
   u8 rm = instr & REG_MASK;                    // bits 3:0
   u8 shift_type = (instr >> SHIFT_TYPE_SHIFT) & 0x3;  // bits 6:5
   u8 shift_imm = (instr >> SHIFT_IMM_SHIFT) & 0x1F;   // bits 11:7

   u32 reg_value = vm_get_reg(vm, rm);
   switch (shift_type) {
   case 0:  return reg_value << shift_imm;      // LSL
   case 1:  return reg_value >> shift_imm;      // LSR
   case 2:  return (reg_value >> shift_imm) |   // ASR
                  ((reg_value & VALUE_SIGN_BIT) >> (32 - shift_imm));
   case 3:  return (reg_value >> shift_imm) |   // ROR
                  (reg_value << (32 - shift_imm));
   }

4. Encoding Examples
--------------------

Example 1: ``mov r0, #42``
~~~~~~~~~~~~~~~~~~~~~~~~~~

Assembly:
   ``mov r0, #42``

Opcode: ``OP_MOV`` = 0x00
Cond: ``AL`` = 0xE
Rd: r0 = 0

Operand encoding for immediate 42:
   - 42 in binary: ``00101010``
   - No rotation needed, rot = 0
   - Operand = (1 << 11) | 42 = 0x82A

Full instruction encoding:
   ``(0x00 << 24) | (0xE << 20) | (0 << 12) | 0x82A = 0x00E0082A``

In memory (little-endian):
   ``2A 08 E0 00``

Example 2: ``add r1, r2, r3``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Assembly:
   ``add r1, r2, r3``

Opcode: ``OP_ADD`` = 0x02
Cond: ``AL`` = 0xE
Rn: r2 = 2
Rd: r1 = 1
Operand: r3 = 3 (register mode, bit 11 = 0)

Full instruction encoding:
   ``(0x02 << 24) | (0xE << 20) | (2 << 16) | (1 << 12) | 3 = 0x02E21003``

5. Decode Algorithm (Pseudocode)
--------------------------------

::

   function decode_operand(instruction):
       operand = instruction & 0xFFF
       is_immediate = (operand >> 11) & 1

       if is_immediate:
           // Decode immediate
           rotate = (operand >> 8) & 0x7
           imm8 = operand & 0xFF
           value = imm8
           if rotate > 0:
               value = rotate_right(value, rotate * 2)
           return value
       else:
           // Decode register
           rm = operand & 0xF
           shift_type = (operand >> 5) & 0x3
           shift_imm = (operand >> 7) & 0x1F
           value = get_register(rm)
           return shift_register(value, shift_type, shift_imm)

6. Complexity Analysis
----------------------

+-------------------+------------+----------------------------------+
| Operation         | Complexity | Notes                            |
+-------------------+------------+----------------------------------+
| Bit extraction    | O(1)       | Fixed shifts and masks           |
| Immediate decode  | O(1)       | Single rotate operation          |
| Register decode   | O(1)       | Register read + optional shift   |
| Total per operand | O(1)       | Constant time for all cases      |
+-------------------+------------+----------------------------------+

The immediate rotate operation is O(1) since it operates on fixed-width
(32-bit) values. Register reads access the register file in O(1) time.

See Also
--------

- :doc:`opcode-decoding` - Opcode extraction
- :doc:`../architecture/register-file` - Register file implementation
- ``src/vm/instruction.c:8-39`` - ``decode_operand2()`` function
- ``src/asm/parser.c:281-311`` - Assembly operand parsing
