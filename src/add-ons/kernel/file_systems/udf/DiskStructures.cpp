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

const charspec kCS0Charspec = { character_set_type: 0,
                                character_set_info: "OSTA Compressed Unicode"
                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              };
                              
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
// udf_tag
//----------------------------------------------------------------------

/*! \brief Calculates the tag's CRC, verifies the tag's checksum, and
	verifies the tag's location on the medium.
	
	\todo Calc the CRC.
*/
status_t 
descriptor_tag::init_check(off_t diskLocation)
{
	status_t err = (diskLocation == (off_t)location) ? B_OK : B_NO_INIT;
	// checksum
	if (!err) {
		uint32 sum = 0;
		for (int i = 0; i <= 3; i++)
			sum += ((uint8*)this)[i];
		for (int i = 5; i <= 15; i++)
			sum += ((uint8*)this)[i];
		err = sum % 256 == checksum ? B_OK : B_NO_INIT;
	}
	
	return err;
	
}

