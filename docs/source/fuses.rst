Fuse and Lock Bits Cheat-Sheet
==============================

The table below lists recommended fuse and lock-bit settings for the
Arduino Uno R3's ATmega328P and ATmega16U2. These values boot the
application MCU at full speed while protecting the USB firmware.

.. list-table::
   :header-rows: 1
   :widths: 15 20 20 45

   * - Item
     - 328P
     - 16U2
     - Purpose
   * - CKDIV8
     - 0
     - 0
     - Start at 16 MHz for stable UART
   * - BODLEVEL
     - 0b101
     - 0b110
     - Ensure reliable USB when VIN sags
   * - BOOTRST
     - 1
     - 0
     - Application jumps to flash, 16U2 stays in DFU
   * - BOOTSIZE
     - ``00`` (512B)
     - ``—``
     - Reserve space for STK500v2 bootloader
   * - LOCK[BLB]
     - ``BLB12=0, BLB11=1``
     - ``BLB12=1, BLB11=0``
     - Protect boot sections and DFU firmware

Program the 328P with:

.. code-block:: bash

   avrdude -c avrisp -p m328p \
           -U lfuse:w:0xFF:m \
           -U hfuse:w:0xD5:m \
           -U efuse:w:0x05:m

Adjust ``0xD5`` if you select a different brown-out threshold.
