# ATmega32U4 Technical Reference
## Comprehensive Implementation Guide for Avrix PSE51 + USB

**Target Profile:** PSE51 Extended (Minimal + USB HID/CDC)
**Chipset:** Atmel/Microchip ATmega32U4
**Architecture:** AVR 8-bit RISC with Native USB
**Used In:** Arduino Leonardo, Arduino Micro, SparkFun Pro Micro

**Official Datasheet:** [ATmega32U4 Datasheet](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-7766-8-bit-AVR-ATmega16U4-32U4_Datasheet.pdf)

---

## Table of Contents

1. [Hardware Specifications](#hardware-specifications)
2. [Memory Architecture](#memory-architecture)
3. [USB Subsystem](#usb-subsystem)
4. [Avrix HAL Implementation](#avrix-hal-implementation)
5. [USB HID Integration](#usb-hid-integration)
6. [PSE51 Resource Budget](#pse51-resource-budget)
7. [Programming Guide](#programming-guide)
8. [USB Debugging](#usb-debugging)

---

## Hardware Specifications

### Core Features

| Parameter              | Value                  | Notes                                    |
|------------------------|------------------------|------------------------------------------|
| **Architecture**       | AVR Enhanced RISC      | Modified Harvard + USB controller        |
| **Clock Speed**        | 0-16 MHz               | 16 MHz typical on Arduino                |
| **Operating Voltage**  | 2.7V - 5.5V            | 5V on Arduino Leonardo/Micro             |
| **Power Consumption**  | Active: 9 mA @ 8 MHz   | With USB enabled                         |
|                        | Power-down: 0.1 µA     | USB suspended                            |
| **Flash Memory**       | 32 KB                  | 4 KB used by USB bootloader              |
| **SRAM**               | 2.5 KB (2560 bytes)    | **+25% vs ATmega328P**                   |
| **EEPROM**             | 1 KB (1024 bytes)      | 100,000 write cycles                     |
| **Instruction Set**    | 131 instructions       | Most single-cycle execution              |
| **Performance**        | 16 MIPS @ 16 MHz       | 1 MIPS per MHz                           |

### Package Options

- **TQFP-44** (Thin Quad Flat Pack) - Arduino Leonardo
- **QFN-44/MLF-44** (Quad Flat No-lead) - Arduino Micro, Pro Micro

### Peripheral Features

| Peripheral             | Count | Details                                  |
|------------------------|-------|------------------------------------------|
| **GPIO Pins**          | 26    | Digital I/O                              |
| **ADC Channels**       | 12    | 10-bit resolution                        |
| **PWM Channels**       | 7     | Via Timer/Counter hardware               |
| **Timers**             | 4     | 1× 8-bit, 2× 16-bit, 1× 10-bit (USB)    |
| **USART**              | 1     | Full-duplex serial                       |
| **SPI**                | 1     | Master/Slave                             |
| **I²C (TWI)**          | 1     | 400 kHz max                              |
| **USB 2.0**            | 1     | **Full-speed (12 Mbps) device**          |
|                        |       | HID, CDC, MIDI, Mass Storage capable     |
| **Watchdog Timer**     | Yes   | Oscillator independent                   |
| **Analog Comparator**  | 1     | Configurable input selection             |
| **External Interrupts**| 5     | INT0-INT6                                |
| **Pin Change Int.**    | 8     | PCINT0-PCINT7                            |

### Key Difference vs ATmega328P

```
ATmega32U4 vs ATmega328P Comparison
═══════════════════════════════════════════════════════════════

Feature                ATmega328P       ATmega32U4      Advantage
──────────────────────────────────────────────────────────────
SRAM                   2048 bytes       2560 bytes      +25% (512 B)
USB Controller         No               Yes (Full-speed) Native USB
Bootloader             Arduino (512 B)  Caterina (4 KB) USB upload
GPIO Pins              23               26              +3 pins
ADC Channels           6                12              +6 channels
External Interrupts    2                5               +3 interrupts
USART                  1                1               Same
Package                DIP-28/TQFP-32   TQFP-44         Larger
Cost                   $2-3             $4-5            +$1-2
──────────────────────────────────────────────────────────────

Conclusion: ATmega32U4 is ATmega328P + USB + 512 bytes RAM
```

---

## Memory Architecture

### Flash Memory Organization

```
Flash Address Space (32 KB = 0x8000 bytes)
═══════════════════════════════════════════════════════════════

0x0000 ┌──────────────────────────────────────────┐
       │ Reset Vector (RJMP/JMP to bootloader)   │
0x0002 ├──────────────────────────────────────────┤
       │ Interrupt Vector Table (43 vectors)     │ 86 bytes
0x0056 ├──────────────────────────────────────────┤
       │                                          │
       │ Application Program Memory               │
       │ (Available: ~28 KB)                      │
       │                                          │
       │ - Avrix kernel: ~8-12 KB                 │
       │ - USB stack (optional): ~2-4 KB          │
       │ - User application: ~12-18 KB            │
       │                                          │
0x7000 ├──────────────────────────────────────────┤
       │ Caterina Bootloader (USB DFU)            │ 4 KB
       │ - USB enumeration                        │
       │ - Firmware upload via USB                │
0x7FFF └──────────────────────────────────────────┘

Notes:
- Flash organized as 16K × 16-bit words
- Caterina bootloader: 4 KB (vs 512 B on Uno)
- Bootloader activates on reset or "1200 baud touch"
- Self-programming via SPM instruction
- RWW section: 0x0000-0x6FFF
- NRWW section: 0x7000-0x7FFF (bootloader)
```

### SRAM Memory Map

```
Data Address Space (2.5 KB = 0x0A00 bytes total addressable)
═══════════════════════════════════════════════════════════════

0x0000 ┌──────────────────────────────────────────┐
       │ 32 General Purpose Registers (R0-R31)   │ 32 bytes
0x001F ├──────────────────────────────────────────┤
       │                                          │
0x0020 │ 64 I/O Registers                         │ 64 bytes
       │ (Standard AVR peripherals)               │
0x005F ├──────────────────────────────────────────┤
       │                                          │
0x0060 │ 160 Extended I/O Registers               │ 160 bytes
       │ (ATmega32U4-specific, including USB)     │
       │                                          │
0x00FF ├──────────────────────────────────────────┤
       │                                          │
       │                                          │
       │ Internal SRAM (2560 bytes)               │
       │ ═══════════════════════════════════      │
       │                                          │
       │ 0x0100: Kernel data segment start        │
       │         - Task Control Blocks (TCBs)     │
       │         - Kernel global variables        │
       │         - VFS file descriptor table      │
       │         (~400 bytes)                     │
       │                                          │
       │ 0x0290: USB buffers (if USB enabled)     │
       │         - Control endpoint (64 B)        │
       │         - CDC TX/RX (128 B each)         │
       │         (~320 bytes total)               │
       │                                          │
       │ 0x03D0: Heap start (for kalloc)          │
       │         - Dynamic allocation region      │
       │         (~1280 bytes available)          │
       │                                          │
       │ 0x0880: Stack bottom (grows downward)    │
       │         - Main task stack                │
       │         - Interrupt stack                │
       │         (~480 bytes)                     │
       │                                          │
0x09FF │         ↓ Stack grows down ↓             │
       └──────────────────────────────────────────┘
       0x09FF = Stack Pointer (SP) initial value

Memory Layout Strategy (with USB):
┌──────┬─────────┬───────┬──────────────────────┐
│Kernel│   USB   │ Heap  │  Stack (← down)      │
│(fixed)│ buffers │(↑ up) │                      │
└──────┴─────────┴───────┴──────────────────────┘
0x0100 0x0290   0x03D0  0x0880             0x09FF

Extra 512 bytes vs ATmega328P allows:
- USB stack integration
- Larger heap for applications
- More comfortable stack space
```

### USB Endpoint Memory

```
USB Endpoint DPRAM (Dual-Port RAM)
═══════════════════════════════════════════════════════════════

The ATmega32U4 has dedicated USB endpoint buffers in DPRAM:

Endpoint 0 (Control):
  - 64 bytes (mandatory for USB enumeration)
  - Bidirectional

Endpoints 1-6 (Configurable):
  - Up to 256 bytes per endpoint
  - Can be IN, OUT, or bidirectional
  - Total DPRAM: 832 bytes

Avrix USB CDC Configuration:
  EP0: Control (64 B)        - USB enumeration, device requests
  EP1: CDC Notification (8 B) - Serial state changes
  EP2: CDC Data OUT (64 B)    - Host → Device data
  EP3: CDC Data IN (64 B)     - Device → Host data

Total USB DPRAM usage: 200 bytes (separate from main SRAM)
```

---

## USB Subsystem

### USB Hardware Features

```
ATmega32U4 USB Controller Specifications
═══════════════════════════════════════════════════════════════

Standard Compliance:     USB 2.0 Full-Speed (12 Mbps)
Device Classes Supported:
  ✓ HID (Human Interface Device) - Keyboard, Mouse, Gamepad
  ✓ CDC (Communications Device)  - Virtual Serial Port
  ✓ MIDI (Musical Instrument)    - MIDI I/O
  ✓ Mass Storage                 - USB Drive (with external storage)
  ✓ Custom/Vendor-specific       - Proprietary protocols

Endpoints:                7 (EP0-EP6)
  - EP0: Control (mandatory, 64 bytes)
  - EP1-EP6: Configurable (IN/OUT/bidirectional)

Max Packet Sizes:
  - Control: 8, 16, 32, 64 bytes
  - Bulk: 8, 16, 32, 64 bytes
  - Interrupt: 8, 16, 32, 64 bytes
  - Isochronous: Not supported

DPRAM: 832 bytes total
PLL: On-chip USB PLL (generates 48 MHz from 16 MHz crystal)

Unique Feature: No external USB-to-serial chip required!
```

### USB Clock Configuration

```
USB Clock Tree (ATmega32U4)
═══════════════════════════════════════════════════════════════

External Crystal (16 MHz)
           │
           ├──────────────────────┐
           │                      │
           ↓                      ↓
    CPU Prescaler          USB PLL (×3)
           │                      │
           ↓                      ↓
     CPU Clock              48 MHz USB Clock
     (16 MHz)                     │
           │                      ↓
           │                USB Controller
           │                 (12 Mbps PHY)
           ↓
    Peripherals
    (Timers, USART, SPI, etc.)

Fuse Configuration:
  - CKSEL3:0 = 0111 (16 MHz external crystal)
  - SUT1:0 = 11 (slowly rising power)
  - CKDIV8 = 1 (unprogrammed, no division)

⚠️ CRITICAL: USB requires 16 MHz crystal (not 8 MHz or internal RC)
           PLL will not lock with incorrect clock source!
```

### USB Enumeration Process

```
USB Device Enumeration Sequence (Caterina Bootloader)
═══════════════════════════════════════════════════════════════

Step 1: Device Attach
  Host detects D+ pullup → Device connected

Step 2: Reset
  Host sends USB reset (SE0 for >10 ms)
  Device: Reset state, EP0 configured

Step 3: Get Device Descriptor (8 bytes)
  Host: GET_DESCRIPTOR (Device, 8 bytes)
  Device: Responds with:
    bLength = 18
    bDescriptorType = 1 (Device)
    bcdUSB = 0x0200 (USB 2.0)
    bDeviceClass = 2 (CDC)
    idVendor = 0x2341 (Arduino)
    idProduct = 0x8036 (Leonardo)

Step 4: Set Address
  Host: SET_ADDRESS (n)
  Device: Acknowledges, switches to address n

Step 5: Get Full Device Descriptor
  Host: GET_DESCRIPTOR (Device, 18 bytes)
  Device: Responds with full descriptor

Step 6: Get Configuration Descriptor
  Host: GET_DESCRIPTOR (Configuration, 9 bytes)
  Device: Responds with config + interfaces + endpoints

Step 7: Set Configuration
  Host: SET_CONFIGURATION (1)
  Device: Activates configuration, endpoints ready

Step 8: CDC Line Coding (for Virtual Serial)
  Host: SET_LINE_CODING (baud rate, stop bits, parity)
  Device: Acknowledges (but ignores, since it's virtual)

Step 9: CDC Control Line State
  Host: SET_CONTROL_LINE_STATE (DTR=1, RTS=1)
  Device: Serial port is now "open" and ready

Total Time: ~200 ms from plug-in to ready
```

---

## Avrix HAL Implementation

### Context Switching (Same as ATmega328P)

The ATmega32U4 uses the same context switching mechanism as the ATmega328P (37 bytes per task, ~40 cycles). See [ATMEGA328P_REFERENCE.md](ATMEGA328P_REFERENCE.md) for details.

### USB Integration in HAL

```c
/* arch/avr8/atmega32u4/hal_usb.c */

/**
 * USB CDC Virtual Serial Port Driver
 * Integrates with Avrix TTY layer for transparent serial I/O
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "drivers/tty/tty.h"

/* USB Endpoint Registers */
#define USB_EP0   0
#define USB_EP1   1  /* CDC Notification */
#define USB_EP2   2  /* CDC Data OUT */
#define USB_EP3   3  /* CDC Data IN */

/* USB CDC State */
typedef struct {
    bool connected;      /* Host has opened serial port */
    bool dtr;            /* Data Terminal Ready */
    bool rts;            /* Request To Send */
    uint8_t line_coding[7]; /* Baud, stop bits, parity (ignored) */
} usb_cdc_state_t;

static usb_cdc_state_t usb_cdc;

/**
 * USB General Interrupt (device-level events)
 */
ISR(USB_GEN_vect)
{
    uint8_t udint = UDINT;

    if (udint & (1 << EORSTI)) {
        /* End of Reset */
        UDINT &= ~(1 << EORSTI);
        usb_configure_endpoints();
        usb_cdc.connected = false;
    }

    if (udint & (1 << SOFI)) {
        /* Start of Frame (1 ms tick) */
        UDINT &= ~(1 << SOFI);
        /* Can be used for USB timing */
    }
}

/**
 * USB Endpoint Interrupt (data transfer)
 */
ISR(USB_COM_vect)
{
    uint8_t ep = UEINT & UEIENX;

    if (ep & (1 << USB_EP0)) {
        /* Control endpoint (setup packets) */
        usb_handle_control();
    }

    if (ep & (1 << USB_EP2)) {
        /* CDC Data OUT (host → device) */
        usb_cdc_rx();
    }

    if (ep & (1 << USB_EP3)) {
        /* CDC Data IN (device → host) */
        usb_cdc_tx();
    }
}

/**
 * CDC RX: Read data from host
 */
static void usb_cdc_rx(void)
{
    UENUM = USB_EP2;  /* Select EP2 */

    if (UEINTX & (1 << RXOUTI)) {
        /* Data available */
        uint8_t count = UEBCLX;  /* Byte count */

        while (count--) {
            uint8_t c = UEDATX;  /* Read byte from FIFO */
            tty_rx_push(&usb_tty, c);  /* Push to TTY buffer */
        }

        UEINTX &= ~(1 << RXOUTI);  /* Clear interrupt */
    }
}

/**
 * CDC TX: Send data to host
 */
static void usb_cdc_tx(void)
{
    UENUM = USB_EP3;  /* Select EP3 */

    if (UEINTX & (1 << TXINI)) {
        /* Endpoint ready for data */
        uint8_t count = 0;

        while (count < 64 && tty_tx_available(&usb_tty)) {
            UEDATX = tty_tx_pop(&usb_tty);  /* Write to FIFO */
            count++;
        }

        UEINTX &= ~(1 << TXINI);  /* Send packet */
    }
}

/**
 * HAL USB CDC Integration
 * Allows Avrix to use USB as transparent serial port
 */
void hal_usb_cdc_init(void)
{
    /* Enable USB controller */
    UHWCON |= (1 << UVREGE);  /* USB pads regulator */

    /* Enable PLL (16 MHz → 48 MHz) */
    PLLCSR = (1 << PLLE);
    while (!(PLLCSR & (1 << PLOCK)));  /* Wait for lock */

    /* Enable USB */
    USBCON = (1 << USBE) | (1 << OTGPADE);

    /* Attach device (D+ pullup) */
    UDCON &= ~(1 << DETACH);

    /* Enable interrupts */
    UDIEN = (1 << EORSTE) | (1 << SOFE);
    UEIENX = (1 << RXOUTE) | (1 << TXINE);

    usb_cdc.connected = false;
}
```

---

## USB HID Integration

### HID Keyboard Example

```c
/* Example: USB HID Keyboard (Alternative to CDC) */

/**
 * HID Keyboard Report Descriptor
 */
const uint8_t keyboard_hid_report[] PROGMEM = {
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0xE0,        /*   Usage Minimum (224) */
    0x29, 0xE7,        /*   Usage Maximum (231) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x01,        /*   Logical Maximum (1) */
    0x75, 0x01,        /*   Report Size (1 bit) */
    0x95, 0x08,        /*   Report Count (8 bits = modifiers) */
    0x81, 0x02,        /*   Input (Data, Variable, Absolute) */
    0x95, 0x01,        /*   Report Count (1 byte = reserved) */
    0x75, 0x08,        /*   Report Size (8 bits) */
    0x81, 0x01,        /*   Input (Constant) */
    0x95, 0x06,        /*   Report Count (6 bytes = keycodes) */
    0x75, 0x08,        /*   Report Size (8 bits) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x65,        /*   Logical Maximum (101) */
    0x05, 0x07,        /*   Usage Page (Key Codes) */
    0x19, 0x00,        /*   Usage Minimum (0) */
    0x29, 0x65,        /*   Usage Maximum (101) */
    0x81, 0x00,        /*   Input (Data, Array) */
    0xC0               /* End Collection */
};

/**
 * HID Keyboard Report (8 bytes)
 */
typedef struct {
    uint8_t modifiers;   /* Ctrl, Shift, Alt, GUI */
    uint8_t reserved;    /* Always 0 */
    uint8_t keys[6];     /* Up to 6 simultaneous keys */
} hid_keyboard_report_t;

/**
 * Send keypress via USB HID
 */
void usb_hid_send_key(uint8_t key, uint8_t modifiers)
{
    hid_keyboard_report_t report = {
        .modifiers = modifiers,
        .reserved = 0,
        .keys = {key, 0, 0, 0, 0, 0}
    };

    UENUM = USB_EP1;  /* Select HID endpoint */

    while (!(UEINTX & (1 << TXINI)));  /* Wait for ready */

    /* Write report to FIFO */
    for (uint8_t i = 0; i < sizeof(report); i++) {
        UEDATX = ((uint8_t *)&report)[i];
    }

    UEINTX &= ~(1 << TXINI);  /* Send packet */

    /* Release key (send empty report) */
    _delay_ms(10);
    report.modifiers = 0;
    report.keys[0] = 0;

    while (!(UEINTX & (1 << TXINI)));
    for (uint8_t i = 0; i < sizeof(report); i++) {
        UEDATX = ((uint8_t *)&report)[i];
    }
    UEINTX &= ~(1 << TXINI);
}

/**
 * Example: Type "Hello" via USB HID
 */
void demo_usb_keyboard(void)
{
    usb_hid_send_key(0x0B, 0x02);  /* 'H' with Shift */
    usb_hid_send_key(0x08, 0x00);  /* 'e' */
    usb_hid_send_key(0x0F, 0x00);  /* 'l' */
    usb_hid_send_key(0x0F, 0x00);  /* 'l' */
    usb_hid_send_key(0x12, 0x00);  /* 'o' */
}
```

### HID Mouse Example

```c
/**
 * HID Mouse Report (4 bytes)
 */
typedef struct {
    uint8_t buttons;  /* Bit 0: Left, Bit 1: Right, Bit 2: Middle */
    int8_t  x;        /* -127 to +127 */
    int8_t  y;        /* -127 to +127 */
    int8_t  wheel;    /* -127 to +127 */
} hid_mouse_report_t;

/**
 * Move mouse cursor
 */
void usb_hid_move_mouse(int8_t dx, int8_t dy)
{
    hid_mouse_report_t report = {
        .buttons = 0,
        .x = dx,
        .y = dy,
        .wheel = 0
    };

    UENUM = USB_EP1;
    while (!(UEINTX & (1 << TXINI)));
    for (uint8_t i = 0; i < sizeof(report); i++) {
        UEDATX = ((uint8_t *)&report)[i];
    }
    UEINTX &= ~(1 << TXINI);
}

/**
 * Example: Draw square with mouse
 */
void demo_usb_mouse(void)
{
    for (int i = 0; i < 100; i++) usb_hid_move_mouse(1, 0);   /* Right */
    for (int i = 0; i < 100; i++) usb_hid_move_mouse(0, 1);   /* Down */
    for (int i = 0; i < 100; i++) usb_hid_move_mouse(-1, 0);  /* Left */
    for (int i = 0; i < 100; i++) usb_hid_move_mouse(0, -1);  /* Up */
}
```

---

## PSE51 Resource Budget

### Memory Allocation (2.5 KB SRAM)

```
ATmega32U4 PSE51 Memory Budget (with USB CDC)
═══════════════════════════════════════════════════════════════

Region                  Start    End      Size     Usage
──────────────────────────────────────────────────────────────
Registers               0x0000   0x001F   32 B     Hardware
I/O Registers           0x0020   0x005F   64 B     Hardware
Extended I/O            0x0060   0x00FF   160 B    Hardware (USB)
──────────────────────────────────────────────────────────────
Kernel Data             0x0100   0x028F   400 B    Avrix kernel
  ├─ TCB (1 task)       0x0100   0x0127   40 B     Task control
  ├─ VFS descriptors    0x0128   0x0167   64 B     File table (8)
  ├─ TTY buffers        0x0168   0x01E7   128 B    RX/TX (64 each)
  ├─ Global vars        0x01E8   0x028F   168 B    Kernel state
──────────────────────────────────────────────────────────────
USB CDC Buffers         0x0290   0x03CF   320 B    USB stack
  ├─ EP0 control        0x0290   0x02CF   64 B     Setup packets
  ├─ CDC RX             0x02D0   0x034F   128 B    Host → Device
  └─ CDC TX             0x0350   0x03CF   128 B    Device → Host
──────────────────────────────────────────────────────────────
Heap (kalloc)           0x03D0   0x087F   1200 B   Dynamic alloc
  ├─ USB HID buffers    Variable          ~64 B    If HID mode
  ├─ Application data   Variable          ~1136 B  User heap
──────────────────────────────────────────────────────────────
Stack (grows down)      0x0880   0x09FF   384 B    Main + interrupt
  ├─ Main stack         Variable          ~300 B   Application
  ├─ ISR stack          Variable          ~84 B    Scheduler + USB
──────────────────────────────────────────────────────────────
TOTAL SRAM              0x0000   0x09FF   2560 B

Memory Safety Margin: ~150 bytes (heap/stack separation)

Advantage vs ATmega328P:
  +512 bytes total SRAM
  Allows USB stack integration without sacrificing heap
```

### Flash Allocation (32 KB)

```
ATmega32U4 PSE51 Flash Budget
═══════════════════════════════════════════════════════════════

Component                Size (KB)   Percentage   Notes
──────────────────────────────────────────────────────────────
Caterina Bootloader      4.0         12.5%        USB DFU upload
Interrupt Vector Table   0.09        0.3%         43 vectors
Avrix Kernel             8-12        25-38%       Core + drivers
  ├─ Scheduler           ~1.5 KB                  Task switching
  ├─ Door RPC            ~0.8 KB                  IPC
  ├─ kalloc              ~0.6 KB                  Memory alloc
  ├─ VFS layer           ~2.0 KB                  Filesystem
  ├─ ROMFS driver        ~0.7 KB                  Read-only FS
  ├─ EEPFS driver        ~1.2 KB                  EEPROM FS
  ├─ TTY driver          ~1.0 KB                  Serial I/O
  ├─ USB CDC stack       ~2-4 KB                  Virtual serial
──────────────────────────────────────────────────────────────
Application Space        14-18       44-56%       User code
──────────────────────────────────────────────────────────────
TOTAL FLASH              28          87.5%        32 KB - bootloader

Trade-off vs ATmega328P:
  -3.5 KB flash (due to larger bootloader)
  +USB CDC/HID capability (no external USB chip needed)
```

---

## Programming Guide

### Caterina Bootloader Upload

```bash
# The ATmega32U4 uses Caterina bootloader, not Arduino ISP

# Method 1: "1200 baud touch" (Arduino IDE method)
stty -F /dev/ttyACM0 1200  # Triggers bootloader reset
sleep 1
avrdude -c avr109 -p m32u4 -P /dev/ttyACM0 -b 57600 \
        -U flash:w:build_32u4/unix0.hex:i

# Method 2: Hold reset button during upload
# (Or short RST to GND twice within 750 ms)

# Note: Bootloader exits after 8 seconds if no upload
```

### Cross-Compilation

```bash
# Build for ATmega32U4
meson setup build_32u4 --cross-file cross/atmega32u4_gcc14.cross
meson compile -C build_32u4

# Flash with Arduino IDE (easiest)
# Tools → Board → Arduino Leonardo
# Sketch → Upload Using Programmer → "AVR ISP mkII"

# Or use avrdude directly
avrdude -c avr109 -p m32u4 -P /dev/ttyACM0 -b 57600 \
        -U flash:w:build_32u4/unix0.hex:i
```

---

## USB Debugging

### Monitoring USB Traffic

```bash
# Install usbmon (Linux)
sudo modprobe usbmon
sudo apt install wireshark

# Capture USB traffic
sudo wireshark &
# Select usbmonX interface
# Filter: usb.device_address == <device_addr>

# View USB device descriptors
lsusb -v -d 2341:8036

# Output:
# Bus 001 Device 045: ID 2341:8036 Arduino SA Leonardo (CDC ACM, HID)
#   idVendor           0x2341 Arduino SA
#   idProduct          0x8036 Leonardo
#   bcdDevice            1.00
#   iManufacturer           1 Arduino LLC
#   iProduct                2 Arduino Leonardo
#   iSerial                 3 <serial_number>
#   bNumConfigurations      1
#     Configuration Descriptor:
#       bNumInterfaces          2
#       Interface Descriptor:
#         bInterfaceClass         2 Communications
#         bInterfaceSubClass      2 Abstract (modem)
```

### Common USB Issues

#### Issue 1: Device Not Enumerated

```
Symptom: lsusb doesn't show device after plug-in

Causes:
  1. No 16 MHz crystal (USB PLL won't lock)
  2. Wrong fuse settings (CKSEL bits incorrect)
  3. USB DP/DM lines not connected
  4. No firmware running (stuck in bootloader timeout)

Debug:
  - Verify 16 MHz crystal with oscilloscope
  - Check fuses: avrdude -c usbasp -p m32u4 -U lfuse:r:-:h
  - Measure voltage on UVCC pin (should be 3.3V)
```

#### Issue 2: CDC Serial Port Doesn't Open

```
Symptom: Device enumerates, but /dev/ttyACM0 doesn't appear

Causes:
  1. Missing CDC descriptors in firmware
  2. Wrong bDeviceClass (should be 0x02 for CDC)
  3. No CDC Abstract Control Management interface

Debug:
  - Check dmesg for kernel errors
  - Verify CDC interface descriptors with lsusb -v
```

---

## Conclusion

The ATmega32U4 is an excellent **PSE51 Extended** target:

**Strengths:**
- ✅ +512 bytes RAM vs ATmega328P (25% more)
- ✅ Native USB 2.0 Full-Speed (12 Mbps)
- ✅ HID/CDC/MIDI support (no external chip)
- ✅ Can emulate keyboard/mouse
- ✅ Same 16 MIPS performance as ATmega328P
- ✅ More GPIO, ADC, and interrupts

**Limitations:**
- ⚠️ 4 KB bootloader (vs 512 B on Uno)
- ⚠️ Still only 2.5 KB RAM (tight for threading)
- ⚠️ USB requires 16 MHz crystal (not flexible)
- ⚠️ Larger package (TQFP-44 vs DIP-28)
- ⚠️ Higher cost ($4-5 vs $2-3)

**Best Use Cases:**
- USB HID devices (keyboards, mice, game controllers)
- Virtual serial ports (no FTDI chip needed)
- MIDI controllers
- USB-to-anything bridges
- Any PSE51 application + USB

**Not Recommended For:**
- Multi-threading (use ATmega1284P)
- Battery-powered (USB consumes 9 mA)
- 3.3V systems (USB needs 5V)

---

*Document Version: 1.0*
*Last Updated: 2025-01-19*
*Datasheet Reference: ATmega32U4 Rev. 7766J (Microchip)*
