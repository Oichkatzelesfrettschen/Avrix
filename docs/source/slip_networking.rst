SLIP networking
===============

``SLIP`` (Serial Line Internet Protocol) provides a byte-oriented framing for
sending IP packets across a serial link.  Avrix includes a tiny, polling-based
implementation that works with any UART-compatible interface.

The driver consists of:

* ``tty.c`` – a minimal ring-buffer TTY abstraction.
* ``slip_uart.c`` – RFC1055 encode/decode over a ``tty``.
* ``ipv4.c`` – helper routines for building IPv4 frames.

These components are entirely optional and meant for experimentation with WiFi
modules or network hats.  The provided ``slip_demo`` example emits a single
IPv4 UDP frame framed with SLIP.
