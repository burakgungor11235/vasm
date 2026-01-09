Introduction to varm
====================

.. warning::

   varm is an experimental project under active development. The architecture,
   instruction set, and toolchain may change at any time. Code that works today
   may not work tomorrow. Use varm for learning and experimentation only—not
   for production code or critical systems.

Welcome to the varm tutorial. This document serves as your entry point into the
world of low-level programming. If you have never written assembly code before,
you are in the right place. We assume you understand programming fundamentals
like variables, functions, and control flow, but we do not assume you know how
computers actually execute your code at the hardware level.

By the end of this tutorial, you will understand what a virtual machine is,
why assembly language exists, and how to write and execute simple programs in
varm's assembly dialect. We will build your understanding from first principles,
using analogies and concrete examples rather than assuming prior knowledge.

What Is a Virtual Machine?
--------------------------

A virtual machine is a program that simulates a complete computer system. Think
of it as a computer within a computer. When you run a virtual machine, you are
creating an isolated environment that behaves like a real piece of hardware,
but executes entirely as software on your actual machine.

Consider an analogy: video game emulators. When you play an old Nintendo game
on your modern computer, you are running a virtual machine that simulates the
Nintendo's hardware. The emulator translates the original game's instructions
into operations your modern processor can understand. varm works similarly,
except instead of simulating a 1980s gaming console, it simulates a simplified
ARM-like processor architecture.

Why use a virtual machine for learning instead of real hardware? Several
reasons make varm an excellent choice for beginners:

**Safety**: Writing assembly directly on real hardware can crash your system,
corrupt data, or leave it in an unstable state. varm runs in a sandboxed
environment where mistakes are harmless. If your program has a bug, only the
virtual machine crashes—not your operating system.

**Accessibility**: You do not need special hardware to learn assembly. Every
student can run varm on any computer, regardless of the underlying processor
architecture. A student using an Intel Mac and a student using an AMD Linux
machine can write identical varm code and see identical results.

**Simplicity**: Real ARM processors have hundreds of instructions, complex
memory management units, and multiple privilege levels. varm implements a
subset of ARM concepts, making it feasible to learn the fundamentals without
being overwhelmed by edge cases and advanced features.

**Debugging**: varm provides tools to inspect registers, memory, and program
execution step by step. Understanding what happens inside a program becomes
possible when you can observe every aspect of its execution.

What Is Assembly Language?
--------------------------

Assembly language is the closest humans can get to writing machine code while
still maintaining readability. Every programming language you have used so far
is *abstracted* from the underlying hardware. When you write Python code like
``x = x + 1``, the language runtime handles all the details of storing values,
performing arithmetic, and managing memory. When you write C code with explicit
memory management, you still operate at a higher level than the processor's
native instruction set.

Assembly language removes these abstractions. Each assembly instruction
corresponds directly to a single processor operation. There are no variables
in the traditional sense—only registers (small storage locations built into
the processor) and memory addresses (locations in RAM). When you want to add
two numbers in assembly, you explicitly tell the processor which registers
contain those numbers and which register should receive the result.

Here is a simple example. In a high-level language, you might write:

.. code-block:: python

   def add_numbers(a, b):
       result = a + b
       return result

In varm assembly, the equivalent looks like this:

.. code-block:: varm

   mov r0, #5      ; Load immediate value 5 into register r0
   mov r1, #3      ; Load immediate value 3 into register r1
   add r2, r0, r1  ; Add r0 and r1, store result in r2

Each line translates directly to processor operations. There are no function
calls, no return values in the traditional sense, and no automatic memory
management. You are explicitly controlling every aspect of the computation.

The varm Architecture Overview
------------------------------

varm implements a reduced ARM-like architecture. ARM (Advanced RISC Machine)
is a family of processor architectures used in billions of devices, from
smartphones to tablets to embedded systems. By learning varm, you are building
a foundation that transfers to real-world ARM programming.

Registers
~~~~~~~~~

The processor contains 16 primary storage locations called **registers**.
Think of registers as extremely fast variables that exist directly inside the
processor chip itself. While your program might store data in RAM (which takes
hundreds of processor cycles to access), registers can be read and written in
a single cycle—making them the fastest storage available.

The registers in varm are named ``r0`` through ``r15``, with some registers
having special purposes:

``r0`` through ``r12`` are general-purpose registers. You can use them for any
calculation, storing intermediate results, or holding data you are currently
processing. These registers have no predetermined meaning and are entirely
under the programmer's control.

``sp`` (stack pointer, alias for r13) has a special purpose: it tracks the
top of the **stack**, a region of memory used for function calls, local
variables, and saving register values. The stack grows downward in memory,
and ``sp`` always points to the current top of the stack.

``lr`` (link register, alias for r14) stores the return address when a
function is called. When you execute a branch instruction that calls a
function, the processor automatically saves the address of the next
instruction in ``lr``, so the processor knows where to resume after the
function completes.

``pc`` (program counter, alias for r15) tracks the address of the next
instruction to execute. As the processor executes instructions, it
automatically increments ``pc`` to point to subsequent instructions.
You can also read from or write to ``pc`` directly, which enables advanced
control flow techniques.

Instruction Format
~~~~~~~~~~~~~~~~~~

All varm instructions are exactly 32 bits (4 bytes) wide. Understanding the
layout of these bits helps explain how the processor decodes and executes
instructions:

::

   ┌─────────────────────────────────────────────────────────────────┐
   │                    32-bit Instruction                            │
   ├───────────┬───────────┬───────────┬───────────┬─────────────────┤
   │  Opcode   │  Cond     │    Rn     │    Rd     │    Operand      │
   │  8 bits   │  4 bits   │  4 bits   │  4 bits   │    12 bits      │
   │  [31:24]  │  [23:20]  │  [19:16]  │  [15:12]  │    [11:0]       │
   └───────────┴───────────┴───────────┴───────────┴─────────────────┘
    MSB                                                      LSB

The **opcode** field (bits 31-24) specifies which operation the instruction
performs—whether it moves data, performs arithmetic, branches to a new
location, or performs some other action.

The **condition** field (bits 23-20) determines whether the instruction
executes. Instructions can be conditional, executing only when certain flags
are set (such as when the previous comparison resulted in equality). This
mechanism implements ``if`` statements and loops at the hardware level.

The **Rn** field (bits 19-16) specifies the first source register for the
operation. Many instructions operate on two source values and write to a
destination register.

The **Rd** field (bits 15-12) specifies the destination register—the register
that receives the result of the operation.

The **operand** field (bits 11-0) provides additional information for the
operation. For arithmetic instructions, this might contain an immediate value
(a constant number embedded directly in the instruction) or encode a shift
operation to apply to a register value.

Little-Endian Byte Order
~~~~~~~~~~~~~~~~~~~~~~~~

varm uses **little-endian** byte ordering, which determines how multi-byte
values are stored in memory. When a 32-bit value (4 bytes) is stored at a
memory address, the least significant byte occupies the lowest address, the
next byte occupies the next address, and so on.

Consider the hexadecimal value ``0x12345678`` stored starting at address
``0x100``:

::

   Address 0x100: 0x78
   Address 0x101: 0x56
   Address 0x102: 0x34
   Address 0x103: 0x12

The bytes appear reversed in memory compared to how we typically write the
value. This ordering may seem counterintuitive, but it has historical reasons
and remains common in modern processors. When debugging or inspecting memory,
always remember that little-endian systems store the least significant byte
at the lowest address.

Your First varm Program
-----------------------

Let us write a program that loads a value into a register and then exits.
This may seem trivial, but understanding every component of this simple
program lays the foundation for everything that follows.

Starter Program
~~~~~~~~~~~~~~~

Save the following code as ``hello.var``:

.. code-block:: varm

   .text
   .global _start

   _start:
       mov r0, #42
       b .

The ``.text`` directive marks the beginning of the code section. In assembly,
directives are instructions to the assembler (the program that translates
assembly into machine code) rather than instructions to the processor. The
``.text`` directive says "everything following this point is executable code."

The ``.global _start`` directive makes the symbol ``_start`` visible to the
linker. Every program needs an entry point—the first instruction that executes
when the program starts. By convention, the entry point is named ``_start``.

The ``_start:`` line defines a label. A label is a symbolic name for a
memory address. When the assembler processes this file, it records that
``_start`` refers to the address of the following instruction (``mov r0, #42``).

The ``mov r0, #42`` instruction loads the immediate value 42 into register
``r0``. The ``mov`` instruction copies data from one location to another.
Here, the source is the immediate value ``#42`` and the destination is
register ``r0``. After this instruction executes, ``r0`` contains the value 42.

The ``b .`` instruction branches (jumps) to the current location—the dot
(``.``) represents the current address. This creates an infinite loop,
preventing the program from continuing past our test code. In a real program,
you would replace this with code that properly exits.

Equivalent Pseudocode
~~~~~~~~~~~~~~~~~~~~

To understand what this program does, consider its equivalent in high-level
pseudocode:

.. code-block:: python

   r0 = 42    # Load immediate value 42 into register r0
   while True:
       pass   # Infinite loop

The program loads 42 into a register and then loops forever. While this does
not do anything useful, it demonstrates the basic structure of a varm program
and provides a starting point for experimentation.

Assembling and Running
~~~~~~~~~~~~~~~~~~~~~~

To assemble and run this program, use the varm toolchain:

.. code-block:: bash

   varm assemble hello.var -o hello.bin
   varm run hello.bin

The ``assemble`` command translates your assembly source code into a binary
file containing the raw machine instructions. The ``run`` command executes
the binary using the varm virtual machine.

A More Complete Example
-----------------------

The previous program was intentionally minimal. Let us now examine a slightly
more substantive program that demonstrates additional concepts:

.. code-block:: varm

   .text
   .global _start

   _start:
       mov r0, #10        ; Load value 10 into r0
       mov r1, #20        ; Load value 20 into r1
       add r2, r0, r1     ; r2 = r0 + r1
       sub r3, r1, r0     ; r3 = r1 - r0
       mul r4, r0, r2     ; r4 = r0 * r2

   loop:
       subs r0, r0, #1    ; Decrement r0 by 1, set flags
       bne loop           ; Branch if not equal (r0 != 0)

   _exit:
       b .                ; Infinite loop (hang)

Line-by-Line Explanation
~~~~~~~~~~~~~~~~~~~~~~~~

Let us examine each instruction in detail:

**``mov r0, #10``** loads the immediate value 10 into register ``r0``.
The hash symbol (``#``) indicates that ``10`` is a constant value, not a
reference to another register. After this instruction, ``r0`` contains 10.

**``mov r1, #20``** similarly loads the immediate value 20 into register
``r1``. Now both ``r0`` and ``r1`` hold values we can use in subsequent
calculations.

**``add r2, r0, r1``** performs addition. The destination register (``r2``)
comes first, followed by the two source registers. This instruction computes
``r0 + r1`` and stores the result in ``r2``. After execution, ``r2`` contains 30.

**``sub r3, r1, r0``** performs subtraction. The instruction computes
``r1 - r0`` and stores the result in ``r3``. After execution, ``r3`` contains 10.

**``mul r4, r0, r2``** performs multiplication. The instruction computes
``r0 * r2`` and stores the result in ``r4``. After execution, ``r4`` contains 300.

**``subs r0, r0, #1``** performs subtraction and sets condition flags.
The ``s`` suffix on the instruction name tells the processor to update the
**condition flags** based on the result. Specifically, this instruction
subtracts 1 from ``r0`` (so ``r0`` becomes 9) and sets the Zero flag if the
result is zero, the Negative flag if the result is negative, and so on.

**``bne loop``** branches (jumps) to the ``loop`` label if the Zero flag is
*not* set. The ``ne`` condition means "not equal." Since we are decrementing
from 10 down to 0, the Zero flag will only be set when ``r0`` reaches 0.
Therefore, this loop runs 10 times.

**``b .``** at the ``_exit`` label creates an infinite loop. After the
decrement loop finishes, execution reaches this point and hangs forever.

Equivalent Pseudocode
~~~~~~~~~~~~~~~~~~~~

Here is the same program expressed in pseudocode:

.. code-block:: python

   r0 = 10
   r1 = 20
   r2 = r0 + r1      # 30
   r3 = r1 - r0      # 10
   r4 = r0 * r2      # 300

   while r0 != 0:
       r0 = r0 - 1

   while True:
       pass

The arithmetic operations are straightforward. The loop demonstrates how
conditional branching works: the ``subs`` instruction sets flags, and the
``bne`` instruction checks those flags to decide whether to branch.

Registers Explained
-------------------

Registers are fundamental to assembly programming. Understanding what they
are and how to use them is essential before proceeding further.

What Is a Register?
~~~~~~~~~~~~~~~~~~~

A register is a small storage location built directly into the processor chip.
Unlike RAM, which requires electrical signals to read and write and involves
complex memory controllers, registers are accessed in a single processor cycle.
Think of registers as the processor's "working memory"—the places it keeps
values it is currently manipulating.

Registers in varm are 32 bits wide, meaning each register can hold any integer
value from 0 to 4,294,967,295 (if interpreted as an unsigned number) or from
-2,147,483,648 to 2,147,483,647 (if interpreted as a signed number using two's
complement representation).

General-Purpose Registers (r0-r12)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Registers ``r0`` through ``r12`` are general-purpose registers. You can use
them for any purpose without restriction. Conventions have emerged over time
for how these registers should be used, but these are software conventions,
not hardware requirements:

- ``r0`` through ``r3`` are often used to pass function arguments. When a
  function is called, the first few arguments typically go in these registers.

- ``r0`` also commonly holds return values from functions.

- ``r4`` through ``r11`` are often called "callee-saved" registers. If a
  function needs to use these registers, it must save and restore their
  original values.

- ``r12`` is sometimes used as an intra-procedure call scratch register.

For simple varm programs without function calls, you can ignore these
conventions and use any general-purpose register for any purpose.

Special Registers
~~~~~~~~~~~~~~~~~

Several registers have special meanings defined by the architecture:

**sp (stack pointer, r13)** tracks the top of the stack. The stack is a region
of memory that grows downward (toward lower addresses). When you push a value
onto the stack, ``sp`` decreases by 4 (the size of a 32-bit value). When you
pop a value from the stack, ``sp`` increases by 4. The stack is used for:

- Allocating local variables inside functions
- Saving register values that need to be preserved across function calls
- Passing arguments to functions (on some architectures)
- Returning from function calls

**lr (link register, r14)** holds the return address for function calls.
When you execute a branch-and-link instruction (``bl``), the processor stores
the address of the next instruction in ``lr`` and jumps to the target address.
When the function completes, it executes ``mov pc, lr`` (or equivalent) to
return to the instruction following the original call.

**pc (program counter, r15)** always contains the address of the *next*
instruction to execute. As the processor fetches and executes instructions,
it automatically increments ``pc`` to point to subsequent instructions. You
can also read from or write to ``pc`` directly, which enables position-
independent code and advanced control flow.

Operational Semantics
~~~~~~~~~~~~~~~~~~~~

We can describe register operations using formal notation. The ``mov``
instruction has the following operational semantics:

.. math::

   \frac{}{\text{pc} \leftarrow \text{pc} + 4, \quad \text{R}[d] \leftarrow \text{imm}_{32}}

   \text{where imm}_{32} \text{ is the 32-bit sign-extended immediate value}

The notation means: after executing a ``mov`` instruction, the program counter
advances by 4 bytes (to the next instruction), and the destination register
``R[d]`` receives the 32-bit immediate value.

The ``add`` instruction semantics:

.. math::

   \frac{}{\text{pc} \leftarrow \text{pc} + 4, \quad \text{R}[d] \leftarrow \text{R}[n] + \text{R}[m]}

The addition instruction reads two source registers, adds their values, and
stores the result in the destination register, while advancing the program
counter.

Memory Model
------------

Memory in varm is organized into distinct sections, each serving a specific
purpose. Understanding these sections helps you write correct programs and
debug memory-related issues.

Text Section (.text)
~~~~~~~~~~~~~~~~~~~~

The text section contains executable code. When you write instructions like
``mov`` and ``add``, they reside in the text section. This section is
typically read-only during execution—once the processor starts running, it
does not modify the code itself.

varm loads code starting at address ``0x20`` (32 in decimal). This is an
arbitrary but fixed choice made when designing the virtual machine. When the
processor starts executing, it begins at address 0x20, which should contain
the first instruction of your ``_start`` function.

Data Section (.data)
~~~~~~~~~~~~~~~~~~~~

The data section contains initialized variables—values that exist when the
program starts. You declare data section entries using directives like
``.word`` (define a 32-bit word) or ``.byte`` (define an 8-bit byte).

varm loads data starting at address ``0x10000`` (65,536 in decimal). This
separation between code and data addresses helps catch programming errors:
if a program accidentally tries to treat code as data or vice versa, the
behavior will be predictable and reproducible.

Example with Data Section
~~~~~~~~~~~~~~~~~~~~~~~~~

Here is a program that uses the data section:

.. code-block:: varm

   .data
   value: .word 42

   .text
   .global _start

   _start:
       ldr r0, =value   ; Load the address of 'value' into r0
       ldr r1, [r0]     ; Load the actual value (42) from that address
       b .

The ``.data`` section contains a single word (32-bit value) named ``value``
initialized to 42. The ``.word`` directive reserves 4 bytes of storage and
initializes it to the specified value.

The ``ldr r0, =value`` instruction loads the *address* of the ``value``
symbol into ``r0``. This is a "load address" pseudo-instruction that the
assembler translates into the appropriate instructions.

The ``ldr r1, [r0]`` instruction loads a 32-bit value from the memory address
contained in ``r0`` and stores it in ``r1``. After this instruction, ``r1``
contains 42.

Address Layout
~~~~~~~~~~~~~~

The complete memory layout in varm looks like this:

::

   Address 0x00000 - 0x0001F: Reserved (unused)
   Address 0x00020 - 0x0FFFF: Text section (code)
   Address 0x10000 - 0x1FFFF: Data section (variables)
   Address 0x20000+: Reserved (undefined behavior)

The exact addresses of code and data are implementation details of the
varm virtual machine. Different virtual machines or real hardware would use
different addresses.

Next Steps
----------

You now have a foundational understanding of varm, its architecture, and
its programming model. The concepts covered here—registers, memory sections,
and instruction formats—apply broadly to assembly programming.

The next tutorial in this series covers:

- Basic arithmetic and logical operations
- Conditional execution and flags
- Memory access with load and store instructions
- Writing functions with proper calling conventions

Before proceeding, ensure you understand:

- What registers are and why they exist
- The difference between code (text section) and data (data section)
- How little-endian byte ordering works

Experiment with the starter programs provided. Change values, add new
instructions, and observe how the behavior changes. The best way to learn
assembly is by writing assembly.

.. note::

   Remember: varm is not stable. APIs, instruction behavior, and tooling
   may change. If something does not work as described, check the latest
   documentation or report an issue on the project repository.
