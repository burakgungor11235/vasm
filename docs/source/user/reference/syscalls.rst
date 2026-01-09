.. _syscalls:

========
Syscalls
========

.. warning::
   varm is not a stable project. Syscall numbers, conventions, and behavior
   may change without notice. This documentation reflects the current implementation.

Syscalls in varm are invoked via the ``SWI`` instruction. The syscall number
is passed in register R7, with arguments in R0, R1, and R2. Return values are
placed in R0.

.. contents:: Table of Contents
   :local:

Syscall Calling Convention
==========================

+-----------+--------------------------------------------------+
| Register  | Purpose                                          |
+-----------+--------------------------------------------------+
| R7        | Syscall number                                   |
| R0        | First argument / Return value                    |
| R1        | Second argument                                  |
| R2        | Third argument                                   |
+-----------+--------------------------------------------------+

**Invocation:**

.. code-block:: asm

   mov r7, #SYSCALL_NUM    ; syscall number
   mov r0, #ARG1           ; first argument
   mov r1, #ARG2           ; second argument
   mov r2, #ARG3           ; third argument
   swi                     ; invoke syscall

**Return:**

The return value is placed in R0. For syscalls that don't return a value,
R0 is undefined.

Syscall Reference
=================

EXIT - Terminate Process
------------------------

**Number:** 1

**Description:** Terminates the program and exits with the specified code.

**Arguments:**

+-----------+------------------+----------------------------------+
| Register  | Name             | Description                      |
+-----------+------------------+----------------------------------+
| R0        | exit_code        | Exit code (0-255)                |
+-----------+------------------+----------------------------------+

**Return:** None (program terminates)

**Encoding:**::

   mov r0, #<exit_code>
   mov r7, #1
   swi

**Example - Exit with code 0:**

.. code-block:: asm

   mov r0, #0
   mov r7, #1
   swi

**Example - Exit with code 42:**

.. code-block:: asm

   mov r0, #42
   mov r7, #1
   swi

**Example - Exit with computed code:**

.. code-block:: asm

   mov r0, r1          ; exit code from r1
   mov r7, #1
   swi

**Complexity:** O(1) time, O(1) space

---

READ - Read from File Descriptor
--------------------------------

**Number:** 2

**Description:** Reads data from a file descriptor into a buffer.

**Arguments:**

+-----------+------------------+----------------------------------+
| Register  | Name             | Description                      |
+-----------+------------------+----------------------------------+
| R0        | fd               | File descriptor (0 = stdin only) |
| R1        | buffer           | Address of buffer in memory      |
| R2        | count            | Maximum bytes to read            |
+-----------+------------------+----------------------------------+

**Return:**

+-----------+------------------+----------------------------------+
| Register  | Value            | Description                      |
+-----------+------------------+----------------------------------+
| R0        | >= 0             | Number of bytes read             |
| R0        | -1 (0xFFFFFFFF)  | Error or invalid fd              |
+-----------+------------------+----------------------------------+

**Encoding:**::

   mov r0, #0              ; fd = stdin
   ldr r1, =buffer         ; buffer address
   mov r2, #<count>        ; max bytes to read
   mov r7, #2
   swi                     ; R0 = bytes read

**Example - Read from stdin:**

.. code-block:: asm

   .data
   buffer: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  ; 10-byte buffer

   .text
   mov r0, #0          ; fd = stdin
   ldr r1, =buffer     ; buffer address = 0x10000
   mov r2, #10         ; read up to 10 bytes
   mov r7, #2
   swi                 ; R0 = bytes read
   ; R0 now contains the number of bytes read

**Example - Read single character:**

.. code-block:: asm

   .data
   char:   .byte 0

   .text
   mov r0, #0          ; stdin
   ldr r1, =char       ; buffer
   mov r2, #1          ; 1 byte
   mov r7, #2
   swi                 ; R0 = 1 on success

**Buffer Validation:**

The VM validates that the buffer address and size are within valid memory:

- Address must be less than MEMORY_SIZE (1 MiB)
- Address + size must not exceed MEMORY_SIZE
- If validation fails, R0 is set to -1

**Complexity:** O(n) time, O(1) space, where n = bytes read

**Limitations:**

- Only fd 0 (stdin) is supported
- Other file descriptors return -1

---

WRITE - Write to File Descriptor
--------------------------------

**Number:** 3

**Description:** Writes data from a buffer to a file descriptor.

**Arguments:**

+-----------+------------------+----------------------------------+
| Register  | Name             | Description                      |
+-----------+------------------+----------------------------------+
| R0        | fd               | File descriptor (1=stdout, 2=stderr)|
| R1        | buffer           | Address of data in memory        |
| R2        | count            | Number of bytes to write         |
+-----------+------------------+----------------------------------+

**Return:**

+-----------+------------------+----------------------------------+
| Register  | Value            | Description                      |
+-----------+------------------+----------------------------------+
| R0        | >= 0             | Number of bytes written          |
| R0        | -1 (0xFFFFFFFF)  | Error or invalid fd              |
+-----------+------------------+----------------------------------+

**Encoding:**::

   mov r0, #1              ; fd = stdout
   ldr r1, =buffer         ; data address
   mov r2, #<count>        ; bytes to write
   mov r7, #3
   swi                     ; R0 = bytes written

**Example - Write string to stdout:**

.. code-block:: asm

   .data
   msg:    .byte 'H', 'e', 'l', 'l', 'o', '!', 10  ; "Hello!\n"

   .text
   mov r0, #1          ; fd = stdout
   ldr r1, =msg        ; msg address = 0x10000
   mov r2, #7          ; length = 7 bytes
   mov r7, #3
   swi                 ; writes "Hello!\n"
   mov r0, #0          ; exit code 0
   mov r7, #1
   swi                 ; exit

**Example - Write to stderr:**

.. code-block:: asm

   .data
   err_msg: .byte 'E', 'r', 'r', 'o', 'r', '!', 10

   .text
   mov r0, #2          ; fd = stderr
   ldr r1, =err_msg
   mov r2, #7
   mov r7, #3
   swi                 ; writes "Error!" to stderr

**Example - Write register value:**

.. code-block:: asm

   mov r1, #65         ; 'A'
   strb r1, [sp, #-1]! ; store at top of stack
   mov r0, #1          ; stdout
   mov r2, #1          ; 1 byte
   mov r7, #3
   swi

**Buffer Validation:**

Same as READ: the buffer must be within valid memory.

**Complexity:** O(n) time, O(1) space, where n = bytes written

**Limitations:**

- Only fd 1 (stdout) and 2 (stderr) are fully supported
- Other file descriptors return -1

---

Syscall Summary Table
====================

+-------+-------------+------+------+------+------------------------------+
| Num   | Name        | R0   | R1   | R2   | Description                  |
+-------+-------------+------+------+------+------------------------------+
| 1     | EXIT        | code | -    | -    | Terminate with exit code     |
| 2     | READ        | fd   | buf  | n    | Read n bytes to buffer       |
| 3     | WRITE       | fd   | buf  | n    | Write n bytes from buffer    |
+-------+-------------+------+------+------+------------------------------+

**Return Values:**

+-------+-------------+----------------------------------------------+
| Num   | Name        | Return in R0                                 |
+-------+-------------+----------------------------------------------+
| 1     | EXIT        | None (program exits)                         |
| 2     | READ        | bytes read, or -1 on error                   |
| 3     | WRITE       | bytes written, or -1 on error                |
+-------+-------------+----------------------------------------------+

Complete Program Examples
=========================

Hello World
-----------

.. code-block:: asm

   ; hello.vasm - Print "Hello, World!" to stdout

   .data
   msg:    .byte 'H', 'e', 'l', 'l', 'o', ',', ' '
   msg2:   .byte 'W', 'o', 'r', 'l', 'd', '!', 10

   .text
           mov r0, #1          ; fd = stdout
           ldr r1, =msg        ; first part
           mov r2, #7          ; 7 bytes
           mov r7, #3          ; syscall = WRITE
           swi                 ; write first part

           mov r0, #1          ; fd = stdout
           ldr r1, =msg2       ; second part
           mov r2, #7          ; 7 bytes
           mov r7, #3          ; syscall = WRITE
           swi                 ; write second part

           mov r0, #0          ; exit code = 0
           mov r7, #1          ; syscall = EXIT
           swi                 ; exit

Echo Program
------------

.. code-block:: asm

   ; echo.vasm - Read from stdin and echo to stdout

   .data
   buffer: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

   .text
           mov r0, #0          ; fd = stdin
           ldr r1, =buffer     ; buffer address
           mov r2, #10         ; max 10 bytes
           mov r7, #2          ; syscall = READ
           swi                 ; R0 = bytes read

           ; Exit if read failed or zero bytes
           cmp r0, #0
           ble .exit

           ; Write what we read
           mov r0, #1          ; fd = stdout
           ldr r1, =buffer     ; buffer address
           mov r2, r0          ; bytes read (from syscall)
           mov r7, #3          ; syscall = WRITE
           swi                 ; write to stdout

   .exit   mov r0, #0          ; exit code = 0
           mov r7, #1          ; syscall = EXIT
           swi

Character Counter
-----------------

.. code-block:: asm

   ; count.vasm - Count characters from stdin

   .data
   buffer: .byte 0
   count:  .word 0

   .text
           mov r4, #0          ; count = 0

   .loop   mov r0, #0          ; stdin
           ldr r1, =buffer     ; single byte buffer
           mov r2, #1          ; 1 byte
           mov r7, #2
           swi                 ; R0 = bytes read

           cmp r0, #0          ; EOF?
           ble .done

           ldrb r5, [r1]       ; get character
           cmp r5, #10         ; newline?
           beq .done

           add r4, r4, #1      ; count++
           b .loop

   .done   mov r0, r4          ; exit code = count
           mov r7, #1
           swi

Error Handling
==============

Invalid syscalls or errors result in:

- ``Unknown syscall: <number>`` printed to stderr
- Program terminates immediately

**Example of unknown syscall:**

.. code-block:: asm

   mov r7, #99         ; invalid syscall
   mov r0, #0
   swi

**Output:**

::

   Unknown syscall: 99

Valid File Descriptors
======================

+-----------+------------+-------------------+
| Value     | Name       | Description       |
+-----------+------------+-------------------+
| 0         | STDIN      | Standard input    |
| 1         | STDOUT     | Standard output   |
| 2         | STDERR     | Standard error    |
+-----------+------------+-------------------+

**Note:** Only 0, 1, and 2 are valid. Other values return -1.

Memory Address Constraints
==========================

Buffer addresses must satisfy:

- ``buffer < MEMORY_SIZE`` (1 MiB = 0x100000)
- ``buffer + count <= MEMORY_SIZE``

Violations result in R0 = -1.

**Example of invalid buffer:**

.. code-block:: asm

   mov r0, #1          ; stdout
   mov r1, #0x200000   ; invalid address
   mov r2, #10
   mov r7, #3
   swi                 ; R0 = -1 (error)
