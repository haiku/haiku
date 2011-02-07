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
	uint32	attributes_strings_length;
	uint32	attributes_strings_count;

	// TOC section
	uint32	toc_compression;
	uint64	toc_length_compressed;
	uint64	toc_length_uncompressed;

	uint64	toc_attribute_types_length;
	uint64	toc_attribute_types_count;
	uint64	toc_strings_length;
	uint64	toc_strings_count;
};


// header
struct hpkg_repo_header {
	uint32	magic;							// "hpkr"
	uint16	header_size;
	uint16	version;
	uint64	total_size;

	// package attributes section
	uint32	attributes_compression;
	uint32	attributes_length_compressed;
	uint32	attributes_length_uncompressed;

	uint64	attributes_strings_length;
	uint64	attributes_strings_count;
};


// attribute tag arithmetics
#define HPKG_ATTRIBUTE_TAG_COMPOSE(index, encoding, hasChildren)	\
	(((uint64(index) << 3) | uint64(encoding) << 1					\
		| ((hasChildren) ? 1 : 0)) + 1)
#define HPKG_ATTRIBUTE_TAG_INDEX(tag)			(uint64((tag) - 1) >> 3)
#define HPKG_ATTRIBUTE_TAG_ENCODING(tag)		((uint64((tag) - 1) >> 1) & 0x3)
#define HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	((uint64((tag) - 1) & 0x1) != 0)

// package attribute tag arithmetics
#define HPKG_PACKAGE_ATTRIBUTE_TAG_COMPOSE(id, type, encoding, hasChildren) \
	(((uint16(encoding) << 9) | (((hasChildren) ? 1 : 0) << 8)				\
		| (uint16(type) << 5) | (uint16(id))) + 1)
#define HPKG_PACKAGE_ATTRIBUTE_TAG_ENCODING(tag)		\
	((uint16((tag) - 1) >> 9) & 0x3)
#define HPKG_PACKAGE_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	\
	(((uint16((tag) - 1) >> 8) & 0x1) != 0)
#define HPKG_PACKAGE_ATTRIBUTE_TAG_TYPE(tag)			\
	((uint16((tag) - 1) >> 5) & 0x7)
#define HPKG_PACKAGE_ATTRIBUTE_TAG_ID(tag)				\
	(uint16((tag) - 1) & 0x1f)


// standard attribute names
#define HPKG_ATTRIBUTE_NAME_DIRECTORY_ENTRY		"dir:entry"
	// path/entry name (string)
#define HPKG_ATTRIBUTE_NAME_FILE_TYPE			"file:type"
	// file type (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_PERMISSIONS	"file:permissions"
	// file permissions (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_USER			"file:user"
	// file user (string)
#define HPKG_ATTRIBUTE_NAME_FILE_GROUP			"file:group"
	// file group (string)
#define HPKG_ATTRIBUTE_NAME_FILE_ATIME			"file:atime"
	// file access time in seconds (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_MTIME			"file:mtime"
	// file modification time in seconds (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_CRTIME			"file:crtime"
	// file creation time in seconds (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_ATIME_NANOS	"file:atime:nanos"
	// file access time nanoseconds fraction (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_MTIME_NANOS	"file:mtime:nanos"
	// file modification time nanoseconds fraction (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_CRTIM_NANOS	"file:crtime:nanos"
	// file creation time nanoseconds fraction (uint)
#define HPKG_ATTRIBUTE_NAME_FILE_ATTRIBUTE		"file:attribute"
	// file attribute (string)
#define HPKG_ATTRIBUTE_NAME_FILE_ATTRIBUTE_TYPE	"file:attribute:type"
	// file attribute type (uint)
#define HPKG_ATTRIBUTE_NAME_DATA				"data"
	// (file/attribute) data (raw)
#define HPKG_ATTRIBUTE_NAME_DATA_COMPRESSION	"data:compression"
	// (file/attribute) data compression (uint, default: none)
#define HPKG_ATTRIBUTE_NAME_DATA_SIZE			"data:size"
	// (file/attribute) uncompressed data size (uint)
#define HPKG_ATTRIBUTE_NAME_DATA_CHUNK_SIZE		"data:chunk_size"
	// the size of compressed (file/attribute) data chunks (uint)
#define HPKG_ATTRIBUTE_NAME_SYMLINK_PATH		"symlink:path"
	// symlink path (string)


// package attribute IDs
enum HPKGPackageAttributeID {
	HPKG_PACKAGE_ATTRIBUTE_NAME = 0,
	HPKG_PACKAGE_ATTRIBUTE_SUMMARY,
	HPKG_PACKAGE_ATTRIBUTE_DESCRIPTION,
	HPKG_PACKAGE_ATTRIBUTE_VENDOR,
	HPKG_PACKAGE_ATTRIBUTE_PACKAGER,
	HPKG_PACKAGE_ATTRIBUTE_ARCHITECTURE,
	HPKG_PACKAGE_ATTRIBUTE_VERSION_MAJOR,
	HPKG_PACKAGE_ATTRIBUTE_VERSION_MINOR,
	HPKG_PACKAGE_ATTRIBUTE_VERSION_MICRO,
	HPKG_PACKAGE_ATTRIBUTE_VERSION_RELEASE,
	HPKG_PACKAGE_ATTRIBUTE_COPYRIGHT,
	HPKG_PACKAGE_ATTRIBUTE_LICENSE,
	HPKG_PACKAGE_ATTRIBUTE_PROVIDES,
	HPKG_PACKAGE_ATTRIBUTE_PROVIDES_TYPE,
	HPKG_PACKAGE_ATTRIBUTE_REQUIRES,
	HPKG_PACKAGE_ATTRIBUTE_SUPPLEMENTS,
	HPKG_PACKAGE_ATTRIBUTE_CONFLICTS,
	HPKG_PACKAGE_ATTRIBUTE_FRESHENS,
	HPKG_PACKAGE_ATTRIBUTE_REPLACES,
	HPKG_PACKAGE_ATTRIBUTE_RESOLVABLE_OPERATOR,
	//
	HPKG_PACKAGE_ATTRIBUTE_ENUM_COUNT,
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_
