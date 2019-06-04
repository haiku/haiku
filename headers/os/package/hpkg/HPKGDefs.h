/*
 * Copyright 2009-2013, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__HPKG_DEFS_H_
#define _PACKAGE__HPKG__HPKG_DEFS_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


// magic & version of package and repository files
enum {
	B_HPKG_MAGIC				= 'hpkg',
	B_HPKG_VERSION				= 2,
	B_HPKG_MINOR_VERSION		= 1,
	//
	B_HPKG_REPO_MAGIC			= 'hpkr',
	B_HPKG_REPO_VERSION			= 2,
	B_HPKG_REPO_MINOR_VERSION	= 1
};


// package attribute IDs
enum BHPKGPackageSectionID {
	B_HPKG_SECTION_HEADER					= 0,
	B_HPKG_SECTION_HEAP						= 1,
	B_HPKG_SECTION_PACKAGE_TOC				= 2,
	B_HPKG_SECTION_PACKAGE_ATTRIBUTES		= 3,
	B_HPKG_SECTION_REPOSITORY_INFO			= 4,
	//
	B_HPKG_SECTION_ENUM_COUNT
};


// attribute types
enum {
	// types
	B_HPKG_ATTRIBUTE_TYPE_INVALID			= 0,
	B_HPKG_ATTRIBUTE_TYPE_INT				= 1,
	B_HPKG_ATTRIBUTE_TYPE_UINT				= 2,
	B_HPKG_ATTRIBUTE_TYPE_STRING			= 3,
	B_HPKG_ATTRIBUTE_TYPE_RAW				= 4,
	//
	B_HPKG_ATTRIBUTE_TYPE_ENUM_COUNT
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


// maximum number of bytes of data to be encoded inline; more will be allocated
// on the heap
enum {
	B_HPKG_MAX_INLINE_DATA_SIZE	= 8
};


// name of file containing package information (in package's root folder)
extern const char* const B_HPKG_PACKAGE_INFO_FILE_NAME;


// package attribute IDs
enum BHPKGAttributeID {
	#define B_DEFINE_HPKG_ATTRIBUTE(id, type, name, constant)	\
		B_HPKG_ATTRIBUTE_ID_##constant	= id,
	#include <package/hpkg/PackageAttributes.h>
	#undef B_DEFINE_HPKG_ATTRIBUTE
	//
	B_HPKG_ATTRIBUTE_ID_ENUM_COUNT,
};


// compression types
enum {
	B_HPKG_COMPRESSION_NONE	= 0,
	B_HPKG_COMPRESSION_ZLIB	= 1,
	B_HPKG_COMPRESSION_ZSTD	= 2
};


// file types (B_HPKG_ATTRIBUTE_ID_FILE_TYPE)
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
	B_HPKG_DEFAULT_SYMLINK_PERMISSIONS		= 0777
};


// Writer Init() flags
enum {
	B_HPKG_WRITER_UPDATE_PACKAGE	= 0x01,
		// update the package (don't truncate)
	B_HPKG_WRITER_FORCE_ADD			= 0x02,
		// when updating a pre-existing entry, don't fail, but replace the
		// entry, if possible (directories will be merged, but won't replace a
		// non-directory)
};


// Reader Init() flags
enum {
	B_HPKG_READER_DONT_PRINT_VERSION_MISMATCH_MESSAGE	= 0x01
		// Fail silently when encountering a package format version mismatch.
		// Don't print anything to the error output.
};


enum {
	B_HPKG_COMPRESSION_LEVEL_NONE		= 0,
	B_HPKG_COMPRESSION_LEVEL_FASTEST	= 1,
	B_HPKG_COMPRESSION_LEVEL_BEST		= 9
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__HPKG_DEFS_H_
