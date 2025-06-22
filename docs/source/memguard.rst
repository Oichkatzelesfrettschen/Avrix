Memory Guard Utilities
======================

The :file:`memguard.h` helpers provide a small runtime check for buffer
overflows. Each guarded region reserves :c:macro:`GUARD_BYTES` sentinel
bytes on either side, filled with the value ``0xA5`` as defined by
:c:macro:`GUARD_PATTERN`.

Guarded buffers are initialised with :c:func:`guard_init` and validated
with :c:func:`check_guard`.  This mechanism is intended for unit tests
where lightweight detection of stack or heap corruption is valuable.

Example usage
-------------

.. code-block:: c

   #include "memguard.h"

   static uint8_t raw[16 + 2 * GUARD_BYTES];
   /* User-facing slice excluding the sentinels */
   static uint8_t *buf = raw + GUARD_BYTES;

   int main(void)
   {
       guard_init(raw, sizeof raw);
       /* application fills `buf` with data */
       if (!check_guard(raw, sizeof raw)) {
           /* overflow detected */
           return 1;
       }
       return 0;
   }

``guard_init`` writes ``0xA5`` at both ends of ``raw``.  After the buffer
is used, ``check_guard`` ensures the sentinels remain untouched.  Any
modification indicates a write outside the intended region.
