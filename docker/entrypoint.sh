#!/bin/sh
# Entrypoint for the Avrix QEMU container.
# Builds the firmware when required and then boots QEMU using the
# pre-generated disk image. Hardware peripherals such as the ATmega16U2
# USB bridge are enabled for completeness.

set -e

if [ ! -e build/unix0.elf ]; then
    meson setup build --wipe --cross-file cross/avr_m328p.txt
    meson compile -C build
    avr-objcopy -O binary build/unix0.elf avrix.bin
    truncate -s 1M avrix.img
    dd if=avrix.bin of=avrix.img conv=notrunc
fi

exec "$@" -device atmega16u2-usb -serial stdio
