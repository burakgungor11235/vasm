Installation
============

.. warning::

   **varm is under active development.** This installation guide may be
   incomplete or contain errors. There are NO guarantees about stability
   or compatibility between versions.

Prerequisites
-------------

Before installing varm, ensure you have the following software:

+---------------+-------------------+----------------------------------------+
| Software      | Minimum Version   | Description                            |
+---------------+-------------------+----------------------------------------+
| C Compiler    | GCC 9, Clang 10   | For compiling varm and vasm            |
| Python        | 3.9               | For build system (Meson)               |
| Meson         | 0.55              | Build system                           |
| Ninja         | 1.10              | Build backend for Meson                |
| Git           | Any recent        | Version control (optional)             |
+---------------+-------------------+----------------------------------------+

Platform-Specific Installation
------------------------------

### Linux (Ubuntu/Debian)

.. code-block:: bash

   # Install build tools
   sudo apt update
   sudo apt install build-essential git python3 python3-pip

   # Install Meson and Ninja
   sudo pip3 install meson ninja

   # Verify installation
   meson --version
   ninja --version

### Linux (Fedora)

.. code-block:: bash

   sudo dnf install @development-tools git python3 meson ninja-build
   sudo pip3 install meson ninja

### macOS

.. code-block:: bash

   # Install Homebrew if not already installed
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

   # Install dependencies
   brew install python3 meson ninja

### Windows

1. Install `Git for Windows <https://git-scm.com/download/win>`_
2. Install `Python 3.9+ <https://www.python.org/downloads/>`_
3. Install `Meson <https://mesonbuild.com/Getting-meson.html>`_
4. Install Visual Studio Build Tools or use WSL

Cloning the Repository
----------------------

.. code-block:: bash

   # Clone the repository
   git clone https://github.com/varm/varm.git
   cd varm

   # Initialize submodules (if any)
   git submodule update --init --recursive

Building varm
--------------

varm uses the Meson build system. The build process has two steps:
configure and compile.

.. code-block:: bash

   # Create a build directory (out-of-source build)
   meson setup build

   # Compile the project
   meson compile -C build

   # Optional: Run the test suite
   meson test -C build

Build Output
~~~~~~~~~~~~

After building, you will have the following executables:

+--------------------------+------------------------------------------+
| Executable               | Description                              |
+--------------------------+------------------------------------------+
| ``build/src/varm``       | The virtual machine                      |
| ``build/src/vasm``       | The assembler                            |
| ``build/src/unit_*``     | Unit test executables                    |
+--------------------------+------------------------------------------+

Verifying the Build
-------------------

Once built, verify the installation:

.. code-block:: bash

   # Check varm version
   ./build/src/varm --help

   # Check vasm version
   ./build/src/vasm --help

   # Run a simple test
   echo "mov r0, #42" > /tmp/test.vasm
   ./build/src/vasm -o /tmp/test.vm /tmp/test.vasm
   ./build/src/varm /tmp/test.vm

   # Should exit with code 42

Directory Structure
-------------------

After cloning and building, the directory structure is:

::

   varm/
   ├── build/                 # Build artifacts (created by Meson)
   │   ├── src/
   │   │   ├── varm          # VM executable
   │   │   ├── vasm          # Assembler executable
   │   │   └── unit_*        # Test executables
   │   └── ...
   ├── src/
   │   ├── vm/               # VM implementation
   │   ├── asm/              # Assembler implementation
   │   ├── include/          # Header files
   │   └── test/             # Test sources
   ├── docs/                 # Documentation (Sphinx)
   ├── scripts/              # Build scripts
   ├── examples/             # Example programs
   └── meson.build           # Meson configuration

Troubleshooting
---------------

### Build Fails with "Could not find Ninja"

Ensure Ninja is installed and in your PATH:

.. code-block:: bash

   which ninja
   ninja --version

### Python Version Too Old

Check your Python version:

.. code-block:: bash

   python3 --version

If below 3.9, install a newer version from python.org or use pyenv.

### C Compiler Not Found

On Linux, install build-essential:

.. code-block:: bash

   sudo apt install build-essential  # Debian/Ubuntu
   sudo dnf install @development-tools  # Fedora

### Rebuilding from Scratch

If you encounter issues, try a clean rebuild:

.. code-block:: bash

   # Remove the build directory
   rm -rf build

   # Reconfigure and rebuild
   meson setup build
   meson compile -C build

Updating varm
-------------

To update to the latest version:

.. code-block:: bash

   git fetch origin
   git pull origin main

   # Rebuild
   rm -rf build
   meson setup build
   meson compile -C build

Dependencies
------------

The only runtime dependency is a C99-compatible C compiler.

Build-time dependencies:

* Python 3.9+
* Meson 0.55+
* Ninja 1.10+

Optional dependencies:

* Doxygen (for API documentation)
* LaTeX (for PDF documentation)
* Sphinx + Breathe (for HTML documentation)

Uninstallation
--------------

To completely remove varm:

.. code-block:: bash

   # Remove the build directory
   rm -rf build

   # Remove cloned repository (if you cloned it)
   cd ..
   rm -rf varm
