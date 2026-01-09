Syscall Interface
=================

.. warning::
   **varm is not stable.** This implementation may change without notice.

1. System Call Overview
-----------------------

System calls (syscalls) provide the interface between varm programs and
the host operating system. They allow programs to perform I/O operations,
allocate memory, and terminate execution.

The ``SWI`` (Software Interrupt) instruction triggers a syscall:

::

   SWI <syscall_number>

On ARM platforms, r7 conventionally holds the syscall number, and r0-r2
hold arguments. varm follows this convention.

2. Syscall Dispatch Mechanism
-----------------------------

The syscall flow is:

::

   1. Execute SWI instruction
   2. Call syscall_handler()
   3. Read r7 for syscall number
   4. Switch on syscall number
   5. Execute syscall
   6. Return result in r0

SWI instruction handling:

.. code-block:: c

   // src/vm/instruction.c:395-400
   void
   exec_swi(vm_state_t* vm, u32 instr)
   {
       (void)instr;
       syscall_handler(vm);
   }

The instruction operand is ignored; only r7 matters for syscall number.

Syscall handler implementation:

.. code-block:: c

   // src/vm/instruction.c:344-393
   static void
   syscall_handler(vm_state_t* vm)
   {
       u32 syscall = vm->regs.r[7];
       u32 r0 = vm->regs.r[0];
       u32 r1 = vm->regs.r[1];
       u32 r2 = vm->regs.r[2];
       u32 result = 0;

       switch (syscall) {
       case SYSCALL_EXIT:
           vm->exit_code = r0 & BYTE_MASK;
           vm->running = 0;
           break;

       case SYSCALL_READ:
           // ... handle read
           break;

       case SYSCALL_WRITE:
           // ... handle write
           break;

       default:
           printf("Unknown syscall: %u\n", syscall);
           vm->running = 0;
           break;
       }
   }

3. Available Syscalls
---------------------

+----------+------------------+----------------------------------------+
| Number   | Name             | Description                            |
+----------+------------------+----------------------------------------+
| 1        | EXIT             | Terminate execution                    |
| 2        | READ             | Read from file descriptor              |
| 3        | WRITE            | Write to file descriptor               |
+----------+------------------+----------------------------------------+

EXIT (syscall 1)
~~~~~~~~~~~~~~~~

Terminates the VM with an exit code.

Arguments:
   - r0: Exit code (8-bit value)

Return value: None

Implementation:

.. code-block:: c

   case SYSCALL_EXIT:
       vm->exit_code = r0 & BYTE_MASK;
       vm->running = 0;
       break;

READ (syscall 2)
~~~~~~~~~~~~~~~~

Reads data from a file descriptor.

Arguments:
   - r0: File descriptor (0 = stdin)
   - r1: Buffer address in VM memory
   - r2: Maximum bytes to read

Return value:
   - r0: Number of bytes read, or -1 on error

Implementation:

.. code-block:: c

   case SYSCALL_READ:
       if (r0 == 0) {
           if (syscall_validate_buffer(vm, r1, r2) != 0) {
               vm->regs.r[0] = (u32)-1;
           } else {
               result = read(STDIN_FILENO,
                            (void*)(vm->mem.memory + r1), r2);
               vm->regs.r[0] = result;
           }
       } else {
           vm->regs.r[0] = (u32)-1;
       }
       break;

WRITE (syscall 3)
~~~~~~~~~~~~~~~~~

Writes data to a file descriptor.

Arguments:
   - r0: File descriptor (1 = stdout, 2 = stderr)
   - r1: Buffer address in VM memory
   - r2: Number of bytes to write

Return value:
   - r0: Number of bytes written, or -1 on error

Implementation:

.. code-block:: c

   case SYSCALL_WRITE:
       if (r0 == 1 || r0 == 2) {
           if (syscall_validate_buffer(vm, r1, r2) != 0) {
               vm->regs.r[0] = (u32)-1;
           } else {
               result = write(STDOUT_FILENO,
                             (void*)(vm->mem.memory + r1), r2);
               vm->regs.r[0] = result;
               fflush(stdout);
           }
       } else {
           vm->regs.r[0] = (u32)-1;
       }
       break;

4. Parameter Passing
--------------------

varm follows the ARM syscall convention:

::

   r0: First argument / return value
   r1: Second argument
   r2: Third argument
   r7: Syscall number

For multi-argument syscalls, additional arguments use r3-r6 (not currently
used in varm).

Register preservation:
   - r0 may be clobbered (return value)
   - r1-r2 may be clobbered during syscall
   - r7 is clobbered (syscall number consumed)

5. Error Handling
-----------------

Syscalls use negative return values to indicate errors:

::

   On success:  r0 >= 0 (byte count)
   On error:    r0 == -1 (0xFFFFFFFF)

Buffer validation
~~~~~~~~~~~~~~~~~

Before any I/O operation, the syscall validates the buffer:

.. code-block:: c

   // src/vm/instruction.c:329-342
   static int
   syscall_validate_buffer(vm_state_t* vm, u32 addr, u32 size)
   {
       if (size == 0) {
           return 0;
       }
       if (addr >= MEMORY_SIZE) {
           return -1;
       }
       if (addr + size > MEMORY_SIZE) {
           return -1;
       }
       return 0;
   }

This prevents:
- Reading/writing outside VM memory
- Integer overflow in address calculations

6. Implementation Details
-------------------------

The syscall interface is implemented in ``src/vm/instruction.c``:

+------------------+--------------------------------------------+
| Component        | Location                                   |
+------------------+--------------------------------------------+
| SWI executor     | ``instruction.c:395-400`` (``exec_swi``)   |
| Syscall handler  | ``instruction.c:344-393`` (``syscall_handler``) |
| Buffer validation| ``instruction.c:329-342`` (``syscall_validate_buffer``) |
| Definitions      | ``include/opcode.h`` (``SYSCALL_*``)       |
+------------------+--------------------------------------------+

7. Complexity Analysis
----------------------

+-------------------+------------+----------------------------------+
| Operation         | Complexity | Notes                            |
+-------------------+------------+----------------------------------+
| Dispatch          | O(1)       | Switch on r7                     |
| Validation        | O(1)       | Bounds check                     |
| EXIT              | O(1)       | Set flag                         |
| READ              | O(n)       | Proportional to bytes read       |
| WRITE             | O(n)       | Proportional to bytes written    |
+-------------------+------------+----------------------------------+

The actual I/O complexity is dominated by the host OS syscall performance.
varm adds O(1) overhead for dispatch and validation.

See Also
--------

- :doc:`../architecture/memory-subsystem` - VM memory layout
- ``src/vm/instruction.c`` - Full syscall implementation
- ``docs/user/reference/syscalls.rst`` - User syscall documentation
- ``include/vm.h`` - Syscall number definitions
