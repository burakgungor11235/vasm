=====
varm
=====

varm is an **educational ARM-like virtual machine** designed for learning assembly language programming. Build and run simple assembly programs to understand how computers execute code at the lowest level.

Quick Links
===========

* **Install** - Build from source in 2 minutes: :doc:`installation`
* **Start** - Your first program: :doc:`quickstart`
* **Learn** - Assembly concepts: :doc:`tutorial/index`
* **Reference** - Instructions & syscalls: :doc:`reference/index`
* **Practice** - Exercises: :doc:`exercises/index`

What is varm?
=============

varm provides a minimal ARM-like instruction set that:

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

See :doc:`installation` for setup instructions.

.. note::

   varm is experimental. APIs and instruction sets may change.


.. toctree::
   :maxdepth: 2
   :caption: Contents

   installation
   quickstart
   examples
   tutorial/index
   reference/index
   exercises/index


