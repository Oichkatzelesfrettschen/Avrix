# AVR Toolchain Setup

Run the script below to install Atmel's AVR-GCC toolchain on Ubuntu 24.04:

```bash
sudo ./setup.sh            # installs via the pmjdebruijn PPA
```

To use Ubuntu's official packages instead, pass `--official`.

Optimised flags for an Arduino Uno (ATmega328P):

```bash
MCU=atmega328p
CFLAGS="-mmcu=$MCU -DF_CPU=16000000UL -Os -flto -ffunction-sections -fdata-sections"
LDFLAGS="-mmcu=$MCU -Wl,--gc-sections -flto"
```

