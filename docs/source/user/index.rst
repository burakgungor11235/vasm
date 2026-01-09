Using varm
==========

This section provides comprehensive documentation for users of the varm
virtual machine and assembler. Whether you are a student learning assembly
language, an educator teaching computer architecture, or a developer
building a language on top of varm, this documentation will help you get
started and become proficient with the varm ecosystem.

.. note::

   **varm is under active development.** This documentation reflects the
   current state of the code and may be incomplete or contain errors.
   There are NO stability guarantees for this release.

.. toctree::
    :maxdepth: 2
    :caption: User Guide Contents

    installation
    quickstart
    tutorial/index
    reference/index
    exercises/index
    roadmap

Quick Navigation
----------------

For users with limited time, here are the most important sections:

+------------------+----------------------------------------------------+
| If you want...   | Read this...                                       |
+==================+====================================================+
| Install varm     | :doc:`installation`                                |
+------------------+----------------------------------------------------+
| Run your first   | :doc:`quickstart`                                  |
| program          |                                                    |
+------------------+----------------------------------------------------+
| Learn the basics | :doc:`tutorial/index` (start with                  |
|                  | :doc:`tutorial/01-introduction`)                   |
+------------------+----------------------------------------------------+
| Look up an       | :doc:`reference/instruction-set`                   |
| instruction      |                                                    |
+------------------+----------------------------------------------------+
| Practice what    | :doc:`exercises/index`                             |
| you've learned   |                                                    |
+------------------+----------------------------------------------------+

Prerequisites
-------------

Before using varm, ensure you have:

* **C compiler** - GCC, Clang, or MSVC
* **Python 3.9+** - For the build system (Meson)
* **Meson & Ninja** - Build system
* **Git** - For version control (optional but recommended)

See :doc:`installation` for detailed setup instructions.

Conventions Used
----------------

This documentation uses the following conventions:

* **Code blocks** show varm assembly or shell commands:

  .. code-block:: vasm

     mov r0, #42
     swi

* **Instruction format** is shown as:

  .. code-block:: text

     MOV Rd, #<imm>

* **Register names** use the format ``r0`` through ``r15``, with ``sp``,
  ``lr``, and ``pc`` as aliases for r13, r14, and r15.

* **Immediate values** are written with ``#`` prefix: ``#42``, ``#0xFF``

* **Bit ranges** use ARM convention: bits 31:24 means bits 24 through 31

Next Steps
----------

Ready to get started? Head to :doc:`installation` to set up varm, or jump
directly to :doc:`quickstart` for a 5-minute introduction.

.. note::

   For internal documentation (architecture, implementation details, contributor
   guidelines), see the :doc:`Developer Guide <internal/index>`.
