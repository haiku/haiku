//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_DISK_STRUCTURES_H
#define _UDF_DISK_STRUCTURES_H

#include <SupportDefs.h>

#include "cpp.h"

/*! \file DiskStructures.h

	UDF on-disk data structure declarations
*/

namespace UDF {

/*! \brief Header for volume structure descriptors

	Each descriptor consumes an entire block. All unused trailing
	bytes in the descriptor should be set to 0.
	
	The following descriptors contain no more information than
	that contained in the header:
	
	- BEA01:
	  - type: 0
	  - id: "BEA01"
	  - version: 1

	- TEA01:
	  - type: 0
	  - id: "TEA01"
	  - version: 1
	
	- NSR03:
	  - type: 0
	  - id: "NSR03"
	  - version: 1
*/	
struct vsd_header {
	uint8 type;
	char id[4];
	uint8 version;
} __attribute__((packed));


// Volume structure descriptor ids 
const char* kVSDID_BEA 			= "BEA01";
const char* kVSDID_TEA 			= "TEA01";
const char* kVSDID_BOOT 		= "BOOT2";
const char* kVSDID_ISO 			= "CD001";
const char* kVSDID_ECMA167_2 	= "NSR02";
const char* kVSDID_ECMA167_3 	= "NSR03";
const char* kVSDID_ECMA168		= "CDW02";


/*! \brief Character set specifications

	The character_set_info field shall be set to the ASCII string
	"OSTA Compressed Unicode" (padded right with NULL chars).
	
	See also: ECMA 167 1/7.2.1, UDF-2.01 2.1.2
*/
struct charspec {
	uint8 character_set_type;		//!< to be set to 0 to indicate CS0
	char character_set_info[63];	//!< "OSTA Compressed Unicode"
} __attribute__((packed));


extern const charspec kCS0Charspec;


/*! \brief Date and time stamp 

	See also: ECMA 167 1/7.3, UDF-2.01 2.1.4
*/
struct timestamp {
	union {
		uint16 type_and_timezone;
		uint16 type:4,
		       timezone:12;
	};
	uint16 year;
	uint8 month;
	uint8 day;
	uint8 hour;
	uint8 minute;
	uint8 second;
	uint8 centisecond;
	uint8 hundred_microsecond;
	uint8 microsecond;
} __attribute__((packed));


/*! \brief Location and length of a contiguous chunk of data

	See also: ECMA 167 3/7.1
*/
struct extent_address {
	uint32 length;
	uint32 location;
} __attribute__((packed));


/*! \brief Identifier used to designate the implementation responsible
	for writing associated data structures on the medium.
	
	See also: ECMA 167 1/7.4, UDF 2.01 2.1.5
*/
struct entity_id {
	uint8 flags;
	char identifier[23];
	char identifier_suffix[8];
} __attribute__((packed));


/*! \brief Common tag found at the beginning of most udf descriptor structures
*/
struct descriptor_tag {
	uint16 id;
	uint16 version;
	uint8 checksum;
	uint8 reserved;			// #00
	uint16 serial_number;
	uint16 crc;
	uint16 crc_length;
	uint32 location;		// address of udf_tag struct for error checking
	
	status_t init_check(off_t diskLocation);
} __attribute__((packed));


/*! \c descriptor_tag::id values
*/
enum {
	TAGID_UNDEFINED	= 0,

	// ECMA 167, PART 3
	TAGID_PRIMARY_VOLUME_DESCRIPTOR,
	TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER,
	TAGID_VOLUME_DESCRIPTOR_POINTER,
	TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR,
	TAGID_PARTITION_DESCRIPTOR,
	TAGID_LOGICAL_VOLUME_DESCRIPTOR,
	TAGID_UNALLOCATED_SPACE_DESCRIPTOR,
	TAGID_TERMINATING_DESCRIPTOR,
	TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR,
	
	TAGID_CUSTOM_START = 65280,
	TAGID_CUSTOM_END = 65535,
	
	// ECMA 167, PART 4
	TAGID_FILE_SET_DESCRIPTOR = 256,
	TAGID_FILE_IDENTIFIER_DESCRIPTOR,
	TAGID_ALLOCATION_EXTENT_DESCRIPTOR,
	TAGID_INDIRECT_ENTRY,
	TAGID_TERMINAL_ENTRY,
	TAGID_FILE_ENTRY,
	TAGID_EXTENDED_ATTRIBUTE_HEADER_DESCRIPTOR,
	TAGID_UNALLOCATED_SPACE_ENTRY,
	TAGID_SPACE_BITMAP_DESCRIPTOR,
	TAGID_PARTITION_INTEGRITY_ENTRY,
	TAGID_EXTENDED_FILE_ENTRY,
} udf_tag_id;


/*! \brief Primary volume descriptor
*/
struct primary_vd {
	descriptor_tag tag;
	uint32 vds_number;
	uint32 primary_volume_descriptor_number;
	char volume_identifier[32];
	uint16 volume_sequence_number;
	uint16 max_volume_sequence_number;
	uint16 interchange_level; //!< to be set to 3 if part of multivolume set, 2 otherwise
	uint16 max_interchange_level; //!< to be set to 3 unless otherwise directed by user
	uint32 character_set_list;
	uint32 max_character_set_list;
	char volume_set_identifier[128];

	/*! \brief Identifies the character set for the \c volume_identifier
		and \c volume_set_identifier fields.
		
		To be set to CS0.
	*/
	charspec descriptor_character_set;	

	/*! \brief Identifies the character set used in the \c volume_abstract
		and \c volume_copyright_notice extents.
		
		To be set to CS0.
	*/
	charspec explanatory_character_set;
	
	extent_address volume_abstract;
	extent_address volume_copyright_notice;
	
	entity_id application_id;
	timestamp recording_data_and_time;
	entity_id implementation_id;
	char implementation_use[64];
	uint32 predecessor_vds_location;
	uint16 flags;
	char reserved[22];

} __attribute__((packed));


/*! \brief Anchor Volume Descriptor Pointer

	vd recorded at preset locations in the partition, used as a reference
	point to the main vd sequences
	
	According to UDF 2.01, an avdp shall be recorded in at least 2 of
	the 3 following locations, where N is the last recordable sector
	of the partition:
	- 256
	- (N - 256)
	- N	
	
	See also: ECMA 167 3/10.2, UDF-2.01 2.2.3
*/
struct anchor_vd_pointer {
	descriptor_tag tag;
	extent_address main_vds;	//!< min length of 16 sectors
	extent_address reserve_vds;	//!< min length of 16 sectors
	char reserved[480];	
} __attribute__((packed));


/*! \brief Volume Descriptor Pointer

	Used to chain extents of volume descriptor sequences together.
	
	See also: ECMA 167 3/10.3
*/
struct vd_pointer {
	descriptor_tag tag;
	uint32 vds_number;
	extent_address next;
} __attribute__((packed));


/*! \brief Implementation Use Volume Descriptor

	See also: ECMA 167 3/10.4
*/
struct implementation_use_vd {
	descriptor_tag tag;
	uint32 vds_number;
	entity_id implementation_id;
	uint8 implementation_use[460];
} __attribute__((packed));


/*! \brief Partition Descriptor

	See also: ECMA 167 3/10.5
*/
struct partition_descriptor {
	descriptor_tag tag;
	uint32 vds_number;
	/*! Bit 0: If 0, shall mean volume space has not been allocated. If 1,
	    shall mean volume space has been allocated.
	*/
	union {
		uint16 partition_flags;
		struct flags {
			uint16 allocated:1,
			       reserved:15;
		};
	};
	uint16 partition_number;

	/*! - "+NSR03" Volume recorded according to ECMA-167, i.e. UDF
		- "+CD001" Volume recorded according to ECMA-119, i.e. iso9660
		- "+FDC01" Volume recorded according to ECMA-107
		- "+CDW02" Volume recorded according to ECMA-168
	*/		
	entity_id partition_contents;
	uint8 partition_contents_use[128];

	/*! See \c partition_access_type enum
	*/
	uint32 access_type;
	uint32 start;
	uint32 length;
	entity_id implementation_id;
	uint8 implementation_use[128];
	uint8 reserved[156];
} __attribute__((packed));


enum partition_access_type {
	PAT_UNSPECIFIED,
	PAT_READ_ONLY,
	PAT_WRITE_ONCE,
	PAT_REWRITABLE,
	PAT_OVERWRITABLE,
};


/*! \brief Logical volume descriptor

	See also: ECMA 167 3/10.6, UDF-2.01 2.2.4
*/
struct logical_vd {
	descriptor_tag tag;
	uint32 vds_number;
	
	/*! \brief Identifies the character set for the
		\c logical_volume_identifier field.
		
		To be set to CS0.
	*/
	charspec character_set;
	char logical_volume_identifier[128];
	uint32 logical_block_size;
	
	/*! \brief To be set to 0 or "*OSTA UDF Compliant". See UDF specs.
	*/
	entity_id domain_id;
	
	/*! \brief For UDF, shall contain a \c long_address which identifies
		the location of the logical volume's first file set.
	*/
	uint8 logical_volume_contents_use[16];
	uint32 map_table_length;
	uint32 partition_map_count;
	entity_id implementation_id;
	uint8 implementation_use[128];
	
	/*! \brief Logical volume integrity sequence location.
	
		For re/overwritable media, shall be a min of 8KB in length.
		For WORM media, shall be quite frickin large, as a new volume
		must be added to the set if the extent fills up (since you
		can't chain lvis's I guess).
	*/
	extent_address integrity_sequence_extent;
	
	/*! \brief Restricted to maps of type 1 for normal maps and
		type 2 for virtual maps or maps on systems not supporting
		defect management.
		
		See UDF-2.01 2.2.8, 2.2.9
	*/
	uint8 partition_maps[0];
};


/*! \brief Generic partition map

	See also: ECMA-167 3/10.7.1
*/
struct generic_partition_map {
	uint8 type;
	uint8 length;
	uint8 map_data[0];
};

/*! \brief Physical partition map (i.e. ECMA-167 Type 1 partition map)

	See also: ECMA-167 3/10.7.2
*/
struct physical_partition_map {
	uint8 type;
	uint8 length;
	uint16 volume_sequence_number;
	uint16 partition_number;	
};

/*! \brief Virtual partition map (i.e. UDF Type 2 partition map)

	Note that this differs from the ECMA-167 type 2 partition map,
	but is of identical size.
	
	See also: UDF-2.01 2.2.8
	
	\todo Handle UDF sparable partition maps as well (UDF-2.01 2.2.9)
*/
struct virtual_partition_map {
	uint8 type;
	uint8 length;
	uint8 reserved1[2];
	
	/*! - flags: 0
	    - identifier: "*UDF Virtual Partition"
	    - identifier_suffix: per UDF-2.01 2.1.5.3
	*/
	entity_id partition_type_id;
	uint16 volume_sequence_number;
	
	/*! corresponding type 1 partition map in same logical volume
	*/
	uint16 partition_number;	
	uint8 reserved2[24];
};

/*! \brief Unallocated space descriptor

	See also: ECMA-167 3/10.8
*/
struct unallocated_space_descriptor {
	descriptor_tag tag;
	uint32 vds_number;
	uint32 allocation_descriptor_count;
	extent_address allocation_descriptors[0];
} __attribute__((packed));


/*! \brief Terminating descriptor

	See also: ECMA-167 3/10.9
*/
struct terminating_descriptor {
	descriptor_tag tag;
	uint8 reserved[496];
} __attribute__((packed));


/*! \brief Logical volume integrity descriptor

	See also: ECMA-167 3/10.10
*/
struct logical_volume_integrity_descriptor {
	descriptor_tag tag;
	timestamp recording_time;
	uint32 integrity_type;
	extent_address next_integrity_extent;
	uint8 logical_volume_contents_use[32];
	uint32 partition_count;
	uint32 implementation_use_length;

	/*! \todo double-check the pointer arithmetic here. */
	uint32* free_space_table() { return (uint32*)(this+80); }
	uint32* size_table() { return (uint32*)(free_space_table()+partition_count*sizeof(uint32)); }
	uint8* implementation_use() { return (uint8*)(size_table()+partition_count*sizeof(uint32)); }	
	
} __attribute__((packed));

/*! \brief Logical volume integrity types
*/
enum {
	LVIT_OPEN = 1,
	LVIT_CLOSED,
};

};	// namespace UDF

#endif	// _UDF_DISK_STRUCTURES_H

