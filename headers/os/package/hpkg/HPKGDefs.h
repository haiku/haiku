/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__HPKG_DEFS_H_
#define _PACKAGE__HPKG__HPKG_DEFS_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


// magic, version
enum {
	B_HPKG_MAGIC	= 'hpkg',
	B_HPKG_VERSION	= 1
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


// maximum number of bytes of data to be encoded inline; more will be allocated
// on the heap
#define B_HPKG_MAX_INLINE_DATA_SIZE	8


// name of file containing package information (in package's root folder)
#define B_HPKG_PACKAGE_INFO_FILE_NAME	".PackageInfo"


// default values
enum {
	B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB		= 64 * 1024
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__HPKG_DEFS_H_
