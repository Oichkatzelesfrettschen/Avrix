Fuse and Lock Bits Cheat-Sheet
==============================

The table below lists recommended fuse and lock-bit settings for the
Arduino Uno R3's ATmega328P and ATmega16U2. These values boot the
application MCU at full speed while protecting the USB firmware.

+----------------+---------------+---------------+-----------------------------------------------------+
| Item           | 328P value    | 16U2 value    | Purpose                                             |
+================+===============+===============+=====================================================+
| CKDIV8         | 0 (disabled)  | 0 (disabled)  | Start at 16\,MHz to maintain UART timing            |
+----------------+---------------+---------------+-----------------------------------------------------+
| BODLEVEL       | 0b101 (2.7 V) | 0b110 (3.3 V) | Ensure reliable USB when VIN sags                   |
+----------------+---------------+---------------+-----------------------------------------------------+
| BOOTRST        | 1             | 0             | App MCU jumps to flash, 16U2 remains in DFU         |
+----------------+---------------+---------------+-----------------------------------------------------+
| BOOTSIZE       | 00 (512 B)    | —             | Leaves space for STK500v2 bootloader                |
+----------------+---------------+---------------+-----------------------------------------------------+
| LOCK[BLB]      | BLB12=0, BLB11=1 | BLB12=1, BLB11=0 | Protect boot section and DFU firmware              |
+----------------+---------------+---------------+-----------------------------------------------------+

Program the 328P with:

.. code-block:: bash

   avrdude -c avrisp -p m328p \
           -U lfuse:w:0xFF:m \
           -U hfuse:w:0xD5:m \
           -U efuse:w:0x05:m

Adjust ``0xD5`` if you select a different brown-out threshold.
