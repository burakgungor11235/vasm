.. _file-format:

============
File Format
============

.varm files contain executable code for the varm VM.

Structure
=========

::

   ┌─────────────────────────────────────┐
   │ Header (32 bytes)                   │
   ├─────────────────────────────────────┤
   │ Text Section (N × 4 bytes)          │
   ├─────────────────────────────────────┤
   │ Data Section (M bytes)              │
   └─────────────────────────────────────┘

Header (32 bytes)
=================

====== ============= =========
Offset Size          Field
====== ============= =========
0x00   4              Magic "VARM"
0x04   4              Text offset
0x08   4              Text size
0x0C   4              Data offset
0x10   4              Data size
0x14   4              Entry point
0x18   4              Version (1)
0x1C   4              Flags (0)
====== ============= =========

All values are little-endian.

Magic Number
============

Bytes 0-3: ASCII "VARM" (0x56 0x41 0x52 0x4D)

Text Section
============

Encoded instructions, 4 bytes each. Loaded at virtual address 0x20.

Data Section
============

Initialized data. Loaded at virtual address 0x10000.

Example Hex Dump
================

::

   $ xxd program.varm
   00000000: 5641 524d 2000 0000 1000 0000 0001 0000  VARM............
   00000010: 0001 0000 1400 0000 0100 0000 0100 0000  ................
   00000020: 0000 00a0 0370 0001 2100 0000 0000 0000  .....p...!......
   00000030: 2a00 0000                                *...

Header at 0x00-0x1F, text at 0x20.
