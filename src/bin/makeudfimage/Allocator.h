//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file Allocator.h

	Physical block allocator class declarations.
*/

#ifndef _ALLOCATOR_H
#define _ALLOCATOR_H

#include <list>

#include "UdfStructures.h"

/*! \brief This class keeps track of allocated and unallocated
	blocks in the range of 0 to 2^32.
	
	By default, all blocks are unallocated.
*/
class Allocator {
public:
	Allocator(uint32 blockSize);
	status_t InitCheck() const;
	
	status_t GetBlock(uint32 block);
	status_t GetExtent(Udf::extent_address extent);

	status_t GetNextBlock(uint32 &block, uint32 minimumBlock = 0);
	status_t GetNextExtent(uint32 length, bool contiguous,
	                       Udf::extent_address &extent,
	                       uint32 minimumStartingBlock = 0);
	                         
	uint32 Length() const { return fLength; }
	uint32 Tail() const { return fLength; }	//!< Returns the first unallocated block in the tail
	uint32 BlockSize() const { return fBlockSize; }
	uint32 BlockShift() const { return fBlockShift; }
	
	uint32 BlocksFor(off_t bytes);
private:
	list<Udf::extent_address> fChunkList;
	uint32 fLength;	//!< Length of allocation so far, in blocks.
	uint32 fBlockSize;
	uint32 fBlockShift;
	status_t fInitStatus;
};

#endif	// _ALLOCATOR_H
