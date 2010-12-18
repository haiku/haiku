/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef DATASTREAM_H
#define DATASTREAM_H


#include "ext2.h"
#include "Transaction.h"


class Volume;


class DataStream
{
public:
					DataStream(Volume* volume, ext2_data_stream* stream,
						off_t size);
					~DataStream();

	status_t		FindBlock(off_t offset, fsblock_t& block,
						uint32 *_count = NULL);
	status_t		Enlarge(Transaction& transaction, off_t& numBlocks);
	status_t		Shrink(Transaction& transaction, off_t& numBlocks);

private:
	uint32			_BlocksNeeded(off_t end);

	status_t		_GetBlock(Transaction& transaction, uint32& block);
	status_t		_PrepareBlock(Transaction& transaction, uint32* pos,
						uint32& blockNum, bool& clear);

	status_t		_AddBlocks(Transaction& transaction, uint32* block,
						off_t count);
	status_t		_AddBlocks(Transaction& transaction, uint32* block,
						off_t start, off_t end, int recursion);

	status_t		_AddForDirectBlocks(Transaction& transaction,
						uint32 numBlocks);
	status_t		_AddForIndirectBlock(Transaction& transaction,
						uint32 numBlocks);
	status_t		_AddForDoubleIndirectBlock(Transaction& transaction,
						uint32 numBlocks);
	status_t		_AddForTripleIndirectBlock(Transaction& transaction,
						uint32 numBlocks);

	status_t		_PerformFree(Transaction& transaction);
	status_t		_MarkBlockForRemoval(Transaction& transaction,
						uint32* block);

	status_t		_FreeBlocks(Transaction& transaction, uint32* block,
						uint32 count);
	status_t		_FreeBlocks(Transaction& transaction, uint32* block,
						off_t start, off_t end, bool freeParent,
						int recursion);

	status_t		_RemoveFromDirectBlocks(Transaction& transaction,
						uint32 numBlocks);
	status_t		_RemoveFromIndirectBlock(Transaction& transaction,
						uint32 numBlocks);
	status_t		_RemoveFromDoubleIndirectBlock(Transaction& transaction,
						uint32 numBlocks);
	status_t		_RemoveFromTripleIndirectBlock(Transaction& transaction,
						uint32 numBlocks);


	const uint32	kBlockSize;
	const uint32	kIndirectsPerBlock;
	const uint32	kIndirectsPerBlock2;
	const uint32	kIndirectsPerBlock3;

	const uint32	kMaxDirect;
	const uint32	kMaxIndirect;
	const uint32	kMaxDoubleIndirect;

	Volume*			fVolume;
	ext2_data_stream* fStream;
	uint32			fFirstBlock;

	uint32			fAllocated;
	fsblock_t		fAllocatedPos;
	uint32			fWaiting;

	uint32			fFreeStart;
	uint32			fFreeCount;

	off_t			fNumBlocks;
	uint32			fRemovedBlocks;
	off_t			fSize;
};

#endif	// DATASTREAM_H

