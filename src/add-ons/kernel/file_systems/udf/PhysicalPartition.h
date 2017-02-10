//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//---------------------------------------------------------------------
#ifndef _UDF_PHYSICAL_PARTITION_H
#define _UDF_PHYSICAL_PARTITION_H

/*! \file PhysicalPartition.h
*/

#include <util/kernel_cpp.h>

#include "Partition.h"
#include "UdfDebug.h"

/*! \brief Standard type 1 physical partition

	PhysicalPartitions map logical block numbers directly to physical
	block numbers.

	See also: ECMA-167 10.7.2
*/
class PhysicalPartition : public Partition {
public:
	PhysicalPartition(uint16 number, uint32 start, uint32 length);
	virtual ~PhysicalPartition();
	virtual status_t MapBlock(uint32 logicalBlock, off_t &physicalBlock);

	uint16 Number() const { return fNumber; }
	uint32 Start() const { return fStart; }
	uint32 Length() const { return fLength; }
private:
	uint16 fNumber;
	uint32 fStart;
	uint32 fLength;
};

#endif	// _UDF_PHYSICAL_PARTITION_H
