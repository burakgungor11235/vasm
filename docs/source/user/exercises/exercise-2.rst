Exercise 2: Control Flow
========================

Learning Objectives
-------------------
- Use CMP instruction for comparing values
- Apply conditional branch instructions (BEQ, BNE, etc.)
- Implement loops using branch instructions
- Understand the CPSR flags (Z, N, C, V) and how they affect branching
- Implement recursive functions using BL and LR

Time Estimate
-------------
20-30 minutes

Starter Code
------------

.. code-block:: vasm

   ; factorial.vasm - Calculate factorial
   ; Task: Compute 5! (factorial of 5) = 120

   .data
   ; No data needed for this exercise

   .text
   ; Factorial definition:
   ; 0! = 1
   ; n! = n * (n-1)! for n > 0
   ;
   ; Iterative approach:
   ; result = 1
   ; for i = 2 to n:
   ;     result = result * i
   ;
   ; 5! = 1 * 2 * 3 * 4 * 5 = 120

   ; TODO: Complete this program
   mov r0, #5          ; n = 5

   ; Initialize result to 1
   ; mov r1, #1         ; result = 1

   ; Initialize counter to 2
   ; mov r2, #2         ; i = 2

   ; Loop body:
   ; mul r3, r1, r2     ; result = result * i
   ; mov r1, r3         ; update result
   ; add r2, r2, #1     ; i++

   ; Check if i <= n, continue loop if true
   ; cmp r2, r0
   ; ble [loop body address]

   ; Exit loop when i > n
   ; mov r0, r1         ; exit with result
   ; mov r7, #1
   ; swi

Task Description
----------------
In this exercise, you will implement a factorial calculator using iteration.

The factorial of n (written n!) is the product of all positive integers
less than or equal to n:

- 0! = 1
- 1! = 1
- 5! = 1 * 2 * 3 * 4 * 5 = 120
- 10! = 3,628,800

Your program should:
1. Set n = 5
2. Initialize result = 1
3. Loop from i = 2 to n, multiplying result by i each iteration
4. Exit with the final result

The loop uses these conditional branches:
- ``ble`` (Branch if Less or Equal) to continue while i <= n
- ``b`` (unconditional branch) to repeat the loop

Expected Output
---------------
The program will exit silently with code 120 (5!):

.. code-block:: bash

   ./qol.sh asmrun factorial.vasm
   echo $?    ; Should print 120

Challenge/Extension (Optional)
------------------------------
For extra practice, try these variations:

1. **Calculate a different factorial**: Modify to compute 8! = 40320
2. **Use recursion instead of iteration**: Implement factorial using recursive
   function calls with BL and LR
3. **Calculate Fibonacci**: Compute the 10th Fibonacci number (F10 = 55)
   where F0=0, F1=1, Fn = Fn-1 + Fn-2
4. **Add overflow check**: Track if the result exceeds 255 and handle it

Solution
--------

One possible solution (iterative approach):

.. code-block:: vasm

   ; factorial.vasm - Complete solution
   ; Compute 5! using iterative approach

   .data
   ; No data needed

   .text
           mov r0, #5          ; n = 5
           mov r1, #1          ; result = 1
           mov r2, #2          ; i = 2

   loop:
           mul r3, r1, r2      ; r3 = result * i
           mov r1, r3          ; result = r3
           add r2, r2, #1      ; i++
           cmp r2, r0          ; compare i with n
           ble loop            ; if i <= n, continue loop
           ; else fall through to exit

   loop_end:
           mov r0, r1          ; exit code = result (120)
           mov r7, #1          ; syscall = EXIT
           swi                 ; exit

Alternative recursive solution (for reference):

.. code-block:: vasm

   ; Recursive factorial
   .text
           mov r0, #5          ; n = 5
           bl factorial        ; call factorial function
           mov r7, #1          ; exit
           swi

   factorial:
           cmp r0, #1          ; if n <= 1
           ble .base           ;   return 1
           push {lr}           ; save return address
           sub r1, r0, #1      ; r1 = n - 1
           bl factorial        ; recursive call
           mul r0, r0, r1      ; result = n * (n-1)!
           pop {pc}            ; return

   .base   mov r0, #1          ; return 1
           bx lr

Notes
-----
- ``cmp rn, operand2`` compares rn with operand2, setting CPSR flags
- ``ble label`` branches if Z=1 or N!=V (rn <= operand2)
- ``mul rd, rm, rs`` multiplies rm and rs, storing in rd
- ``push {lr}`` saves the return address on the stack
- ``pop {pc}`` restores PC from stack, returning from function
