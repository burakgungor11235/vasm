VM Architecture
==============

.. warning::
   **varm is not stable.** The architecture described here reflects the
   current implementation and may change without notice.

1. Overview of varm Architecture
--------------------------------

varm implements a simplified von Neumann architecture with a unified
address space for instructions and data. The design prioritizes
simplicity and educational value over performance.

1.1 Von Neumann Architecture
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. mermaid::
   :caption: Von Neumann Architecture

   graph TB
       subgraph Memory["Memory"]
           Instructions["Instructions"]
           Data["Data"]
       end

       subgraph CPU["CPU"]
           ALU["ALU"]
           Registers["Registers"]
           Control["Control Unit"]
       end

       Instructions <--->|"Fetch"| Control
       Control <---> ALU
       Control <---> Registers
       Data <---> Control

Key characteristics:

- **Unified memory**: Instructions and data share the same address space
- **Sequential execution**: By default, instructions execute in order
- **Stored program**: Programs are loaded into memory as data

1.2 Bus Architecture
~~~~~~~~~~~~~~~~~~~~

+----------+-------------------+--------------------------------+
| Property | Value             | Rationale                      |
+==========+===================+================================+
| Data bus | 32 bits           | Simple word operations         |
| Address  | 32 bits           | 4GB addressable (implemented   |
| bus      |                   | as 64KB for simplicity)        |
| Endian   | Little-endian     | x86/ARM compatibility          |
+----------+-------------------+--------------------------------+

.. code-block:: c

   // Source: src/common/types.h
   typedef uint32_t varm_word_t;
   typedef uint8_t  varm_byte_t;

1.3 Fetch-Decode-Execute Cycle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The CPU operates on a three-stage cycle:

.. mermaid::
   :caption: Fetch-Decode-Execute Cycle

   flowchart LR
       Fetch["Fetch\n读取指令"] --> Decode["Decode\n译码"]
       Decode --> Execute["Execute\n执行"]
       Execute -->|"PC += 4"| Fetch

1. **Fetch**: Read instruction from memory at PC address
2. **Decode**: Extract opcode, operands, and condition codes
3. **Execute**: Perform the operation specified by the opcode

.. code-block:: c

   // Source: src/vm/cpu.c
   void cpu_cycle(vm_t *vm) {
       uint32_t instruction = fetch(vm, vm->pc);
       decode_and_execute(vm, instruction);
   }

2. System Block Diagram
-----------------------

.. mermaid::
   :caption: varm System Block Diagram

   graph TB
       subgraph VM["varm VM"]
           Memory["Memory Subsystem\n64KB RAM"] <--->|"32-bit"| CPU["CPU Subsystem"]
           CPU <--->|"32-bit"| IO["I/O"]
           Memory <--->|"映射"| Data["Data Section"]
           Memory <--->|"映射"| Text["Text Section"]
           CPU <---> Reg["Register File\n16 x 32-bit"]
       end

2.1 CPU Subsystem Components
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+--------------------------+--------------------------------------------+
| Component                | Purpose                                    |
+==========================+============================================+
| Control Unit             | Orchestrates instruction cycle             |
| ALU                      | Performs arithmetic/logic operations       |
| Register File            | Fast access storage (16 x 32-bit)          |
| Condition Code Register  | Flags (N, Z, C, V) for conditional        |
|                          | execution                                  |
| Program Counter          | Tracks next instruction address            |
+--------------------------+--------------------------------------------+

2.2 Memory Subsystem Components
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+--------------------------+----------------------------------------+
| Component                | Purpose                                |
+==========================+========================================+
| RAM                      | 64KB unified address space             |
| Memory Mapper            | Maps addresses to memory regions       |
| MMU (future)             | Virtual memory management              |
+--------------------------+----------------------------------------+

2.3 I/O Subsystem
~~~~~~~~~~~~~~~~~

I/O is memory-mapped. Devices appear at specific address ranges
and are accessed via standard load/store instructions.

4. Instruction Set Architecture
-------------------------------

4.1 32-bit Format Overview
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. mermaid::
   :caption: Instruction Format

   flowchart LR
       direction TB
       OPCODE["Opcode\n8 bits"] --> COND["Cond\n4 bits"]
       COND --> RN["Rn\n4 bits"]
       RN --> RD["Rd\n4 bits"]
       RD --> RM["Operand2\n12 bits"]

+------------------+-------+-----------------------------------+
| Field            | Bits  | Description                       |
+==================+=======+===================================+
| opcode           | 31-24 | Instruction operation code        |
| cond             | 23-20 | Condition for execution           |
| rd               | 19-16 | Destination register              |
| rn               | 15-12 | First source register             |
| operand2         | 11-0  | Second source register/immediate  |
+------------------+-------+-----------------------------------+

.. code-block:: c

   // Source: src/common/opcodes.h
   #define OP_ADD  0x01
   #define OP_SUB  0x02
   #define OP_MOV  0x03
   #define OP_LDR  0x04
   #define OP_STR  0x05
   #define OP_B    0x06
   // ... etc

5. Execution Model
------------------

5.1 Single-Threaded Execution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

varm executes a single instruction stream. There is no hardware
support for:

- Multiple cores
- Hyperthreading
- Out-of-order execution

The execution model is strictly sequential.

5.2 Sequential Instruction Fetch
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The program counter (PC) tracks the current instruction address:

- PC always points to the instruction being executed
- After fetch, PC increments by 4 (one word)
- Branches modify PC directly

.. code-block:: c

   // Source: src/vm/cpu.c - fetch stage
   uint32_t fetch(vm_t *vm, uint32_t address) {
       // Read one word (4 bytes) from memory
       return memory_read_word(vm->memory, address);
   }

5.3 Pipeline Stages
~~~~~~~~~~~~~~~~~~~

.. mermaid::
   :caption: Instruction Pipeline

   gantt
       title Pipeline Stages
       dateFormat X
       axisFormat %s

       Fetch   :0, 1
       Decode  :1, 2
       Execute :2, 3
       Writeback :3, 4

       Fetch   :crit, 4, 5
       Decode  :crit, 5, 6
       Execute :crit, 6, 7
       Writeback :crit, 7, 8

The basic varm implementation does not use instruction pipelining.
Each instruction completes all stages before the next begins.
This simplifies the implementation but limits throughput.

6. Performance Characteristics
------------------------------

All basic VM operations run in constant time O(1):

- Instruction fetch, decode, and execute: 1 cycle each
- Memory load/store: 1 cycle (direct array access)
- ALU operations: 1 cycle

This simplicity comes from:

- Flat memory model (no paging)
- No cache hierarchy
- Fixed instruction format

.. note::
   Future versions may add caching or virtual memory, which would change these characteristics.
