/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU_PACKAGE_H
#define _HAIKU_PACKAGE_H


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


// header
struct hpkg_header {
	uint32	magic;							// "hpkg"
	uint16	header_size;
	uint16	version;
	uint64	total_size;

	// package attributes section
	uint32	attributes_compression;
	uint32	attributes_length_compressed;
	uint32	attributes_length_uncompressed;

	// TOC section
	uint32	toc_compression;
	uint64	toc_length_compressed;
	uint64	toc_length_uncompressed;

	uint64	toc_attribute_types_length;
	uint64	toc_attribute_types_count;
	uint64	toc_strings_length;
	uint64	toc_strings_count;
};


// magic, version
enum {
	B_HPKG_MAGIC	= 'hpkg',
	B_HPKG_VERSION	= 1
};


// compression types
enum {
	B_HPKG_COMPRESSION_NONE	= 0,
	B_HPKG_COMPRESSION_ZLIB	= 1
};


// attribute types
enum {
	// types
	B_HPKG_ATTRIBUTE_TYPE_INVALID			= 0,
	B_HPKG_ATTRIBUTE_TYPE_INT				= 1,
	B_HPKG_ATTRIBUTE_TYPE_UINT				= 2,
	B_HPKG_ATTRIBUTE_TYPE_STRING			= 3,
	B_HPKG_ATTRIBUTE_TYPE_RAW				= 4
};


// attribute encodings
enum {
	// signed/unsigned int encodings
	B_HPKG_ATTRIBUTE_ENCODING_INT_8_BIT		= 0,
	B_HPKG_ATTRIBUTE_ENCODING_INT_16_BIT	= 1,
	B_HPKG_ATTRIBUTE_ENCODING_INT_32_BIT	= 2,
	B_HPKG_ATTRIBUTE_ENCODING_INT_64_BIT	= 3,

	// string encodings
	B_HPKG_ATTRIBUTE_ENCODING_STRING_INLINE	= 0,
		// null-terminated string
	B_HPKG_ATTRIBUTE_ENCODING_STRING_TABLE	= 1,
		// unsigned LEB128 index into string table

	// raw data encodings
	B_HPKG_ATTRIBUTE_ENCODING_RAW_INLINE	= 0,
		// unsigned LEB128 size, raw bytes
	B_HPKG_ATTRIBUTE_ENCODING_RAW_HEAP		= 1
		// unsigned LEB128 size, unsigned LEB128 offset into the heap
};


// attribute tag arithmetics
#define B_HPKG_ATTRIBUTE_TAG_COMPOSE(index, encoding, hasChildren)	\
	(((uint64(index) << 3) | uint64(encoding) << 1				\
		| ((hasChildren) ? 1 : 0)) + 1)
#define B_HPKG_ATTRIBUTE_TAG_INDEX(tag)			(uint64((tag) - 1) >> 3)
#define B_HPKG_ATTRIBUTE_TAG_ENCODING(tag)		((uint64((tag) - 1) >> 1) & 0x3)
#define B_HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	((uint64((tag) - 1) & 0x1) != 0)


// standard attribute names
#define B_HPKG_ATTRIBUTE_NAME_DIRECTORY_ENTRY		"dir:entry"
	// path/entry name (string)
#define B_HPKG_ATTRIBUTE_NAME_FILE_TYPE				"file:type"
	// file type (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_PERMISSIONS		"file:permissions"
	// file permissions (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_USER				"file:user"
	// file user (string)
#define B_HPKG_ATTRIBUTE_NAME_FILE_GROUP			"file:group"
	// file group (string)
#define B_HPKG_ATTRIBUTE_NAME_FILE_ATIME			"file:atime"
	// file access time in seconds (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_MTIME			"file:mtime"
	// file modification time in seconds (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_CRTIME			"file:crtime"
	// file creation time in seconds (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_ATIME_NANOS		"file:atime:nanos"
	// file access time nanoseconds fraction (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_MTIME_NANOS		"file:mtime:nanos"
	// file modification time nanoseconds fraction (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_CRTIM_NANOS		"file:crtime:nanos"
	// file creation time nanoseconds fraction (uint)
#define B_HPKG_ATTRIBUTE_NAME_FILE_ATTRIBUTE		"file:attribute"
	// file attribute (string)
#define B_HPKG_ATTRIBUTE_NAME_FILE_ATTRIBUTE_TYPE	"file:attribute:type"
	// file attribute type (uint)
#define B_HPKG_ATTRIBUTE_NAME_DATA					"data"
	// (file/attribute) data (raw)
#define B_HPKG_ATTRIBUTE_NAME_DATA_COMPRESSION		"data:compression"
	// (file/attribute) data compression (uint, default: none)
#define B_HPKG_ATTRIBUTE_NAME_DATA_SIZE				"data:size"
	// (file/attribute) uncompressed data size (uint)
#define B_HPKG_ATTRIBUTE_NAME_DATA_CHUNK_SIZE		"data:chunk_size"
	// the size of compressed (file/attribute) data chunks (uint)
#define B_HPKG_ATTRIBUTE_NAME_SYMLINK_PATH			"symlink:path"
	// symlink path (string)


// file types (B_HPKG_ATTRIBUTE_NAME_FILE_TYPE)
enum {
	B_HPKG_FILE_TYPE_FILE		= 0,
	B_HPKG_FILE_TYPE_DIRECTORY	= 1,
	B_HPKG_FILE_TYPE_SYMLINK	= 2
};


// maximum number of bytes of data to be encoded inline; more will be allocated
// on the heap
#define B_HPKG_MAX_INLINE_DATA_SIZE	8


// name of file containing package information (in package's root folder)
#define B_HPKG_PACKAGE_INFO_FILE_NAME	".PackageInfo"


// default values
enum {
	B_HPKG_DEFAULT_FILE_TYPE				= B_HPKG_FILE_TYPE_FILE,
	B_HPKG_DEFAULT_FILE_PERMISSIONS			= 0644,
	B_HPKG_DEFAULT_DIRECTORY_PERMISSIONS	= 0755,
	B_HPKG_DEFAULT_SYMLINK_PERMISSIONS		= 0777,
	B_HPKG_DEFAULT_DATA_COMPRESSION			= B_HPKG_COMPRESSION_NONE,
	B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB		= 64 * 1024
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// _HAIKU_PACKAGE_H
