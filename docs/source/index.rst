Avrix Documentation
===================

This documentation is generated using Sphinx and Doxygen.
The code targets the Arduino Uno R3 (ATmega328P with ATmega16U2 USB
controller). Run
```
meson compile doc-doxygen
meson compile doc-sphinx
```
from the build directory to generate HTML output.  These targets may fail if
``doxygen`` is not installed or if the XML files have not been generated yet.

.. toctree::
   :maxdepth: 2

   hardware
   toolchain
   monograph
   fuses
