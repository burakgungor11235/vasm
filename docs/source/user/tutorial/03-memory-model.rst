.. _memory-model:

==============
Memory Model
==============

.. warning::
   varm is not stable. The memory layout, addressing modes, and memory-related
   behavior may change in future versions. The examples in this section work
   with the current version but verify behavior in your specific installation.

Memory Overview
===============

varm provides a simplified memory model that makes it accessible for learning
while still demonstrating key concepts of how computers organize and access
data. Understanding this model is essential because all programs ultimately
operate on data stored in memory.

Memory Size and Organization
----------------------------

varm's memory consists of 65,536 bytes, which is 64 kilobytes (KB) or 16,384
32-bit words. This is a deliberately small address space that fits entirely
within the virtual machine's addressable range, making it easy to visualize
and reason about.

The memory is divided into two main sections:

1. **Text Section (.text):** Contains program code (instructions)
2. **Data Section (.data):** Contains initialized constants and data

These sections are separated because code and data have different properties:
code is typically read-only and executed, while data is read and written during
program execution. Separating them allows the system to apply different
protections and access rules.

Memory Layout Diagram
---------------------

The following diagram shows varm's memory organization:

.. code-block:: text

    Address          Content              Section
    ┌──────────┬───────────────────────┬──────────┐
    │ 0x00000  │                       │          │
    │   ...    │  (Reserved/Unused)    │          │
    │ 0x00020  ├───────────────────────┤  .text   │
    │   ...    │  Instructions         │          │
    │ 0x0FFFF  ├───────────────────────┤          │
    │ 0x10000  │                       │          │
    │   ...    │  Initialized Data     │  .data   │
    │ 0x1FFFF  │                       │          │
    │ 0x20000  │                       │  (End of │
    │   ...    │  (Beyond addressable) │   Memory)│
    └──────────┴───────────────────────┴──────────┘

    Text section:  0x0020  to 0x0FFFF  (65,440 bytes)
    Data section:  0x10000 to 0x1FFFF  (65,536 bytes)

Text Section (.text)
====================

The text section is where program instructions are stored. It begins at address
0x20 (hexadecimal 32), leaving the lower addresses reserved for special
purposes and potential future extensions.

Why Start at 0x20?
------------------

The text section starts at address 0x20 rather than 0x0 for historical and
practical reasons. This leaves space at low addresses for:

* Interrupt vectors (in real ARM systems)
* Special-purpose registers that might be memory-mapped
* Potential future extensions to the virtual machine

Instruction Storage
-------------------

Each instruction occupies exactly 4 bytes (one word). Instructions are stored
consecutively in memory with no padding between them. When the processor
fetches an instruction, it reads 4 bytes from the current program counter (PC)
address and decodes them as a single instruction.

**Example:** If a program starts at address 0x20 and contains three
instructions, they occupy:

* Instruction 1: addresses 0x20, 0x21, 0x22, 0x23
* Instruction 2: addresses 0x24, 0x25, 0x26, 0x27
* Instruction 3: addresses 0x28, 0x29, 0x2A, 0x2B

**Complexity of Instruction Fetch:** O(1) - single memory access

**Starter Code:**

.. code-block:: arm

    @ text_section.asm - Understanding the text section
    @ Run with: varm text_section.asm

    .text
    .global _start

    _start:
        MOV  r0, #42       @ This instruction stored at 0x20
        HALT               @ This instruction stored at 0x24

Data Section (.data)
====================

The data section stores initialized constants and data that the program needs
to reference. It begins at address 0x10000 (65,536), which places it after the
entire text section and all possible instruction storage.

Data Declaration
----------------

You declare data in the .data section using assembler directives. The most
common directives are:

* ``.word <value>`` - Define a 32-bit word
* ``.hword <value>`` - Define a 16-bit halfword
* ``.byte <value>`` - Define an 8-bit byte
* ``.ascii "string"`` - Define an ASCII string (not null-terminated)
* ``.asciz "string"`` - Define a null-terminated ASCII string
* ``.space <n>`` - Reserve n bytes of zero-initialized space

**Starter Code:**

.. code-block:: arm

    @ data_section.asm - Declaring data in .data section
    @ Run with: varm data_section.asm

    .data
    my_value:    .word   42          @ 32-bit integer constant
    my_byte:     .byte   0xFF        @ 8-bit byte constant
    my_message:  .asciz "Hello"      @ Null-terminated string

    .text
    .global _start

    _start:
        @ Code to use the declared data follows

Load and Store Instructions
===========================

In ARM-like architectures, there is a fundamental distinction between:

1. **Data Processing Instructions:** Operate on registers only (ADD, MOV, etc.)
2. **Load and Store Instructions:** Transfer data between registers and memory

This is a key difference from many high-level languages where you can write
directly to memory locations using assignment. In assembly, you must explicitly
load data from memory into registers, process it, and then store results back.

LDR: Load from Memory
---------------------

The ``LDR`` instruction loads a 32-bit word from memory into a register.

**Syntax:**

.. code-block:: text

    LDR  rd, [rn, #<offset>]

**Examples:**

.. code-block:: arm

    LDR  r0, [r1]          @ Load word from address in r1 into r0
    LDR  r2, [r3, #4]      @ Load word from (r3 + 4) into r2
    LDR  r4, [r5, #8]      @ Load word from (r5 + 8) into r4

**Addressing Modes:**

* **Base register only:** ``LDR rd, [rn]`` - Address is rn
* **Immediate offset:** ``LDR rd, [rn, #offset]`` - Address is rn + offset
* **The offset can be positive or negative**

**Operational Semantics:**

.. math::

    \text{LDR}(rd, [rn, \#\text{offset}]) \equiv
    rd \leftarrow \text{mem}_{32}(rn + \text{offset})

**Complexity:** O(1) - single memory access

**Starter Code:**

.. code-block:: arm

    @ demonstrate_ldr.asm - Loading data from memory
    @ Run with: varm demonstrate_ldr.asm

    .data
    value:  .word   12345          @ Define a word in data section

    .text
    .global _start

    _start:
        LDR  r0, [value]           @ Load the value from memory into r0
        @ r0 now contains 12345
        HALT

STR: Store to Memory
--------------------

The ``STR`` instruction stores a 32-bit word from a register into memory.

**Syntax:**

.. code-block:: text

    STR  rd, [rn, #<offset>]

**Examples:**

.. code-block:: arm

    STR  r0, [r1]          @ Store r0 to address in r1
    STR  r2, [r3, #4]      @ Store r2 to address (r3 + 4)
    STR  r4, [r5, #8]      @ Store r4 to address (r5 + 8)

**Operational Semantics:**

.. math::

    \text{STR}(rd, [rn, \#\text{offset}]) \equiv
    \text{mem}_{32}(rn + \text{offset}) \leftarrow rd

**Complexity:** O(1) - single memory access

**Starter Code:**

.. code-block:: arm

    @ demonstrate_str.asm - Storing data to memory
    @ Run with: varm demonstrate_str.asm

    .data
    result:  .space  4          @ Reserve 4 bytes (one word) for result

    .text
    .global _start

    _start:
        MOV   r0, #42           @ Prepare value to store
        LDR   r1, =result       @ Load address of result
        STR   r0, [r1]          @ Store 42 to the result location
        HALT

Labels and Addressing
=====================

Labels are symbolic names for memory addresses. They make code readable and
maintainable by allowing you to reference locations by name rather than
numerical address.

Defining Labels
---------------

A label is created by writing a name followed by a colon at the beginning of
a line. The label then refers to the address of the next instruction or data
item.

**Examples:**

.. code-block:: arm

    .text
    .global _start

    _start:                  @ _start is a label for this address
        MOV  r0, #10

    loop:                    @ loop is a label for the next instruction
        ADD  r0, r0, #1
        CMP  r0, #5
        BNE  loop

Referencing Labels
------------------

Labels can be used in instructions that expect address values:

.. code-block:: arm

    B     loop              @ Branch to the 'loop' label
    LDR   r0, [data]        @ Load from address labeled 'data'

When you write ``B loop``, the assembler calculates the distance from the
current instruction to the loop label and encodes this as a relative offset.

The ldr rd, =label Pseudo-Instruction
-------------------------------------

The ``LDR rd, =label`` is a **pseudo-instruction**, meaning it looks like a
single instruction but the assembler translates it into one or more actual
instructions. This construct loads the *address* of a label into a register.

**Examples:**

.. code-block:: arm

    LDR  r0, =my_data       @ Load address of 'my_data' into r0
    LDR  r1, =_start        @ Load address of '_start' into r1

**How it works:** The assembler places the address in a nearby **literal pool**
(a special area in the text section for storing constants) and generates an
LDR instruction that loads from that location using PC-relative addressing.

**Starter Code:**

.. code-block:: arm

    @ demonstrate_labels.asm - Using labels and addresses
    @ Run with: varm demonstrate_labels.asm

    .data
    message:  .asciz "Stored in data section"

    .text
    .global _start

    _start:
        LDR  r0, =message     @ Load address of message into r0
        @ r0 now contains the address of the string, not the string itself
        HALT

Literal Pools
=============

A literal pool is a section of memory in the text section that stores constant
values that cannot be encoded directly in instructions. When you use constructs
like ``LDR rd, =<constant>`` or ``LDR rd, =label``, the assembler places the
constant in a nearby literal pool and generates an LDR instruction that loads
from that location.

How LDR rd, =label Works
------------------------

When the assembler encounters ``LDR r0, =my_label``, it:

1. Adds the address of my_label to a literal pool (usually placed at the end
   of the current code section or at a specific point you indicate)
2. Generates ``LDR r0, [PC, #offset]`` where offset is the distance from the
   current instruction to the literal pool entry

This is called **PC-relative addressing** because the address is calculated
relative to the Program Counter (PC).

**Example:**

.. code-block:: arm

    .text
    _start:
        LDR  r0, =value      @ Assembler generates:
                             @   1. A literal pool entry with value's address
                             @   2. LDR r0, [PC, #12] to load from that entry

    .ltorg                 @ Place literal pool here (directive)
                             @ The literal pool contains:
                             @   value_address: .word 0x10000

PC-Relative Addressing
----------------------

PC-relative addressing is important because it makes code **position-independent**.
The same code can be loaded at different memory addresses and still work
correctly, because addresses are calculated relative to the current instruction
rather than as absolute values.

**Starter Code:**

.. code-block:: arm

    @ demonstrate_literal_pool.asm - Understanding literal pools
    @ Run with: varm demonstrate_literal_pool.asm

    .data
    number:  .word  100

    .text
    .global _start

    _start:
        @ This LDR uses a literal pool internally
        LDR  r0, =number      @ Load address of 'number'
        LDR  r1, [r0]         @ Load the actual value (100)

    .ltorg                  @ Explicitly place literal pool here

Byte vs Word Access
===================

varm supports both word (32-bit) and byte (8-bit) access to memory. The
instructions differ in their effect on data size.

LDRB/STRB: Byte Access
----------------------

The ``LDRB`` (Load Register Byte) and ``STRB`` (Store Register Byte) instructions
transfer single bytes rather than full words.

**Syntax:**

.. code-block:: text

    LDRB  rd, [rn, #<offset>]
    STRB  rd, [rn, #<offset>]

**Examples:**

.. code-block:: arm

    LDRB  r0, [r1]          @ Load single byte into r0
    STRB  r2, [r3, #5]      @ Store low byte of r2 to (r3 + 5)

**Note:** When loading a byte with LDRB, the value is zero-extended to fill
the 32-bit register.

**Operational Semantics:**

.. math::

    \text{LDRB}(rd, [rn, \#\text{offset}]) \equiv
    rd \leftarrow \text{mem}_8(rn + \text{offset}) \text{ (zero-extended)}

    \text{STRB}(rd, [rn, \#\text{offset}]) \equiv
    \text{mem}_8(rn + \text{offset}) \leftarrow rd \bmod 256

**Complexity:** O(1) - single memory access

**Starter Code:**

.. code-block:: arm

    @ demonstrate_byte_access.asm - Byte-level memory operations
    @ Run with: varm demonstrate_byte_access.asm

    .data
    byte_array:  .byte  0x12, 0x34, 0x56, 0x78

    .text
    .global _start

    _start:
        LDR   r0, =byte_array     @ Load base address
        LDRB  r1, [r0]            @ Load first byte (0x12)
        LDRB  r2, [r0, #1]        @ Load second byte (0x34)
        LDRB  r3, [r0, #2]        @ Load third byte (0x56)
        LDRB  r4, [r0, #3]        @ Load fourth byte (0x78)

Complete Memory Layout Example
==============================

The following program demonstrates the complete memory model with code and
data sections:

.. code-block:: arm

    @ complete_memory_example.asm - Complete memory demonstration
    @ Run with: varm complete_memory_example.asm

    .data
    @ Data section starts at 0x10000
    counter:     .word   0           @ 4 bytes
    message:     .asciz "Hello varm!" @ 12 bytes (including null)
    buffer:      .space  16          @ 16 bytes of zeroed space

    .text
    @ Text section starts at 0x20
    .global _start

    _start:
        @ Instructions here at addresses 0x20, 0x24, etc.

        MOV   r0, #0           @ 0x20: Initialize counter
        LDR   r1, =counter     @ 0x24: Load address of counter
        STR   r0, [r1]         @ 0x28: Store 0 to counter location

        LDR   r0, =message     @ 0x2C: Load address of message
        @ r0 now contains 0x10000 (address of message in .data)

    loop:
        CMP   r0, #10          @ 0x30: Compare counter with 10
        BEQ   done             @ 0x34: Exit if equal

        ADD   r0, r0, #1       @ 0x38: Increment counter
        B     loop             @ 0x3C: Loop back

    done:
        HALT                   @ 0x40: Stop execution

**Memory Map After Loading:**

=== =========== =====================================================
Addr Range      Content
=== =========== =====================================================
0x20-0x3F       Code: _start, loop, done, etc.
0x10000         counter (word, value 0)
0x10004         message[0] ('H')
0x10005         message[1] ('e')
...             ... (continuation of string)
0x1000B         message[11] (null terminator)
0x1000C-0x1001B buffer (16 zero bytes)
=== =========== =====================================================

**Summary of Complexity:**

* Instruction fetch: O(1)
* Data load/store (word): O(1)
* Data load/store (byte): O(1)
* Label resolution: O(1) at assembly time
