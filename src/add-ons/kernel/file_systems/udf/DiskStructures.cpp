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

using namespace UDF;

//----------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------

//const charspec kCS0Charspec = { fCharacterSetType: 0,
//                                fCharacterSetInfo: "OSTA Compressed Unicode"
//                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
//                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
//                              };
                              
// Volume structure descriptor ids 
const char* UDF::kVSDID_BEA 			= "BEA01";
const char* UDF::kVSDID_TEA 			= "TEA01";
const char* UDF::kVSDID_BOOT 		= "BOOT2";
const char* UDF::kVSDID_ISO 			= "CD001";
const char* UDF::kVSDID_ECMA167_2 	= "NSR02";
const char* UDF::kVSDID_ECMA167_3 	= "NSR03";
const char* UDF::kVSDID_ECMA168		= "CDW02";

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
charspec::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "extent_address");
	PRINT(("character_set_type: %d\n", character_set_type()));
	PRINT(("character_set_info: `%s'\n", character_set_info()));
}


//----------------------------------------------------------------------
// timestamp
//----------------------------------------------------------------------

void
timestamp::dump()
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

void
entity_id::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "entity_id");
	PRINT(("flags:             %d\n", flags()));
	PRINT(("identifier:        `%s'\n", identifier()));
	PRINT(("identifier_suffix: `%s'\n", identifier_suffix()));
}


//----------------------------------------------------------------------
// extent_address
//----------------------------------------------------------------------

void
extent_address::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_DUMP, "extent_address");
	PRINT(("length:   %ld\n", length()));
	PRINT(("location: %ld\n", location()));
}


//----------------------------------------------------------------------
// udf_tag
//----------------------------------------------------------------------

void
descriptor_tag::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "descriptor_tag");
	PRINT(("id:            %d\n", id()));
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
descriptor_tag::init_check(uint32 diskBlock)
{
	DEBUG_INIT(CF_PUBLIC | CF_VOLUME_OPS, "descriptor_tag");
	PRINT(("diskLocation == %ld\n", diskBlock));
	PRINT(("location() == %ld\n", location()));
	status_t err = (diskBlock == location()) ? B_OK : B_NO_INIT;
	// checksum
	if (!err) {
		uint32 sum = 0;
		for (int i = 0; i <= 3; i++)
			sum += ((uint8*)this)[i];
		for (int i = 5; i <= 15; i++)
			sum += ((uint8*)this)[i];
		err = sum % 256 == checksum() ? B_OK : B_NO_INIT;
	}
	
	RETURN(err);
	
}

//----------------------------------------------------------------------
// primary_vd
//----------------------------------------------------------------------

void
primary_vd::dump()
{
	DUMP_INIT(CF_PUBLIC | CF_VOLUME_OPS | CF_DUMP, "primary_vd");
	PRINT(("tag:\n"));
	tag().dump();
	PRINT(("volume_descriptor_sequence_number: %ld\n", volume_descriptor_sequence_number()));
	PRINT(("primary_volume_descriptor_number:  %ld\n", primary_volume_descriptor_number()));
	PRINT(("volume_identifier:                 `%s'\n", volume_identifier()));
	PRINT(("volume_sequence_number:            %d\n", volume_sequence_number()));
	PRINT(("max_volume_sequence_number:        %d\n", max_volume_sequence_number()));
	PRINT(("interchange_level:                 %d\n", interchange_level()));
	PRINT(("max_interchange_level:             %d\n", max_interchange_level()));
	PRINT(("character_set_list:                %ld\n", character_set_list()));
	PRINT(("max_character_set_list:            %ld\n", max_character_set_list()));
	PRINT(("volume_set_identifier:             `%s'\n", volume_set_identifier()));
	PRINT(("descriptor_character_set:\n"));
	descriptor_character_set().dump();
	PRINT(("explanatory_character_set:\n"));
	explanatory_character_set().dump();
	PRINT(("volume_abstract:\n"));
	volume_abstract().dump();
	PRINT(("volume_copyright_notice:\n"));
	volume_copyright_notice().dump();
	PRINT(("application_id:\n"));
	application_id().dump();
	PRINT(("recording_date_and_time:\n"));
	recording_date_and_time().dump();
	PRINT(("implementation_id:\n"));
	implementation_id().dump();
	PRINT(("implementation_use:\n"));
//	for (int i = 0; i < 64/4; i++) {
//		SIMPLE_PRINT(("%.2x,", implementation_use()[i]));
//		if (i % 16 == 0)
//			SIMPLE_PRINT(("\n"));
//	}
}
