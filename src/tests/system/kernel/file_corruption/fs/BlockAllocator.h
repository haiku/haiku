/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H


#include <lock.h>


struct Transaction;
struct Volume;


class BlockAllocator {
public:
								BlockAllocator(Volume* volume);
								~BlockAllocator();

			uint64				BaseBlock() const
									{ return fAllocationGroupBlock; }
			uint64				FreeBlocks() const	{ return fFreeBlocks; }

			status_t			Init(uint64 blockBitmap, uint64 freeBlocks);
			status_t			Initialize(Transaction& transaction);

			status_t			Allocate(uint64 baseHint, uint64 count,
									Transaction& transaction,
									uint64& _allocatedBase,
									uint64& _allocatedCount);
			status_t			AllocateExactly(uint64 base,
									uint64 count, Transaction& transaction);
			status_t			Free(uint64 base, uint64 count,
									Transaction& transaction);

			void				ResetFreeBlocks(uint64 count);
									// interface for Transaction only

private:
			status_t			_Allocate(uint64 base, uint64 searchEnd,
									uint64 count, Transaction& transaction,
									uint64* _allocatedBase,
									uint64& _allocatedCount);
			status_t			_AllocateInGroup(uint64 base, uint64 searchEnd,
									uint32 count, Transaction& transaction,
									uint64* _allocatedBase,
									uint32& _allocatedCount);
			status_t			_AllocateInBitmapBlock(uint64 base,
									uint32 count, Transaction& transaction,
									uint64* _allocatedBase,
									uint32& _allocatedCount);

			status_t			_Free(uint64 base, uint64 count,
									Transaction& transaction);
			status_t			_FreeInGroup(uint64 base, uint32 count,
									Transaction& transaction);
			status_t			_FreeInBitmapBlock(uint64 base, uint32 count,
									Transaction& transaction);

			status_t			_UpdateSuperBlock(Transaction& transaction);

private:
			mutex				fLock;
			Volume*				fVolume;
			uint64				fTotalBlocks;
			uint64				fFreeBlocks;
			uint64				fAllocationGroupBlock;
			uint64				fAllocationGroupCount;
			uint64				fBitmapBlock;
			uint64				fBitmapBlockCount;
};


class AllocatedBlock {
public:
	AllocatedBlock(BlockAllocator* allocator, Transaction& transaction)
		:
		fAllocator(allocator),
		fTransaction(transaction),
		fIndex(0)
	{
	}

	~AllocatedBlock()
	{
		if (fIndex > 0)
			fAllocator->Free(fIndex, 1, fTransaction);
	}

	uint64 Index() const
	{
		return fIndex;
	}

	status_t Allocate(uint64 baseHint = 0)
	{
		uint64 allocatedBlocks;
		status_t error = fAllocator->Allocate(0, 1, fTransaction, fIndex,
			allocatedBlocks);
		if (error != B_OK)
			fIndex = 0;
		return error;
	}

	uint64 Detach()
	{
		uint64 index = fIndex;
		fIndex = 0;
		return index;
	}

private:
	BlockAllocator*	fAllocator;
	Transaction&	fTransaction;
	uint64			fIndex;
};


#endif	// BLOCK_ALLOCATOR_H
