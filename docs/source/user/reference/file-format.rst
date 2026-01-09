.. _file-format:

============
File Format
============

.. warning::
   varm is not a stable project. The .varm file format may change without
   notice. This documentation reflects the current implementation.

This reference documents the complete byte-level layout of ``.varm`` executable
files. The format is designed to be simple, self-describing, and portable
(little-endian).

.. contents:: Table of Contents
   :local:

Overview
========

A ``.varm`` file consists of three main sections:

1. **Header** (32 bytes) - Metadata and section layout
2. **Text Section** - Encoded instructions (read-only code)
3. **Data Section** - Initialized data (read-write)

::

   ┌─────────────────────────────────────────────┐
   │             Header (32 bytes)               │
   ├─────────────────────────────────────────────┤
   │          Text Section (N × 4 bytes)         │
   ├─────────────────────────────────────────────┤
   │           Data Section (M bytes)            │
   └─────────────────────────────────────────────┘

All multi-byte values are stored in **little-endian** byte order.

Header Specification
====================

The header is exactly 32 bytes and contains all metadata needed to load
and execute the program.

**Header Layout:**

::

   Offset   Size   Field
   ───────  ─────  ──────────────────────────────────────────────
   0x00     4      Magic number ("VARM")
   0x04     4      Text section file offset
   0x08     4      Text section size in bytes
   0x0C     4      Data section file offset
   0x10     4      Data section size in bytes
   0x14     4      Entry point virtual address
   0x18     4      Version (currently 1)
   0x1C     4      Flags (reserved, set to 0)

**Byte-Level Diagram:**

::

   Byte:   0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
   ─────── ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
   0x00    │  V  │  A  │  R  │  M  │ text_off_lsb              │ text_off_msb              │
   0x10    │ text_size_lsb           │ text_size_msb           │ data_off_lsb              │
   0x20    │ data_off_msb            │ data_size_lsb           │ data_size_msb             │
   0x30    │ entry_lsb               │ entry_msb               │ version   │  flags    │
   ─────── └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘

**Field Descriptions:**

Magic Number (Offset 0x00, Size 4 bytes)
-----------------------------------------

ASCII string "VARM" (0x4D524156 in big-endian).

+-----------+-------+-----------------------------------------------+
| Byte      | Value | Description                                   |
+-----------+-------+-----------------------------------------------+
| 0x00      | 0x56  | 'V' (ASCII 86)                                |
| 0x01      | 0x41  | 'A' (ASCII 65)                                |
| 0x02      | 0x52  | 'R' (ASCII 82)                                |
| 0x03      | 0x4D  | 'M' (ASCII 77)                                |
+-----------+-------+-----------------------------------------------+

Text Section File Offset (Offset 0x04, Size 4 bytes)
----------------------------------------------------

Little-endian 32-bit offset from start of file to text section.

- **Typical value:** 32 (0x00000020)
- Always follows immediately after header

Text Section Size (Offset 0x08, Size 4 bytes)
---------------------------------------------

Little-endian 32-bit size of text section in bytes.

- Must be multiple of 4 (each instruction is 4 bytes)
- Maximum: 4096 × 4 = 16384 bytes (16384 = 0x4000)

Data Section File Offset (Offset 0x0C, Size 4 bytes)
----------------------------------------------------

Little-endian 32-bit offset from start of file to data section.

- Calculated as: ``text_offset + text_size``
- Must be >= 32

Data Section Size (Offset 0x10, Size 4 bytes)
---------------------------------------------

Little-endian 32-bit size of data section in bytes.

- Maximum: 65536 bytes (64 KiB)

Entry Point (Offset 0x14, Size 4 bytes)
---------------------------------------

Little-endian 32-bit virtual address of program entry point.

- Typically: 0 (start of text section)
- PC is set to this value on program start

Version (Offset 0x18, Size 4 bytes)
-----------------------------------

Little-endian 32-bit version number.

- **Current value:** 1 (0x00000001)
- Future versions may use different formats

Flags (Offset 0x1C, Size 4 bytes)
---------------------------------

Reserved for future use.

- **Current value:** 0 (0x00000000)
- Must be ignored by loader

Example Header
--------------

For a program with:
- Text section: 4 instructions (16 bytes)
- Data section: 12 bytes

**Hex dump of header:**

::

   Offset 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
   ───────────────────────────────────────────────────────
   0x00  56 41 52 4D 20 00 00 00 10 00 00 00 30 00 00 00
   0x10  0C 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00

**Interpretation:**

+-----------+------------+--------------------------------------------+
| Offset    | Value      | Field                                      |
+-----------+------------+--------------------------------------------+
| 0x00-0x03 | "VARM"     | Magic number                               |
| 0x04-0x07 | 0x00000020 | Text offset = 32                           |
| 0x08-0x0B | 0x00000010 | Text size = 16 bytes (4 instructions)      |
| 0x0C-0x0F | 0x00000030 | Data offset = 32 + 16 = 48                  |
| 0x10-0x13 | 0x0000000C | Data size = 12 bytes                       |
| 0x14-0x17 | 0x00000000 | Entry point = 0                            |
| 0x18-0x1B | 0x00000001 | Version = 1                                |
| 0x1C-0x1F | 0x00000000 | Flags = 0                                  |
+-----------+------------+--------------------------------------------+

Text Section
============

The text section contains encoded instructions, stored as 32-bit words in
little-endian byte order.

**Memory Layout (per instruction):**

::

   Byte Offset within Instruction:
   +0        +1        +2        +3
   ┌────────┬────────┬────────┬────────┐
   │ operand │ operand │  cond   │ opcode  │
   │  [7:0]  │ [11:8] │  [3:0] │  [7:0] │
   └────────┴────────┴────────┴────────┘
    LSB                            MSB

**Instruction Encoding (32 bits):**

::

   31:24    │  23:20   │  19:16  │  15:12  │  11:0
   ─────────┼──────────┼─────────┼─────────┼──────────────────
   opcode   │   cond   │   rn    │   rd    │   operand

**Example Instruction: MOV R0, #42**

- Opcode: 0x00 (MOV)
- Cond: 0xE (AL - always)
- Rn: 0x0 (unused for MOV)
- Rd: 0x0 (R0)
- Operand: 0x82A (immediate flag | 42)

**Encoding:** 0x00E0082A

**Memory bytes (little-endian at address 0x20):**

::

   Address +0: 0x2A  (operand[7:0])
   Address +1: 0x08  (operand[11:8])
   Address +2: 0xE0  (cond[3:0] | rn[3:0])
   Address +3: 0x00  (opcode)

**File representation (hex):** 2A 08 E0 00

Data Section
============

The data section contains raw bytes emitted by ``.word`` and ``.byte``
directives. No special encoding is applied.

**.word Directive:**

For ``.word 12345`` (0x00003039):

::

   Offset  Value
   ──────  ─────────────────────────
   +0      0x39  (least significant byte)
   +1      0x30
   +2      0x00
   +3      0x00  (most significant byte)

**.byte Directive:**

For ``.byte 'H', 'e', 'l', 'l', 'o'``:

::

   Offset  Value  ASCII
   ──────  ─────  ─────
   +0      0x48   'H'
   +1      0x65   'e'
   +2      0x6C   'l'
   +3      0x6C   'l'
   +4      0x6F   'o'

Loading and Execution
=====================

When the VM loads a ``.varm`` file:

1. **Read header** from file
2. **Validate magic number** ("VARM")
3. **Read text section** into memory at address 0x00000000
4. **Read data section** into memory at address 0x00010000
5. **Set PC** to entry point address
6. **Begin execution**

**Memory Map after Loading:**

+-----------------------+--------+--------------------------------+
| Region                | Addr   | Contents                       |
+-----------------------+--------+--------------------------------+
| Text                  | 0x00000000 | Instructions (read-only)       |
| Reserved              | 0x00004000 | Unused                         |
| Data                  | 0x00010000 | Initialized data               |
| Stack                 | 0x000FFFFF | Stack (grows down)             |
+-----------------------+--------+--------------------------------+

**Total Memory:** 1 MiB (1048576 bytes = 0x100000)

Complete File Example
=====================

**Source assembly (hello.vasm):**

.. code-block:: asm

   .data
   msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10

   .text
           mov r0, #1          ; fd = stdout
           ldr r1, =msg        ; r1 = 0x10000
           mov r2, #7          ; length = 7
           mov r7, #3          ; syscall = WRITE
           swi

           mov r0, #0          ; exit code = 0
           mov r7, #1          ; syscall = EXIT
           swi

**Compiled .varm file (hex dump):**

::

   Offset 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
   ───────────────────────────────────────────────────────
   0x00  56 41 52 4D 20 00 00 00 28 00 00 00 38 00 00 00
   0x10  0C 00 00 00 00 00 00 00 01 00 00 00 00 00 00 00
   0x20  01 20 A0 E3 10 FF 1F E3 07 20 A0 E3 03 70 A0 E3
   0x30  EF FF 00 00 01 00 A0 E3 01 70 A0 E3 EF FF 00 00
   0x40  48 65 6C 6C 6F 21 0A

**Header Analysis:**

+-----------+------------+--------------------------------------------+
| Offset    | Value      | Field                                      |
+-----------+------------+--------------------------------------------+
| 0x00-0x03 | "VARM"     | Magic number                               |
| 0x04-0x07 | 0x00000020 | Text offset = 32                           |
| 0x08-0x0B | 0x00000028 | Text size = 40 bytes (10 instructions)     |
| 0x0C-0x0F | 0x00000038 | Data offset = 32 + 40 = 56                  |
| 0x10-0x13 | 0x0000000C | Data size = 12 bytes                       |
| 0x14-0x17 | 0x00000000 | Entry point = 0                            |
| 0x18-0x1B | 0x00000001 | Version = 1                                |
| 0x1C-0x1F | 0x00000000 | Flags = 0                                  |
+-----------+------------+--------------------------------------------+

**Text Section (40 bytes, 10 instructions):**

+--------+------------------------------------------+--------------------------+
| Addr   | Bytes (hex)                              | Instruction              |
+--------+------------------------------------------+--------------------------+
| 0x20   | 01 20 A0 E3                              | MOV R0, #1               |
| 0x24   | 10 FF 1F E3                              | LDR R1, [PC, #16]        |
| 0x28   | 07 20 A0 E3                              | MOV R2, #7               |
| 0x2C   | 03 70 A0 E3                              | MOV R7, #3               |
| 0x30   | EF FF 00 00                              | SWI (offset 0)           |
| 0x34   | 01 00 A0 E3                              | MOV R0, #0               |
| 0x38   | 01 70 A0 E3                              | MOV R7, #1               |
| 0x3C   | EF FF 00 00                              | SWI (offset 0)           |
| 0x40   | 00 00 00 00                              | Literal pool (msg addr)  |
| 0x44   | 00 00 01 00                              | Reserved (padding)       |
+--------+------------------------------------------+--------------------------+

**Data Section (12 bytes):**

+--------+-------+-------+
| Offset | Hex   | ASCII |
+--------+-------+-------+
| 0x48   | 48    | 'H'   |
| 0x49   | 65    | 'e'   |
| 0x4A   | 6C    | 'l'   |
| 0x4B   | 6C    | 'l'   |
| 0x4C   | 6F    | 'o'   |
| 0x4D   | 21    | '!'   |
| 0x4E   | 0A    | '\\n' |
+--------+-------+-------+

Validation
==========

The loader validates the following:

1. **Magic number** must be "VARM"
2. **Version** must be 1 (or recognized version)
3. **Offsets** must be within file bounds
4. **Sizes** must not exceed maximum limits

**Maximum Limits:**

+--------------------------+------------+
| Item                     | Maximum    |
+--------------------------+------------+
| Text section size        | 16384 bytes|
| Data section size        | 65536 bytes|
| Total instructions       | 4096       |
| Total file size          | 81920 bytes|
+--------------------------+------------+

Future Extensions
=================

The file format reserves space for future extensions:

- **Version field:** Allows format changes with backward compatibility
- **Flags field:** Reserved for feature flags (e.g., debug info, compressed sections)
- **Flexible offsets:** Section offsets can be anywhere in file

Potential future features:

- Symbol table section for debugging
- Relocation information
- Multiple code sections
- Read-only data flag
