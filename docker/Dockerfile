# Dockerfile - Avrix QEMU environment
# ----------------------------------
# Provides a reproducible Ubuntu-based container with the AVR tool-chain
# and QEMU pre-configured. The resulting image builds the firmware and
# prepares a minimal disk image for the QEMU AVR target.

FROM ubuntu:24.04

RUN apt-get -qq update
RUN apt-get -yqq install --no-install-recommends \
        gcc-avr avr-libc binutils-avr qemu-system-misc \
        meson ninja-build make git ca-certificates \
        python3-pip
RUN rm -rf /var/lib/apt/lists/*

WORKDIR /avrix
COPY . /avrix

# Build firmware using Meson and convert ELF to a raw binary.
RUN meson setup build --wipe --cross-file cross/avr_m328p.txt || true && \
    meson compile -C build || true && \
    avr-objcopy -O binary build/unix0.elf avrix.bin && \
    truncate -s 1M avrix.img && \
    dd if=avrix.bin of=avrix.img conv=notrunc

COPY docker/entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

ENTRYPOINT ["/entrypoint.sh"]
CMD ["qemu-system-avr", "-M", "arduino-uno", "-bios", "build/unix0.elf", "-nographic"]
