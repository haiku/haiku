//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file PartitionAllocator.h

	Udf physical partition allocator (implementation).
*/

#include "PhysicalPartitionAllocator.h"

Udf::extent_address PhysicalPartitionAllocator::dummyExtent;

PhysicalPartitionAllocator::PhysicalPartitionAllocator(uint16 number,
                                                       uint32 offset,
                                                       Allocator &allocator)
	: fNumber(number)
	, fOffset(offset)
	, fAllocator(allocator)
{

}                                                       

/*! \brief Allocates the next available block.

	\param block Output parameter into which the number of the
	             allocated block (in the partition) is stored.
	\param physicalBlock Output parameter into which the number of the
	                     allocated block (on the physical volume) is
	                     stored.

	\return
	- B_OK: Success.
	- error code: Failure, no blocks available.
*/
status_t
PhysicalPartitionAllocator::GetNextBlock(uint32 &block, uint32 &physicalBlock)
{
	status_t error = fAllocator.GetNextBlock(physicalBlock, fOffset);
	if (!error) 
		block = physicalBlock-fOffset;
	return error;
}

/*! \brief Allocates the next available extent of given length.

	\param length The desired length (in bytes) of the extent.
	\param contiguous If false, signals that an extent of shorter length will
	                  be accepted. This allows for small chunks of
	                  unallocated space to be consumed, provided a
	                  contiguous chunk is not needed. 
	\param extent Output parameter into which the extent as allocated
	              in the partition is stored. Note that the length
	              field of the extent may be shorter than the length
	              parameter passed to this function is \a contiguous is
	              false.
	\param physicalExtent Output parameter into which the extent as allocated
	                      on the physical volume is stored. Note that the length
	                      field of the extent may be shorter than the length
	                      parameter passed to this function is \a contiguous is
	                      false.

	\return
	- B_OK: Success.
	- error code: Failure.
*/
status_t
PhysicalPartitionAllocator::GetNextExtent(uint32 length,
	                                      bool contiguous,
                                          Udf::long_address &extent,
                                          Udf::extent_address &physicalExtent)
{
	status_t error = fAllocator.GetNextExtent(length, contiguous, physicalExtent, fOffset);
	if (!error) {
		extent.set_partition(PartitionNumber());
		extent.set_block(physicalExtent.location()-fOffset);
		extent.set_length(physicalExtent.length());
	}
	return error;
}	                                      

/*! \brief Returns the length of the partition in blocks.
*/
uint32
PhysicalPartitionAllocator::Length() const
{
	uint32 length = fAllocator.Length();
	return fOffset >= length ? 0 : length-fOffset;
}
