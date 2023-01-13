The XFS File System
===================

This document describes how to test the XFS file system, XFS file system API for haiku
and Its current status on haiku.


Testing XFS File System
-----------------------

There are three ways we can test XFS :

-  Using xfs_shell.
-  Using userlandfs.
-  Building a version of haiku with XFS support and then mounting a file system.

But before that we will need to create XFS images for all testing purposes.

Creating File System Images
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Currently only linux has full XFS support so we will use linux for generating file system images.

First we need to create an empty sparse image using command::

   $ dd if=/dev/zero of=fs.img count=0 bs=1 seek=5G

The output will be::

   0+0 records in
   0+0 records out
   0 bytes (0 B) copied, 0.000133533 s, 0.0 kB/s

Do note that we can create images of whatever size or name we want, for example the above command
creates fs.img of size 5 GB, if we alter seek = 10G it will create fs.img with size 10 GB.

The XFS file system on linux supports two versions, V4 and V5.

To put XFS V5 file system on our sparse image run command::

   $ /sbin/mkfs.xfs fs.img

The output will be::

   meta-data   =fs.img                 isize=512    agcount=4, agsize=65536 blks
               =                       sectsz=512   attr=2, projid32bit=1
               =                       crc=1        finobt=1, sparse=1, rmapbt=0
               =                       reflink=1
   data        =                       bsize=4096   blocks=262144, imaxpct=25
               =                       sunit=0      swidth=0 blks
   naming      =version 2              bsize=4096   ascii-ci=0, ftype=1
   log         =internal log           bsize=4096   blocks=2560, version=2
               =                       sectsz=512   sunit=0 blks, lazy-count=1
   realtime    =none                   extsz=4096   blocks=0, rtextents=0

To put XFS V4 file system on our sparse image run command::

   $ /sbin/mkfs.xfs -m crc=0 file.img

The output will be::

    meta-data=fs.img                 isize=256    agcount=4, agsize=327680 blks
             =                       sectsz=512   attr=2, projid32bit=0
    data     =                       bsize=4096   blocks=1310720, imaxpct=25
             =                       sunit=0      swidth=0 blks
    naming   =version 2              bsize=4096   ascii-ci=0
    log      =internal log           bsize=4096   blocks=2560, version=2
             =                       sectsz=512   sunit=0 blks, lazy-count=1
    realtime =none                   extsz=4096   blocks=0, rtextents=0

**The linux kernel will support older XFS v4 filesystems by default until 2025 and
Support for the V4 format will be removed entirely in September 2030**

Now we can mount our file system image and create entries for testing XFS haiku driver.

Test using xfs_shell
^^^^^^^^^^^^^^^^^^^^^^^

The idea of fs_shell is to run the file system code outside of haiku. We can run it
as an application, it provides a simple command line interface to perform various
operations on the file system (list directories, read and display files, etc).

First we have to compile it::

   jam "<build>xfs_shell"

Then run it::

   jam run ":<build>xfs_shell" fs.img

Where fs.img is the file system image we created from linux kernel.

Test directly inside Haiku
^^^^^^^^^^^^^^^^^^^^^^^^^^

First build a version of haiku with XFS support, to do this we need to add "xfs" to the `image
definition <https://git.haiku-os.org/haiku/tree/build/jam/images/definitions/minimum#n239>`__.

Then compile haiku as usual and run the resulting system in a virtual machine or on real hardware.

We can then try to mount an XFS file system using command on Haiku::

   mount -t xfs <path to image> <path to mount folder>

for example::

   mount -t xfs /boot/home/Desktop/fs.img /boot/home/Desktop/Testing

Here fs.img is file system image and Testing is mount point.

Test using userlandfs
^^^^^^^^^^^^^^^^^^^^^

To be updated


Haiku XFS API
-------------

All the necessary hooks for file system like xfs_mount(), open_dir(), read_dir() etc.. are
implemented in the **kernel_interface.cpp** file. It acts as an interface between the Haiku kernel
and the XFS file system. Documentation for all necessary file system hooks can be found
`in the API reference <https://www.haiku-os.org/docs/api/fs_modules.html>`_

Whenever we run a file system under fs_shell we can't use system headers, fs_shell compatible
headers are there which needs to be used whenever we try to mount XFS file system using xfs_shell.
To resolve this problem we use **system_dependencies.h** header file which takes care to use
correct headers whenever we mount XFS file system either using xfs_shell or directly inside Haiku.

XFS stores data on disk in Big Endian byte order, to convert data into host order
all classes and data headers has **SwapEndian()** function, Its better to have all data
conversions at one place to avoid future problems related to data byte order.

XFS SuperBlock starts at ondisk offset 0, the definition of SuperBlock is in **xfs.h** file.

A Volume is an instance of file system and defined in **Volume.h** file.
XFS Volume contains SuperBlock, file system device and essential functions
like Identify(), mount() etc...

*  *Identify()* function reads SuperBlock from disk and verifies it.
*  *Mount()* function mounts file system device and publishes root inode of file system
   (Typically root inode number for XFS is 128).

XFS uses TRACE Macro to debug file system, definitions for TRACE, ERROR and ASSERT
are defined at **Debug.h** in the form of Macro.

To enable TRACE calls just add ``#define TRACE_XFS`` in Debug.h file and
vice versa to disable it.


XFS V5 introduced metadata checksums to ensure the integrity of metadata in file system,
It uses CRC32C checksum algorithm. For XFS all checksums related functions are defined in
**Checksum.h** header file.
It contains following functions :

*  *xfs_verify_cksum()* to verify checksum for buffer.
*  *xfs_update_cksum()* to update checksum for buffer.

**XFS stores checksum in little endian byte order unlike other ondisk data which is stored
in big endian byte order**

XFS V5 introduced many other fields for metadata verification like *BlockNo* *UUID* *Owner*
etc.. All this fields are common in every data header and so are their checks. So to not
repeat same checks again and again for all headers we created a *VerifyHeader* template
function which is defined in **VerifyHeader.h** file. This function is commonly used in
all forms of headers for verification purposes.

Inodes
^^^^^^

XFS inodes comes in three versions:

*  Inode V1 & V2. (Version 4 XFS)
*  Inode V3. (Version 5 XFS)

Version 1 inode support is already deprecated on linux kernel, Haiku XFS supports it only
in read format. When we will have write support for XFS we will only support V2 and V3 inodes.

V1 & V2 inodes are 256 bytes while V3 inodes are 512 bytes in size allowing more data to be
stored directly inside inode.

**CoreInodeSize()** is a helper funtion which returns size of inode based on version of XFS and
is used throughout our XFS code.

**DIR_DFORK_PTR** is a Macro which expands to void pointer to the data offset in inode, which
could be either shortform entries, extents or B+Tree root node depending on the data format
of inode (di_format).

Similarly **DIR_AFORK_PTR** Macro expands to void pointer to the attribute offset in inode,
which could be either shortform attributes, attributes extents or B+Tree node depending on
the attribute format of Inode (di_aformat).

Since size of inodes could differ based on different versions of XFS we pass CoreInodeSize()
function as a parameter to DIR_DFORK_PTR and DIR_AFORK_PTR macros to return correct pointer offset.

**di_forkoff** specifies the offset into the inode's literal area where the extended attribute
fork starts. This value is initially zero until an extended attribute is created.
It is fixed for V1 & V2 inode's while for V3 Inodes it is dynamic in size,
allowing complete use of inode's literal area.

Directories
^^^^^^^^^^^

Depending on the number of entries inside directory, XFS divides directories into five formats :

*  Shortform directory.
*  Block directory.
*  Leaf directory.
*  Node directory.
*  B+Tree directorcy.

Class DirectoryIterator in **Directory.h** file provides an interface between kernel request
to open, read directory and all forms of directories. It first identifies correct format of
entries inside inode and then returns request as per format found.

**Shortform directory**

*  When the number of entries inside directory are small enough such that we can store all
   metadata inside inode itself, this form of directory is known as shortform directory.
*  We can check if a directory is shortform if the format of inode is *XFS_DINODE_FMT_LOCAL*.
*  The header for ShortForm entries is located at data fork pointer inside inode, which we cast
   directly to *ShortFormHeader*.
*  Since number of entries are short we can simply iterate over all entries for *Lookup()* and
   *GetNext()* functions.

**Block directory**

*  When number of entries expand such that we can no longer store all directory metadata
   inside inode we use extents.
*  We can check if a directory is extent based if the format of inode is *XFS_DINODE_FMT_EXTENTS*.
*  In Block directory we have a single directory block for Data header, leaf header
   and free data header. This simple fact helps us to determine if given extent format
   in inode is block directory.
*  Since XFS V4 & V5 data headers differs we use a virtual class *ExtentDataHeader* which
   acts as an interface between V4 & V5 data header, this class only stores pure virtual
   functions and no data.
*  *CreateDataHeader* returns a class instance based on the version of XFS mounted.
*  Since now we have a virtual class with V_PTRS we need to be very careful with data stored
   ondisk and data inside class, for example we now can't use sizeof() operator on class to
   return its size which is consistent with its size inside disk. To handle this issue helper
   function like *SizeOfDataHeader* are created which needs to be used instead of sizeof() operator.
*  In *GetNext()* function we simply iterate over all entries inside buffer, though a found
   entry could be unused entry so we need to have checks if a entry found is proper entry.
*  In *Lookup()* function first we generate a hash value of entry for lookup, then we find
   lowerbound of this hash value inside leaf entries to get address of entry inside data.
   At last if entry matches we return B_OK else we return B_ENTRY_NOT_FOUND.

**Leaf directory**

*  When number of entries expand such that we can no longer store all directory metadata inside
   directory block we use leaf format.
*  In leaf directory we have a multiple directory block for Data header and free data header,
   while single directory block for leaf header.
*  To check if given extent based inode is leaf type, we simply check for offset inside last
   extent map, if its equal to *LEAF_STARTOFFSET* then the given inode is leaf type else it is
   node type.
*  Since XFS V4 & V5 leaf headers differs we use a virtual class *ExtentLeafHeader* which acts
   as an interface between V4 & V5 leaf header, this class only stores pure virtual functions
   and no data.
*  *CreateLeafHeader* returns a class instance based on the version of XFS mounted.
*  Instead of sizeof() operator on ExtentLeafHeader we should always use *SizeOfLeafHeader()* function
   to return correct size of class inside disk.
*  *Lookup()* and *GetNext()* functions are similar to block directories except now we don't use single
   directory block buffer.

TODO : Document Node and B+Tree based directories.

Files
^^^^^

XFS stores files in two formats :

*  Extent based file.
*  B+Tree based file.

All implementation of read support for files is inside *Inode()* class in **Inode.h** file.

When the format inside inode of file is *XFS_DINODE_FMT_EXTENTS* it is an extent based file,
to read all data of file we simply iterate over all extents which is very similar to how we
do it in Extent based directories.

When the file becomes too large such that we cannot store more extent maps inside inode the
format of file is changed to B+Tree. When the format inside inode of file is
*XFS_DINODE_FMT_BTREE* it is an B+Tree based file, to read all data of file
first we read blocks of B+Tree to extract extent maps and then read extents
to get file's data.


Current Status of XFS
---------------------

Currently we only have read support for XFS, below briefly summarises read support for all formats.


Directories
^^^^^^^^^^^

**Short-Directory**
   Stable read support for both V4 and V5 inside Haiku.

**Block-Directory**
   Stable read support for both V4 and V5 inside Haiku.

**Leaf-Directory**
   Stable read support for both V4 and V5 inside Haiku.

**Node-Directory**
   Stable read support for both V4 and V5 inside Haiku.

**B+Tree-Directory**
   Unstable read support for both V4 and V5, due to so many read from disk entire
   process inside Haiku is too slow.

Files
^^^^^

**Extent based Files**
   |  *xfs_shell* - stable read support for both V4 and V5.
   |  *Haiku* - Unstable, Cat command doesn't print entire file and never terminates process.

**B+Tree based Files**
   |  *xfs_shell* - stable read support for both V4 and V5.
   |  *Haiku* - Unstable, Cat command doesn't print entire file and never terminates process.

Attributes
^^^^^^^^^^

Currently we have no extended attributes support for xfs.

Symlinks
^^^^^^^^

Currently we have no symlinks support for xfs.

XFS V5 exclusive features
^^^^^^^^^^^^^^^^^^^^^^^^^

**MetaData Checksumming**
   Metadata checksums for superblock, Inodes, and data headers are implemented.

**Big Timestamps**
   Currently we have no support.

**Reverse mapping btree**
   Currently we have no support, this data structure is still under construction
   and testing inside linux kernel.

**Refrence count btree**
   Currently we have no support, this data structure is still under construction
   and testing inside linux kernel.

Write Support
^^^^^^^^^^^^^

Currently we have no write support for xfs.


References
----------

The best and only reference for xfs is latest version of "xfs_filesystem_structure"
written by Linux-XFS developers.

The pdf version of above Doc can be found
`here <http://ftp.ntu.edu.tw/linux/utils/fs/xfs/docs/xfs_filesystem_structure.pdf>`_
