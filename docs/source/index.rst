=================================================
Avrix — µ-UNIX for the Arduino Uno R3 (ATmega328P)
=================================================

**Avrix** is a ≤ 10 kB C23 nanokernel with a wear-levelled EEPROM log-FS,
zero-copy “Door” RPC, and a lock suite tuned for the AVR **ATmega328P**
found on the *Arduino Uno R3*.
The unified ``nk_superlock`` couples a global *Big Kernel Lock* with optional
real-time bypass (``*_lock_rt`` / ``*_unlock_rt``).  Encoding helpers snapshot
its copy-on-write matrix for transport.  The
``tests/unified_spinlock_test.c`` target demonstrates typical usage.
All source, schematics, and build artefacts are cross-compiled on a modern
desktop, then unit-tested inside QEMU’s ``arduino-uno`` machine model.

This manual is generated with **Doxygen + Sphinx**.  If the optional
``sphinx-rtd-theme`` is available the HTML output mirrors Read-the-Docs.

-------------------------------------------------
Building the documentation
-------------------------------------------------

From the *build* directory you can build the individual generators:

.. code-block:: bash

   meson compile -C build doc-doxygen   # C API reference                (build/docs/doxygen/html)
   meson compile -C build doc-sphinx    # User / dev manual             (build/docs)
   meson compile -C build doc           # Runs Doxygen → Sphinx and fails on warnings

*Hints*

* ``doc-doxygen`` requires **Doxygen** ≥ 1.9 with XML export enabled.
  ``dot`` from Graphviz is optional; diagrams are built when available.
* ``doc-sphinx`` needs **Sphinx**, **Breathe** and **Exhale**
  (``pip install breathe exhale sphinx-rtd-theme``).
* ``doc`` is available only when at least one documentation tool was found.

-------------------------------------------------
Contents
-------------------------------------------------

.. toctree::
   :maxdepth: 2
   :caption: Reference

   hardware
   toolchain
   monograph
   filesystem
   slip_networking
   memguard
   fuses
   api/library_root
   roadmap-qemu-avr

-------------------------------------------------
Indices & tables
-------------------------------------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
