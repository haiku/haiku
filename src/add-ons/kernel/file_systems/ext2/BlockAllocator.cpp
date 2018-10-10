/*
 * Copyright 2001-2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 *		Jérôme Duval
 */


#include "BlockAllocator.h"

#include <util/AutoLock.h>

#include "BitmapBlock.h"
#include "Inode.h"


#undef ASSERT
//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("ext2: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


class AllocationBlockGroup : public TransactionListener {
public:
						AllocationBlockGroup();
						~AllocationBlockGroup();

			status_t	Initialize(Volume* volume, uint32 blockGroup,
							uint32 numBits);

			bool		IsFull() const;

			status_t	Allocate(Transaction& transaction, fsblock_t start,
							uint32 length);
			status_t	Free(Transaction& transaction, uint32 start,
							uint32 length);
			status_t	FreeAll(Transaction& transaction);
			status_t	Check(uint32 start, uint32 length);

			uint32		NumBits() const;
			uint32		FreeBits() const;
			fsblock_t	Start() const;

			fsblock_t	LargestStart() const;
			uint32		LargestLength() const;

			// TransactionListener implementation
			void		TransactionDone(bool success);
			void		RemovedFromTransaction();

private:
			status_t	_ScanFreeRanges(bool verify = false);
			void		_AddFreeRange(uint32 start, uint32 length);
			void		_LockInTransaction(Transaction& transaction);
			status_t	_InitGroup(Transaction& transaction);
			bool		_IsSparse();
			uint32		_FirstFreeBlock();
			void		_SetBlockBitmapChecksum(BitmapBlock& block);
			bool		_VerifyBlockBitmapChecksum(BitmapBlock& block);

			Volume*		fVolume;
			uint32		fBlockGroup;
			ext2_block_group* fGroupDescriptor;

			mutex		fLock;
			mutex		fTransactionLock;
			int32		fCurrentTransaction;

			fsblock_t	fStart;
			uint32		fNumBits;
			fsblock_t	fBitmapBlock;

			uint32		fFreeBits;
			uint32		fFirstFree;
			uint32		fLargestStart;
			uint32		fLargestLength;
			
			uint32		fPreviousFreeBits;
			uint32		fPreviousFirstFree;
			uint32		fPreviousLargestStart;
			uint32		fPreviousLargestLength;
};


AllocationBlockGroup::AllocationBlockGroup()
	:
	fVolume(NULL),
	fBlockGroup(0),
	fGroupDescriptor(NULL),
	fCurrentTransaction(-1),
	fStart(0),
	fNumBits(0),
	fBitmapBlock(0),
	fFreeBits(0),
	fFirstFree(0),
	fLargestStart(0),
	fLargestLength(0),
	fPreviousFreeBits(0),
	fPreviousFirstFree(0),
	fPreviousLargestStart(0),
	fPreviousLargestLength(0)
{
	mutex_init(&fLock, "ext2 allocation block group");
	mutex_init(&fTransactionLock, "ext2 allocation block group transaction");
}


AllocationBlockGroup::~AllocationBlockGroup()
{
	mutex_destroy(&fLock);
	mutex_destroy(&fTransactionLock);
}


status_t
AllocationBlockGroup::Initialize(Volume* volume, uint32 blockGroup,
	uint32 numBits)
{
	fVolume = volume;
	fBlockGroup = blockGroup;
	fNumBits = numBits;
	fStart = blockGroup * numBits + fVolume->FirstDataBlock();

	status_t status = fVolume->GetBlockGroup(blockGroup, &fGroupDescriptor);
	if (status != B_OK)
		return status;

	fBitmapBlock = fGroupDescriptor->BlockBitmap(fVolume->Has64bitFeature());
	if (fBitmapBlock == 0) {
		ERROR("AllocationBlockGroup(%" B_PRIu32 ")::Initialize(): "
			"blockBitmap is zero\n", fBlockGroup);
		return B_BAD_VALUE;
	}

	if (fGroupDescriptor->Flags() & EXT2_BLOCK_GROUP_BLOCK_UNINIT) {
		fFreeBits = fGroupDescriptor->FreeBlocks(fVolume->Has64bitFeature());
		fLargestLength = fFreeBits;
		fLargestStart = _FirstFreeBlock();
		TRACE("Group %" B_PRIu32 " is uninit\n", fBlockGroup);
		return B_OK;
	}
	
	status = _ScanFreeRanges(true);
	if (status != B_OK)
		return status;

	if (fGroupDescriptor->FreeBlocks(fVolume->Has64bitFeature())
		!= fFreeBits) {
		ERROR("AllocationBlockGroup(%" B_PRIu32 ",%" B_PRIu64 ")::Initialize()"
			": Mismatch between counted free blocks (%" B_PRIu32 "/%" B_PRIu32
			") and what is set on the group descriptor (%" B_PRIu32 ")\n",
			fBlockGroup, fBitmapBlock, fFreeBits, fNumBits,
			fGroupDescriptor->FreeBlocks(fVolume->Has64bitFeature()));
		return B_BAD_DATA;
	}

	fPreviousFreeBits = fFreeBits;
	fPreviousFirstFree = fFirstFree;
	fPreviousLargestStart = fLargestStart;
	fPreviousLargestLength = fLargestLength;
	
	return status;
}


status_t
AllocationBlockGroup::_ScanFreeRanges(bool verify)
{
	TRACE("AllocationBlockGroup::_ScanFreeRanges() for group %" B_PRIu32 "\n",
		fBlockGroup);
	BitmapBlock block(fVolume, fNumBits);

	if (!block.SetTo(fBitmapBlock))
		return B_ERROR;

	if (verify && !_VerifyBlockBitmapChecksum(block)) {
		ERROR("AllocationBlockGroup(%" B_PRIu32 ")::_ScanFreeRanges(): "
			"blockGroup verification failed\n", fBlockGroup);
		return B_BAD_DATA;
	}

	fFreeBits = 0;
	uint32 start = 0;
	uint32 end = 0;

	while (end < fNumBits) {
		block.FindNextUnmarked(start);
		ASSERT(block.CheckMarked(end, start - end));
		end = start;

		if (start != block.NumBits()) {
			block.FindNextMarked(end);
			_AddFreeRange(start, end - start);
			ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));
			start = end;
		}
	}

	return B_OK;
}


bool
AllocationBlockGroup::IsFull() const
{
	return fFreeBits == 0;
}


status_t
AllocationBlockGroup::Allocate(Transaction& transaction, fsblock_t _start,
	uint32 length)
{
	uint32 start = _start - fStart;
	TRACE("AllocationBlockGroup::Allocate(%" B_PRIu32 ",%" B_PRIu32 ")\n",
		start, length);
	if (length == 0)
		return B_OK;

	uint32 end = start + length;
	if (end > fNumBits)
		return B_BAD_VALUE;

	_LockInTransaction(transaction);
	_InitGroup(transaction);

	BitmapBlock block(fVolume, fNumBits);

	if (!block.SetToWritable(transaction, fBitmapBlock))
		return B_ERROR;

	TRACE("AllocationBlockGroup::Allocate(): Largest range in %" B_PRIu32 "-%"
		B_PRIu32 "\n", fLargestStart, fLargestStart + fLargestLength);
	ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));
	
	if (!block.Mark(start, length)) {
		ERROR("Failed to allocate blocks from %" B_PRIu32 " to %" B_PRIu32
			". Some were already allocated.\n", start, start + length);
		return B_ERROR;
	}

	fFreeBits -= length;
	fGroupDescriptor->SetFreeBlocks(fFreeBits, fVolume->Has64bitFeature());
	_SetBlockBitmapChecksum(block);
	fVolume->WriteBlockGroup(transaction, fBlockGroup);

	if (start == fLargestStart) {
		if (fFirstFree == fLargestStart)
			fFirstFree += length;

		fLargestStart += length;
		fLargestLength -= length;
	} else if (start + length == fLargestStart + fLargestLength) {
		fLargestLength -= length;
	} else if (start < fLargestStart + fLargestLength
			&& start > fLargestStart) {
		uint32 firstLength = start - fLargestStart;
		uint32 secondLength = fLargestStart + fLargestLength
			- (start + length);

		if (firstLength >= secondLength) {
			fLargestLength = firstLength;
		} else {
			fLargestLength = secondLength;
			fLargestStart = start + length;
		}
	} else {
		// No need to revalidate the largest free range
		return B_OK;
	}

	TRACE("AllocationBlockGroup::Allocate(): Largest range in %" B_PRIu32 "-%"
		B_PRIu32 "\n", fLargestStart, fLargestStart + fLargestLength);
	ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));

	if (fLargestLength < fNumBits / 2)
		block.FindLargestUnmarkedRange(fLargestStart, fLargestLength);
	ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));

	return B_OK;
}


status_t
AllocationBlockGroup::Free(Transaction& transaction, uint32 start,
	uint32 length)
{
	TRACE("AllocationBlockGroup::Free(): start: %" B_PRIu32 ", length %"
		B_PRIu32 "\n", start, length);

	if (length == 0)
		return B_OK;

	uint32 end = start + length;
	if (end > fNumBits)
		return B_BAD_VALUE;

	_LockInTransaction(transaction);
	if (fGroupDescriptor->Flags() & EXT2_BLOCK_GROUP_BLOCK_UNINIT)
		panic("AllocationBlockGroup::Free() can't free blocks if uninit\n");

	BitmapBlock block(fVolume, fNumBits);

	if (!block.SetToWritable(transaction, fBitmapBlock))
		return B_ERROR;

	TRACE("AllocationBlockGroup::Free(): Largest range in %" B_PRIu32 "-%"
		B_PRIu32 "\n", fLargestStart, fLargestStart + fLargestLength);
	ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));

	if (!block.Unmark(start, length)) {
		ERROR("Failed to free blocks from %" B_PRIu32 " to %" B_PRIu32
			". Some were already freed.\n", start, start + length);
		return B_ERROR;
	}

	TRACE("AllocationGroup::Free(): Unmarked bits in bitmap\n");

	if (fFirstFree > start)
		fFirstFree = start;

	if (start + length == fLargestStart) {
		fLargestStart = start;
		fLargestLength += length;
	} else if (start == fLargestStart + fLargestLength) {
		fLargestLength += length;
	} else if (fLargestLength <= fNumBits / 2) {
		// May have merged with some free blocks, becoming the largest
		uint32 newEnd = start + length;
		block.FindNextMarked(newEnd);

		uint32 newStart = start;
		block.FindPreviousMarked(newStart);
		newStart++;

		if (newEnd - newStart > fLargestLength) {
			fLargestLength = newEnd - newStart;
			fLargestStart = newStart;
		}
	}

	TRACE("AllocationBlockGroup::Free(): Largest range in %" B_PRIu32 "-%"
		B_PRIu32 "\n", fLargestStart, fLargestStart + fLargestLength);
	ASSERT(block.CheckUnmarked(fLargestStart, fLargestLength));

	fFreeBits += length;
	fGroupDescriptor->SetFreeBlocks(fFreeBits, fVolume->Has64bitFeature());
	_SetBlockBitmapChecksum(block);
	fVolume->WriteBlockGroup(transaction, fBlockGroup);

	return B_OK;
}


status_t
AllocationBlockGroup::FreeAll(Transaction& transaction)
{
	return Free(transaction, 0, fNumBits);
}


uint32
AllocationBlockGroup::NumBits() const
{
	return fNumBits;
}


uint32
AllocationBlockGroup::FreeBits() const
{
	return fFreeBits;
}


fsblock_t
AllocationBlockGroup::Start() const
{
	return fStart;
}


fsblock_t
AllocationBlockGroup::LargestStart() const
{
	return fStart + fLargestStart;
}


uint32
AllocationBlockGroup::LargestLength() const
{
	return fLargestLength;
}


void
AllocationBlockGroup::_AddFreeRange(uint32 start, uint32 length)
{
	if (IsFull()) {
		fFirstFree = start;
		fLargestStart = start;
		fLargestLength = length;
	} else if (length > fLargestLength) {
		fLargestStart = start;
		fLargestLength = length;
	}

	fFreeBits += length;
}


void
AllocationBlockGroup::_LockInTransaction(Transaction& transaction)
{
	mutex_lock(&fLock);

	if (transaction.ID() != fCurrentTransaction) {
		mutex_unlock(&fLock);

		mutex_lock(&fTransactionLock);
		mutex_lock(&fLock);

		fCurrentTransaction = transaction.ID();
		transaction.AddListener(this);
	}

	mutex_unlock(&fLock);
}


status_t
AllocationBlockGroup::_InitGroup(Transaction& transaction)
{
	TRACE("AllocationBlockGroup::_InitGroup()\n");
	uint16 flags = fGroupDescriptor->Flags();
	if ((flags & EXT2_BLOCK_GROUP_BLOCK_UNINIT) == 0)
		return B_OK;

	TRACE("AllocationBlockGroup::_InitGroup() initing\n");

	BitmapBlock blockBitmap(fVolume, fNumBits);
	if (!blockBitmap.SetToWritable(transaction, fBitmapBlock))
		return B_ERROR;

	blockBitmap.Mark(0, _FirstFreeBlock(), true);
	blockBitmap.Unmark(0, fNumBits, true);
	
	fGroupDescriptor->SetFlags(flags & ~EXT2_BLOCK_GROUP_BLOCK_UNINIT);
	_SetBlockBitmapChecksum(blockBitmap);
	fVolume->WriteBlockGroup(transaction, fBlockGroup);

	status_t status = _ScanFreeRanges();
	if (status != B_OK)
		return status;

	if (fGroupDescriptor->FreeBlocks(fVolume->Has64bitFeature())
		!= fFreeBits) {
		ERROR("AllocationBlockGroup(%" B_PRIu32 ",%" B_PRIu64 ")::_InitGroup()"
			": Mismatch between counted free blocks (%" B_PRIu32 "/%" B_PRIu32
			") and what is set on the group descriptor (%" B_PRIu32 ")\n",
			fBlockGroup, fBitmapBlock, fFreeBits, fNumBits,
			fGroupDescriptor->FreeBlocks(fVolume->Has64bitFeature()));
		return B_BAD_DATA;
	}

	TRACE("AllocationBlockGroup::_InitGroup() init OK\n");

	return B_OK;
}


bool
AllocationBlockGroup::_IsSparse()
{
	if (fBlockGroup <= 1)
		return true;
	if (fBlockGroup & 0x1)
		return false;

	uint32 i = fBlockGroup;
	while (i % 7 == 0)
		i /= 7;
	if (i == 1)
		return true;

	i = fBlockGroup;
	while (i % 5 == 0)
		i /= 5;
	if (i == 1)
		return true;

	i = fBlockGroup;
	while (i % 3 == 0)
		i /= 3;
	if (i == 1)
		return true;

	return false;
}


uint32
AllocationBlockGroup::_FirstFreeBlock()
{
	uint32 first = 1;
	if (_IsSparse())
		first = 0;
	else if (!fVolume->HasMetaGroupFeature()) {
		first += fVolume->SuperBlock().ReservedGDTBlocks();
		first += fVolume->NumGroups();
	}
	return first;
}


void
AllocationBlockGroup::_SetBlockBitmapChecksum(BitmapBlock& block)
{
	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = block.Checksum(fVolume->BlocksPerGroup());
		fGroupDescriptor->block_bitmap_csum = checksum & 0xffff;
		if (fVolume->GroupDescriptorSize() >= offsetof(ext2_block_group,
			inode_bitmap_high)) {
			fGroupDescriptor->block_bitmap_csum_high = checksum >> 16;
		}
	}
}


bool
AllocationBlockGroup::_VerifyBlockBitmapChecksum(BitmapBlock& block) {
	if (fVolume->HasMetaGroupChecksumFeature()) {
		uint32 checksum = block.Checksum(fVolume->BlocksPerGroup());
		uint32 provided = fGroupDescriptor->block_bitmap_csum;
		if (fVolume->GroupDescriptorSize() >= offsetof(ext2_block_group,
			inode_bitmap_high)) {
			provided |= (fGroupDescriptor->block_bitmap_csum_high << 16);
		} else
			checksum &= 0xffff;
		return provided == checksum;
	}
	return true;
}


void
AllocationBlockGroup::TransactionDone(bool success)
{
	if (success) {
		TRACE("AllocationBlockGroup::TransactionDone(): The transaction "
			"succeeded, discarding previous state\n");
		fPreviousFreeBits = fFreeBits;
		fPreviousFirstFree = fFirstFree;
		fPreviousLargestStart = fLargestStart;
		fPreviousLargestLength = fLargestLength;
	} else {
		TRACE("AllocationBlockGroup::TransactionDone(): The transaction "
			"failed, restoring to previous state\n");
		fFreeBits = fPreviousFreeBits;
		fFirstFree = fPreviousFirstFree;
		fLargestStart = fPreviousLargestStart;
		fLargestLength = fPreviousLargestLength;
	}
}


void
AllocationBlockGroup::RemovedFromTransaction()
{
	mutex_unlock(&fTransactionLock);
	fCurrentTransaction = -1;
}


BlockAllocator::BlockAllocator(Volume* volume)
	:
	fVolume(volume),
	fGroups(NULL),
	fBlocksPerGroup(0),
	fNumBlocks(0),
	fNumGroups(0),
	fFirstBlock(0)
{
	mutex_init(&fLock, "ext2 block allocator");
}


BlockAllocator::~BlockAllocator()
{
	mutex_destroy(&fLock);

	if (fGroups != NULL)
		delete [] fGroups;
}


status_t
BlockAllocator::Initialize()
{
	fBlocksPerGroup = fVolume->BlocksPerGroup();
	fNumGroups = fVolume->NumGroups();
	fFirstBlock = fVolume->FirstDataBlock();
	fNumBlocks = fVolume->NumBlocks();
	
	TRACE("BlockAllocator::Initialize(): blocks per group: %" B_PRIu32
		", block groups: %" B_PRIu32 ", first block: %" B_PRIu64
		", num blocks: %" B_PRIu64 "\n", fBlocksPerGroup, fNumGroups,
		fFirstBlock, fNumBlocks);

	fGroups = new(std::nothrow) AllocationBlockGroup[fNumGroups];
	if (fGroups == NULL)
		return B_NO_MEMORY;

	TRACE("BlockAllocator::Initialize(): allocated allocation block groups\n");

	mutex_lock(&fLock);
		// Released by _Initialize

#if 0
	thread_id id = spawn_kernel_thread(
		(thread_func)BlockAllocator::_Initialize, "ext2 block allocator",
		B_LOW_PRIORITY, this);
	if (id < B_OK)
		return _Initialize(this);

	mutex_transfer_lock(&fLock, id);

	return resume_thread(id);
#else
	return _Initialize(this);
#endif
}


status_t
BlockAllocator::AllocateBlocks(Transaction& transaction, uint32 minimum,
	uint32 maximum, uint32& blockGroup, fsblock_t& start, uint32& length)
{
	TRACE("BlockAllocator::AllocateBlocks()\n");
	MutexLocker lock(fLock);
	TRACE("BlockAllocator::AllocateBlocks(): Acquired lock\n");

	TRACE("BlockAllocator::AllocateBlocks(): transaction: %" B_PRId32 ", min: "
		"%" B_PRIu32 ", max: %" B_PRIu32 ", block group: %" B_PRIu32 ", start:"
		" %" B_PRIu64 ", num groups: %" B_PRIu32 "\n", transaction.ID(),
		minimum, maximum, blockGroup, start, fNumGroups);

	fsblock_t bestStart = 0;
	uint32 bestLength = 0;
	uint32 bestGroup = 0;

	uint32 groupNum = blockGroup;

	AllocationBlockGroup* last = &fGroups[fNumGroups];
	AllocationBlockGroup* group = &fGroups[blockGroup];

	for (int32 iterations = 0; iterations < 2; iterations++) {
		for (; group < last; ++group, ++groupNum) {
			TRACE("BlockAllocator::AllocateBlocks(): Group %" B_PRIu32
				" has largest length of %" B_PRIu32 "\n", groupNum,
				group->LargestLength());

			if (group->LargestLength() > bestLength) {
				if (start <= group->LargestStart()) {
					bestStart = group->LargestStart();
					bestLength = group->LargestLength();
					bestGroup = groupNum;

					TRACE("BlockAllocator::AllocateBlocks(): Found a better "
						"range: block group: %" B_PRIu32 ", %" B_PRIu64 "-%"
						B_PRIu64 "\n", groupNum, bestStart,
						bestStart + bestLength);

					if (bestLength >= maximum)
						break;
				}
			}

			start = 0;
		}

		if (bestLength >= maximum)
			break;

		groupNum = 0;

		group = &fGroups[0];
		last = &fGroups[blockGroup + 1];
	}

	if (bestLength < minimum) {
		TRACE("BlockAllocator::AllocateBlocks(): best range (length %" B_PRIu32
			") doesn't have minimum length of %" B_PRIu32 "\n", bestLength,
			minimum);
		return B_DEVICE_FULL;
	}

	if (bestLength > maximum)
		bestLength = maximum;

	TRACE("BlockAllocator::AllocateBlocks(): Selected range: block group %"
		B_PRIu32 ", %" B_PRIu64 "-%" B_PRIu64 "\n", bestGroup, bestStart,
		bestStart + bestLength);

	status_t status = fGroups[bestGroup].Allocate(transaction, bestStart,
		bestLength);
	if (status != B_OK) {
		TRACE("BlockAllocator::AllocateBlocks(): Failed to allocate %" B_PRIu32
		" blocks inside block group %" B_PRIu32 ".\n", bestLength, bestGroup);
		return status;
	}

	start = bestStart;
	length = bestLength;
	blockGroup = bestGroup;

	return B_OK;
}


status_t
BlockAllocator::Allocate(Transaction& transaction, Inode* inode,
	off_t numBlocks, uint32 minimum, fsblock_t& start, uint32& allocated)
{
	if (numBlocks <= 0)
		return B_ERROR;
	if (start > fNumBlocks)
		return B_BAD_VALUE;

	uint32 group = inode->ID() / fVolume->InodesPerGroup();
	uint32 preferred = 0;

	if (inode->Size() > 0) {
		// Try to allocate near it's last blocks
		ext2_data_stream* dataStream = &inode->Node().stream;
		uint32 numBlocks = inode->Size() / fVolume->BlockSize() + 1;
		uint32 lastBlock = 0;

		// DANGER! What happens with sparse files?
		if (numBlocks < EXT2_DIRECT_BLOCKS) {
			// Only direct blocks
			lastBlock = dataStream->direct[numBlocks];
		} else {
			numBlocks -= EXT2_DIRECT_BLOCKS - 1;
			uint32 numIndexes = fVolume->BlockSize() / 4;
				// block size / sizeof(int32)
			uint32 numIndexes2 = numIndexes * numIndexes;
			uint32 numIndexes3 = numIndexes2 * numIndexes;
			uint32 indexesInIndirect = numIndexes;
			uint32 indexesInDoubleIndirect = indexesInIndirect
				+ numIndexes2;
			// uint32 indexesInTripleIndirect = indexesInDoubleIndirect
				// + numIndexes3;

			uint32 doubleIndirectBlock = EXT2_DIRECT_BLOCKS + 1;
			uint32 indirectBlock = EXT2_DIRECT_BLOCKS;

			CachedBlock cached(fVolume);
			uint32* indirectData;

			if (numBlocks > indexesInDoubleIndirect) {
				// Triple indirect blocks
				indirectData = (uint32*)cached.SetTo(EXT2_DIRECT_BLOCKS + 2);
				if (indirectData == NULL)
					return B_IO_ERROR;

				uint32 index = (numBlocks - indexesInDoubleIndirect)
					/ numIndexes3;
				doubleIndirectBlock = indirectData[index];
			}

			if (numBlocks > indexesInIndirect) {
				// Double indirect blocks
				indirectData = (uint32*)cached.SetTo(doubleIndirectBlock);
				if (indirectData == NULL)
					return B_IO_ERROR;

				uint32 index = (numBlocks - indexesInIndirect) / numIndexes2;
				indirectBlock = indirectData[index];
			}

			indirectData = (uint32*)cached.SetTo(indirectBlock);
				if (indirectData == NULL)
					return B_IO_ERROR;

			uint32 index = numBlocks / numIndexes;
			lastBlock = indirectData[index];
		}

		group = (lastBlock - fFirstBlock) / fBlocksPerGroup;
		preferred = (lastBlock - fFirstBlock) % fBlocksPerGroup + 1;
	}

	// TODO: Apply some more policies

	return AllocateBlocks(transaction, minimum, minimum + 8, group, start,
		allocated);
}


status_t
BlockAllocator::Free(Transaction& transaction, fsblock_t start, uint32 length)
{
	TRACE("BlockAllocator::Free(%" B_PRIu64 ", %" B_PRIu32 ")\n", start,
		length);
	MutexLocker lock(fLock);

	if (start <= fFirstBlock) {
		panic("Trying to free superblock!\n");
		return B_BAD_VALUE;
	}

	if (length == 0)
		return B_OK;
	if (start > fNumBlocks || length > fNumBlocks)
		return B_BAD_VALUE;

	TRACE("BlockAllocator::Free(): first block: %" B_PRIu64
		", blocks per group: %" B_PRIu32 "\n", fFirstBlock, fBlocksPerGroup);

	start -= fFirstBlock;
	off_t end = start + length - 1;

	uint32 group = start / fBlocksPerGroup;
	if (group >= fNumGroups) {
		panic("BlockAllocator::Free() group %" B_PRIu32 " too big (fNumGroups "
			"%" B_PRIu32 ")\n", group, fNumGroups);
	}
	uint32 lastGroup = end / fBlocksPerGroup;
	start = start % fBlocksPerGroup;

	if (group == lastGroup)
		return fGroups[group].Free(transaction, start, length);

	TRACE("BlockAllocator::Free(): Freeing from group %" B_PRIu32 ": %"
		B_PRIu64 ", %" B_PRIu64 "\n", group,
		start, fGroups[group].NumBits() - start);

	status_t status = fGroups[group].Free(transaction, start,
		fGroups[group].NumBits() - start);
	if (status != B_OK)
		return status;

	for (++group; group < lastGroup; ++group) {
		TRACE("BlockAllocator::Free(): Freeing all from group %" B_PRIu32 "\n",
			group);
		status = fGroups[group].FreeAll(transaction);
		if (status != B_OK)
			return status;
	}

	TRACE("BlockAllocator::Free(): Freeing from group %" B_PRIu32 ": 0-%"
		B_PRIu64 " \n", group, end % fBlocksPerGroup);
	return fGroups[group].Free(transaction, 0, (end + 1) % fBlocksPerGroup);
}


/*static*/ status_t
BlockAllocator::_Initialize(BlockAllocator* allocator)
{
	TRACE("BlockAllocator::_Initialize()\n");
	// fLock is already heald
	Volume* volume = allocator->fVolume;

	AllocationBlockGroup* groups = allocator->fGroups;
	uint32 numGroups = allocator->fNumGroups - 1;

	off_t freeBlocks = 0;
	TRACE("BlockAllocator::_Initialize(): free blocks: %" B_PRIdOFF "\n",
		freeBlocks);

	for (uint32 i = 0; i < numGroups; ++i) {
		status_t status = groups[i].Initialize(volume, i, 
			allocator->fBlocksPerGroup);
		if (status != B_OK) {
			mutex_unlock(&allocator->fLock);
			return status;
		}

		freeBlocks += groups[i].FreeBits();
		TRACE("BlockAllocator::_Initialize(): free blocks: %" B_PRIdOFF "\n",
			freeBlocks);
	}
	
	// Last block group may have less blocks
	status_t status = groups[numGroups].Initialize(volume, numGroups,
		allocator->fNumBlocks - allocator->fBlocksPerGroup * numGroups
		- allocator->fFirstBlock);
	if (status != B_OK) {
		mutex_unlock(&allocator->fLock);
		return status;
	}
	
	freeBlocks += groups[numGroups].FreeBits();

	TRACE("BlockAllocator::_Initialize(): free blocks: %" B_PRIdOFF "\n",
		freeBlocks);

	mutex_unlock(&allocator->fLock);

	if (freeBlocks != volume->NumFreeBlocks()) {
		TRACE("Counted free blocks (%" B_PRIdOFF ") doesn't match value in the"
			" superblock (%" B_PRIdOFF ").\n", freeBlocks,
			volume->NumFreeBlocks());
		return B_BAD_DATA;
	}

	return B_OK;
}

