//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_PARTITION_H
#define _UDF_PARTITION_H

/*! \file Partition.h
*/

#include <SupportDefs.h>

/*! \brief Abstract base class for various UDF partition types.
*/
class Partition {
public:
	virtual ~Partition() {}
	virtual status_t MapBlock(uint32 logicalBlock, off_t &physicalBlock) = 0;
//	virtual status_t MapExtent(uint32 logicalBlock, uint32 logicalLength,
//	                           uint32 &physicalBlock, uint32 &physicalLength) = 0;
};

#endif	// _UDF_PARTITION_H
