Memory Subsystem
================

.. warning::
   **varm is not stable.** The memory architecture described here
   reflects the current implementation. The address space, memory map,
   and access patterns are subject to change.

1. Memory Architecture
----------------------

1.1 Address Space
~~~~~~~~~~~~~~~~~

varm implements a flat 32-bit address space, currently limited to 64KB
for simplicity:

+------------------+-------------------+
| Property         | Value             |
+==================+===================+
| Total size       | 64 KB (65,536 B)  |
| Address range    | 0x0000 - 0xFFFF   |
| Addressable unit | Byte (8 bits)     |
| Word size        | 4 bytes (32 bits) |
| Endianness       | Little-endian     |
+------------------+-------------------+

.. code-block:: c

   // Source: src/memory/ram.h
   #define VARM_MEMORY_SIZE     (64 * 1024)  // 64 KB
   #define VARM_MEMORY_MASK     (VARM_MEMORY_SIZE - 1)

1.2 Memory Interface
~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/memory/memory.h
   typedef struct {
       uint8_t *data;      // Raw memory array
       size_t size;        // Total size in bytes
   } memory_t;

   uint32_t memory_read_word(memory_t *mem, uint32_t address);
   uint16_t memory_read_half(memory_t *mem, uint32_t address);
   uint8_t  memory_read_byte(memory_t *mem, uint32_t address);

   void memory_write_word(memory_t *mem, uint32_t address, uint32_t value);
   void memory_write_half(memory_t *mem, uint32_t address, uint16_t value);
   void memory_write_byte(memory_t *mem, uint32_t address, uint8_t value);

2. Memory Map
-------------

2.1 Overview
~~~~~~~~~~~~

.. mermaid::
   :caption: Memory Map Overview

   flowchart TB
       subgraph MEM["64KB Address Space"]
           RES["0x0000-0x001F\nReserved\nBoot Vectors"]
           TEXT["0x0020-0x7FFF\n.text\nCode Segment"]
           DATA["0x8000-0xBFFF\n.data\nData Segment"]
           RODATA["0xC000-0xDFFF\n.rodata\nRead-only Data"]
           HEAP["0xE000-0xEFFF\nHeap\n(Future)"]
           STACK["0xF000-0xFFFF\nStack\n(Grows Down)"]
       end

2.2 Detailed Memory Layout
~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+----------+------------------------------------------+
| Start    | End      | Region                                   |
+==========+==========+========================================+
| 0x0000   | 0x001F   | Reserved (boot vectors)                  |
| 0x0020   | 0x7FFF   | Text section (code)                      |
| 0x8000   | 0xBFFF   | Data section (initialized data)          |
| 0xC000   | 0xDFFF   | Read-only data (constants, strings)      |
| 0xE000   | 0xEFFF   | Heap (future)                            |
| 0xF000   | 0xFFFF   | Stack (grows downward)                   |
+----------+----------+------------------------------------------+

2.3 Memory Map Diagram
~~~~~~~~~~~~~~~~~~~~~~

.. mermaid::
   :caption: Complete Memory Layout

   block-beta
       columns 1
       block RES["0x0000\nReserved\nBoot Vectors"]
       block TEXT["0x0020\n.text\nCode"]
       block DATA["0x8000\n.data\nData"]
       block RODATA["0xC000\n.rodata\nConstants"]
       block HEAP["0xE000\nHeap"]
       block STACK["0xF000-0xFFFF\nStack\n↓ Grows Down"]


3. Text Section
---------------

3.1 Overview
~~~~~~~~~~~~

The text section contains executable code. It is loaded at address
0x20 (32 bytes past zero) and consists of 32-bit instruction words.

3.2 Text Section Properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+-----------------------------------+
| Property | Value                             |
+==========+===================================+
| Base     | 0x20                              |
| Entry    | 0x20                              |
| Alignment| Word-aligned (4 bytes)           |
| Access   | Read-only during execution        |
| Size     | Variable (set at link time)       |
+----------+-----------------------------------+

3.3 Instruction Storage
~~~~~~~~~~~~~~~~~~~~~~~

Instructions are stored sequentially in memory:

::

   Address     Content         Instruction
   --------    -------------   ----------------
   0x0020      0x01120000     ADD r0, r1, r2
   0x0024      0x02130000     SUB r0, r1, r3
   0x0028      0x03040105     MOV r4, 0x105

3.4 Read-Only Enforcement
~~~~~~~~~~~~~~~~~~~~~~~~~

In the basic implementation, the text section is not enforced as
read-only. Future versions may add memory protection.

.. code-block:: c

   // Current implementation: no protection
   void memory_write_word(memory_t *mem, uint32_t addr, uint32_t val) {
       mem->data[addr] = val & 0xFF;
       mem->data[addr + 1] = (val >> 8) & 0xFF;
       mem->data[addr + 2] = (val >> 16) & 0xFF;
       mem->data[addr + 3] = (val >> 24) & 0xFF;
   }

4. Data Section
---------------

4.1 Overview
~~~~~~~~~~~~

The data section stores initialized global and static variables.
It is located at address 0x8000 and is read-write during execution.

4.2 Data Section Properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+-----------------------------------+
| Property | Value                             |
+==========+===================================+
| Base     | 0x8000                            |
| Access   | Read-write during execution       |
| Size     | Variable (set at link time)       |
| Alignment| Word-aligned for variables       |
+----------+-----------------------------------+

4.3 Data Storage
~~~~~~~~~~~~~~~~

::

   Address     Content         Meaning
   --------    -------------   ----------------
   0x8000      0x00000064     int counter = 100
   0x8004      0x00000001     char flags = 1
   0x8008      0x41424344     "ABCD" string
   0x800C      0x00000000     int initialized = 0

4.4 Data Types in Memory
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/memory/memory.c
   void memory_write_word(memory_t *mem, uint32_t addr, uint32_t val) {
       // Little-endian storage
       mem->data[addr + 0] = (val >> 0)  & 0xFF;  // Byte 0 (LSB)
       mem->data[addr + 1] = (val >> 8)  & 0xFF;  // Byte 1
       mem->data[addr + 2] = (val >> 16) & 0xFF;  // Byte 2
       mem->data[addr + 3] = (val >> 24) & 0xFF;  // Byte 3 (MSB)
   }

5. Memory Access
----------------

5.1 Byte vs Word Access
~~~~~~~~~~~~~~~~~~~~~~~

varm supports both byte and word (32-bit) memory access:

+----------+------------------+------------------+
| Access   | Size             | Alignment        |
+==========+==================+==================+
| Byte     | 8 bits           | Any address      |
| Halfword | 16 bits          | Even addresses   |
| Word     | 32 bits          | Divisible by 4   |
+----------+------------------+------------------+

5.2 Word Access Algorithm
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   // Source: src/memory/memory.c
   uint32_t memory_read_word(memory_t *mem, uint32_t address) {
       // Check alignment
       if (address & 0x3) {
           // Unaligned access - may fault in future versions
           // Currently: allows with warning
           log_warning("Unaligned word access: 0x%08X", address);
       }

       // Read 4 bytes, combine as little-endian
       uint32_t byte0 = mem->data[address + 0];
       uint32_t byte1 = mem->data[address + 1];
       uint32_t byte2 = mem->data[address + 2];
       uint32_t byte3 = mem->data[address + 3];

       return byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24);
   }

5.3 Byte Access Algorithm
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: c

   uint8_t memory_read_byte(memory_t *mem, uint32_t address) {
       return mem->data[address];
   }

5.4 Alignment Requirements
~~~~~~~~~~~~~~~~~~~~~~~~~~

+------------------+----------------------------------+
| Data Type        | Requirement                      |
+==================+==================================+
| uint8_t (byte)   | Any address                      |
| uint16_t (half)  | address % 2 == 0                 |
| uint32_t (word)  | address % 4 == 0                 |
+------------------+----------------------------------+

.. note::
   The current implementation permits unaligned access with a warning.
   Future versions may generate exceptions on unaligned access.

5.5 Complexity Analysis
~~~~~~~~~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Operation | Time             | Notes            |
+==========+==================+==================+
| Read byte | O(1)             | Single array op  |
| Write byte| O(1)             | Single array op  |
| Read word | O(1)             | 4 array reads    |
| Write word| O(1)             | 4 array writes   |
+----------+------------------+------------------+

6. Memory-Mapped I/O
--------------------

6.1 MMIO Regions
~~~~~~~~~~~~~~~~

varm uses memory-mapped I/O. Devices appear at specific addresses
and are accessed using standard load/store instructions.

+----------+----------+------------------------------------------+
| Address  | Size     | Device                                   |
+==========+==========+==========================================
| 0xFF00   | 4 bytes  | UART0 (serial output)                    |
| 0xFF04   | 4 bytes  | UART0 status                             |
| 0xFF10   | 4 bytes  | Timer                                    |
| 0xFF20   | 4 bytes  | GPIO                                     |
+----------+----------+------------------------------------------+

6.2 UART Example
~~~~~~~~~~~~~~~~

.. code-block:: c

   // Write character to UART
   void uart_putc(char c) {
       volatile uint32_t *uart_data = (uint32_t *)0xFF00;
       *uart_data = (uint32_t)c;
   }

   // Read UART status
   int uart_ready(void) {
       volatile uint32_t *uart_status = (uint32_t *)0xFF04;
       return (*uart_status & 0x01) != 0;
   }

6.3 I/O Memory Characteristics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Property | RAM              | MMIO             |
+==========+==================+==================+
| Reads    | Return last write| Trigger device   |
| Writes   | Store value      | Trigger action   |
| Side     | None             | I/O operation    |
| Timing   | Immediate        | May block        |
+----------+------------------+------------------+

7. Memory Operations Complexity
-------------------------------

7.1 Flat Memory Model
~~~~~~~~~~~~~~~~~~~~~

The basic implementation uses a flat memory model with no paging:

- No page table lookup
- No TLB (Translation Lookaside Buffer)
- No cache hierarchy

7.2 Operation Costs
~~~~~~~~~~~~~~~~~~~

+----------+------------------+------------------+
| Operation | Time (cycles)   | Memory refs      |
+==========+==================+==================+
| Load byte | 1               | 1                |
| Load word | 1               | 1                |
| Store byte| 1               | 1                |
| Store word| 1               | 1                |
+----------+------------------+------------------+

.. note::
   These are approximate cycle counts for the basic implementation.
   The actual time depends on host system performance.

7.3 Address Translation Cost
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the current implementation:

- Physical address = Virtual address (no translation)
- Cost: O(1), single addition and mask

.. code-block:: c

   uint32_t translate_address(uint32_t virt_addr) {
       return virt_addr & VARM_MEMORY_MASK;  // Simple wrap
   }

7.4 Future Extensions
~~~~~~~~~~~~~~~~~~~~~

Planned memory subsystem improvements:

- **Virtual memory**: Page tables and TLB
- **Cache**: L1 instruction and data cache
- **MMU**: Address translation and protection

.. warning::
   These features are not implemented in the current version.
   The complexity guarantees above apply only to the basic
   flat memory model.
