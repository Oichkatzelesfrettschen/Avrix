RAM Filesystem
==============

This section summarises the temporary filesystem used by the \ :ref:`nanokernel <monograph>`\ .
The implementation mirrors UNIX V7 but stores the entire data structure in
SRAM for simplicity.

Block Size
----------

``FS_BLOCK_SIZE`` defines the block granularity at :c:macro:`32` bytes. Only
``FS_NUM_BLOCKS`` blocks exist, yielding a 512 byte disk image suitable for
resource-constrained demonstrations.

Inode Layout
------------

Each file or directory is represented by a :c:type:`dinode_t` structure
exposed in :file:`include/fs.h`:

.. code-block:: c

   typedef struct {
       uint8_t  type;   /* 0 = free, 1 = file, 2 = directory */
       uint8_t  nlink;  /* reference count                 */
       uint16_t size;   /* file size in bytes               */
       uint16_t addrs[4]; /* direct block addresses          */
   } dinode_t;

A fixed array of ``FS_NUM_INODES`` inodes is allocated at startup. Inode ``0``
is reserved for the root directory.

Sample API Usage
----------------

The convenience wrapper :file:`examples/fs_demo.c` shows typical usage. A
condensed snippet is reproduced below:

.. code-block:: c

   fs_init();
   fs_create("greeting.txt", 1);      /* regular file */
   file_t f;
   fs_open("greeting.txt", &f);
   fs_write(&f, "hello", 5);

   f.off = 0;
   char buf[6];
   int n = fs_read(&f, buf, sizeof buf - 1);
   buf[n] = '\0';

Limitations
-----------

* Entirely RAM-backed; contents vanish on reset.
* Flat directory only—no support for subdirectories or indirect blocks.


ROMFS (Flash)
-------------

The ROMFS implementation stores directories and file data directly in
program memory.  Every directory entry and file descriptor occupies
four bytes and resides in flash, allowing complex trees without
allocating SRAM.  A directory points either to a list of child entries
or to a file descriptor.  The root directory included with the
repository demonstrates three levels of nesting::

   /
   ├── README.TXT
   ├── etc/
   │   └── config.txt
   └── usr/
       └── bin/
           └── hello

Access files via :c:func:`romfs_open` and copy bytes with
:c:func:`romfs_read`.  The data is fetched using ``LPM`` instructions on
AVR and simple pointer indirection on the host.  No dynamic memory is
required.

EEPROM Filesystem
=================
Small calibration values or user settings may be baked into EEPROM at
programming time. The ``eepfs`` API mirrors ``romfs`` but uses the
``eeprom_read_*`` routines for retrieval.

.. code-block:: c

   const eepfs_file_t *ef = eepfs_open("/sys/message.txt");
   char msg[12];
   eepfs_read(ef, 0, msg, sizeof msg);
