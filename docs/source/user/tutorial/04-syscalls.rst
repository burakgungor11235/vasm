.. _syscalls:

=========
Syscalls
=========

.. warning::
   varm is not stable. The syscall interface, syscall numbers, and available
   system calls may change in future versions. The examples in this section
   work with the current version but verify behavior in your specific
   installation.

What Is a System Call?
======================

In high-level programming languages, functions like ``printf()``, ``scanf()``,
``malloc()``, and ``exit()`` are provided by the standard library. These
functions ultimately need to interact with the operating system to perform
operations like displaying text, reading input, allocating memory, or stopping
the program.

A **system call** (or syscall) is the fundamental interface between user
programs and the operating system kernel. When your program needs a service
that only the OS can provide, it makes a system call. This is a controlled
transition from user mode (where your code runs with limited privileges) to
kernel mode (where the OS has full access to hardware).

How Syscalls Work in varm
-------------------------

In varm, system calls are made using the ``SWI`` (Software Interrupt)
instruction. The convention is:

1. **r7** contains the syscall number (which determines what service to request)
2. **Other registers** contain arguments specific to that syscall
3. **The SWI instruction** triggers the system call
4. **Return values** are placed in specific registers

This follows the Linux ARM syscall convention, making it easier to understand
if you later learn Linux system programming.

**System Call Sequence:**

.. code-block:: text

    ┌─────────────────────────────────────────────────────────┐
    │  1. Load syscall number into r7                         │
    │  2. Load arguments into r0-r6 as required               │
    │  3. Execute SWI instruction                             │
    │  4. Kernel handles the request                          │
    │  5. Result returned in r0 (usually)                     │
    └─────────────────────────────────────────────────────────┘

Available Syscalls in varm
==========================

The current version of varm supports the following system calls. Remember that
varm is not stable, so additional syscalls may be added or these may change.

EXIT: Terminate Program
-----------------------

The EXIT syscall stops the program and returns control to the operating system.

**Syscall Number:** 1

**Arguments:**

* **r0:** Exit code (integer value returned to the system)

**Return Value:** Does not return (program terminates)

**Example:**

.. code-block:: arm

    MOV  r0, #0           @ Exit code 0 (success)
    MOV  r7, #1           @ Syscall number for exit
    SWI  0                @ Make the system call

READ: Read from File Descriptor
-------------------------------

The READ syscall reads data from a file descriptor (such as stdin).

**Syscall Number:** 3

**Arguments:**

* **r0:** File descriptor (0 = stdin)
* **r1:** Buffer address (where to store read data)
* **r2:** Maximum bytes to read

**Return Value:**

* **r0:** Number of bytes actually read, or negative error code
* **r0 = 0** indicates end of file

**Example:**

.. code-block:: arm

    MOV  r0, #0           @ File descriptor 0 (stdin)
    LDR  r1, =buffer      @ Address of input buffer
    MOV  r2, #80          @ Read up to 80 bytes
    MOV  r7, #3           @ Syscall number for read
    SWI  0                @ Make the system call

WRITE: Write to File Descriptor
-------------------------------

The WRITE syscall writes data to a file descriptor (such as stdout or stderr).

**Syscall Number:** 4

**Arguments:**

* **r0:** File descriptor (1 = stdout, 2 = stderr)
* **r1:** Buffer address (data to write)
* **r2:** Number of bytes to write

**Return Value:**

* **r0:** Number of bytes actually written, or negative error code

**Example:**

.. code-block:: arm

    MOV  r0, #1           @ File descriptor 1 (stdout)
    LDR  r1, =message     @ Address of message to write
    MOV  r2, #13          @ Message length (13 bytes including newline)
    MOV  r7, #4           @ Syscall number for write
    SWI  0                @ Make the system call

File Descriptors
----------------

File descriptors are small non-negative integers that reference open files,
devices, or pipes. varm uses the standard Unix convention:

=== =========== =============================================
FD  Name        Purpose
=== =========== =============================================
0   stdin       Standard input (keyboard, pipe input, etc.)
1   stdout      Standard output (terminal display)
2   stderr      Standard error (error messages)
=== =========== =============================================

Using the SWI Instruction
=========================

The SWI (Software Interrupt) instruction triggers a system call. In varm, the
SWI instruction takes an optional immediate value that is ignored by the
current implementation but is included for compatibility.

**Syntax:**

.. code-block:: text

    SWI  <immediate>

**Examples:**

.. code-block:: arm

    SWI  0              @ Trigger syscall (number in r7)
    SWI  0x9001         @ Same effect, immediate value ignored

**Operational Semantics:**

.. math::

    \text{SWI}(imm) \equiv
    \begin{cases}
    \text{exit}(r0) & \text{if } r7 = 1 \\
    \text{read}(r0, r1, r2) \rightarrow r0 & \text{if } r7 = 3 \\
    \text{write}(r0, r1, r2) \rightarrow r0 & \text{if } r7 = 4
    \end{cases}

**Complexity:** O(n) where n is the number of bytes read/written for I/O
syscalls, O(1) for exit.

Exit Syscall Example
====================

The simplest useful syscall is EXIT. Every program should call it to terminate
cleanly.

**Starter Code:**

.. code-block:: arm

    @ exit_example.asm - Using the exit syscall
    @ Run with: varm exit_example.asm
    @
    @ This program exits with code 42

    .text
    .global _start

    _start:
        MOV   r0, #42           @ Set exit code to 42
        MOV   r7, #1            @ Syscall number 1 = exit
        SWI   0                 @ Make the system call

    @ Program terminates here

I/O Syscalls
============

Reading and writing data is fundamental to most programs. varm provides basic
I/O through the READ and WRITE syscalls.

Writing Output
--------------

To display text, you need to:

1. Store the text in memory (typically in the .data section)
2. Set up the WRITE syscall arguments
3. Execute the SWI instruction

**Starter Code:**

.. code-block:: arm

    @ write_example.asm - Writing output to stdout
    @ Run with: varm write_example.asm

    .data
    message:  .asciz "Hello, World!\n"

    .text
    .global _start

    _start:
        @ Calculate string length (excluding null terminator)
        MOV   r2, #13           @ Length of "Hello, World!\n"

        @ Set up WRITE syscall arguments
        MOV   r0, #1            @ File descriptor 1 = stdout
        LDR   r1, =message      @ Address of message
        MOV   r7, #4            @ Syscall number 4 = write

        SWI   0                 @ Make the system call

        @ Exit the program
        MOV   r0, #0            @ Exit code 0 = success
        MOV   r7, #1            @ Syscall number 1 = exit
        SWI   0                 @ Make the system call

Reading Input
-------------

To read input, you need to:

1. Allocate a buffer in memory to store the input
2. Set up the READ syscall arguments
3. Execute the SWI instruction
4. The number of bytes actually read is returned in r0

**Starter Code:**

.. code-block:: arm

    @ read_example.asm - Reading input from stdin
    @ Run with: varm read_example.asm
    @ (Provide input via echo or terminal)

    .data
    prompt:     .asciz "Enter text: "
    buffer:     .space  80          @ Input buffer (80 bytes)
    newline:    .asciz "\n"

    .text
    .global _start

    _start:
        @ Display prompt
        MOV   r0, #1            @ stdout
        LDR   r1, =prompt       @ Prompt message
        MOV   r2, #12           @ "Enter text: " is 12 bytes
        MOV   r7, #4            @ write syscall
        SWI   0

        @ Read input
        MOV   r0, #0            @ stdin
        LDR   r1, =buffer       @ Input buffer address
        MOV   r2, #80           @ Maximum bytes to read
        MOV   r7, #3            @ read syscall
        SWI   0

        @ r0 now contains number of bytes read

        @ Echo what was read (optional)
        MOV   r5, r0            @ Save byte count
        MOV   r0, #1            @ stdout
        LDR   r1, =buffer       @ Buffer address
        MOV   r2, r5            @ Number of bytes read
        MOV   r7, #4            @ write syscall
        SWI   0

        @ Display newline
        MOV   r0, #1
        LDR   r1, =newline
        MOV   r2, #1
        MOV   r7, #4
        SWI   0

        @ Exit
        MOV   r0, #0
        MOV   r7, #1
        SWI   0

Complete Example: Echo Program
==============================

The following program reads input from stdin and echoes it back to stdout.
This demonstrates the complete cycle of I/O syscalls.

.. code-block:: arm

    @ echo_program.asm - Complete echo program
    @ Run with: varm echo_program.asm
    @ Type some text and press Ctrl+D (Unix) or Ctrl+Z (Windows) to end

    .data
    buffer:     .space  256          @ Input buffer (256 bytes)
    prompt:     .asciz "> "          @ Simple prompt
    newline:    .asciz "\n"

    .text
    .global _start

    _start:
        @ Print prompt
        MOV   r0, #1                @ stdout
        LDR   r1, =prompt
        MOV   r2, #2                @ "> " is 2 bytes
        MOV   r7, #4                @ write
        SWI   0

    read_loop:
        @ Read a line from stdin
        MOV   r0, #0                @ stdin
        LDR   r1, =buffer
        MOV   r2, #255              @ Leave room for null
        MOV   r7, #3                @ read
        SWI   0

        @ Check if we got any input
        CMP   r0, #0
        BEQ   exit                  @ EOF reached, exit

        @ Echo the input back
        MOV   r4, r0                @ Save byte count (r0 clobbered by syscall)
        MOV   r0, #1                @ stdout
        LDR   r1, =buffer
        MOV   r2, r4                @ Byte count
        MOV   r7, #4                @ write
        SWI   0

        @ Loop back for more input
        B     read_loop

    exit:
        @ Exit cleanly
        MOV   r0, #0
        MOV   r7, #1
        SWI   0

Error Handling
==============

Syscalls can fail. When a syscall fails, it typically returns a negative
value in r0. Your programs should check for errors when appropriate.

**Example with Error Checking:**

.. code-block:: arm

    @ write_with_error_check.asm - Syscall with error handling
    @ Run with: varm write_with_error_check.asm

    .data
    message:  .asciz "Test message\n"

    .text
    .global _start

    _start:
        MOV   r0, #1            @ stdout
        LDR   r1, =message
        MOV   r2, #13
        MOV   r7, #4
        SWI   0

        @ Check for error (negative return value)
        CMP   r0, #0
        BLT   write_error       @ Branch if negative

        @ Exit successfully
        MOV   r0, #0
        MOV   r7, #1
        SWI   0

    write_error:
        @ Handle write error
        @ r0 contains negative error code
        MOV   r0, #1            @ Still try to write to stderr
        LDR   r1, =error_msg
        MOV   r2, #17
        MOV   r7, #4
        SWI   0

        MOV   r0, #1            @ Exit with code 1
        MOV   r7, #1
        SWI   0

    .data
    error_msg:  .asciz "Write error!\n"
