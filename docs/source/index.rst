=================================================
Avrix — µ-UNIX for the Arduino Uno R3 (ATmega328P)
=================================================

**Avrix** is a ≤ 10 kB C23 nanokernel with a wear-levelled EEPROM log-FS,
zero-copy “Door” RPC, and a lock suite tuned for the AVR **ATmega328P**
found on the *Arduino Uno R3*.  
All source, schematics, and build artefacts are cross-compiled on a modern
desktop, then unit-tested inside QEMU’s ``arduino-uno`` machine model.

This manual is generated with **Doxygen + Sphinx**.  If the optional
``sphinx-rtd-theme`` is available the HTML output mirrors Read-the-Docs.

-------------------------------------------------
Building the documentation
-------------------------------------------------

From the *build* directory you can build the individual generators:

.. code-block:: bash

   meson compile -C build doc-doxygen   # C API reference                (docs/doxygen/html)
   meson compile -C build doc-sphinx    # User / dev manual             (build/docs)
   meson compile -C build doc           # Aggregates both when present

*Hints*

* ``doc-doxygen`` requires **Doxygen** ≥ 1.9 with XML export enabled.  
* ``doc-sphinx`` needs **Sphinx**, **Breathe** and **Exhale**
  (``pip install breathe exhale sphinx-rtd-theme``).
* The aggregated ``doc`` target is created only when at least one
  generator has been found by Meson.

-------------------------------------------------
Contents
-------------------------------------------------

.. toctree::
   :maxdepth: 2
   :caption: Reference

   hardware               <!-- PCB walk-through & schematics           -->
   toolchain              <!-- GCC-14 / Clang-20 cross-file guide      -->
   monograph              <!-- All subsystems in one deep-dive         -->
   filesystem             <!-- TinyLog-4 design & API                   -->
   fuses                  <!-- AVR lock-/boot-fuse cheat-sheet          -->
   roadmap-qemu-avr       <!-- QEMU board status & future work          -->
   contributing           <!-- Code-size rules, CI, coding style        -->

-------------------------------------------------
Indices & tables
-------------------------------------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
