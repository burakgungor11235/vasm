==========
Exercises
==========

.. warning::
   varm is not a stable project. The exercises in this section work with the
   current version but may need adjustment for future versions. Always verify
   behavior in your specific installation.

This section provides hands-on programming exercises for varm. After completing
the tutorial, these exercises will help you practice and reinforce the concepts
you've learned.

Prerequisites
-------------

Before attempting these exercises, you should have:

1. Completed the :doc:`../tutorial/index`
2. Built varm successfully (``./qol.sh build``)
3. Understanding of basic assembly concepts (registers, memory, instructions)

How to Use These Exercises
---------------------------

Each exercise includes:

- **Learning Objectives**: What concepts you'll practice
- **Time Estimate**: How long the exercise should take
- **Starter Code**: A template to build upon
- **Task Description**: What the program should do
- **Expected Output**: What correct output looks like
- **Challenge/Extension**: Optional harder problems
- **Solution**: A complete working implementation (hidden by default)

To work through an exercise:

1. Read the learning objectives and task description
2. Study the starter code and understand what needs to be completed
3. Write your solution using the starter code as a base
4. Assemble and run your program
5. Compare your output to the expected output
6. Check the solution if you get stuck

Running Your Programs
---------------------

.. code-block:: bash

   # Assemble and run
   ./qol.sh asmrun program.vasm

   # Assemble separately
   ./qol.sh asm program.vasm -o program.varm

   # Run assembled program
   ./qol.sh run program.varm

   # Debug mode (shows instructions and syscalls)
   ./qol.sh run -d INSTR -d SYSCALL program.varm

Exercise Overview
-----------------

+----------+---------------------------+--------+
| Exercise | Topic                     | Level  |
+==========+===========================+========+
| 1        | Basic Operations          | Easy   |
+----------+---------------------------+--------+
| 2        | Control Flow              | Medium |
+----------+---------------------------+--------+
| 3        | Data & I/O                | Med.-Hard |
+----------+---------------------------+--------+

Start with Exercise 1 and work your way up. Each exercise builds on concepts
from previous ones.

.. toctree::
   :maxdepth: 1
   :caption: Exercises

   exercise-1
   exercise-2
   exercise-3
