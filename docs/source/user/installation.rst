============
Installation
============

Build varm from source to start learning assembly programming.

Requirements
============

* C compiler (GCC, Clang)
* Python 3.9+
* Meson & Ninja

Linux (Ubuntu/Debian):

.. code-block:: bash

   sudo apt install build-essential python3 python3-pip
   pip3 install meson ninja

macOS:

.. code-block:: bash

   brew install python3 meson ninja

Build
=====

.. code-block:: bash

   git clone https://github.com/varm/varm.git
   cd varm
   meson setup build
   meson compile -C build

Verify:

.. code-block:: bash

   ./build/src/varm --help
   ./build/src/vasm --help

Quick test:

.. code-block:: bash

   echo "mov r0, #42" > /tmp/test.vasm
   ./build/src/vasm -o /tmp/test.vm /tmp/test.vasm
   ./build/src/varm /tmp/test.vm
   echo $?  # Should output 42

Output
======

* ``build/src/varm`` - Virtual machine
* ``build/src/vasm`` - Assembler

.. note::

   varm is experimental. APIs may change.
