//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Allocator.cpp

	Physical block allocator class implementation.
*/

#include "Allocator.h"

#include <limits.h>

#include "Utils.h"

/*! \brief Creates a new Allocator object.
*/
Allocator::Allocator(uint32 blockSize)
	: fLength(0)
	, fBlockSize(blockSize)
	, fBlockShift(0)
	, fInitStatus(B_NO_INIT)
{
	status_t error = get_block_shift(BlockSize(), fBlockShift);
	if (!error)
		fInitStatus = B_OK;
}

status_t
Allocator::InitCheck() const
{
	return fInitStatus;
}

/*! \brief Allocates the given block, if available.

	\return
	- B_OK: Success.
	- error code: Failure, the block has already been allocated.
*/
status_t
Allocator::GetBlock(uint32 block)
{
	extent_address extent(block, BlockSize());
	return GetExtent(extent);
}

/*! \brief Allocates the given extent, if available.

	\return
	- B_OK: Success.
	- error code: Failure, the extent (or some portion of it) has already
	              been allocated.
*/
status_t
Allocator::GetExtent(extent_address extent)
{
	status_t error = InitCheck();
	if (!error) {
		uint32 offset = extent.location();
		uint32 length = BlocksFor(extent.length());
		// First see if the extent is past the allocation tail,
		// since we then don't have to do any chunklist traversal
		if (offset >= Length()) {
			// Add a new chunk to the end of the chunk list if
			// necessary
			if (offset > Length()) {
				extent_address chunk(Length(), (offset-Length())<<BlockShift());
				fChunkList.push_back(chunk);
			}
			// Adjust the tail
			fLength = offset+length;
			return B_OK;
		} else {
			// Block is not past tail, so check the chunk list
			for (list<extent_address>::iterator i = fChunkList.begin();
			       i != fChunkList.end();
			         i++)
			{
				uint32 chunkOffset = i->location();
				uint32 chunkLength = BlocksFor(i->length());
				if (chunkOffset <= offset && (offset+length) <= (chunkOffset+chunkLength)) {
					// Found it. Split the chunk. First look for an orphan
					// before the block, then after.
					if (chunkOffset < offset) {
						// Orhpan before; add a new chunk in front
						// of the current one
						extent_address chunk(chunkOffset, (offset-chunkOffset)<<BlockShift());
						fChunkList.insert(i, chunk);
					}
					if ((offset+length) < (chunkOffset+chunkLength)) {
						// Orphan after; resize the original chunk
						i->set_location(offset+length);
						i->set_length(((chunkOffset+chunkLength)-(offset+length))<<BlockSize());					
					} else {
						// No orphan after; remove the original chunk
						fChunkList.erase(i);
					}
					return B_OK;
				}
			}
			// No matching chunk found, we're SOL.			
			error = B_ERROR;
		}		
	}
	return error;
}

/*! \brief Allocates the next available block.

	\param block Output parameter into which the number of the
	             allocated block is stored.
	\param minimumBlock The minimum acceptable block number (used
	                    by the physical partition allocator).              

	\return
	- B_OK: Success.
	- error code: Failure, no blocks available.
*/
status_t
Allocator::GetNextBlock(uint32 &block, uint32 minimumBlock)
{
	status_t error = InitCheck();
	if (!error) {
		extent_address extent;
		error = GetNextExtent(BlockSize(), true, extent, minimumBlock);
		if (!error)
			block = extent.location();
	}
	return error;
}

/*! \brief Allocates the next available extent of given length.

	\param length The desired length (in bytes) of the extent.
	\param contiguous If false, signals that an extent of shorter length will
	                  be accepted. This allows for small chunks of
	                  unallocated space to be consumed, provided a
	                  contiguous chunk is not needed. 
	\param extent Output parameter into which the extent as allocated
	              is stored. Note that the length field of the extent
	              may be shorter than the length parameter passed
	              to this function is \a contiguous is false.
	\param minimumStartingBlock The minimum acceptable starting block
	                            for the extent (used by the physical
	                            partition allocator).
	                            
	\return
	- B_OK: Success.
	- error code: Failure.
*/
status_t
Allocator::GetNextExtent(uint32 _length, bool contiguous,
                         extent_address &extent,
	                     uint32 minimumStartingBlock)
{
	DEBUG_INIT_ETC("Allocator", ("length: %lld, contiguous: %d", _length, contiguous));
	uint32 length = BlocksFor(_length);
	bool isPartial = false;
	status_t error = InitCheck();
	PRINT(("allocation length: %lu\n", Length()));
	if (!error) {
		for (list<extent_address>::iterator i = fChunkList.begin();
		       i != fChunkList.end();
		         i++)
		{
			uint32 chunkOffset = i->location();
			uint32 chunkLength = BlocksFor(i->length());
			if (chunkOffset < minimumStartingBlock)
			{
				if (minimumStartingBlock < chunkOffset+chunkLength) {
					// Start of chunk is below min starting block. See if
					// any part of the chunk would make for an acceptable
					// allocation
					uint32 difference = minimumStartingBlock - chunkOffset;
					uint32 newOffset = minimumStartingBlock;
					uint32 newLength = chunkLength-difference;
					if (length <= newLength) {
						// new chunk is still long enough
						extent_address newExtent(newOffset, _length);
						if (GetExtent(newExtent) == B_OK) {
							extent = newExtent;
							return B_OK;
						}
					} else if (!contiguous) {
						// new chunk is too short, but we're allowed to
						// allocate a shorter extent, so we'll do it.
						extent_address newExtent(newOffset, newLength<<BlockShift());
						if (GetExtent(newExtent) == B_OK) {
							extent = newExtent;
							return B_OK;
						}
					}
				}
			} else if (length <= chunkLength) {
				// Chunk is larger than necessary. Allocate first
				// length blocks, and resize the chunk appropriately.
				extent.set_location(chunkOffset);
				extent.set_length(_length);
				if (length != chunkLength) {
					i->set_location(chunkOffset+length);
					i->set_length((chunkLength-length)<<BlockShift());
				} else {
					fChunkList.erase(i);
				}
				return B_OK;					
			} else if (!contiguous) {
				extent.set_location(chunkOffset);
				extent.set_length(chunkLength<<BlockShift());
				fChunkList.erase(i);
				return B_OK;					
			}			 
		}
		// No sufficient chunk found, so try to allocate from the tail
		PRINT(("ULONG_MAX: %lu\n", ULONG_MAX));
		uint32 maxLength = ULONG_MAX-Length();
		PRINT(("maxLength: %lu\n", maxLength));
		error = maxLength > 0 ? B_OK : B_DEVICE_FULL;
		if (!error) {
			if (minimumStartingBlock > Tail())
				maxLength -= minimumStartingBlock - Tail();
			uint32 tail = minimumStartingBlock > Tail() ? minimumStartingBlock : Tail();
			if (length > maxLength) {
				if (contiguous)
					error = B_DEVICE_FULL;
				else {
					isPartial = true;
					length = maxLength;
				}
			}
			if (!error) {
				extent_address newExtent(tail, isPartial ? length<<BlockShift() : _length);
				if (GetExtent(newExtent) == B_OK) {
					extent = newExtent;
					return B_OK;
				}
			}
		}
	}
	return error;
}

/*! \brief Returns the number of blocks needed to accomodate the
	given number of bytes.
*/
uint32
Allocator::BlocksFor(off_t bytes)
{
	if (BlockSize() == 0) {
		DEBUG_INIT_ETC("Allocator", ("bytes: %ld\n", bytes));
		PRINT(("WARNING: Allocator::BlockSize() == 0!\n")); 
		return 0;
	} else {
		off_t blocks = bytes >> BlockShift();
		if (bytes % BlockSize() != 0)
			blocks++;
		uint64 mask = 0xffffffff;
		mask <<= 32;
		if (blocks & mask) {
			// ToDo: Convert this to actually signal an error
			DEBUG_INIT_ETC("Allocator", ("bytes: %ld\n", bytes));
			PRINT(("WARNING: bytes argument too large for corresponding number "
			       "of blocks to be specified with a uint32! (bytes: %Ld, blocks: %Ld, "
			       "maxblocks: %ld).\n", bytes, blocks, ULONG_MAX));
			blocks = 0;
		}					
		return blocks;
	}
}
