/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_
#define _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_


#include <SupportDefs.h>

#include <package/hpkg/HPKGDefs.h>


namespace BPackageKit {

namespace BHPKG {

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


// compression types
enum {
	B_HPKG_COMPRESSION_NONE	= 0,
	B_HPKG_COMPRESSION_ZLIB	= 1
};


// attribute tag arithmetics
#define B_HPKG_ATTRIBUTE_TAG_COMPOSE(index, encoding, hasChildren)	\
	(((uint64(index) << 3) | uint64(encoding) << 1					\
		| ((hasChildren) ? 1 : 0)) + 1)
#define B_HPKG_ATTRIBUTE_TAG_INDEX(tag)			(uint64((tag) - 1) >> 3)
#define B_HPKG_ATTRIBUTE_TAG_ENCODING(tag)		((uint64((tag) - 1) >> 1) & 0x3)
#define B_HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	((uint64((tag) - 1) & 0x1) != 0)

// package attribute tag arithmetics
#define B_HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(id, type, encoding, hasChildren) \
	(((uint16(encoding) << 9) | (((hasChildren) ? 1 : 0) << 8)				  \
		| (uint16(type) << 5) | (uint16(id))) + 1)
#define B_HPKG_PACKAGE_ATTRIBUTE_TAG_ENCODING(tag)		\
	((uint16((tag) - 1) >> 9) & 0x3)
#define B_HPKG_PACKAGE_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	\
	(((uint16((tag) - 1) >> 8) & 0x1) != 0)
#define B_HPKG_PACKAGE_ATTRIBUTE_TAG_TYPE(tag)			\
	((uint16((tag) - 1) >> 5) & 0x7)
#define B_HPKG_PACKAGE_ATTRIBUTE_TAG_ID(tag)			\
	(uint16((tag) - 1) & 0x1f)


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


// default values
enum {
	B_HPKG_DEFAULT_FILE_TYPE				= B_HPKG_FILE_TYPE_FILE,
	B_HPKG_DEFAULT_FILE_PERMISSIONS			= 0644,
	B_HPKG_DEFAULT_DIRECTORY_PERMISSIONS	= 0755,
	B_HPKG_DEFAULT_SYMLINK_PERMISSIONS		= 0777,
	B_HPKG_DEFAULT_DATA_COMPRESSION			= B_HPKG_COMPRESSION_NONE,
};


// package attribute IDs
enum {
	B_HPKG_PACKAGE_ATTRIBUTE_NAME = 0,
	B_HPKG_PACKAGE_ATTRIBUTE_SUMMARY,
	B_HPKG_PACKAGE_ATTRIBUTE_DESCRIPTION,
	B_HPKG_PACKAGE_ATTRIBUTE_VENDOR,
	B_HPKG_PACKAGE_ATTRIBUTE_PACKAGER,
	B_HPKG_PACKAGE_ATTRIBUTE_ARCHITECTURE,
	B_HPKG_PACKAGE_ATTRIBUTE_VERSION_MAJOR,
	B_HPKG_PACKAGE_ATTRIBUTE_VERSION_MINOR,
	B_HPKG_PACKAGE_ATTRIBUTE_VERSION_MICRO,
	B_HPKG_PACKAGE_ATTRIBUTE_VERSION_RELEASE,
	B_HPKG_PACKAGE_ATTRIBUTE_COPYRIGHT,
	B_HPKG_PACKAGE_ATTRIBUTE_LICENSE,
	B_HPKG_PACKAGE_ATTRIBUTE_PROVIDES,
	B_HPKG_PACKAGE_ATTRIBUTE_PROVIDES_TYPE,
	B_HPKG_PACKAGE_ATTRIBUTE_REQUIRES,
	B_HPKG_PACKAGE_ATTRIBUTE_SUPPLEMENTS,
	B_HPKG_PACKAGE_ATTRIBUTE_CONFLICTS,
	B_HPKG_PACKAGE_ATTRIBUTE_FRESHENS,
	B_HPKG_PACKAGE_ATTRIBUTE_REPLACES,
	B_HPKG_PACKAGE_ATTRIBUTE_RESOLVABLE_OPERATOR,
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_
