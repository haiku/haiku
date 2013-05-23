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


// package file header
struct hpkg_header {
	uint32	magic;							// "hpkg"
	uint16	header_size;
	uint16	version;
	uint64	total_size;
	uint16	minor_version;

	// heap
	uint16	heap_compression;
	uint32	heap_chunk_size;
	uint64	heap_size_compressed;
	uint64	heap_size_uncompressed;

	// package attributes section
	uint32	attributes_length;
	uint32	attributes_strings_length;
	uint32	attributes_strings_count;
	uint32	reserved1;

	// TOC section
	uint64	toc_length;
	uint64	toc_strings_length;
	uint64	toc_strings_count;
};


// repository file header
struct hpkg_repo_header {
	uint32	magic;							// "hpkr"
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


// attribute tag arithmetics
// (using 7 bits for id, 3 for type, 1 for hasChildren and 2 for encoding)
static inline uint16
compose_attribute_tag(uint16 id, uint16 type, uint16 encoding, bool hasChildren)
{
	return ((encoding << 11) | (uint16(hasChildren ? 1 : 0) << 10) | (type << 7)
			| id)
		+ 1;
}


static inline uint16
attribute_tag_encoding(uint16 tag)
{
	return ((tag - 1) >> 11) & 0x3;
}


static inline bool
attribute_tag_has_children(uint16 tag)
{
	return (((tag - 1) >> 10) & 0x1) != 0;
}


static inline uint16
attribute_tag_type(uint16 tag)
{
	return ((tag - 1) >> 7) & 0x7;
}


static inline uint16
attribute_tag_id(uint16 tag)
{
	return (tag - 1) & 0x7f;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__HAIKU_PACKAGE_H_
