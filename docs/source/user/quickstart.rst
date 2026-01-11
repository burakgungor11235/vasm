Quick Start
===========

.. warning::

   **varm is under active development.** This quick start guide reflects
   the current state of the code and may be incomplete.

This guide will get you up and running with varm in about 5 minutes.
We'll write a simple program, assemble it, and run it on the varm VM.

Prerequisites
-------------

Before you begin, ensure you have:

* A working varm installation (see :doc:`installation`)
* A text editor (nano, vim, VS Code, etc.)

Your First varm Program
-----------------------

Create a file called ``hello.vasm`` with the following content:

.. code-block:: vasm

   ; Exit with code 42
   mov r0, #42      ; Load immediate value 42 into register r0
   mov r7, #1       ; Syscall number 1 = exit
   swi              ; Make system call

Let's break this down:

* ``mov r0, #42`` - Move the value 42 into register r0
* ``mov r7, #1``  - Move the syscall number 1 (exit) into r7
* ``swi`` - Trigger a system call (in varm, this invokes the syscall)

Assembling the Program
----------------------

Use the assembler (``vasm``) to convert your assembly code to a ``.vm`` file:

.. code-block:: bash

   ./build/src/vasm -o hello.vm hello.vasm

If successful, you'll see:

.. code-block:: text

   Assembled: 12 bytes of code, 0 bytes of data
   Labels: 3

Running the Program
-------------------

Use the virtual machine (``varm``) to execute the program:

.. code-block:: bash

   ./build/src/varm hello.vm

The program should exit with code 42:

.. code-block:: bash

   echo $?
   42

A Slightly More Complex Program
-------------------------------

Let's try something that does actual computation. Create ``add.vasm``:

.. code-block:: vasm

   ; Compute (42 + 58) * 2 = 200
   mov r0, #42      ; r0 = 42
   mov r1, #58      ; r1 = 58
   add r0, r0, r1   ; r0 = r0 + r1 = 100
   mov r1, #2       ; r1 = 2
   mul r2, r0, r1   ; r2 = r0 * r1 = 200

   ; Exit with result in r0
   mov r0, r2       ; Move result to r0
   mov r7, #1       ; Syscall: exit
   swi              ; Exit with code in r0

Assemble and run:

.. code-block:: bash

   ./build/src/vasm -o add.vm add.vasm
   ./build/src/varm add.vm
   echo $?
   200

Using Data Sections
-------------------

You can also define data in your programs. Create ``data.vasm``:

.. code-block:: vasm

   ; Define a constant in the data section
   .data
   value:  .word 12345

   ; Use the constant
   .text
   ldr r0, =value    ; Load address of 'value' into r0
   ldr r1, [r0]      ; Load the value 12345 into r1

   ; Exit with the value as exit code
   mov r0, r1
   mov r7, #1
   swi

Assemble and run:

.. code-block:: bash

   ./build/src/vasm -o data.vm data.vasm
   ./build/src/varm data.vm
   echo $?
   12345

Debugging with Verbose Output
-----------------------------

Both varm and varm support debug output:

.. code-block:: bash

   # Verbose assembly
   ./build/src/vasm -d -o hello.vm hello.vasm

   # Verbose VM execution
   ./build/src/varm -v 5 hello.vm

Common Errors
-------------

### "Unknown instruction"

Make sure your instruction names are correct:

.. code-block:: vasm

   ; Correct: lowercase instruction names
   mov r0, #42
   add r1, r2, r3

### "Invalid register"

Register names must be lowercase:

.. code-block:: vasm

   ; Correct
   mov r0, #42

   ; Incorrect (will fail)
   MOV R0, #42

### "Assembly failed"

Check for syntax errors. Make sure you have commas in the right places.

Next Steps
----------

Now that you've run your first varm programs:

1. Read the :doc:`tutorial/index` for a comprehensive introduction
2. Look at the :doc:`reference/index` for complete instruction reference
3. Try the :doc:`exercises/index` to practice your skills

For more information:

* See :doc:`installation` for advanced build options
* See :doc:`reference/syscalls` for available system calls
* See :doc:`reference/file-format` for the .varm file specification
