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

	\brief UDF on-disk data structure declarations
	
	UDF is a specialization of the ECMA-167 standard. For the most part,
	ECMA-167 structures are used by UDF with special restrictions. In a
	few instances, UDF introduces its own structures to augment those
	supplied by ECMA-167; those structures are clearly marked.
	
	For UDF info: <a href='http://www.osta.org'>http://www.osta.org</a>
	For ECMA info: <a href='http://www.ecma-international.org'>http://www.ecma-international.org</a>

	\todo Add in comments about max struct sizes from UDF-2.01

*/

namespace UDF {

//----------------------------------------------------------------------
// ECMA-167 Part 1
//----------------------------------------------------------------------

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


/*! \brief Identifier used to designate the implementation responsible
	for writing associated data structures on the medium.
	
	See also: ECMA 167 1/7.4, UDF 2.01 2.1.5
*/
struct entity_id {
	uint8 flags;
	char identifier[23];
	char identifier_suffix[8];
} __attribute__((packed));


//----------------------------------------------------------------------
// ECMA-167 Part 2
//----------------------------------------------------------------------


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

	See also: ECMA 167 2/9.1
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


//----------------------------------------------------------------------
// ECMA-167 Part 3
//----------------------------------------------------------------------


/*! \brief Location and length of a contiguous chunk of data

	See also: ECMA 167 3/7.1
*/
struct extent_address {
	uint32 length;
	uint32 location;
} __attribute__((packed));


/*! \brief Common tag found at the beginning of most udf descriptor structures.

	For error checking, \c descriptor_tag structures have:
	- The disk location of the tag redundantly stored in the tag itself
	- A checksum value for the tag
	- A CRC value and length

	See also: ECMA 167 1/7.2, UDF 2.01 2.2.1, UDF 2.01 2.3.1
*/
struct descriptor_tag {
	uint16 id;
	uint16 version;
	uint8 checksum;			//!< Sum modulo 256 of bytes 0-3 and 5-15 of this struct.
	uint8 reserved;			//!< Set to #00.
	uint16 serial_number;
	uint16 crc;				//!< May be 0 if \c crc_length field is 0.
	/*! \brief Length of the data chunk used to calculate CRC.
	
		If 0, no CRC was calculated, and the \c crc field must be 0.
		
		According to UDF-2.01 2.3.1.2, the CRC shall be calculated for all descriptors
		unless otherwise noted, and this field shall be set to:
		
		<code>(descriptor length) - (descriptor tag length)</code>
	*/
	uint16 crc_length;
	/*! \brief Address of this tag within its partition (for error checking).
	
		For virtually addressed structures (i.e. those accessed thru a VAT), this
		shall be the virtual address, not the physical or logical address.
	*/
	uint32 location;		
	
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
} tag_id;


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
	
	union {
		/*! \brief For UDF, shall contain a \c long_allocation_descriptor which identifies
			the location of the logical volume's first file set.
		*/
		uint8 logical_volume_contents_use[16];
		extent_address file_set_address;
	};

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
		UDF type 2 for virtual maps or maps on systems not supporting
		defect management.
		
		See UDF-2.01 2.2.8, 2.2.9
	*/
	uint8 partition_maps[0];
} __attribute__((packed));


/*! \brief Generic partition map

	See also: ECMA-167 3/10.7.1
*/
struct generic_partition_map {
	uint8 type;
	uint8 length;
	uint8 map_data[0];
} __attribute__((packed));


/*! \brief Physical partition map (i.e. ECMA-167 Type 1 partition map)

	See also: ECMA-167 3/10.7.2
*/
struct physical_partition_map {
	uint8 type;
	uint8 length;
	uint16 volume_sequence_number;
	uint16 partition_number;	
} __attribute__((packed));


/* ----UDF Specific---- */
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
} __attribute__((packed));


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

//----------------------------------------------------------------------
// ECMA-167 Part 4
//----------------------------------------------------------------------


/*! \brief Location of a logical block within a logical volume.

	See also: ECMA 167 4/7.1
*/
struct logical_block_address {
	uint32 location;	//!< Block location relative to start of corresponding partition
	uint16 partition;	//!< Numeric partition id within logical volume
} __attribute__((packed));


/*! \brief Allocation descriptor.

	See also: ECMA 167 4/14.14.1
*/
struct short_allocation_descriptor {
	uint32 length;
	logical_block_address location;
} __attribute__((packed));


/*! \brief Allocation descriptor w/ 6 byte implementation use field.

	See also: ECMA 167 4/14.14.2
*/
struct long_allocation_descriptor {
	uint32 length;
	logical_block_address location;
	uchar implementation_use[6];
} __attribute__((packed));


/*! \brief File set descriptor

	Contains all the pertinent info about a file set (i.e. a hierarchy of files)
	
	According to UDF-2.01, only one file set descriptor shall be recorded,
	except on WORM media, where the following rules apply:
	- Multiple file sets are allowed only on WORM media
	- The default file set shall be the one with highest value \c file_set_number field.
	- Only the default file set may be flagged as writeable. All others shall be
	  flagged as "hard write protect".
	- No writeable file set may reference metadata structures which are referenced
	  (directly or indirectly) by any other file set. Writeable file sets may, however,
	  reference actual file data extents that are also referenced by other file sets.
*/
struct file_set_descriptor {
	descriptor_tag tag;
	timestamp recording_date_and_time;
	uint16 interchange_level;			//!< To be set to 3 (see UDF-2.01 2.3.2.1)
	uint16 max_interchange_level;		//!< To be set to 3 (see UDF-2.01 2.3.2.2)
	uint32 character_set_list;
	uint32 max_character_set_list;
	uint32 file_set_number;
	uint32 file_set_descriptor_number;
	charspec logical_volume_id_character_set;	//!< To be set to kCSOCharspec
	char logical_volume_id[128];
	charspec file_set_charspec;
	char file_set_id[32];
	char copyright_file_id[32];
	char abstract_file_id[32];
	long_allocation_descriptor root_directory_icb;
	entity_id domain_id;	
	long_allocation_descriptor next_extent;
	long_allocation_descriptor system_stream_directory_icb;
	uint8 reserved[32];
} __attribute__((packed));


/*! \brief Partition header descriptor
	
	Contains references to unallocated and freed space data structures.
	
	Note that unallocated space is space ready to be written with no
	preprocessing. Freed space is space needing preprocessing (i.e.
	a special write pass) before use.
	
	Per UDF-2.01 2.3.3, the use of tables or bitmaps shall be consistent,
	i.e. only one type or the other shall be used, not both.
	
	To indicate disuse of a certain field, the fields of the allocation
	descriptor shall all be set to 0.
	
	See also: ECMA-167 4/14.3, UDF-2.01 2.2.3
*/
struct partition_header_descriptor {
	short_allocation_descriptor unallocated_space_table;
	short_allocation_descriptor unallocated_space_bitmap;
	/*! Unused, per UDF-2.01 2.2.3 */
	short_allocation_descriptor partition_integrity_table;
	short_allocation_descriptor freed_space_table;
	short_allocation_descriptor freed_space_bitmap;
	uint8 reserved[88];
} __attribute__((packed));


/*! \brief File identifier descriptor

	Identifies the name of a file entry, and the location of its corresponding
	ICB.
	
	See also: ECMA-167 4/14.4, UDF-2.01 2.3.4
	
	\todo Check pointer arithmetic
*/
struct file_id_descriptor {
	descriptor_tag tag;
	/*! According to ECMA-167: 1 <= valid version_number <= 32767, 32768 <= reserved <= 65535.
		
		However, according to UDF-2.01, there shall be exactly one version of
		a file, and it shall be 1.
	 */
	uint16 version_number;
	/*! \todo Check UDF-2.01 2.3.4.2 for some more restrictions. */
	union {
		uint8 characteristics;
		uint8	may_be_hidden:1,
				is_directory:1,
				is_deleted:1,
				is_parent:1,
				is_metadata_stream:1,
				reserved_characteristics:3;
	};	
	uint8 id_length;
	long_allocation_descriptor icb;
	uint8 implementation_use_length;
	
	/*! If implementation_use_length is greater than 0, the first 32
		bytes of implementation_use() shall be an entity_id identifying
		the implementation that generated the rest of the data in the
		implementation_use() field.
	*/
	uint8* implementation_use() { return (uint8*)(this+38); }
	char* id() { return (char*)(this+38+implementation_use_length); }	
	
} __attribute__((packed));


/*! \brief Allocation extent descriptor

	See also: ECMA-167 4/14.5
*/
struct allocation_extent_descriptor {
	descriptor_tag tag;
	uint32 previous_allocation_extent_location;
	uint32 length_of_allocation_descriptors;
	
	/*! \todo Check that this is really how things work: */
	uint8* allocation_descriptors() { return (uint8*)(this+sizeof(allocation_extent_descriptor)); }
} __attribute__((packed));


/*! \brief icb_tag::file_type values

	See also ECMA-167 4/14.6.6
*/
enum icb_file_types {
	ICB_TYPE_UNSPECIFIED = 0,
	ICB_TYPE_UNALLOCATED_SPACE_ENTRY,
	ICB_TYPE_PARTITION_INTEGRITY_ENTRY,
	ICB_TYPE_INDIRECT_ENTRY,
	ICB_TYPE_DIRECTORY,
	ICB_TYPE_REGULAR_FILE,
	ICB_TYPE_BLOCK_SPECIAL_DEVICE,
	ICB_TYPE_CHARACTER_SPECIAL_DEVICE,
	ICB_TYPE_EXTENDED_ATTRIBUTES_FILE,
	ICB_TYPE_FIFO,
	ICB_TYPE_ISSOCK,
	ICB_TYPE_TERMINAL,
	ICB_TYPE_SYMLINK,
	ICB_TYPE_STREAM_DIRECTORY,

	ICB_TYPE_RESERVED_START = 14,
	ICB_TYPE_RESERVED_END = 247,
	
	ICB_TYPE_CUSTOM_START = 248,
	ICB_TYPE_CUSTOM_END = 255,
};


/*! \brief ICB entry tag

	Common tag found in all ICB entries (in addition to, and immediately following,
	the descriptor tag).

	See also: ECMA-167 4/14.6, UDF-2.01 2.3.5
*/
struct icb_entry_tag {
	uint32 prior_recorded_number_of_direct_entries;
	/*! Per UDF-2.01 2.3.5.1, only strategy types 4 and 4096 shall be supported.
	
		\todo Describe strategy types here.
	*/
	uint16 strategy_type;
	uint8 strategy_parameters[2];
	uint16 entry_count;
	uint8 reserved;
	/*! \brief icb_file_type value identifying the type of this icb entry */
	uint8 file_type;
	logical_block_address parent_icb_location;
	union {
		uint16 flags;
		uint16	descriptor_flags:3,			
				if_directory_then_sort:1,	//!< To be set to 0 per UDF-2.01 2.3.5.4
				non_relocatable:1,
				archive:1,
				setuid:1,
				setgid:1,
				sticky:1,
				contiguous:1,
				system:1,
				transformed:1,
				multi_version:1,			//!< To be set to 0 per UDF-2.01 2.3.5.4
				is_stream:1,
				reserved_icb_entry_flags:2;
	};
} __attribute__((packed));


/*! \brief Indirect ICB entry
*/
struct indirect_icb_entry {
	descriptor_tag tag;
	icb_entry_tag icb_tag;
	long_allocation_descriptor indirect_icb;
} __attribute__((packed));


/*! \brief Terminal ICB entry
*/
struct terminal_icb_entry {
	descriptor_tag tag;
	icb_entry_tag icb_tag;
} __attribute__((packed));


/*! \brief File ICB entry

	See also: ECMA-167 4/14.9

	\todo Check pointer math.
*/
struct file_icb_entry {
	descriptor_tag tag;
	icb_entry_tag icb_tag;
	uint32 uid;
	uint32 gid;
	/*! \todo List perms in comment and add handy union thingy */
	uint32 permissions;
	/*! Identifies the number of file identifier descriptors referencing
		this icb.
	*/
	uint16 file_link_count;
	uint8 record_format;				//!< To be set to 0 per UDF-2.01 2.3.6.1
	uint8 record_display_attributes;	//!< To be set to 0 per UDF-2.01 2.3.6.2
	uint8 record_length;				//!< To be set to 0 per UDF-2.01 2.3.6.3
	uint64 information_length;
	uint64 logical_blocks_recorded;		//!< To be 0 for files and dirs with embedded data
	timestamp access_date_and_time;
	timestamp modification_date_and_time;
	timestamp attribute_date_and_time;
	/*! \brief Initially 1, may be incremented upon user request. */
	uint32 checkpoint;
	long_allocation_descriptor extended_attribute_icb;
	entity_id implementation_id;
	/*! \brief The unique id identifying this file entry
	
		The id of the root directory of a file set shall be 0.
		
		\todo Detail the system specific requirements for unique ids from UDF-2.01
	*/
	uint64 unique_id;
	uint32 extended_attributes_length;
	uint32 allocation_descriptors_length;
	
	uint8* extended_attributes() { return (uint8*)(this+sizeof(file_icb_entry)); }
	uint8* allocation_descriptors() { return (uint8*)(this+sizeof(file_icb_entry)+extended_attributes_length); }

} __attribute__((packed));
		

/*! \brief Extended file ICB entry

	See also: ECMA-167 4/14.17
	
	\todo Check pointer math.
*/
struct extended_file_icb_entry {
	descriptor_tag tag;
	icb_entry_tag icb_tag;
	uint32 uid;
	uint32 gid;
	/*! \todo List perms in comment and add handy union thingy */
	uint32 permissions;
	/*! Identifies the number of file identifier descriptors referencing
		this icb.
	*/
	uint16 file_link_count;
	uint8 record_format;				//!< To be set to 0 per UDF-2.01 2.3.6.1
	uint8 record_display_attributes;	//!< To be set to 0 per UDF-2.01 2.3.6.2
	uint8 record_length;				//!< To be set to 0 per UDF-2.01 2.3.6.3
	uint64 information_length;
	uint64 logical_blocks_recorded;		//!< To be 0 for files and dirs with embedded data
	timestamp access_date_and_time;
	timestamp modification_date_and_time;
	timestamp creation_date_and_time;
	timestamp attribute_date_and_time;
	/*! \brief Initially 1, may be incremented upon user request. */
	uint32 checkpoint;
	uint32 reserved;
	long_allocation_descriptor extended_attribute_icb;
	long_allocation_descriptor stream_directory_icb;
	entity_id implementation_id;
	/*! \brief The unique id identifying this file entry
	
		The id of the root directory of a file set shall be 0.
		
		\todo Detail the system specific requirements for unique ids from UDF-2.01
	*/
	uint64 unique_id;
	uint32 extended_attributes_length;
	uint32 allocation_descriptors_length;
	
	uint8* extended_attributes() { return (uint8*)(this+sizeof(file_icb_entry)); }
	uint8* allocation_descriptors() { return (uint8*)(this+sizeof(file_icb_entry)+extended_attributes_length); }

} __attribute__((packed));
		

};	// namespace UDF

#endif	// _UDF_DISK_STRUCTURES_H

