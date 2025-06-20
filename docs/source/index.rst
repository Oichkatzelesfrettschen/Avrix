Avrix Documentation
===================

This documentation is generated using Sphinx and Doxygen.  When the
``sphinx_rtd_theme`` package is installed, the output mirrors the Read the
Docs appearance.
The code targets the Arduino Uno R3 (ATmega328P with ATmega16U2 USB
controller). Run
```
meson compile -C build doc-doxygen   # optional
meson compile -C build doc-sphinx    # optional
meson compile -C build doc           # runs every generator
```
from the build directory to generate HTML output.  These targets may fail if
``doxygen`` is not installed or if the XML files have not been generated yet.

.. toctree::
   :maxdepth: 2

   hardware
   toolchain
   monograph
   fuses
   roadmap-qemu-avr
