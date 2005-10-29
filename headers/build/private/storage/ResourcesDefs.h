// ResourcesDefs.h

#ifndef _DEF_RESOURCES_H
#define _DEF_RESOURCES_H

#include <SupportDefs.h>

// x86 resource file constants
const char		kX86ResourceFileMagic[4]		= { 'R', 'S', 0, 0 };
const uint32	kX86ResourcesOffset				= 0x00000004;

// PPC resource file constants
const char		kPPCResourceFileMagic[4]		= { 'r', 'e', 's', 'f' };
const uint32	kPPCResourcesOffset				= 0x00000028;

// ELF file related constants
const uint32	kELFMinResourceAlignment		= 32;

// the unused data pattern
const uint32	kUnusedResourceDataPattern[3]	= {
	0xffffffff, 0x000003e9, 0x00000000
};


// the resources header
struct resources_header {
	uint32	rh_resources_magic;
	uint32	rh_resource_count;
	uint32	rh_index_section_offset;
	uint32	rh_admin_section_size;
	uint32	rh_pad[13];
};

const uint32	kResourcesHeaderMagic			= 0x444f1000;
const uint32	kResourceIndexSectionOffset		= 0x00000044;
const uint32	kResourceIndexSectionAlignment	= 0x00000600;
const uint32	kResourcesHeaderSize			= 68;


// the header of the index section
struct resource_index_section_header {
	uint32	rish_index_section_offset;
	uint32	rish_index_section_size;
	uint32	rish_unused_data1;
	uint32	rish_unknown_section_offset;
	uint32	rish_unknown_section_size;
	uint32	rish_unused_data2[25];
	uint32	rish_info_table_offset;
	uint32	rish_info_table_size;
	uint32	rish_unused_data3;
};

const uint32	kUnknownResourceSectionSize		= 0x00000168;
const uint32	kResourceIndexSectionHeaderSize	= 132;


// an entry of the index table
struct resource_index_entry {
	uint32	rie_offset;
	uint32	rie_size;
	uint32	rie_pad;
};

const uint32	kResourceIndexEntrySize			= 12;


// a resource info
struct resource_info {
	int32	ri_id;
	int32	ri_index;
	uint16	ri_name_size;
	char	ri_name[1];
};

const uint32	kMinResourceInfoSize			= 10;


// a resource info block
struct resource_info_block {
	type_code		rib_type;
	resource_info	rib_info[1];
};

const uint32	kMinResourceInfoBlockSize		= 4 + kMinResourceInfoSize;


// the structure separating resource info blocks
struct resource_info_separator {
	uint32	ris_value1;
	uint32	ris_value2;
};

const uint32	kResourceInfoSeparatorSize		= 8;


// the end of the info table
struct resource_info_table_end {
	uint32	rite_check_sum;
	uint32	rite_terminator;
};

const uint32	kResourceInfoTableEndSize		= 8;


#endif	// _DEF_RESOURCES_H


