/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Extent.h"


Extent::Extent(Inode* inode)
	:
	fInode(inode)
{
}


void
Extent::FillMapEntry(void* pointerToMap)
{
	uint64 firstHalf = *((uint64*)pointerToMap);
	uint64 secondHalf = *((uint64*)pointerToMap + 1);
		//dividing the 128 bits into 2 parts.
	firstHalf = B_BENDIAN_TO_HOST_INT64(firstHalf);
	secondHalf = B_BENDIAN_TO_HOST_INT64(secondHalf);
	fMap->br_state = (firstHalf >> 63);
	fMap->br_startoff = (firstHalf & MASK(63)) >> 9;
	fMap->br_startblock = ((firstHalf & MASK(9)) << 43) | (secondHalf >> 21);
	fMap->br_blockcount = (secondHalf & MASK(21));
	TRACE("Extent::Init: startoff:(%ld), startblock:(%ld), blockcount:(%ld),"
			"state:(%d)\n",
		fMap->br_startoff,
		fMap->br_startblock,
		fMap->br_blockcount,
		fMap->br_state
		);
}


status_t
Extent::Init()
{
	fMap = new(std::nothrow) ExtentMapEntry;
	if (fMap == NULL)
		return B_NO_MEMORY;

	ASSERT(BlockType() == true);
	void* pointerToMap = DIR_DFORK_PTR(fInode->Buffer());
	FillMapEntry(pointerToMap);

	return B_NOT_SUPPORTED;
}


ExtentBlockTail*
Extent::BlockTail(ExtentDataHeader* header)
{
	return (ExtentBlockTail*)
		((char*)header + fInode->DirBlockSize() - sizeof(ExtentBlockTail));
}


ExtentLeafEntry*
Extent::BlockFirstLeaf(ExtentBlockTail* tail)
{
	return (ExtentLeafEntry*)tail - B_BENDIAN_TO_HOST_INT32(tail->count);
}


bool
Extent::BlockType()
{
	bool status = true;
	if (fInode->NoOfBlocks() != 1)
		status = false;
	if (fInode->Size() != fInode->DirBlockSize())
		status = false;
	return status;
}

