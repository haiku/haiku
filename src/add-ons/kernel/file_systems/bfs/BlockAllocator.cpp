/* BlockAllocator - block bitmap handling and allocation policies
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "cpp.h"
#include "Debug.h"
#include "BlockAllocator.h"
#include "Volume.h"
#include "Inode.h"

#ifdef USER
#	define spawn_kernel_thread spawn_thread
#endif

// Things the BlockAllocator should do:

// - find a range of blocks of a certain size nearby a specific position
// - allocating an unsharp range of blocks for pre-allocation
// - free blocks
// - know how to deal with each allocation, special handling for directories,
//   files, symlinks, etc. (type sensitive allocation policies)

// What makes the code complicated is the fact that we are not just reading
// in the whole bitmap and operate on that in memory - e.g. a 13 GB partition
// with a block size of 2048 bytes already has a 800kB bitmap, and the size
// of partitions will grow even more - so that's not an option.
// Instead we are reading in every block when it's used - since an allocation
// group can span several blocks in the block bitmap, the AllocationBlock
// class is there to make handling those easier.

// The current implementation is only slightly optimized and could probably
// be improved a lot. Furthermore, the allocation policies used here should
// have some real world tests.


class AllocationBlock : public CachedBlock {
	public:
		AllocationBlock(Volume *volume);
		
		void Allocate(uint16 start, uint16 numBlocks = 0xffff);
		void Free(uint16 start, uint16 numBlocks = 0xffff);
		inline bool IsUsed(uint16 block);

		status_t SetTo(AllocationGroup &group, uint16 block);

		int32 NumBlockBits() const { return fNumBits; }

	private:
		int32 fNumBits;
};


class AllocationGroup {
	public:
		AllocationGroup();

		void AddFreeRange(int32 start, int32 blocks);
		bool IsFull() const { return fFreeBits == 0; }

		status_t Allocate(Transaction *transaction, uint16 start, int32 length);
		status_t Free(Transaction *transaction, uint16 start, int32 length);

		int32 fNumBits;
		int32 fStart;
		int32 fFirstFree, fLargest, fLargestFirst;
		int32 fFreeBits;
};


AllocationBlock::AllocationBlock(Volume *volume)
	: CachedBlock(volume)
{
}


status_t 
AllocationBlock::SetTo(AllocationGroup &group, uint16 block)
{
	// 8 blocks per byte
	fNumBits = fVolume->BlockSize() << 3;
	// the last group may have less bits in the last block
	if ((group.fNumBits % fNumBits) != 0)
		fNumBits = group.fNumBits % fNumBits;

	return CachedBlock::SetTo(group.fStart + block) != NULL ? B_OK : B_ERROR;
}


bool 
AllocationBlock::IsUsed(uint16 block)
{
	if (block > fNumBits)
		return true;
	// the block bitmap is accessed in 32-bit blocks
	return ((uint32 *)fBlock)[block >> 5] & (1UL << (block % 32));
}


void
AllocationBlock::Allocate(uint16 start, uint16 numBlocks)
{
	start = start % fNumBits;
	if (numBlocks == 0xffff) {
		// allocate all blocks after "start"
		numBlocks = fNumBits - start;
	} else if (start + numBlocks > fNumBits) {
		FATAL(("Allocation::Allocate(): tried to allocate too many blocks: %u (numBlocks = %u)!\n", numBlocks, fNumBits));
		DEBUGGER(("Allocation::Allocate(): tried to allocate too many blocks"));

		numBlocks = fNumBits - start;
	}

	int32 block = start >> 5;

	while (numBlocks > 0) {
		uint32 mask = 0;
		for (int32 i = start % 32;i < 32 && numBlocks;i++, numBlocks--)
			mask |= 1UL << (i % 32);

#ifdef DEBUG
/*
		if (mask & ((uint32 *)fBlock)[block]) {
			FATAL(("AllocationBlock::Allocate(): some blocks are already allocated, start = %u, numBlocks = %u\n", start, numBlocks));
			DEBUGGER(("blocks already occupied!"));
		}
*/
#endif
		((uint32 *)fBlock)[block++] |= mask;
		start = 0;
	}
}


void
AllocationBlock::Free(uint16 start, uint16 numBlocks)
{
	start = start % fNumBits;
	if (numBlocks == 0xffff) {
		// free all blocks after "start"
		numBlocks = fNumBits - start;
	} else if (start + numBlocks > fNumBits) {
		FATAL(("Allocation::Free(): tried to free too many blocks: %u (numBlocks = %u)!\n", numBlocks, fNumBits));
		DEBUGGER(("Allocation::Free(): tried to free too many blocks"));

		numBlocks = fNumBits - start;
	}

	int32 block = start >> 5;

	while (numBlocks > 0) {
		uint32 mask = 0;
		for (int32 i = start % 32;i < 32 && numBlocks;i++,numBlocks--)
			mask |= 1UL << (i % 32);

		((uint32 *)fBlock)[block++] &= ~mask;
		start = 0;
	}
}


//	#pragma mark -


AllocationGroup::AllocationGroup()
	:
	fFirstFree(-1),
	fLargest(-1),
	fLargestFirst(-1),
	fFreeBits(0)
{
}


void 
AllocationGroup::AddFreeRange(int32 start, int32 blocks)
{
	//D(if (blocks > 512)
	//	PRINT(("range of %ld blocks starting at %ld\n",blocks,start)));

	if (fFirstFree == -1)
		fFirstFree = start;

	if (fLargest < blocks) {
		fLargest = blocks;
		fLargestFirst = start;
	}

	fFreeBits += blocks;
}


/** Allocates the specified run in the allocation group.
 *	Doesn't check if the run is valid or already allocated
 *	partially, nor does it maintain the free ranges hints
 *	or the volume's used blocks count.
 *	It only does the low-level work of allocating some bits
 *	in the block bitmap.
 *	Assumes that the block bitmap lock is hold.
 */

status_t
AllocationGroup::Allocate(Transaction *transaction, uint16 start, int32 length)
{
	Volume *volume = transaction->GetVolume();

	// calculate block in the block bitmap and position within
	uint32 bitsPerBlock = volume->BlockSize() << 3;
	uint32 block = start / bitsPerBlock;
	start = start % bitsPerBlock;

	AllocationBlock cached(volume);

	while (length > 0) {
		if (cached.SetTo(*this, block) < B_OK)
			RETURN_ERROR(B_IO_ERROR);

		uint32 numBlocks = length;
		if (start + numBlocks > cached.NumBlockBits())
			numBlocks = cached.NumBlockBits() - start;

		cached.Allocate(start, numBlocks);

		if (cached.WriteBack(transaction) < B_OK)
			return B_IO_ERROR;

		length -= numBlocks;
		start = 0;
		block++;
	}
	return B_OK;
}


/** Frees the specified run in the allocation group.
 *	Doesn't check if the run is valid or was not completely
 *	allocated, nor does it maintain the free ranges hints
 *	or the volume's used blocks count.
 *	It only does the low-level work of freeing some bits
 *	in the block bitmap.
 *	Assumes that the block bitmap lock is hold.
 */

status_t
AllocationGroup::Free(Transaction *transaction, uint16 start, int32 length)
{
	Volume *volume = transaction->GetVolume();

	// calculate block in the block bitmap and position within
	uint32 bitsPerBlock = volume->BlockSize() << 3;
	uint32 block = start / bitsPerBlock;
	start = start % bitsPerBlock;

	AllocationBlock cached(volume);

	while (length > 0) {
		if (cached.SetTo(*this, block) < B_OK)
			RETURN_ERROR(B_IO_ERROR);

		uint16 freeLength = length;
		if (start + length > cached.NumBlockBits())
			freeLength = cached.NumBlockBits() - start;

		cached.Free(start, freeLength);

		if (cached.WriteBack(transaction) < B_OK)
			return B_IO_ERROR;

		length -= freeLength;
		start = 0;
		block++;
	}
}


//	#pragma mark -


BlockAllocator::BlockAllocator(Volume *volume)
	:
	fVolume(volume),
	fGroups(NULL),
	fCheckBitmap(NULL)
{
}


BlockAllocator::~BlockAllocator()
{
	delete[] fGroups;
}


status_t 
BlockAllocator::Initialize()
{
	if (fLock.InitCheck() < B_OK)
		return B_ERROR;

	fNumGroups = fVolume->AllocationGroups();
	fBlocksPerGroup = fVolume->SuperBlock().blocks_per_ag;
	fGroups = new AllocationGroup[fNumGroups];
	if (fGroups == NULL)
		return B_NO_MEMORY;

	thread_id id = spawn_kernel_thread((thread_func)BlockAllocator::initialize,
		"bfs block allocator", B_LOW_PRIORITY, (void *)this);
	if (id < B_OK)
		return initialize(this);

	return resume_thread(id);
}


status_t 
BlockAllocator::initialize(BlockAllocator *allocator)
{
	Locker lock(allocator->fLock);

	Volume *volume = allocator->fVolume;
	uint32 blocks = allocator->fBlocksPerGroup;
	uint32 numBits = 8 * blocks * volume->BlockSize();
	off_t freeBlocks = 0;

	uint32 *buffer = (uint32 *)malloc(numBits >> 3);
	if (buffer == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	AllocationGroup *groups = allocator->fGroups;
	off_t offset = 1;
	int32 num = allocator->fNumGroups;

	for (int32 i = 0;i < num;i++) {
		if (cached_read(volume->Device(), offset, buffer, blocks, volume->BlockSize()) < B_OK)
			break;

		// the last allocation group may contain less blocks than the others
		groups[i].fNumBits = i == num - 1 ? allocator->fVolume->NumBlocks() - i * numBits : numBits;
		groups[i].fStart = offset;

		// finds all free ranges in this allocation group
		int32 start,range = 0;
		int32 size = groups[i].fNumBits, num = 0;

		for (int32 k = 0;k < (size >> 2);k++) {
			for (int32 j = 0; j < 32 && num < size; j++, num++) {
				if (buffer[k] & (1UL << j)) {
					if (range > 0) {
						groups[i].AddFreeRange(start, range);
						range = 0;
					}
				} else if (range++ == 0)
					start = num;
			}
		}
		if (range)
			groups[i].AddFreeRange(start, range);

		freeBlocks += groups[i].fFreeBits;

		offset += blocks;
	}
	free(buffer);

	// check if block bitmap and log area are reserved
	uint32 reservedBlocks = volume->Log().start + volume->Log().length;
	if (allocator->CheckBlockRun(block_run::Run(0, 0, reservedBlocks)) < B_OK) {
		Transaction transaction(volume, 0);
		if (groups[0].Allocate(&transaction, 0, reservedBlocks) < B_OK) {
			FATAL(("could not allocate reserved space for block bitmap/log!\n"));
			volume->Panic();
		} else {
			FATAL(("space for block bitmap or log area was not reserved!\n"));

			transaction.Done();
		}
	}

	off_t usedBlocks = volume->NumBlocks() - freeBlocks;
	if (volume->UsedBlocks() != usedBlocks) {
		// If the disk in a dirty state at mount time, it's
		// normal that the values don't match
		INFORM(("volume reports %Ld used blocks, correct is %Ld\n", volume->UsedBlocks(), usedBlocks));
		volume->SuperBlock().used_blocks = usedBlocks;
	}

	return B_OK;
}


status_t
BlockAllocator::AllocateBlocks(Transaction *transaction, int32 group, uint16 start,
	uint16 maximum, uint16 minimum, block_run &run)
{
	if (maximum == 0)
		return B_BAD_VALUE;

	AllocationBlock cached(fVolume);
	Locker lock(fLock);

	// the first scan through all allocation groups will look for the
	// wanted maximum of blocks, the second scan will just look to
	// satisfy the minimal requirement
	uint16 numBlocks = maximum;

	for (int32 i = 0; i < fNumGroups * 2; i++, group++, start = 0) {
		group = group % fNumGroups;

		if (start >= fGroups[group].fNumBits || fGroups[group].IsFull())
			continue;

		if (i >= fNumGroups) {
			// if the minimum is the same as the maximum, it's not necessary to
			// search for in the allocation groups a second time
			if (maximum == minimum)
				return B_DEVICE_FULL;

			numBlocks = minimum;
		}

		// The wanted maximum is smaller than the largest free block in the group
		// or already smaller than the minimum
		// ToDo: disabled because it's currently not maintained after the first allocation
		//if (numBlocks > fGroups[group].fLargest)
		//	continue;

		if (start < fGroups[group].fFirstFree)
			start = fGroups[group].fFirstFree;

		// there may be more than one block per allocation group - and
		// we iterate through it to find a place for the allocation.
		// (one allocation can't exceed one allocation group)

		uint32 block = start / (fVolume->BlockSize() << 3);
		int32 range = 0, rangeStart = 0;

		for (;block < fBlocksPerGroup;block++) {
			if (cached.SetTo(fGroups[group], block) < B_OK)
				RETURN_ERROR(B_ERROR);

			// find a block large enough to hold the allocation
			for (int32 bit = start % cached.NumBlockBits(); bit < cached.NumBlockBits(); bit++) {
				if (!cached.IsUsed(bit)) {
					if (range == 0) {
						// start new range
						rangeStart = block * cached.NumBlockBits() + bit;
					}

					// have we found a range large enough to hold numBlocks?
					if (++range >= maximum)
						break;
				} else if (i >= fNumGroups && range >= minimum) {
					// we have found a block larger than the required minimum (second pass)
					break;
				} else {
					// end of a range
					range = 0;
				}
			}

			// if we found a suitable block, mark the blocks as in use, and write
			// the updated block bitmap back to disk
			if (range >= numBlocks) {
				// adjust allocation size
				if (numBlocks < maximum)
					numBlocks = range;

				// Update the allocation group info
				// Note, the fFirstFree block doesn't have to be really free
				if (rangeStart == fGroups[group].fFirstFree)
					fGroups[group].fFirstFree = rangeStart + numBlocks;
				fGroups[group].fFreeBits -= numBlocks;

				if (fGroups[group].Allocate(transaction, rangeStart, numBlocks) < B_OK)
					RETURN_ERROR(B_IO_ERROR);

				run.allocation_group = group;
				run.start = rangeStart;
				run.length = numBlocks;

				fVolume->SuperBlock().used_blocks += numBlocks;
					// We are not writing back the disk's super block - it's
					// either done by the journaling code, or when the disk
					// is unmounted.
					// If the value is not correct at mount time, it will be
					// fixed anyway.

				return B_OK;
			}

			// start from the beginning of the next block
			start = 0;
		}
	}
	return B_DEVICE_FULL;
}


status_t 
BlockAllocator::AllocateForInode(Transaction *transaction,const block_run *parent, mode_t type, block_run &run)
{
	// apply some allocation policies here (AllocateBlocks() will break them
	// if necessary) - we will start with those described in Dominic Giampaolo's
	// "Practical File System Design", and see how good they work
	
	// files are going in the same allocation group as its parent, sub-directories
	// will be inserted 8 allocation groups after the one of the parent
	uint16 group = parent->allocation_group;
	if ((type & (S_DIRECTORY | S_INDEX_DIR | S_ATTR_DIR)) == S_DIRECTORY)
		group += 8;

	return AllocateBlocks(transaction, group, 0, 1, 1, run);
}


status_t 
BlockAllocator::Allocate(Transaction *transaction,const Inode *inode, off_t numBlocks, block_run &run, uint16 minimum)
{
	if (numBlocks <= 0)
		return B_ERROR;

	// one block_run can't hold more data than it is in one allocation group
	if (numBlocks > fGroups[0].fNumBits)
		numBlocks = fGroups[0].fNumBits;

	// since block_run.length is uint16, the largest number of blocks that
	// can be covered by a block_run is 65535
	// ToDo: if we drop compatibility, couldn't we do this any better?
	// There are basically two possibilities:
	// a) since a length of zero doesn't have any sense, take that for 65536 -
	//    but that could cause many problems (bugs) in other areas
	// b) reduce the maximum amount of blocks per block_run, so that the remaining
	//    number of free blocks can be used in a useful manner (like 4 blocks) -
	//    but that would also reduce the maximum file size
	if (numBlocks == 65536)
		numBlocks = 65535;

	// apply some allocation policies here (AllocateBlocks() will break them
	// if necessary)
	uint16 group = inode->BlockRun().allocation_group;
	uint16 start = 0;

	// are there already allocated blocks? (then just try to allocate near the last one)
	if (inode->Size() > 0) {
		data_stream *data = &inode->Node()->data;
		// we currently don't care for when the data stream is
		// already grown into the indirect ranges
		if (data->max_double_indirect_range == 0
			&& data->max_indirect_range == 0) {
			int32 last = 0;
			for (;last < NUM_DIRECT_BLOCKS - 1;last++)
				if (data->direct[last + 1].IsZero())
					break;
			
			group = data->direct[last].allocation_group;
			start = data->direct[last].start + data->direct[last].length;
		}
	} else if (inode->IsDirectory()) {
		// directory data will go in the same allocation group as the inode is in
		// but after the inode data
		start = inode->BlockRun().start;
	} else {
		// file data will start in the next allocation group
		group = inode->BlockRun().allocation_group + 1;
	}

	return AllocateBlocks(transaction, group, start, numBlocks, minimum, run);
}


status_t 
BlockAllocator::Free(Transaction *transaction, block_run run)
{
	Locker lock(fLock);

	int32 group = run.allocation_group;
	uint16 start = run.start;
	uint16 length = run.length;

	// doesn't use Volume::IsValidBlockRun() here because it can check better
	// against the group size (the last group may have a different length)
	if (group < 0 || group >= fNumGroups
		|| start > fGroups[group].fNumBits
		|| start + length > fGroups[group].fNumBits
		|| length == 0) {
		FATAL(("tried to free an invalid block_run (%ld, %u, %u)\n", group, start, length));
		DEBUGGER(("tried to free invalid block_run"));
		return B_BAD_VALUE;
	}
	// check if someone tries to free reserved areas at the beginning of the drive
	if (group == 0 && start < fVolume->Log().start + fVolume->Log().length) {
		FATAL(("tried to free a reserved block_run (%ld, %u, %u)\n", group, start, length));
		DEBUGGER(("tried to free reserved block"));
		return B_BAD_VALUE;
	}
#ifdef DEBUG	
	if (CheckBlockRun(run) < B_OK)
		return B_BAD_DATA;
#endif

	if (fGroups[group].fFirstFree > start)
		fGroups[group].fFirstFree = start;
	fGroups[group].fFreeBits += length;

	if (fGroups[group].Free(transaction, start, length) < B_OK)
		RETURN_ERROR(B_IO_ERROR);

	fVolume->SuperBlock().used_blocks -= run.length;
	return B_OK;
}


status_t 
BlockAllocator::StartChecking()
{
	status_t status = fLock.Lock();
	if (status < B_OK)
		return status;
	
	size_t size = fVolume->BlockSize() * fNumGroups * fBlocksPerGroup;
	fCheckBitmap = (uint32 *)malloc(size);
	if (fCheckBitmap == NULL) {
		fLock.Unlock();
		return B_NO_MEMORY;
	}
	memset(fCheckBitmap, 0, size);

	return B_OK;
}


status_t 
BlockAllocator::StopChecking()
{
	free(fCheckBitmap);
	fCheckBitmap = NULL;
	fLock.Unlock();

	return B_OK;
}


status_t
BlockAllocator::CheckBlockRun(block_run run)
{
	uint32 block = run.start / (fVolume->BlockSize() << 3);
	uint32 start = run.start;
	uint32 pos = 0;

	AllocationBlock cached(fVolume);

	for (; block < fBlocksPerGroup; block++) {
		if (cached.SetTo(fGroups[run.allocation_group], block) < B_OK)
			RETURN_ERROR(B_IO_ERROR);

		start = start % cached.NumBlockBits();
		while (pos < run.length && start + pos < cached.NumBlockBits()) {
			if (!cached.IsUsed(start + pos)) {
				PRINT(("block_run(%ld,%u,%u) is only partially allocated!\n", run.allocation_group, run.start, run.length));
				return B_BAD_DATA;
			}
			pos++;
		}
		start = 0;
	}
	return B_OK;
}


status_t
BlockAllocator::CheckInode(Inode *inode)
{
	if (fCheckBitmap == NULL)
		return B_NO_INIT;

	status_t status = CheckBlockRun(inode->BlockRun());
	if (status < B_OK)
		return status;

	// check the direct range

	data_stream *data = &inode->Node()->data;
	for (int32 i = 0;i < NUM_DIRECT_BLOCKS;i++) {
		if (data->direct[i].IsZero())
			break;

		status = CheckBlockRun(data->direct[i]);
		if (status < B_OK)
			return status;
	}

	CachedBlock cached(fVolume);

	// check the indirect range

	if (data->max_indirect_range) {
		status = CheckBlockRun(data->indirect);
		if (status < B_OK)
			return status;

		off_t block = fVolume->ToBlock(data->indirect);

		for (int32 i = 0; i < data->indirect.length; i++) {
			block_run *runs = (block_run *)cached.SetTo(block + i);
			if (runs == NULL)
				RETURN_ERROR(B_IO_ERROR);

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			int32 index = 0;
			for (; index < runsPerBlock; index++) {
				if (runs[index].IsZero())
					break;

				status = CheckBlockRun(runs[index]);
				if (status < B_OK)
					return status;
			}
			if (index < runsPerBlock)
				break;
		}
	}

	// check the double indirect range

	if (data->max_double_indirect_range) {
		status = CheckBlockRun(data->double_indirect);
		if (status < B_OK)
			return status;
		
		
	}

	return B_OK;
}

