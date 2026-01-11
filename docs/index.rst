==================
vasm Documentation
==================

vasm is an **educational ARM-like virtual machine** designed for learning assembly language programming. Build and run simple assembly programs to understand how computers execute code at the lowest level.

.. code-block:: vasm

   mov r0, #42
   mov r7, #1
   swi

Quick Links
===========

* **Install** - Build from source in 2 minutes: :doc:`user/installation`
* **Start** - Your first program: :doc:`user/quickstart`
* **Learn** - Assembly concepts: :doc:`user/tutorial/index`
* **Reference** - Instructions & syscalls: :doc:`user/reference/index`
* **Practice** - Exercises: :doc:`user/exercises/index`

What is vasm?
=============

vasm provides a minimal ARM-like instruction set that:

* Uses 32-bit little-endian instructions
* Has 16 registers (r0-r15, plus sp, lr, pc aliases)
* Supports basic arithmetic, memory, and branching
* Includes a simple syscall interface for I/O

Perfect for students, educators, and anyone wanting to **learn assembly** without the complexity of real hardware or full-featured architectures.

Requirements
============

* C compiler (GCC, Clang, or MSVC)
* Python 3.9+
* Meson & Ninja build system
* Git (optional)

See :doc:`user/installation` for setup instructions.

.. note::

   vasm is experimental. APIs and instruction sets may change.

.. toctree::
   :maxdepth: 2
   :caption: Getting Started
   :hidden:

   user/installation
   user/quickstart
   user/examples

.. toctree::
   :maxdepth: 2
   :caption: User Guide
   :hidden:

   user/tutorial/index
   user/exercises/index

.. toctree::
   :maxdepth: 2
   :caption: Reference
   :hidden:

   user/reference/index

* :ref:`genindex`
* :ref:`search`
