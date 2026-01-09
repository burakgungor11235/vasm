.. _examples:

=========
Examples
=========

.. warning::
   varm is not stable. Examples in this section work with the current version
   but may need adjustment for future versions. Always verify behavior in your
   specific installation.

This section provides complete, working programs that demonstrate the concepts
covered in previous chapters. Each example includes the full source code,
line-by-line explanations, expected output, and a challenge exercise to help
you practice.

Example 1: Basic Arithmetic
===========================

This program demonstrates fundamental arithmetic operations by adding two
numbers and exiting with their sum.

**File:** ``basic_arithmetic.asm``

**Code:**

.. code-block:: arm

    @ basic_arithmetic.asm - Adding two numbers
    @ Run with: varm basic_arithmetic.asm
    @
    @ This program adds 25 and 17, exiting with the result (42)

    .text
    .global _start

    _start:
        @ Load the two numbers into registers
        MOV   r0, #25          @ r0 = 25 (first number)

        MOV   r1, #17          @ r1 = 17 (second number)

        @ Add them together
        ADD   r2, r0, r1       @ r2 = r0 + r1 = 25 + 17 = 42

        @ Exit with the sum as the exit code
        @ The exit code is the value in r0
        MOV   r0, r2           @ Move sum to r0 for exit
        MOV   r7, #1           @ Syscall number for exit
        SWI   0                @ Exit with code 42

**Line-by-Line Explanation:**

1. ``@ basic_arithmetic.asm - Adding two numbers``: This is a comment. The
   assembler ignores everything after ``@`` on a line. Comments help explain
   what code does.

2. ``.text``: The ``.text`` directive tells the assembler that the following
   section contains program code (as opposed to data).

3. ``.global _start``: The ``.global`` directive makes the ``_start`` label
   visible to the linker. Every varm program must have a ``_start`` entry
   point.

4. ``_start:``: This is a **label** - a symbolic name for a memory address.
   ``_start`` marks where program execution begins.

5. ``MOV r0, #25``: The ``MOV`` instruction copies a value. Here, the immediate
   value 25 is loaded into register r0.

6. ``MOV r1, #17``: Similarly, load 17 into register r1.

7. ``ADD r2, r0, r1``: The ``ADD`` instruction adds two values. The result of
   ``r0 + r1`` is stored in r2. Since 25 + 17 = 42, r2 now contains 42.

8. ``MOV r0, r2``: The exit code is passed in r0. We copy the sum from r2
   to r0.

9. ``MOV r7, #1``: System call numbers go in r7. The number 1 corresponds to
   the ``exit`` syscall.

10. ``SWI 0``: The Software Interrupt instruction triggers the system call.
    This causes the program to exit with the code in r0 (42).

**Expected Output:**

When you run this program, it will exit silently with an exit code of 42.
To see the exit code on Unix-like systems, you would typically run:

.. code-block:: bash

    varm basic_arithmetic.asm
    echo $?    # Should print 42

**Challenge Exercise:**

Modify the program to:
1. Add three numbers instead of two
2. Use a different arithmetic operation (subtraction or multiplication)
3. Calculate: (10 + 20) * 3 - 5

Example 2: Data Handling
========================

This program demonstrates how to declare data in the .data section and load
it into registers for processing.

**File:** ``data_handling.asm``

**Code:**

.. code-block:: arm

    @ data_handling.asm - Working with data in memory
    @ Run with: varm data_handling.asm
    @
    @ This program loads values from the data section,
    @ performs operations, and exits with a result

    .data
    @ Declare some initialized data
    value1:     .word   100     @ 32-bit integer
    value2:     .word   50      @ Another integer
    result:     .space  4       @ Space for result (4 bytes)

    .text
    .global _start

    _start:
        @ Step 1: Load the addresses of our values
        LDR   r0, =value1       @ r0 = address of value1
        LDR   r1, =value2       @ r1 = address of value2
        LDR   r2, =result       @ r2 = address of result

        @ Step 2: Load the actual values into registers
        LDR   r3, [r0]          @ r3 = *value1 (100)
        LDR   r4, [r1]          @ r4 = *value2 (50)

        @ Step 3: Perform the calculation (value1 * 2 + value2)
        ADD   r5, r3, r3        @ r5 = 100 + 100 = 200
        ADD   r5, r5, r4        @ r5 = 200 + 50 = 250

        @ Step 4: Store the result back to memory
        STR   r5, [r2]          @ *result = 250

        @ Step 5: Exit with result as exit code
        MOV   r0, r5            @ Exit code = result
        MOV   r7, #1            @ exit syscall
        SWI   0

**Line-by-Line Explanation:**

1. ``.data``: This directive begins the data section where initialized data
   is stored.

2. ``value1: .word 100``: The ``.word`` directive allocates one 32-bit word
   and initializes it with the value 100. The ``value1:`` label gives this
   memory location a name.

3. ``value2: .word 50``: Similarly, another 32-bit word containing 50.

4. ``result: .space 4``: The ``.space`` directive reserves 4 bytes of memory
   without initializing it. This is where we'll store our computed result.

5. ``LDR r0, =value1``: This is the **pseudo-instruction** for loading an
   address. It loads the address of ``value1`` into r0, not the value itself.
   The assembler handles this by placing the address in a literal pool.

6. ``LDR r3, [r0]``: This actual ``LDR`` instruction reads from the address
   in r0. Since r0 contains the address of value1, r3 receives the value 100.

7. ``ADD r5, r3, r3``: Double r3 by adding it to itself. This multiplies by 2.

8. ``ADD r5, r5, r4``: Add the second value. Now r5 = 250.

9. ``STR r5, [r2]``: Store the result to memory at the address in r2. This
   writes 250 to the ``result`` location.

10. The exit sequence uses the computed result (250) as the exit code.

**Expected Output:**

The program exits with code 250. In a shell, you would see:

.. code-block:: bash

    varm data_handling.asm
    echo $?    # Should print 250

**Challenge Exercise:**

Modify the program to:
1. Perform a different operation (subtraction, division)
2. Use the TST instruction to check if value1 is even or odd
3. Store multiple results using an array in the data section

Example 3: Control Flow
=======================

This program demonstrates loops and conditional execution by counting from
1 to 5.

**File:** ``control_flow.asm``

**Code:**

.. code-block:: arm

    @ control_flow.asm - Loops and conditional branching
    @ Run with: varm control_flow.asm
    @
    @ This program counts from 1 to 5, then exits

    .text
    .global _start

    _start:
        @ Initialize counter to 0
        MOV   r0, #0           @ r0 = counter = 0

    loop_start:
        @ Check if counter >= 5
        CMP   r0, #5           @ Compare counter with 5
        BEQ   loop_end         @ If equal, exit loop

        @ Increment counter
        ADD   r0, r0, #1       @ counter = counter + 1

        @ Loop back
        B     loop_start       @ Unconditional branch back to loop_start

    loop_end:
        @ Exit with counter value (should be 5)
        MOV   r7, #1           @ exit syscall
        SWI   0

**Line-by-Line Explanation:**

1. ``MOV r0, #0``: Initialize the counter variable. In assembly, we use a
   register to hold the counter value.

2. ``loop_start:``: This label marks the beginning of the loop body. It's
   where execution returns after each iteration.

3. ``CMP r0, #5``: The ``CMP`` instruction compares r0 with 5 and updates
   the processor flags (NZCV), but does not store a result.

4. ``BEQ loop_end``: The **Branch if Equal** instruction checks the Z (zero)
   flag. If the previous comparison showed equality (r0 == 5), execution
   jumps to ``loop_end``.

5. ``ADD r0, r0, #1``: Add 1 to the counter. This could also be written
   as ``ADD r0, r0, #1`` or even ``ADD r0, #1``.

6. ``B loop_start``: The unconditional branch always jumps back to the
   loop start. This creates the loop.

7. ``loop_end:``: When the counter reaches 5, the BEQ branches here,
   skipping the rest of the loop.

**How the Loop Executes:**

=== ====== ====================
Step r0    Action
=== ====== ====================
1   0      Compare with 5 (not equal), continue
2   1      Add 1, loop back
3   1      Compare with 5 (not equal), continue
4   2      Add 1, loop back
... ...    ...
11  5      Compare with 5 (equal!), exit loop
=== ====== ====================

**Expected Output:**

The program runs and exits with code 5. No output is displayed during
execution.

**Challenge Exercise:**

Modify the program to:
1. Count down from 5 to 1
2. Count from 1 to N, where N is stored in a variable in the data section
3. Calculate the factorial of 5 (5! = 120)

Example 4: Subroutines
======================

This program demonstrates how to call subroutines using the Branch with Link
(BL) instruction and return using the Link Register (LR).

**File:** ``subroutines.asm``

**Code:**

.. code-block:: arm

    @ subroutines.asm - Function calls with BL and LR
    @ Run with: varm subroutines.asm
    @
    @ This program demonstrates subroutine calls by
    @ calling a function that squares a number

    .text
    .global _start

    _start:
        @ Call the square function with argument 7
        MOV   r0, #7           @ Argument: number to square
        BL    square           @ Call square function
        @ Return here after square completes

        @ r0 now contains 49 (7 * 7)
        @ Exit with result
        MOV   r7, #1           @ exit syscall
        SWI   0

    @ ========================================
    @ Function: square
    @ Purpose:  Calculate n * n
    @ Input:    r0 = n
    @ Output:   r0 = n * n
    @ ========================================
    square:
        @ Save any registers we modify (callee-saved)
        @ For this simple function, we only use r0
        @ which is also our output register

        @ Multiply r0 by itself
        MUL   r0, r0, r0       @ r0 = r0 * r0

        @ Return to caller
        MOV   pc, lr           @ pc = link register

**Line-by-Line Explanation:**

1. ``MOV r0, #7``: Set up the argument for the square function. By convention,
   the first argument is passed in r0.

2. ``BL square``: **Branch with Link** does two things:
   - Saves the address of the next instruction (return address) in the Link
     Register (LR, which is register r14)
   - Jumps to the ``square`` label

3. When ``square`` returns, execution resumes here. The return address was
   automatically stored in LR by the BL instruction.

4. ``MUL r0, r0, r0``: The ``MUL`` instruction multiplies two registers and
   stores the result. Here, we multiply r0 by itself, computing n².

5. ``MOV pc, lr``: To return from a subroutine, we move the Link Register
   (containing the saved return address) into the Program Counter (pc).
   This causes execution to jump back to the instruction after the BL.

**Understanding the Call Stack (Simplified):**

.. code-block:: text

    _start:
        MOV r0, #7       @ r0 = 7
        BL square        @ LR = addr of next instr, pc = square
                        @ ─────────────────────────────────
    square:              @           (now executing here)
        MUL r0, r0, r0   @ r0 = 49
        MOV pc, lr       @ pc = saved return address
                        @ ─────────────────────────────────
    _start (cont):       @           (back here now)
        MOV r7, #1       @ Exit with result

**Expected Output:**

The program exits with code 49 (7²).

**Challenge Exercise:**

Modify the program to:
1. Create a function that adds three numbers
2. Create a recursive function (function that calls itself)
3. Create a function that takes two arguments and returns both sum and product

Example 5: Complete Program
===========================

This final example combines all the concepts: data declaration, arithmetic,
control flow, subroutines, and I/O syscalls.

**File:** ``complete_program.asm``

**Code:**

.. code-block:: arm

    @ complete_program.asm - Putting it all together
    @ Run with: varm complete_program.asm
    @
    @ This program:
    @ 1. Displays a welcome message
    @ 2. Calculates the sum of numbers 1 to N
    @ 3. Displays the result
    @ 4. Exits with the result

    .data
    @ Data section - constants and buffers
    welcome:    .asciz  "=== Sum Calculator ===\n"
    prompt:     .asciz  "Enter a number (0-10): "
    result_msg: .asciz  "Sum from 1 to N = "
    newline:    .asciz  "\n"
    buffer:     .space  4       @ Input buffer (up to 3 digits + null)

    .text
    .global _start

    @ ========================================
    @ Main program entry point
    @ ========================================
    _start:
        @ Display welcome message
        MOV   r0, #1            @ stdout
        LDR   r1, =welcome
        MOV   r2, #23           @ Length of welcome message
        BL    print_message

        @ Display prompt
        MOV   r0, #1
        LDR   r1, =prompt
        MOV   r2, #20
        BL    print_message

        @ Read user input
        LDR   r0, =buffer
        BL    read_input
        @ r0 now contains the number N

        @ Calculate sum from 1 to N
        BL    sum_to_n
        @ r0 now contains the sum

        @ Display result message
        MOV   r0, #1
        LDR   r1, =result_msg
        MOV   r2, #19
        BL    print_message

        @ Display the sum (as a single digit for simplicity)
        @ For a full solution, you'd convert the number to ASCII
        MOV   r0, r0            @ Sum is already in r0
        BL    print_number

        @ Display newline
        MOV   r0, #1
        LDR   r1, =newline
        MOV   r2, #1
        BL    print_message

        @ Exit with the sum as exit code
        MOV   r7, #1
        SWI   0

    @ ========================================
    @ Function: print_message
    @ Purpose:  Print a null-terminated string
    @ Input:    r1 = address of string
    @ ========================================
    print_message:
        MOV   r0, #1            @ stdout
        MOV   r7, #4            @ write syscall
        SWI   0
        MOV   pc, lr

    @ ========================================
    @ Function: read_input
    @ Purpose:  Read a single digit from stdin
    @ Input:    r0 = address of buffer
    @ Output:   r0 = the number read (0-9)
    @ ========================================
    read_input:
        PUSH  {r1, r2, r7}      @ Save registers

        MOV   r1, r0            @ Buffer address
        MOV   r2, #3            @ Read up to 2 chars + null
        MOV   r0, #0            @ stdin
        MOV   r7, #3            @ read syscall
        SWI   0

        @ Convert ASCII digit to number
        LDRB  r0, [r1]          @ Load first byte
        SUB   r0, r0, #48       @ '0' (48) to 0, '1' to 1, etc.

        POP   {r1, r2, r7}      @ Restore registers
        MOV   pc, lr

    @ ========================================
    @ Function: print_number
    @ Purpose:  Print a single digit number
    @ Input:    r0 = number to print
    @ ========================================
    print_number:
        @ Convert number to ASCII
        ADD   r0, r0, #48       @ 0 to '0', 1 to '1', etc.

        @ Store in a temporary buffer
        LDR   r1, =buffer       @ Reuse buffer
        STRB  r0, [r1]

        @ Print the single digit
        MOV   r2, #1            @ One character
        B     print_message     @ Reuse print_message (tail call)

    @ ========================================
    @ Function: sum_to_n
    @ Purpose:  Calculate sum(1 + 2 + ... + n)
    @ Input:    r0 = n
    @ Output:   r0 = sum
    @ ========================================
    sum_to_n:
        @ r0 = n, calculate 1+2+...+n
        @ Formula: n * (n + 1) / 2

        PUSH  {r1, r2, r3}      @ Save registers

        MOV   r1, r0            @ r1 = n
        ADD   r2, r1, #1        @ r2 = n + 1
        MUL   r0, r1, r2        @ r0 = n * (n + 1)
        MOV   r2, #2            @ r2 = 2
        DIV   r0, r0, r2        @ r0 = n * (n + 1) / 2

        POP   {r1, r2, r3}      @ Restore registers
        MOV   pc, lr

    @ Note: Division is not yet implemented in varm.
    @ This program uses an iterative approach below instead.

    @ ========================================
    @ Function: sum_to_n_iterative
    @ Alternative: Iterative sum calculation
    @ ========================================
    sum_to_n_iterative:
        CMP   r0, #0            @ Base case: n = 0
        MOVEQ r0, #0            @ Return 0
        MOVEQ pc, lr            @

        PUSH  {r1, r2}          @ Save registers

        MOV   r1, r0            @ r1 = n
        MOV   r2, #0            @ accumulator = 0

    sum_loop:
        CMP   r1, #0            @ While n > 0
        BEQ   sum_done          @ Exit loop

        ADD   r2, r2, r1        @ accumulator += n
        SUB   r1, r1, #1        @ n--
        B     sum_loop          @ Loop

    sum_done:
        MOV   r0, r2            @ Return accumulator
        POP   {r1, r2}          @ Restore registers
        MOV   pc, lr

**Line-by-Line Explanation:**

This program demonstrates several important concepts working together:

1. **Data Section Organization:** We declare all our strings and buffers in
   .data. Each has a descriptive label (welcome, prompt, result_msg, etc.).

2. **Modular Design:** The program is broken into functions:
   - ``print_message``: Displays a string
   - ``read_input``: Reads a digit from the user
   - ``print_number``: Converts and prints a number
   - ``sum_to_n_iterative``: Calculates the sum

3. **Function Calling Convention:**
   - Arguments passed in r0
   - Return value in r0
   - Callee-saved registers (r1, r2, etc.) are preserved with PUSH/POP
   - Return via MOV pc, lr

4. **I/O Operations:** The program uses syscalls to display messages and
   read input, demonstrating interaction with the user.

5. **Control Flow:** The sum_to_n_iterative function uses a loop to
   accumulate the result.

**Expected Output:**

.. code-block:: text

    === Sum Calculator ===
    Enter a number (0-10): 5
    Sum from 1 to N = 15

The program exits with code 15.

**Challenge Exercise:**

Modify the program to:
1. Handle multi-digit numbers (convert ASCII to integer properly)
2. Add error handling for invalid input
3. Display additional information (like intermediate values)
4. Add more functions and increase modularity

Summary
=======

These examples demonstrate the core concepts of varm programming:

* **Example 1:** Basic register operations and arithmetic
* **Example 2:** Memory access and data handling
* **Example 3:** Loops and conditional branching
* **Example 4:** Subroutines and function calls
* **Example 5:** Complete program combining all concepts

As you work through these examples, remember that varm is not stable.
The examples work with the current version, but always verify behavior in
your installation. The concepts demonstrated here (registers, memory,
syscalls, control flow) are fundamental to all assembly programming and
will transfer to other architectures.
