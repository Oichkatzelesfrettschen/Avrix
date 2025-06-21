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

