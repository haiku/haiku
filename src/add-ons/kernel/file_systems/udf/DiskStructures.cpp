//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file DiskStructures.cpp

	UDF on-disk data structure definitions
*/

#include "DiskStructures.h"

#include <string.h>

#include "CS0String.h"

using namespace Udf;

//----------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------

//const charspec kCS0Charspec = { _character_set_type: 0,
//                                _character_set_info: "OSTA Compressed Unicode"
//                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
//                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
//                              };
                              
// Volume structure descriptor ids 
const char* Udf::kVSDID_BEA 		= "BEA01";
const char* Udf::kVSDID_TEA 		= "TEA01";
const char* Udf::kVSDID_BOOT 		= "BOOT2";
const char* Udf::kVSDID_ISO 		= "CD001";
const char* Udf::kVSDID_ECMA167_2 	= "NSR02";
const char* Udf::kVSDID_ECMA167_3 	= "NSR03";
const char* Udf::kVSDID_ECMA168		= "CDW02";

// entity_ids
const entity_id Udf::kMetadataPartitionMapId(0, "*UDF Metadata Partition");
const entity_id Udf::kSparablePartitionMapId(0, "*UDF Sparable Partition");
const entity_id Udf::kVirtualPartitionMapId(0, "*UDF Virtual Partition");

//----------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------

const char *Udf::tag_id_to_string(tag_id id)
{
	switch (id) {
		case TAGID_UNDEFINED:
			return "undefined";

		case TAGID_PRIMARY_VOLUME_DESCRIPTOR:
			return "primary volume descriptor";			
		case TAGID_ANCHOR_VOLUME_DESCRIPTOR_POINTER:
			return "anchor volume descriptor pointer";
		case TAGID_VOLUME_DESCRIPTOR_POINTER:
			return "volume descriptor pointer";
		case TAGID_IMPLEMENTATION_USE_VOLUME_DESCRIPTOR:
			return "implementation use volume descriptor";
		case TAGID_PARTITION_DESCRIPTOR:
			return "partition descriptor";
		case TAGID_LOGICAL_VOLUME_DESCRIPTOR:
			return "logical volume descriptor";
		case TAGID_UNALLOCATED_SPACE_DESCRIPTOR:
			return "unallocated space descriptor";
		case TAGID_TERMINATING_DESCRIPTOR:
			return "terminating descriptor";
		case TAGID_LOGICAL_VOLUME_INTEGRITY_DESCRIPTOR:
			return "logical volume integrity descriptor";

		case TAGID_FILE_SET_DESCRIPTOR:
			return "file set descriptor";
		case TAGID_FILE_IDENTIFIER_DESCRIPTOR:
			return "file identifier descriptor";
		case TAGID_ALLOCATION_EXTENT_DESCRIPTOR:
			return "allocation extent descriptor";
		case TAGID_INDIRECT_ENTRY:
			return "indirect entry";
		case TAGID_TERMINAL_ENTRY:
			return "terminal entry";
		case TAGID_FILE_ENTRY:
			return "file entry";
		case TAGID_EXTENDED_ATTRIBUTE_HEADER_DESCRIPTOR:
			return "extended attribute header descriptor";
		case TAGID_UNALLOCATED_SPACE_ENTRY:
			return "unallocated space entry";
		case TAGID_SPACE_BITMAP_DESCRIPTOR:
			return "space bitmap descriptor";
		case TAGID_PARTITION_INTEGRITY_ENTRY:
			return "partition integrity entry";
		case TAGID_EXTENDED_FILE_ENTRY:
			return "extended file entry";

		default:
			if (TAGID_CUSTOM_START <= id && id <= TAGID_CUSTOM_END)
				return "custom";
			return "reserved";	
	}
}


//----------------------------------------------------------------------
// volume_structure_descriptor_header
//----------------------------------------------------------------------

/*! \brief Returns true if the given \a id matches the header's id.
*/
bool
volume_structure_descriptor_header::id_matches(const char *id)
{
	return strncmp(this->id, id, 5) == 0;
}


//----------------------------------------------------------------------
// charspec
//----------------------------------------------------------------------

void
charspec::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "charspec");
	PRINT(("character_set_type: %d\n", character_set_type()));
	PRINT(("character_set_info: `%s'\n", character_set_info()));
}


//----------------------------------------------------------------------
// timestamp
//----------------------------------------------------------------------

void
timestamp::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "timestamp");
	PRINT(("type:                %d\n", type()));
	PRINT(("timezone:            %d\n", timezone()));
	PRINT(("year:                %d\n", year()));
	PRINT(("month:               %d\n", month()));
	PRINT(("day:                 %d\n", day()));
	PRINT(("hour:                %d\n", hour()));
	PRINT(("minute:              %d\n", minute()));
	PRINT(("second:              %d\n", second()));
	PRINT(("centisecond:         %d\n", centisecond()));
	PRINT(("hundred_microsecond: %d\n", hundred_microsecond()));
	PRINT(("microsecond:         %d\n", microsecond()));
}

//----------------------------------------------------------------------
// entity_id
//----------------------------------------------------------------------

entity_id::entity_id(uint8 flags, char *identifier, char *identifier_suffix)
	: _flags(flags)
{
	if (identifier)
		strncpy(_identifier, identifier, kIdentifierLength);
	else
		memset(_identifier, 0, kIdentifierLength);
	if (identifier_suffix)
		strncpy(_identifier_suffix, identifier_suffix, kIdentifierSuffixLength);
	else
		memset(_identifier_suffix, 0, kIdentifierSuffixLength);
}

void
entity_id::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "entity_id");
	PRINT(("flags:             %d\n", flags()));
	PRINT(("identifier:        `%.23s'\n", identifier()));
	PRINT(("identifier_suffix: `%s'\n", identifier_suffix()));
}

bool
entity_id::matches(const entity_id &id) const
{
	bool result = true;
	for (int i = 0; i < entity_id::kIdentifierLength; i++) {
		if (identifier()[i] != id.identifier()[i]) {
			result = false;
			break;
		}
	}
	return result;
}

//----------------------------------------------------------------------
// extent_address
//----------------------------------------------------------------------

void
extent_address::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "extent_address");
	PRINT(("length:   %ld\n", length()));
	PRINT(("location: %ld\n", location()));
}

void
logical_block_address::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "logical_block_address");
	PRINT(("block:     %ld\n", block()));
	PRINT(("partition: %d\n", partition()));
}

void
long_address::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "long_address");
	PRINT(("length:   %ld\n", length()));
	PRINT(("block:    %ld\n", block()));
	PRINT(("partiton: %d\n", partition()));
	PRINT(("implementation_use:\n"));
	DUMP(implementation_use());
}

//----------------------------------------------------------------------
// descriptor_tag 
//----------------------------------------------------------------------

void
descriptor_tag ::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "descriptor_tag");
	PRINT(("id:            %d (%s)\n", id(), tag_id_to_string(tag_id(id()))));
	PRINT(("version:       %d\n", version()));
	PRINT(("checksum:      %d\n", checksum()));
	PRINT(("serial_number: %d\n", serial_number()));
	PRINT(("crc:           %d\n", crc()));
	PRINT(("crc_length:    %d\n", crc_length()));
	PRINT(("location:      %ld\n", location()));
}


/*! \brief Calculates the tag's CRC, verifies the tag's checksum, and
	verifies the tag's location on the medium.
	
	\todo Calc the CRC.
*/
status_t 
descriptor_tag ::init_check(uint32 diskBlock)
{
	DEBUG_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_HIGH_VOLUME, "descriptor_tag");
	PRINT(("location (paramater)    == %ld\n", diskBlock));
	PRINT(("location (in structure) == %ld\n", location()));
	status_t error = (diskBlock == location()) ? B_OK : B_NO_INIT;
	// checksum
	if (!error) {
		uint32 sum = 0;
		for (int i = 0; i <= 3; i++)
			sum += ((uint8*)this)[i];
		for (int i = 5; i <= 15; i++)
			sum += ((uint8*)this)[i];
		error = sum % 256 == checksum() ? B_OK : B_NO_INIT;
	}
	
	RETURN(error);
	
}


//----------------------------------------------------------------------
// primary_descriptor
//----------------------------------------------------------------------

void
primary_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "primary_descriptor");
	
	CS0String string;
	
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("vds_number:                       %ld\n", vds_number()));
	PRINT(("primary_volume_descriptor_number: %ld\n", primary_volume_descriptor_number()));
	string = volume_identifier();
	PRINT(("volume_identifier:                `%s'\n", string.String()));
	PRINT(("volume_sequence_number:           %d\n", volume_sequence_number()));
	PRINT(("max_volume_sequence_number:       %d\n", max_volume_sequence_number()));
	PRINT(("interchange_level:                %d\n", interchange_level()));
	PRINT(("max_interchange_level:            %d\n", max_interchange_level()));
	PRINT(("character_set_list:               %ld\n", character_set_list()));
	PRINT(("max_character_set_list:           %ld\n", max_character_set_list()));
	string = volume_set_identifier();
	PRINT(("volume_set_identifier:            `%s'\n", string.String()));
	PRINT(("descriptor_character_set:\n"));
	DUMP(descriptor_character_set());
	PRINT(("explanatory_character_set:\n"));
	DUMP(explanatory_character_set());
	PRINT(("volume_abstract:\n"));
	DUMP(volume_abstract());
	PRINT(("volume_copyright_notice:\n"));
	DUMP(volume_copyright_notice());
	PRINT(("application_id:\n"));
	DUMP(application_id());
	PRINT(("recording_date_and_time:\n"));
	DUMP(recording_date_and_time());
	PRINT(("implementation_id:\n"));
	DUMP(implementation_id());
	PRINT(("implementation_use:\n"));
	DUMP(implementation_use());
}


//----------------------------------------------------------------------
// anchor_volume_descriptor_pointer
//----------------------------------------------------------------------

void
anchor_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "anchor_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("main_vds:\n"));
	DUMP(main_vds());
	PRINT(("reserve_vds:\n"));
	DUMP(reserve_vds());
}


//----------------------------------------------------------------------
// implementation_use_descriptor
//----------------------------------------------------------------------

void
implementation_use_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "implementation_use_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("vds_number: %ld\n", vds_number()));
	PRINT(("implementation_id:\n"));
	DUMP(implementation_id());
	PRINT(("implementation_use: XXX\n"));
	DUMP(implementation_use());
}

//----------------------------------------------------------------------
// partition_descriptor
//----------------------------------------------------------------------

const uint8 Udf::kMaxPartitionDescriptors = 2;

void
partition_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "partition_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("vds_number:                %ld\n", vds_number()));
	PRINT(("partition_flags:           %d\n", partition_flags()));
	PRINT(("partition_flags.allocated: %s\n", allocated() ? "true" : "false"));
	PRINT(("partition_number:          %d\n", partition_number()));
	PRINT(("partition_contents:\n"));
	DUMP(partition_contents());
	PRINT(("partition_contents_use:    XXX\n"));
	DUMP(partition_contents_use());
	PRINT(("access_type:               %ld\n", access_type()));
	PRINT(("start:                     %ld\n", start()));
	PRINT(("length:                    %ld\n", length()));
	PRINT(("implementation_id:\n"));
	DUMP(implementation_id());
	PRINT(("implementation_use:        XXX\n"));
	DUMP(implementation_use());
}

//----------------------------------------------------------------------
// logical_descriptor
//----------------------------------------------------------------------

void
logical_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "logical_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("vds_number:                %ld\n", vds_number()));
	PRINT(("character_set:\n"));
	DUMP(character_set());
	CS0String string(logical_volume_identifier());
	PRINT(("logical_volume_identifier: `%s'\n", string.String()));
	PRINT(("logical_block_size:        %ld\n", logical_block_size()));
	PRINT(("domain_id:\n"));
	DUMP(domain_id());
	PRINT(("logical_volume_contents_use:\n"));
	DUMP(logical_volume_contents_use());
	PRINT(("file_set_address:\n"));
	DUMP(file_set_address());
	PRINT(("map_table_length:          %ld\n", map_table_length()));
	PRINT(("partition_map_count:       %ld\n", partition_map_count()));
	PRINT(("implementation_id:\n"));
	DUMP(implementation_id());
	PRINT(("implementation_use:\n"));
	DUMP(implementation_use());
	PRINT(("integrity_sequence_extent:\n"));
	DUMP(integrity_sequence_extent());
//	PRINT(("partition_maps:\n"));
	const uint8 *maps = partition_maps();
	int offset = 0;
	for (uint i = 0; i < partition_map_count(); i++) {
		PRINT(("partition_map #%d:\n", i));
		uint8 type = maps[offset];
		uint8 length = maps[offset+1];
		PRINT(("  type: %d\n", type));
		PRINT(("  length: %d\n", length));
		switch (type) {
			case 1:
				for (int j = 0; j < length-2; j++) 
					PRINT(("  data[%d]: %d\n", j, maps[offset+2+j]));
				break;
			case 2: {
				PRINT(("  partition_number: %d\n", *reinterpret_cast<const uint16*>(&(maps[offset+38]))));
				PRINT(("  entity_id:\n"));
				const entity_id *id = reinterpret_cast<const entity_id*>(&(maps[offset+4]));
				if (id)	// To kill warning when DEBUG==0
					PDUMP(id);
				break;
			}
		}
		offset += maps[offset+1];
	}
	// \todo dump partition_maps
}


logical_descriptor&
logical_descriptor::operator=(const logical_descriptor &rhs)
{
	_tag = rhs._tag;
	_vds_number = rhs._vds_number;
	_character_set = rhs._character_set;
	_logical_volume_identifier = rhs._logical_volume_identifier;
	_logical_block_size = rhs._logical_block_size;
	_domain_id = rhs._domain_id;
	_logical_volume_contents_use = rhs._logical_volume_contents_use;
	_map_table_length = rhs._map_table_length;
	_partition_map_count = rhs._partition_map_count;
	_implementation_id = rhs._implementation_id;
	_implementation_use = rhs._implementation_use;
	_integrity_sequence_extent = rhs._integrity_sequence_extent;
	// copy the partition maps one by one
	uint8 *lhsMaps = partition_maps();
	const uint8 *rhsMaps = rhs.partition_maps();
	int offset = 0;
	for (uint8 i = 0; i < rhs.partition_map_count(); i++) {
		uint8 length = rhsMaps[offset+1];
		memcpy(&lhsMaps[offset], &rhsMaps[offset], length);
		offset += length;		
	}
	return *this;
}


//----------------------------------------------------------------------
// physical_partition_map 
//----------------------------------------------------------------------

void
physical_partition_map::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "physical_partition_map");
	PRINT(("type: %d\n", type()));
	PRINT(("length: %d\n", length()));
	PRINT(("volume_sequence_number: %d\n", volume_sequence_number()));
	PRINT(("partition_number: %d\n", partition_number()));
}

//----------------------------------------------------------------------
// sparable_partition_map 
//----------------------------------------------------------------------

void
sparable_partition_map::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "sparable_partition_map");
	PRINT(("type: %d\n", type()));
	PRINT(("length: %d\n", length()));
	PRINT(("partition_type_id:"));
	DUMP(partition_type_id());
	PRINT(("volume_sequence_number: %d\n", volume_sequence_number()));
	PRINT(("partition_number: %d\n", partition_number()));
	PRINT(("sparing_table_count: %d\n", sparing_table_count()));
	PRINT(("sparing_table_size: %ld\n", sparing_table_size()));
	PRINT(("sparing_table_locations:"));
	for (uint8 i = 0; i < sparing_table_count(); i++)
		PRINT(("  %d: %ld\n", i, sparing_table_location(i)));
}

//----------------------------------------------------------------------
// unallocated_space_descriptor
//----------------------------------------------------------------------

void
unallocated_space_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "unallocated_space_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("vds_number:                  %ld\n", vds_number()));
	PRINT(("allocation_descriptor_count: %ld\n", allocation_descriptor_count()));
	// \todo dump alloc_descriptors
}


//----------------------------------------------------------------------
// terminating_descriptor
//----------------------------------------------------------------------

void
terminating_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "terminating_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
}

//----------------------------------------------------------------------
// file_set_descriptor
//----------------------------------------------------------------------

void
file_set_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "file_set_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("recording_date_and_time:\n"));
	DUMP(recording_date_and_time());
	PRINT(("interchange_level: %d\n", interchange_level()));
	PRINT(("max_interchange_level: %d\n", max_interchange_level()));
	PRINT(("character_set_list: %ld\n", character_set_list()));
	PRINT(("max_character_set_list: %ld\n", max_character_set_list()));
	PRINT(("file_set_number: %ld\n", file_set_number()));
	PRINT(("file_set_descriptor_number: %ld\n", file_set_descriptor_number()));
	PRINT(("logical_volume_id_character_set:\n"));
	DUMP(logical_volume_id_character_set());
	PRINT(("logical_volume_id:\n"));
	DUMP(logical_volume_id());
	PRINT(("file_set_charspec:\n"));
	DUMP(file_set_charspec());
	PRINT(("file_set_id:\n"));
	DUMP(file_set_id());
	PRINT(("copyright_file_id:\n"));
	DUMP(copyright_file_id());
	PRINT(("abstract_file_id:\n"));
	DUMP(abstract_file_id());
	PRINT(("root_directory_icb:\n"));
	DUMP(root_directory_icb());
	PRINT(("domain_id:\n"));
	DUMP(domain_id());
	PRINT(("next_extent:\n"));
	DUMP(next_extent());
	PRINT(("system_stream_directory_icb:\n"));
	DUMP(system_stream_directory_icb());
}

void
file_id_descriptor::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS, "file_id_descriptor");
	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("version_number:            %d\n", version_number()));
	PRINT(("may_be_hidden:             %d\n", may_be_hidden()));
	PRINT(("is_directory:              %d\n", is_directory()));
	PRINT(("is_deleted:                %d\n", is_deleted()));
	PRINT(("is_parent:                 %d\n", is_parent()));
	PRINT(("is_metadata_stream:        %d\n", is_metadata_stream()));
	PRINT(("id_length:                 %d\n", id_length()));
	PRINT(("icb:\n"));
	DUMP(icb());
	PRINT(("implementation_use_length: %d\n", is_parent()));
	PRINT(("id: `"));
	for (int i = 0; i < id_length(); i++)
		SIMPLE_PRINT(("%c", id()[i]));
	SIMPLE_PRINT(("'\n"));
}

void
icb_entry_tag::dump() const
{
	DUMP_INIT(CF_PUBLIC, "icb_entry_tag");
	PRINT(("prior_entries: %ld\n", prior_recorded_number_of_direct_entries()));
	PRINT(("strategy_type: %d\n", strategy_type()));
	PRINT(("strategy_parameters:\n"));
	DUMP(strategy_parameters());
	PRINT(("entry_count: %d\n", entry_count()));
	PRINT(("file_type: %d\n", file_type()));
	PRINT(("parent_icb_location:\n"));
	DUMP(parent_icb_location());
	PRINT(("all_flags: %d\n", flags()));
	
/*
	uint32 prior_recorded_number_of_direct_entries;
	uint16 strategy_type;
	array<uint8, 2> strategy_parameters;
	uint16 entry_count;
	uint8 reserved;
	uint8 file_type;
	logical_block_address parent_icb_location;
	union {
		uint16 all_flags;
		struct {
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
		} flags;
	};

*/

}

void
icb_header::dump() const
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "icb_header");

	PRINT(("tag:\n"));
	DUMP(tag());
	PRINT(("icb_tag:\n"));
	DUMP(icb_tag());
	
}
