Exercise 2: Control Flow
========================

Learning Objectives
-------------------
- Use CMP instruction for comparing values
- Apply conditional branch instructions (BEQ, BNE, etc.)
- Implement loops using branch instructions
- Understand the CPSR flags (Z, N, C, V) and how they affect branching

Time Estimate
-------------
20-30 minutes

Task Description
----------------
In this exercise, you will implement a loop that counts to 5.

Your program should:
1. Initialize a counter to 0
2. Increment the counter until it reaches 5
3. Exit with the final count (5)

Working Solution
----------------

.. code-with-test:: varm
   :expected_exit: 5

   mov r0, #0          ; counter = 0
   mov r1, #5          ; target = 5

   loop:
   cmp r0, r1          ; compare counter with target
   bge loop_end        ; if counter >= target, exit loop
   add r0, r0, #1      ; counter++
   b loop              ; continue loop

   loop_end:
   mov r7, #1          ; syscall = EXIT
   swi                 ; exit with counter value

Explanation
-----------
- ``cmp rn, rm`` compares rn with rm, setting CPSR flags
- ``bge label`` branches if rn >= rm (Greater or Equal)
- ``add rd, rn, #imm`` adds immediate value to rn, storing in rd
- The loop increments the counter until it reaches the target

Notes
-----
- ``ble`` (Branch if Less or Equal) continues while counter <= target
- ``b`` (unconditional branch) repeats the loop
- The CPSR flags are set by CMP: Z=1 if equal, N=1 if negative, etc.
