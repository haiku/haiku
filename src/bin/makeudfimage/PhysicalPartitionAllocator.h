//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file PhysicalPartitionAllocator.h

	Udf physical partition allocator (declarations).
*/

#ifndef _PHYSICAL_PARTITION_ALLOCATOR_H
#define _PHYSICAL_PARTITION_ALLOCATOR_H

#include <list>

#include "Allocator.h"
#include "UdfStructures.h"

/*! \brief Allocates blocks and extents from a Udf physical partition.
*/
class PhysicalPartitionAllocator {
public:
	PhysicalPartitionAllocator(uint16 number, uint32 offset, Allocator &allocator);

	status_t GetNextBlock(uint32 &block, uint32 &physicalBlock);
	status_t GetNextExtent(uint32 length, bool contiguous, long_address &extent,
	                       extent_address &physicalExtent = dummyExtent);                
	status_t GetNextExtents(off_t length, std::list<long_address> &extents,
	                        std::list<extent_address> &physicalExtents);
	                       
	                       
	uint16 PartitionNumber() const { return fNumber; }
	uint32 Length() const;
private:
	static extent_address dummyExtent;
	uint16 fNumber;	//!< The partition number of this partition
	uint32 fOffset;	//!< The offset of the start of this partition in physical space
	Allocator &fAllocator;
};

#endif	// _PHYSICAL_PARTITION_ALLOCATOR_H
