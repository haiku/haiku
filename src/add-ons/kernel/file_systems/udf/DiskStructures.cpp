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

using namespace UDF;

//----------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------

const charspec kCS0Charspec = { character_set_type: 0,
                                character_set_info: "OSTA Compressed Unicode"
                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                                                    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
                              };

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

