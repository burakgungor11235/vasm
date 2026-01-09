Exercise 3: Data and I/O Operations
====================================

Learning Objectives
-------------------
- Declare and use data in the .data section
- Use LDR and STR instructions for memory access
- Use the =label pseudo-instruction for loading addresses
- Perform syscalls for READ and WRITE operations
- Manipulate string data (case conversion)
- Understand memory addressing and buffers

Time Estimate
-------------
25-35 minutes

Starter Code
------------

.. code-block:: vasm

   ; case_convert.vasm - Read input and convert case
   ; Task: Read a string, convert lowercase to uppercase and vice versa

   .data
   prompt:  .byte 'E', 'n', 't', 'e', 'r', ' ', 't', 'e', 'x', 't', ':', ' ', 10
   result_msg: .byte 'C', 'o', 'n', 'v', 'e', 'r', 't', 'e', 'd', ':', ' ', 10
   newline: .byte 10
   buffer: .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  ; 16-byte buffer

   .text
   ; This program:
   ; 1. Displays a prompt
   ; 2. Reads user input (up to 15 characters)
   ; 3. Converts lowercase to uppercase and vice versa
   ; 4. Displays the converted result
   ;
   ; ASCII values:
   ; 'A'-'Z' = 65-90
   ; 'a'-'z' = 97-122
   ; Difference between uppercase and lowercase = 32

   ; TODO: Complete this program

   ; Display prompt
   mov r0, #1              ; stdout
   ldr r1, =prompt         ; address of prompt
   mov r2, #12             ; length of prompt
   mov r7, #3              ; syscall = WRITE
   swi

   ; Read input
   mov r0, #0              ; stdin
   ldr r1, =buffer         ; buffer address
   mov r2, #15             ; max 15 bytes (leave room for null)
   mov r7, #2              ; syscall = READ
   swi

   ; r0 now contains bytes read (including newline if present)
   ; Save the count for later
   ; mov r4, r0             ; r4 = byte count

   ; Process each character in the buffer
   ; Convert lowercase to uppercase (subtract 32)
   ; Convert uppercase to lowercase (add 32)

   ; r1 = buffer address
   ; r2 = loop counter (0 to byte_count-1)
   ; r3 = current character

   ; Loop through buffer:
   ; ldrb r3, [r1, r2]      ; load byte at buffer[r2]
   ; cmp r3, #'a'           ; if c >= 'a'
   ; blt .not_lower         ;   skip
   ; cmp r3, #'z'           ; if c <= 'z'
   ; bgt .not_lower         ;   skip
   ; sub r3, r3, #32        ;   convert to uppercase
   ; strb r3, [r1, r2]      ;   store back
   ; b .next_char           ; continue

   ; .not_lower:
   ; cmp r3, #'A'           ; if c >= 'A'
   ; blt .next_char         ;   skip
   ; cmp r3, #'Z'           ; if c <= 'Z'
   ; bgt .next_char         ;   skip
   ; add r3, r3, #32        ;   convert to lowercase
   ; strb r3, [r1, r2]      ;   store back

   ; .next_char:
   ; add r2, r2, #1         ; increment counter
   ; cmp r2, r4             ; compare with byte count
   ; blt loop               ; if counter < count, continue

   ; Display result message
   mov r0, #1              ; stdout
   ldr r1, =result_msg     ; address of result message
   mov r2, #11             ; length
   mov r7, #3              ; syscall = WRITE
   swi

   ; Display converted text
   mov r0, #1              ; stdout
   ldr r1, =buffer         ; buffer address
   mov r2, r4              ; byte count (from READ)
   mov r7, #3              ; syscall = WRITE
   swi

   ; Exit
   mov r0, #0              ; exit code
   mov r7, #1
   swi

Task Description
----------------
In this exercise, you will create a program that reads user input, manipulates
the data, and displays the result.

The program should:
1. Display a prompt asking for input
2. Read up to 15 characters from stdin
3. Convert each character:
   - Lowercase letters (a-z) become uppercase (A-Z) by subtracting 32
   - Uppercase letters (A-Z) become lowercase (a-z) by adding 32
   - Non-letter characters remain unchanged
4. Display "Converted: " followed by the modified text

ASCII Character Codes:
- 'A' = 65, 'Z' = 90
- 'a' = 97, 'z' = 122
- Difference between cases = 32

Expected Output
---------------
Example interaction:

.. code-block:: text

   Enter text: Hello World
   Converted: hELLO wORLD

The program exits with code 0.

Challenge/Extension (Optional)
------------------------------
For extra practice, try these variations:

1. **Count characters**: After conversion, display the character count
2. **Handle null-terminated strings**: Properly handle null-terminated input
3. **Multiple lines**: Process until EOF instead of fixed length
4. **ROT13 cipher**: Implement ROT13 encoding instead of case conversion
   (A->N, B->O, ..., M->Z, N->A, ..., Z->M)

Solution
--------

One possible solution:

.. code-block:: vasm

   ; case_convert.vasm - Complete solution
   ; Read input, convert case, display result

   .data
   prompt:     .byte 'E', 'n', 't', 'e', 'r', ' ', 't', 'e', 'x', 't', ':', ' ', 10
   result_msg: .byte 'C', 'o', 'n', 'v', 'e', 'r', 't', 'e', 'd', ':', ' ', 10
   buffer:     .byte 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  ; 16 bytes

   .text
           ; Display prompt
           mov r0, #1              ; stdout
           ldr r1, =prompt         ; prompt address
           mov r2, #12             ; 12 bytes
           mov r7, #3              ; WRITE syscall
           swi

           ; Read input
           mov r0, #0              ; stdin
           ldr r1, =buffer         ; buffer address
           mov r2, #15             ; max 15 bytes
           mov r7, #2              ; READ syscall
           swi

           ; r0 = bytes read (including newline)
           mov r4, r0              ; save count in r4
           mov r2, #0              ; counter = 0

   loop:
           cmp r2, r4              ; if counter >= count
           bge done                ;   exit loop

           ldrb r3, [r1, r2]       ; r3 = buffer[counter]

           ; Check if lowercase (a-z)
           cmp r3, #'a'            ; if c < 'a'
           blt .not_lower          ;   skip
           cmp r3, #'z'            ; if c > 'z'
           bgt .not_lower          ;   skip
           sub r3, r3, #32         ;   convert to uppercase
           strb r3, [r1, r2]       ;   store back
           b .next_char            ;   continue

   .not_lower:
           ; Check if uppercase (A-Z)
           cmp r3, #'A'            ; if c < 'A'
           blt .next_char          ;   skip
           cmp r3, #'Z'            ; if c > 'Z'
           bgt .next_char          ;   skip
           add r3, r3, #32         ;   convert to lowercase
           strb r3, [r1, r2]       ;   store back

   .next_char:
           add r2, r2, #1          ; counter++
           b loop

   done:
           ; Display result message
           mov r0, #1              ; stdout
           ldr r1, =result_msg     ; message address
           mov r2, #11             ; 11 bytes
           mov r7, #3              ; WRITE
           swi

           ; Display converted text
           mov r0, #1              ; stdout
           ldr r1, =buffer         ; buffer address
           mov r2, r4              ; byte count
           mov r7, #3              ; WRITE
           swi

           ; Exit
           mov r0, #0              ; exit code
           mov r7, #1              ; EXIT
           swi

Notes
-----
- ``.byte 'H', 'e', 'l', 'l', 'o'`` declares a string as individual bytes
- ``ldr r1, =buffer`` loads the address of buffer (pseudo-instruction)
- ``ldrb rd, [rn, #offset]`` loads a single byte, zero-extended
- ``strb rd, [rn, #offset]`` stores a single byte
- ``mov r2, #0`` initializes the loop counter
- The READ syscall returns the number of bytes read in r0
- Character literals like #'a' are converted to their ASCII values at assembly time
