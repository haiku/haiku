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

/*! \brief Calculates the tag's CRC and verifies the tag's location on
	the medium.
	
	\todo Calc the CRC.
*/
status_t 
descriptor_tag::init_check(off_t diskLocation)
{
	if (diskLocation == (off_t)location)
		return B_OK;
	else
		return B_NO_INIT;
}

