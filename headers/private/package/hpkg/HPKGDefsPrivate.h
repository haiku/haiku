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
	uint64	toc_strings_length;
	uint64	toc_strings_count;
};


// header
struct hpkg_repo_header {
	uint32	magic;							// "hpkr"
	uint16	header_size;
	uint16	version;
	uint64	total_size;

	// repository info section
	uint32	info_compression;
	uint32	info_length_compressed;
	uint32	info_length_uncompressed;

	// package attributes section
	uint32	packages_compression;
	uint64	packages_length_compressed;
	uint64	packages_length_uncompressed;
	uint64	packages_strings_length;
	uint64	packages_strings_count;
};


// attribute tag arithmetics
// (using 6 bits for id, 3 for type, 1 for hasChildren and 2 for encoding)
#define HPKG_ATTRIBUTE_TAG_COMPOSE(id, type, encoding, hasChildren) 	\
	(((uint16(encoding) << 10) | (uint16((hasChildren) ? 1 : 0) << 9)	\
		| (uint16(type) << 6) | (uint16(id))) + 1)
#define HPKG_ATTRIBUTE_TAG_ENCODING(tag)		\
	((uint16((tag) - 1) >> 10) & 0x3)
#define HPKG_ATTRIBUTE_TAG_HAS_CHILDREN(tag)	\
	(((uint16((tag) - 1) >> 9) & 0x1) != 0)
#define HPKG_ATTRIBUTE_TAG_TYPE(tag)			\
	((uint16((tag) - 1) >> 6) & 0x7)
#define HPKG_ATTRIBUTE_TAG_ID(tag)				\
	(uint16((tag) - 1) & 0x3f)


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_
