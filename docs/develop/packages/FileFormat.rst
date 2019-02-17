=========================
Haiku Package File Format
=========================

.. contents::
  :depth: 2
  :backlinks: none

This document specifies the Haiku Package (HPKG) file format, which was designed
for efficient use by Haiku's package file system. It is somewhat inspired by the
`XAR format`_ (separate TOC and data heap), but aims for greater compactness
(no XML for the TOC).

.. _XAR format: http://code.google.com/p/xar/

Three stacked format layers can be identified:

- A generic container format for structured data.
- An archive format specifying how file system data are stored in the container.
- A package format, extending the archive format with attributes for package
  management.

The Data Container Format
=========================
A HPKG file consists of four sections:

Header
  Identifies the file as HPKG file and provides access to the other sections.

Heap
  Contains arbitrary (mostly unstructured) data referenced by the next two
  sections.

TOC (table of contents)
  The main section, containing structured data with references to unstructured
  data in the heap section.

Package Attributes
  A section similar to the TOC. Rather than describing the data contained in
  the file, it specifies meta data of the package as a whole.

The TOC and Package Attributes sections aren't really separate sections, as they
are stored at the end of the heap.

All numbers in the HPKG are stored in big endian format or `LEB128`_ encoding.

.. _LEB128: http://en.wikipedia.org/wiki/LEB128

Header
------
The header has the following structure::

  struct hpkg_header {
  	uint32	magic;
  	uint16	header_size;
  	uint16	version;
  	uint64	total_size;
  	uint16	minor_version;

  	uint16	heap_compression;
  	uint32	heap_chunk_size;
  	uint64	heap_size_compressed;
  	uint64	heap_size_uncompressed;

  	uint32	attributes_length;
  	uint32	attributes_strings_length;
  	uint32	attributes_strings_count;
  	uint32	reserved1;

  	uint64	toc_length;
  	uint64	toc_strings_length;
  	uint64	toc_strings_count;
  };

magic
  The string 'hpkg' (B_HPKG_MAGIC).

header_size
  The size of the header. This is also the absolute offset of the heap.

version
  The version of the HPKG format the file conforms to. The current version is
  2 (B_HPKG_VERSION).

total_size
  The total file size.

minor_version
  The minor version of the HPKG format the file conforms to. The current minor
  version is 0 (B_HPKG_MINOR_VERSION). Additions of new attributes to the
  attributes or TOC sections should generally only increment the minor version.
  When a file with a greater minor version is encountered, the reader should
  ignore unknown attributes.

..

heap_compression
  Compression format used for the heap.

heap_chunk_size
  The size of the chunks the uncompressed heap data are divided into.

heap_size_compressed
  The compressed size of the heap. This includes all administrative data (the
  chunk size array).

heap_size_uncompressed
  The uncompressed size of the heap. This is only the size of the raw data
  (including the TOC and attributes section), not including administrative data
  (the chunk size array).

..

attributes_length
  The uncompressed size of the package attributes section.

attributes_strings_length
  The size of the strings subsection of the package attributes section.

attributes_strings_count
  The number of entries in the strings subsection of the package attributes
  section.

..

reserved1
  Reserved for later use.

..

toc_length
  The uncompressed size of the TOC section.

toc_strings_length
  The size of the strings subsection of the TOC section.

toc_strings_count
  The number of entries in the strings subsection of the TOC section.

Heap
----
The heap provides storage for arbitrary data. Data from various sources are
concatenated without padding or separator, forming the uncompressed heap. A
specific section of data is usually referenced (e.g. in the TOC and attributes
sections) by an offset and the number of bytes. These references always point
into the uncompressed heap, even if the heap is actually stored in a compressed
format. The ``heap_compression`` field in the header specifies which format is
used. The following values are defined:

= ======================= =======================
0 B_HPKG_COMPRESSION_NONE no compression
1 B_HPKG_COMPRESSION_ZLIB zlib (LZ77) compression
= ======================= =======================

The uncompressed heap data are divided into equally sized chunks (64 KiB). The
last chunk in the heap may have a different uncompressed length from the
preceding chunks. The uncompressed length of the last chunk can be derived. Each
individual chunk may be stored compressed or not.

Unless B_HPKG_COMPRESSION_NONE is specified, a uint16 array at the end of the
heap contains the actual in-file (compressed) size of each chunk (minus 1 -- 0
means 1 byte), save for the last one, which is omitted since it is implied. A
chunk is only stored compressed, if compression actually saves space. That is
if the chunk's compressed size equals its uncompressed size, the data aren't
compressed. If B_HPKG_COMPRESSION_NONE is specified, the chunk size table is
omitted entirely.

The TOC and the package attributes sections are stored (in this order) at the
end of the uncompressed heap. The offset of the package attributes section data
is therefore ``heap_size_uncompressed - attributes_length`` and the offset of
the TOC section data
``heap_size_uncompressed - attributes_length - toc_length``.

TOC
---
The TOC section contains a list of attribute trees. An attribute has an ID, a
data type, and a value, and can have child attributes. E.g.:

- ATTRIBUTE_ID_SHOPPING_LIST : string : "bakery"

  - ATTRIBUTE_ID_ITEM : string : "rye bread"
  - ATTRIBUTE_ID_ITEM : string : "bread roll"

    - ATTRIBUTE_ID_COUNT : int : 10

  - ATTRIBUTE_ID_ITEM : string : "cookie"

    - ATTRIBUTE_ID_COUNT : int : 5

- ATTRIBUTE_ID_SHOPPING_LIST : string : "hardware store"

  - ATTRIBUTE_ID_ITEM : string : "hammer"
  - ATTRIBUTE_ID_ITEM : string : "nail"

    - ATTRIBUTE_ID_SIZE : int : 10
    - ATTRIBUTE_ID_COUNT : int : 100

The main TOC section refers to any attribute by its unique ID (see below) and
stores the attribute's value, either as a reference into the heap or as inline
data.

An optimization exists for shared string attribute values. A string value used
by more than one attribute is stored in the strings subsection and is referenced
by an index.

Hence the TOC section consists of two subsections:

Strings
  A table of commonly used strings.

Main TOC
  The attribute trees.

Attribute Data Types
`````````````````````
These are the specified data type values for attributes:

= ============================= =================
0 B_HPKG_ATTRIBUTE_TYPE_INVALID invalid
1 B_HPKG_ATTRIBUTE_TYPE_INT     signed integer
2 B_HPKG_ATTRIBUTE_TYPE_UINT    unsigned integer
3 B_HPKG_ATTRIBUTE_TYPE_STRING  UTF-8 string
4 B_HPKG_ATTRIBUTE_TYPE_RAW     raw data
= ============================= =================

Strings
```````
The strings subsections consists of a list of null-terminated UTF-8 strings. The
section itself is terminated by a 0 byte.

Each string is implicitly assigned the (null-based) index at which it appears in
the list, i.e. the nth string has the index n - 1. The string is referenced by
this index in the main TOC subsection.

Main TOC
````````
The main TOC subsection consists of a list of attribute entries terminated by a
0 byte. An attribute entry is stored as:

Attribute tag
  An unsigned LEB128 encoded number.

Attribute value
  The value of the attribute encoded as described below.

Attribute child list
  Only if this attribute is marked to have children: A list of attribute entries
  terminated by a 0 byte.

The attribute tag encodes four pieces of information::

  (encoding << 11) + (hasChildren << 10) + (dataType << 7) + id + 1

encoding
  Specifies the encoding of the attribute value as described below.

hasChildren
  1, if the attribute has children, 0 otherwise.

dataType
  The data type of the attribute (B_HPKG_ATTRIBUTE_TYPE\_...).

id
  The ID of the attribute (B_HPKG_ATTRIBUTE_ID\_...).

Attribute Values
````````````````
A value of each of the data types can be encoded in different ways, which is
defined by the encoding value:

- B_HPKG_ATTRIBUTE_TYPE_INT and B_HPKG_ATTRIBUTE_TYPE_UINT:

  = ==================================== ============
  0 B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT  int8/uint8
  1 B_HPKG_ATTRIBUTE_ENCODING_INT_16_BIT int16/uint16
  2 B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT int32/uint32
  3 B_HPKG_ATTRIBUTE_ENCODING_INT_64_BIT int64/uint64
  = ==================================== ============

- B_HPKG_ATTRIBUTE_TYPE_STRING:

  = ======================================= ==================================
  0 B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE null-terminated UTF-8 string
  1 B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE  unsigned LEB128: index into string
                                            table
  = ======================================= ==================================

- B_HPKG_ATTRIBUTE_TYPE_RAW:

  = ==================================== =======================================
  0 B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE unsigned LEB128: size; followed by raw
                                         bytes
  1 B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP   unsigned LEB128: size; unsigned LEB128:
                                         offset into the uncompressed heap
  = ==================================== =======================================

Package Attributes
------------------
The package attributes section contains a list of attribute trees, just like the
TOC section. The structure of this section follows the TOC, i.e. there's a
subsection for shared strings and a subsection that stores a list of attribute
entries terminated by a 0 byte. An entry has the same format as the ones in the
TOC (only using different attribute IDs).

The Archive Format
==================
This section specifies how file system objects (files, directories, symlinks)
are stored in a HPKG file. It builds on top of the container format, defining
the types of attributes, their order, and allowed values.

E.g. a "bin" directory, containing a symlink and a file::

  bin           0  2009-11-13 12:12:09  drwxr-xr-x
    awk         0  2009-11-13 12:11:16  lrwxrwxrwx  -> gawk
    gawk   301699  2009-11-13 12:11:16  -rwxr-xr-x

could be represented by this attribute tree:

- B_HPKG_ATTRIBUTE_ID_DIR_ENTRY : string : "bin"

  - B_HPKG_ATTRIBUTE_ID_FILE_TYPE : uint : 1 (0x1)
  - B_HPKG_ATTRIBUTE_ID_FILE_MTIME : uint : 1258110729 (0x4afd3f09)
  - B_HPKG_ATTRIBUTE_ID_DIR_ENTRY : string : "awk"

    - B_HPKG_ATTRIBUTE_ID_FILE_TYPE : uint : 2 (0x2)
    - B_HPKG_ATTRIBUTE_ID_FILE_MTIME : uint : 1258110676 (0x4afd3ed4)
    - B_HPKG_ATTRIBUTE_ID_SYMLINK_PATH : string : "gawk"

  - B_HPKG_ATTRIBUTE_ID_DIR_ENTRY : string : "gawk"

    - B_HPKG_ATTRIBUTE_ID_FILE_PERMISSIONS : uint : 493 (0x1ed)
    - B_HPKG_ATTRIBUTE_ID_FILE_MTIME : uint : 1258110676 (0x4afd3ed4)
    - B_HPKG_ATTRIBUTE_ID_DATA : raw : size: 301699, offset: 0
    - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE : string : "BEOS:APP_VERSION"

      - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE : uint : 1095782486 (0x41505056)
      - B_HPKG_ATTRIBUTE_ID_DATA : raw : size: 680, offset: 301699

    - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE : string : "BEOS:TYPE"

      - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE : uint : 1296649555 (0x4d494d53)
      - B_HPKG_ATTRIBUTE_ID_DATA : raw : size: 35, offset: 302379

Attribute IDs
-------------
B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY ("dir:entry")
  :Type: string
  :Value: File name of the entry.
  :Allowed Values: Any valid file (not path!) name, save "." and "..".
  :Child Attributes:

    - B_HPKG_ATTRIBUTE_ID_FILE_TYPE: The file type of the entry.
    - B_HPKG_ATTRIBUTE_ID_FILE_PERMISSIONS: The file permissions of the entry.
    - B_HPKG_ATTRIBUTE_ID_FILE_USER: The owning user of the entry.
    - B_HPKG_ATTRIBUTE_ID_FILE_GROUP: The owning group of the entry.
    - B_HPKG_ATTRIBUTE_ID_FILE_ATIME[_NANOS]: The entry's file access time.
    - B_HPKG_ATTRIBUTE_ID_FILE_MTIME[_NANOS]: The entry's file modification
      time.
    - B_HPKG_ATTRIBUTE_ID_FILE_CRTIME[_NANOS]: The entry's file creation time.
    - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE: An extended file attribute associated
      with entry.
    - B_HPKG_ATTRIBUTE_ID_DATA: Only if the entry is a file: The file data.
    - B_HPKG_ATTRIBUTE_ID_SYMLINK_PATH: Only if the entry is a symlink: The path
      the symlink points to.
    - B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY: Only if the entry is a directory: A
      child entry in that directory.

B_HPKG_ATTRIBUTE_ID_FILE_TYPE ("file\:type")
  :Type: uint
  :Value: Type of the entry.
  :Allowed Values:

    = ========================== =========
    0 B_HPKG_FILE_TYPE_FILE      file
    1 B_HPKG_FILE_TYPE_DIRECTORY directory
    2 B_HPKG_FILE_TYPE_SYMLINK   symlink
    = ========================== =========

  :Default Value: B_HPKG_FILE_TYPE_FILE
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_PERMISSIONS ("file\:permissions")
  :Type: uint
  :Value: File permissions.
  :Allowed Values: Any valid permission mask.
  :Default Value:

    - For files: 0644 (octal).
    - For directories: 0755 (octal).
    - For symlinks: 0777 (octal).

  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_USER ("file\:user")
  :Type: string
  :Value: Name of the user owning the file.
  :Allowed Values: Any non-empty string.
  :Default Value: The user owning the installation location where the package is
    activated.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_GROUP ("file\:group")
  :Type: string
  :Value: Name of the group owning the file.
  :Allowed Values: Any non-empty string.
  :Default Value: The group owning the installation location where the package
    is activated.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_ATIME ("file\:atime")
  :Type: uint
  :Value: File access time (seconds since the Epoch).
  :Allowed Values: Any value.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_ATIME_NANOS ("file\:mtime:nanos")
  :Type: uint
  :Value: The nano seconds fraction of the file access time.
  :Allowed Values: Any value in [0, 999999999].
  :Default Value: 0
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_MTIME ("file\:mtime")
  :Type: uint
  :Value: File modified time (seconds since the Epoch).
  :Allowed Values: Any value.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_MTIME_NANOS ("file\:mtime:nanos")
  :Type: uint
  :Value: The nano seconds fraction of the file modified time.
  :Allowed Values: Any value in [0, 999999999].
  :Default Value: 0
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_CRTIME ("file\:crtime")
  :Type: uint
  :Value: File creation time (seconds since the Epoch).
  :Allowed Values: Any value.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_CRTIM_NANOS ("file\:crtime:nanos")
  :Type: uint
  :Value: The nano seconds fraction of the file creation time.
  :Allowed Values: Any value in [0, 999999999].
  :Default Value: 0
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE ("file\:attribute")
  :Type: string
  :Value: Name of the extended file attribute.
  :Allowed Values: Any valid attribute name.
  :Child Attributes:

    - B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE: The type of the file attribute.
    - B_HPKG_ATTRIBUTE_ID_DATA: The file attribute data.

B_HPKG_ATTRIBUTE_ID_FILE_ATTRIBUTE_TYPE ("file\:attribute:type")
  :Type: uint
  :Value: Type of the file attribute.
  :Allowed Values: Any value in [0, 0xffffffff].
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_DATA ("data")
  :Type: data
  :Value: Raw data of a file or attribute.
  :Allowed Values: Any value.

B_HPKG_ATTRIBUTE_ID_SYMLINK_PATH ("symlink:path")
  :Type: string
  :Value: The path the symlink refers to.
  :Allowed Values: Any valid symlink path.
  :Default Value: Empty string.
  :Child Attributes: none

TOC Attributes
--------------
The TOC can directly contain any number of attributes of the
B_HPKG_ATTRIBUTE_ID_DIRECTORY_ENTRY type, which in turn contain descendant
attributes as specified in the previous section. Any other attributes are
ignored.

The Package Format
==================
This section specifies how informative package attributes (package-name,
version, provides, requires, ...) are stored in a HPKG file. It builds on top of
the container format, defining the types of attributes, their order, and allowed
values.

E.g. a ".PackageInfo" file, containing a package description that is being
converted into a package file::

  name		mypackage
  version	0.7.2-1
  architecture	x86
  summary	"is a very nice package"
  description	"has lots of cool features\nand is written in MyC++"
  vendor	"Me, Myself & I, Inc."
  packager	"me@test.com"
  copyrights	{ "(C) 2009-2011, Me, Myself & I, Inc." }
  licenses	{ "Me, Myself & I Commercial License"; "MIT" }
  provides {
  	cmd:me
  	lib:libmyself = 0.7
  }
  requires {
  	haiku >= r1
  	wget
  }

could be represented by this attribute tree:

- B_HPKG_ATTRIBUTE_ID_PACKAGE_NAME : string : "mypackage"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR : string : "0"

  - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR : string : "7"
  - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO : string : "2"
  - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION : uint : 1

- B_HPKG_ATTRIBUTE_ID_PACKAGE_ARCHITECTURE : uint : 1
- B_HPKG_ATTRIBUTE_ID_PACKAGE_SUMMARY : string : "is a very nice package"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_DESCRIPTION : string : "has lots of cool features
  \nand is written in MyC++"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_VENDOR : string : "Me, Myself & I, Inc."
- B_HPKG_ATTRIBUTE_ID_PACKAGE_PACKAGER : string : "me@test.com"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_COPYRIGHT : string : "(C) 2009-2011, Me, Myself &
  I, Inc."
- B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE : string : "Me, Myself & I Commercial
  License"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE : string : "MIT"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES : string : "cmd:me"
- B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES : string : "lib:libmyself"

  - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR : string : "0"

    - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR : string : "7"

- B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES : string : "haiku"

  - B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR : uint : 4
  - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR : string : "r1"

- B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES : string : "wget"

.. _The Package Format/Attribute IDs:

Attribute IDs
-------------
B_HPKG_ATTRIBUTE_ID_PACKAGE_NAME ("package:name")
  :Type: string
  :Value: Name of the package.
  :Allowed Values: Any string matching <entity_name_char>+, with
    <entity_name_char> being any character but '-', '/', '=', '!', '<', '>', or
    whitespace.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_SUMMARY ("package:summary")
  :Type: string
  :Value: Short description of the package.
  :Allowed Values: Any single-lined string.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_DESCRIPTION ("package:description")
  :Type: string
  :Value: Long description of the package.
  :Allowed Values: Any string (may contain multiple lines).
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_VENDOR ("package:vendor")
  :Type: string
  :Value: Name of the person/organization that is publishing this package.
  :Allowed Values: Any single-lined string.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_PACKAGER ("package:packager")
  :Type: string
  :Value: E-Mail address of person that created this package.
  :Allowed Values: Any single-lined string, but e-mail preferred.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_BASE_PACKAGE ("package:base-package")
  :Type: string
  :Value: Name of the package that is the base package for this package. The
    base package must also be listed as a requirement for this package (cf.
    B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES). The package manager shall ensure that
    this package is installed in the same installation location as its base
    package.
  :Allowed Values: Valid package names.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_FLAGS ("package:flags")
  :Type: uint
  :Value: Set of boolean flags applying to package.
  :Allowed Values: Any combination of the following.

    = ============================== ==========================================
    1 B_PACKAGE_FLAG_APPROVE_LICENSE this package's license requires approval
                                     (i.e. must be shown to and acknowledged by
                                     user before installation)
    2 B_PACKAGE_FLAG_SYSTEM_PACKAGE  this is a system package (i.e. lives under
                                     /boot/system)
    = ============================== ==========================================

  :Default Value: 0
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_ARCHITECTURE ("package:architecture")
  :Type: uint
  :Value: System architecture this package was built for.
  :Allowed Values:

    = =============================== =========================================
    0 B_PACKAGE_ARCHITECTURE_ANY      this package doesn't depend on the system
                                      architecture
    1 B_PACKAGE_ARCHITECTURE_X86      x86, 32-bit, built with gcc4
    2 B_PACKAGE_ARCHITECTURE_X86_GCC2 x86, 32-bit, built with gcc2
    3 B_PACKAGE_ARCHITECTURE_SOURCE   source code, doesn't depend on the system
                                      architecture
    4 B_PACKAGE_ARCHITECTURE_X86_64   x86-64
    5 B_PACKAGE_ARCHITECTURE_PPC      PowerPC
    6 B_PACKAGE_ARCHITECTURE_ARM      ARM
    7 B_PACKAGE_ARCHITECTURE_M68K     m68k
    8 B_PACKAGE_ARCHITECTURE_SPARC    sparc
    = =============================== =========================================

  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR ("package:version.major")
 :Type: string
  :Value: Major (first) part of package version.
  :Allowed Values: Any single-lined string, composed of <alphanum_underline>
  :Child Attributes:

   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR: The minor part of the package
     version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO: The micro part of the package
     version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE: The pre-release part of
     the package version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION: The revision part of the
     package version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR ("package:version.minor")
 :Type: string
 :Value: Minor (second) part of package version.
 :Allowed Values: Any single-lined string, composed of <alphanum_underline>.
 :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO ("package:version.micro")
  :Type: string
  :Value: Micro (third) part of package version.
  :Allowed Values: Any single-lined string, composed of
    <alphanum_underline_dot>.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE ("package:version.prerelease")
  :Type: string
  :Value: Pre-release (fourth) part of package version. Typically something like
    "alpha1", "beta2", "rc3".
  :Allowed Values: Any single-lined string, composed of
    <alphanum_underline_dot>.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION ("package:version.revision")
  :Type: uint
  :Value: Revision (fifth) part of package version.
  :Allowed Values: Any integer greater than 0.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_COPYRIGHT ("package:copyright")
  :Type: string
  :Value: Copyright applying to the software contained in this package.
  :Allowed Values: Any (preferably single-lined) string.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_LICENSE ("package:license")
  :Type: string
  :Value: Name of license applying to the software contained in this package.
  :Allowed Values: Any single-lined string.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_URL ("package:url")
  :Type: string
  :Value: URL of the packaged software's project home page.
  :Allowed Values: A regular URL or an email-like named URL (e.g.
    "Project Foo <http://foo.example.com>").
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_SOURCE_URL ("package:source-url")
  :Type: string
  :Value: URL of the packaged software's source code or build instructions.
  :Allowed Values: A regular URL or an email-like named URL (e.g.
    "Project Foo <http://foo.example.com>").
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES ("package:provides")
  :Type: string
  :Value: Name of a (optionally typed) entity that is being provided by this
    package.
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:

   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR: The major part of the resolvable
     version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_COMPATIBLE: The major part of the
     resolvable compatible version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_PROVIDES_COMPATIBLE ("package:provides.compatible")
  :Type: string
  :Value: Major (first) part of the resolvable compatible version, structurally
    identical to B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR.
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MINOR: The minor part of the resolvable
     compatible version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MICRO: The micro part of the resolvable
     compatible version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_PRE_RELEASE: The pre-release part of
     the resolvable compatible version.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_REVISION: The revision part of the
     resolvable compatible version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_REQUIRES ("package:requires")
  :Type: string
  :Value: Name of an entity that is required by this package (and hopefully
    being provided by another).
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR: The resolvable operator as
     int.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR: The major part of the resolvable
     version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR ("package:resolvable.operator")
  :Type: uint
  :Value: Comparison operator for versions.
  :Allowed Values:

    = ===================================== ===================================
    0 B_PACKAGE_RESOLVABLE_OP_LESS          less than the specified version
    1 B_PACKAGE_RESOLVABLE_OP_LESS_EQUAL    less than or equal to the specified
                                            version
    2 B_PACKAGE_RESOLVABLE_OP_EQUAL         equal to the specified version
    3 B_PACKAGE_RESOLVABLE_OP_NOT_EQUAL     not equal to the specified version
    4 B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL greater than the specified version
    5 B_PACKAGE_RESOLVABLE_OP_GREATER       greater than or equal to the
                                            specified version
    = ===================================== ===================================

  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_SUPPLEMENTS ("package:supplements")
  :Type: string
  :Value: Name of an entity that is supplemented by this package (i.e. this
    package will automatically be selected for installation if the supplemented
    resolvables are already installed).
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR: The resolvable operator as
     int.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR: The major part of the resolvable
     version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_CONFLICTS ("package:conflicts")
  :Type: string
  :Value: Name of an entity that this package conflicts with (i.e. only one of
    both can be installed at any time).
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR: The resolvable operator as
     int.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR: The major part of the resolvable
     version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_FRESHENS ("package:freshens")
  :Type: string
  :Value: Name of an entity that is being freshened by this package (i.e. this
    package will patch one or more files of the package that provide this
    resolvable).
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_RESOLVABLE_OPERATOR: The resolvable operator as
     int.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_VERSION_MAJOR: The major part of the resolvable
     version.

B_HPKG_ATTRIBUTE_ID_PACKAGE_REPLACES ("package:replaces")
  :Type: string
  :Value: Name of an entity that is being replaced by this package (used if the
    name of a package changes, or if a package has been split).
  :Allowed Values: Any string matching <entity_name_char>+.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_CHECKSUM ("package:checksum")
  :Type: string
  :Value: SHA256-chechsum of this package, in hexdump format. N.B.: this
    attribute can only be found in package repository files, not in package
    files.
  :Allowed Values: 64-bytes of hexdump.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_GLOBAL_WRITABLE_FILE ("package:global-writable-file")
  :Type: string
  :Value: Relative path of a global writable file either included in the package
    or created by the included software. If the file is included in the package,
    it will be installed upon activation. In this case the attribute must
    contain a B_HPKG_ATTRIBUTE_ID_PACKAGE_WRITABLE_FILE_UPDATE_TYPE child
    attribute. The file may actually be a directory, which is indicated by the
    B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY child attribute.
  :Allowed Values: Installation location relative path (e.g. "settings/...").
  :Child Attributes:
    - B_HPKG_ATTRIBUTE_ID_PACKAGE_WRITABLE_FILE_UPDATE_TYPE: Specifies what to do
      with the writable file on package update.
    - B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY: Specifies whether the
      file is actually a directory.

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SETTINGS_FILE ("package:user-settings-file")
  :Type: string
  :Value: Relative path of a user settings file created by the included software
    or required by the software to be created by the user. The file may actually
    be a directory, which is indicated by the
    B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY child attribute.
  :Allowed Values: Installation location relative path (i.e. "settings/...").
  :Child Attributes:
    - B_HPKG_ATTRIBUTE_ID_PACKAGE_SETTINGS_FILE_TEMPLATE: A template for the
      settings file.
    - B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY: Specifies whether the
      file is actually a directory.

B_HPKG_ATTRIBUTE_ID_PACKAGE_WRITABLE_FILE_UPDATE_TYPE ("package:writable-file-update-type")
  :Type: uint
  :Value: Specifies what to do on package update when the writable file provided
    by the package has been changed by the user.
  :Allowed Values:

    = ====================================== ==================================
    0 B_WRITABLE_FILE_UPDATE_TYPE_KEEP_OLD   the old file shall be kept
    1 B_WRITABLE_FILE_UPDATE_TYPE_MANUAL     the old file needs to be updated
                                             manually
    2 B_WRITABLE_FILE_UPDATE_TYPE_AUTO_MERGE an automatic three-way merge shall
                                             be attempted
    = ====================================== ==================================

  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_IS_WRITABLE_DIRECTORY ("package:is-writable-directory")
  :Type: uint
  :Value: Specifies whether the parent global writable file or user settings file attribute actually refers to a directory.
  :Allowed Values:

    = ===========================================
    0 The parent attribute refers to a file.
    1 The parent attribute refers to a directory.
    = ===========================================

  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_SETTINGS_FILE_TEMPLATE ("package:settings-file-template")
  :Type: string
  :Value: Relative path of an included template file for the user settings file.
  :Allowed Values: Installation location relative path of a file included in the
    package.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER ("package:user")
  :Type: string
  :Value: Name of a user required by the package. Upon package activation the
    user will be created, if necessary.
  :Allowed Values: Any valid user name, i.e. must be non-empty composed of
    <alphanum_underline>.
  :Child Attributes:
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_REAL_NAME: The user's real name.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_HOME: The user's home directory.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SHELL: The user's shell.
   - B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_GROUP: The user's group(s).

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_REAL_NAME ("package:user.real-name")
  :Type: string
  :Value: The real name of the user.
  :Allowed Values: Any string.
  :Default Value: The user name.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_HOME ("package:user.home")
  :Type: string
  :Value: The path to the home directory of the user.
  :Allowed Values: Any valid path.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_SHELL ("package:user.shell")
  :Type: string
  :Value: The path to the shell to be used for the user.
  :Allowed Values: Any valid path.
  :Default Value: "/bin/bash".
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_USER_GROUP ("package:user.group")
  :Type: string
  :Value: A group the user belongs to. At least one must be specified.
  :Allowed Values: Any valid group name, i.e. must be non-empty composed of
    <alphanum_underline>.
  :Default Value: The default group for users.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_GROUP ("package:group")
  :Type: string
  :Value: Name of a group required by the package. Upon package activation the
    group will be created, if necessary.
  :Allowed Values: Any valid group name, i.e. must be non-empty composed of
    <alphanum_underline>.
  :Child Attributes: none

B_HPKG_ATTRIBUTE_ID_PACKAGE_POST_INSTALL_SCRIPT ("package:post-install-script")
  :Type: string
  :Value: Relative path of a script that shall be executed after package
    activation.
  :Allowed Values: Installation location relative path of a file included in the
    package.
  :Child Attributes: none

Haiku Package Repository Format
===============================
Very similar to the package format, there's a Haiku Package Repository (HPKR)
file format. Such a file contains informative attributes about the package
repository and package attributes for all packages contained in the repository.
However, this format does not contain any files.

Two stacked format layers can be identified:

- A generic container format for structured data.
- A package format, extending the archive format with attributes for package
  management.

The Data Container Format
-------------------------
A HPKR file consists of three sections:

Header
  Identifies the file as HPKR file and provides access to the other sections.

Heap
  Contains the next two sections.

Repository Info
  A section containing an archived BMessage of a BRepositoryInfo object.

Package Attributes
  A section just like the package attributes section of the HPKG, only that this
  section contains the package attributes of all the packages contained in the
  repository (not just one).

The Repository Info and Package Attributes sections aren't really separate
sections, as they are stored at the end of the heap.

Header
``````
The header has the following structure::

  struct hpkg_repo_header {
  	uint32	magic;
  	uint16	header_size;
  	uint16	version;
  	uint64	total_size;
  	uint16	minor_version;

  	// heap
  	uint16	heap_compression;
  	uint32	heap_chunk_size;
  	uint64	heap_size_compressed;
  	uint64	heap_size_uncompressed;

  	// repository info section
  	uint32	info_length;
  	uint32	reserved1;

  	// package attributes section
  	uint64	packages_length;
  	uint64	packages_strings_length;
  	uint64	packages_strings_count;
  };

magic
  The string 'hpkr' (B_HPKG_REPO_MAGIC).

header_size
  The size of the header. This is also the absolute offset of the heap.

version
  The version of the HPKR format the file conforms to. The current version is
  2 (B_HPKG_REPO_VERSION).

total_size
  The total file size.

minor_version
  The minor version of the HPKR format the file conforms to. The current minor
  version is 0 (B_HPKG_REPO_MINOR_VERSION). Additions of new attributes to the
  attributes section shouldgenerally only increment the minor version. When a
  file with a greater minor version is encountered, the reader should ignore
  unknown attributes.

..

heap_compression
  Compression format used for the heap.

heap_chunk_size
  The size of the chunks the uncompressed heap data are divided into.

heap_size_compressed
  The compressed size of the heap. This includes all administrative data (the
  chunk size array).

heap_size_uncompressed
  The uncompressed size of the heap. This is only the size of the raw data
  (including the repository info and attributes section), not including
  administrative data (the chunk size array).

..

info_length
  The uncompressed size of the repository info section.

..

reserved1
  Reserved for later use.

..

packages_length
  The uncompressed size of the package attributes section.

packages_strings_length
  The size of the strings subsection of the package attributes section.

packages_strings_count
  The number of entries in the strings subsection of the package attributes
  section.

Attribute IDs
-------------
The package repository format defines only the top-level attribute ID
B_HPKG_ATTRIBUTE_ID_PACKAGE. An attribute with that ID represents a package. Its
child attributes specify the various meta information for the package as defined
in the `The Package Format/Attribute IDs`_ section.

