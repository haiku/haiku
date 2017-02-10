//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file PartitionAllocator.h

	Udf physical partition allocator (implementation).
*/

#include "PhysicalPartitionAllocator.h"

extent_address PhysicalPartitionAllocator::dummyExtent;

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
                                          long_address &extent,
                                          extent_address &physicalExtent)
{
	status_t error = fAllocator.GetNextExtent(length, contiguous, physicalExtent, fOffset);
	if (!error) {
		extent.set_partition(PartitionNumber());
		extent.set_block(physicalExtent.location()-fOffset);
		extent.set_length(physicalExtent.length());
	}
	return error;
}	                                      

/*! \brief Allocates enough extents to add up to length bytes and stores said
           extents in the given address lists.

	\param length The desired length (in bytes) to be allocated.
	\param extents Output parameter into which the extents as allocated
	               in the partition are stored. 
	\param physicalExtent Output parameter into which the extents as allocated
	                      on the physical volume are stored. 

	\return
	- B_OK: Success.
	- error code: Failure.
*/
status_t
PhysicalPartitionAllocator::GetNextExtents(off_t length, std::list<long_address> &extents,
	                                       std::list<extent_address> &physicalExtents)
{
	DEBUG_INIT_ETC("PhysicalPartitionAllocator", ("length: %lld", length));
	extents.empty();
	physicalExtents.empty();
	
	// Allocate extents until we're done or we hit an error
	status_t error = B_OK;
	while (error == B_OK) {
		long_address extent;
		extent_address physicalExtent;
		uint32 chunkLength = length <= ULONG_MAX ? uint32(length) : ULONG_MAX; 
		error = GetNextExtent(chunkLength, false, extent, physicalExtent);
		if (!error) {
			extents.push_back(extent);
			physicalExtents.push_back(physicalExtent);
			if (physicalExtent.length() > chunkLength) {
				// This should never happen, but just to be safe
				PRINT(("ERROR: allocated extent length longer than requested "
				       " extent length (allocated: %ld, requested: %ld)\n",
				       physicalExtent.length(), chunkLength));
				error = B_ERROR;
			} else {
				// ToDo: Might want to add some checks for 0 length allocations here
				length -= physicalExtent.length();
				if (length == 0) {
					// All done
					break;
				}
			}
		}
	}
	RETURN(error);
}

/*! \brief Returns the length of the partition in blocks.
*/
uint32
PhysicalPartitionAllocator::Length() const
{
	uint32 length = fAllocator.Length();
	return fOffset >= length ? 0 : length-fOffset;
}
