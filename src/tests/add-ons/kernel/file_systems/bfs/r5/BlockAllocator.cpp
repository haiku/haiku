/* BlockAllocator - block bitmap handling and allocation policies
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Debug.h"
#include "BlockAllocator.h"
#include "Volume.h"
#include "Inode.h"
#include "BPlusTree.h"
#include "Stack.h"
#include "bfs_control.h"

#include <util/kernel_cpp.h>
#include <stdlib.h>

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


struct check_cookie {
	check_cookie() {}

	block_run			current;
	Inode				*parent;
	mode_t				parent_mode;
	Stack<block_run>	stack;
	TreeIterator		*iterator;
};


class AllocationBlock : public CachedBlock {
	public:
		AllocationBlock(Volume *volume);

		void Allocate(uint16 start, uint16 numBlocks);
		void Free(uint16 start, uint16 numBlocks);
		inline bool IsUsed(uint16 block);

		status_t SetTo(AllocationGroup &group, uint16 block);

		uint32 NumBlockBits() const { return fNumBits; }
		uint32 &Block(int32 index) { return ((uint32 *)fBlock)[index]; }

	private:
		uint32 fNumBits;
};


class AllocationGroup {
	public:
		AllocationGroup();

		void AddFreeRange(int32 start, int32 blocks);
		bool IsFull() const { return fFreeBits == 0; }

		status_t Allocate(Transaction *transaction, uint16 start, int32 length);
		status_t Free(Transaction *transaction, uint16 start, int32 length);

		uint32 fNumBits;
		int32 fStart;
		int32 fFirstFree, fLargest, fLargestFirst;
			// ToDo: fLargest & fLargestFirst are not maintained (and therefore used) yet!
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
	if ((block + 1) * fNumBits > group.fNumBits)
		fNumBits = group.fNumBits % fNumBits;

	return CachedBlock::SetTo(group.fStart + block) != NULL ? B_OK : B_ERROR;
}


bool 
AllocationBlock::IsUsed(uint16 block)
{
	if (block > fNumBits)
		return true;
	// the block bitmap is accessed in 32-bit blocks
	return Block(block >> 5) & HOST_ENDIAN_TO_BFS_INT32(1UL << (block % 32));
}


void
AllocationBlock::Allocate(uint16 start, uint16 numBlocks)
{
	ASSERT(start < fNumBits);
	ASSERT(uint32(start + numBlocks) <= fNumBits);

	if (uint32(start + numBlocks) > fNumBits) {
		FATAL(("Allocation::Allocate(): tried to allocate too many blocks: %u (numBlocks = %lu)!\n", numBlocks, fNumBits));
		DIE(("Allocation::Allocate(): tried to allocate too many blocks"));
	}

	int32 block = start >> 5;

	while (numBlocks > 0) {
		uint32 mask = 0;
		for (int32 i = start % 32; i < 32 && numBlocks; i++, numBlocks--)
			mask |= 1UL << (i % 32);

#ifdef DEBUG
		// check for already set blocks
		if (HOST_ENDIAN_TO_BFS_INT32(mask) & ((uint32 *)fBlock)[block]) {
			FATAL(("AllocationBlock::Allocate(): some blocks are already allocated, start = %u, numBlocks = %u\n", start, numBlocks));
			DEBUGGER(("blocks already set!"));
		}
#endif
		Block(block++) |= HOST_ENDIAN_TO_BFS_INT32(mask);
		start = 0;
	}
}


void
AllocationBlock::Free(uint16 start, uint16 numBlocks)
{
	ASSERT(start < fNumBits);
	ASSERT(uint32(start + numBlocks) <= fNumBits);

	if (uint32(start + numBlocks) > fNumBits) {
		FATAL(("Allocation::Free(): tried to free too many blocks: %u (numBlocks = %lu)!\n", numBlocks, fNumBits));
		DIE(("Allocation::Free(): tried to free too many blocks"));
	}

	int32 block = start >> 5;

	while (numBlocks > 0) {
		uint32 mask = 0;
		for (int32 i = start % 32; i < 32 && numBlocks; i++, numBlocks--)
			mask |= 1UL << (i % 32);

		Block(block++) &= HOST_ENDIAN_TO_BFS_INT32(~mask);
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
	if (start > fNumBits)
		return B_ERROR;

	// Update the allocation group info
	// ToDo: this info will be incorrect if something goes wrong later
	// Note, the fFirstFree block doesn't have to be really free
	if (start == fFirstFree)
		fFirstFree = start + length;
	fFreeBits -= length;

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
	if (start > fNumBits)
		return B_ERROR;

	// Update the allocation group info
	// ToDo: this info will be incorrect if something goes wrong later
	if (fFirstFree > start)
		fFirstFree = start;
	fFreeBits += length;

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
		if (uint32(start + length) > cached.NumBlockBits())
			freeLength = cached.NumBlockBits() - start;

		cached.Free(start, freeLength);

		if (cached.WriteBack(transaction) < B_OK)
			return B_IO_ERROR;

		length -= freeLength;
		start = 0;
		block++;
	}
	return B_OK;
}


//	#pragma mark -


BlockAllocator::BlockAllocator(Volume *volume)
	:
	fVolume(volume),
	fLock("bfs allocator"),
	fGroups(NULL),
	fCheckBitmap(NULL)
{
}


BlockAllocator::~BlockAllocator()
{
	delete[] fGroups;
}


status_t 
BlockAllocator::Initialize(bool full)
{
	if (fLock.InitCheck() < B_OK)
		return B_ERROR;

	fNumGroups = fVolume->AllocationGroups();
	fBlocksPerGroup = fVolume->SuperBlock().BlocksPerAllocationGroup();
	fGroups = new AllocationGroup[fNumGroups];
	if (fGroups == NULL)
		return B_NO_MEMORY;

	if (!full)
		return B_OK;

	fLock.Lock();
		// the lock will be released by the initialize() function

	thread_id id = spawn_kernel_thread((thread_func)BlockAllocator::initialize,
			"bfs block allocator", B_LOW_PRIORITY, (void *)this);
	if (id < B_OK)
		return initialize(this);

	return resume_thread(id);
}


status_t 
BlockAllocator::InitializeAndClearBitmap(Transaction &transaction)
{
	status_t status = Initialize(false);
	if (status < B_OK)
		return status;

	uint32 blocks = fBlocksPerGroup;
	uint32 numBits = 8 * blocks * fVolume->BlockSize();

	uint32 *buffer = (uint32 *)malloc(numBits >> 3);
	if (buffer == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	memset(buffer, 0, numBits >> 3);

	off_t offset = 1;

	// initialize the AllocationGroup objects and clear the on-disk bitmap

	for (int32 i = 0; i < fNumGroups; i++) {
		if (cached_write(fVolume->Device(), offset, buffer, blocks, fVolume->BlockSize()) < B_OK)
			return B_ERROR;

		// the last allocation group may contain less blocks than the others
		fGroups[i].fNumBits = i == fNumGroups - 1 ? fVolume->NumBlocks() - i * numBits : numBits;
		fGroups[i].fStart = offset;
		fGroups[i].fFirstFree = fGroups[i].fLargestFirst = 0;
		fGroups[i].fFreeBits = fGroups[i].fLargest = fGroups[i].fNumBits;

		offset += blocks;
	}
	free(buffer);

	// reserve the boot block, the log area, and the block bitmap itself
	uint32 reservedBlocks = fVolume->Log().Start() + fVolume->Log().Length();

	if (fGroups[0].Allocate(&transaction, 0, reservedBlocks) < B_OK) {
		FATAL(("could not allocate reserved space for block bitmap/log!\n"));
		return B_ERROR;
	}
	fVolume->SuperBlock().used_blocks = HOST_ENDIAN_TO_BFS_INT64(reservedBlocks);

	return B_OK;
}


status_t 
BlockAllocator::initialize(BlockAllocator *allocator)
{
	// The lock must already be held at this point

	Volume *volume = allocator->fVolume;
	uint32 blocks = allocator->fBlocksPerGroup;
	uint32 numBits = 8 * blocks * volume->BlockSize();
	off_t freeBlocks = 0;

	uint32 *buffer = (uint32 *)malloc(numBits >> 3);
	if (buffer == NULL) {
		allocator->fLock.Unlock();
		RETURN_ERROR(B_NO_MEMORY);
	}

	AllocationGroup *groups = allocator->fGroups;
	off_t offset = 1;
	int32 num = allocator->fNumGroups;

	for (int32 i = 0;i < num;i++) {
		if (cached_read(volume->Device(), offset, buffer, blocks, volume->BlockSize()) < B_OK)
			break;

		// the last allocation group may contain less blocks than the others
		groups[i].fNumBits = i == num - 1 ? volume->NumBlocks() - i * numBits : numBits;
		groups[i].fStart = offset;

		// finds all free ranges in this allocation group
		int32 start = -1, range = 0;
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
	uint32 reservedBlocks = volume->Log().Start() + volume->Log().Length();
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
		volume->SuperBlock().used_blocks = HOST_ENDIAN_TO_BFS_INT64(usedBlocks);
	}

	allocator->fLock.Unlock();
	return B_OK;
}


status_t
BlockAllocator::AllocateBlocks(Transaction *transaction, int32 group, uint16 start,
	uint16 maximum, uint16 minimum, block_run &run)
{
	if (maximum == 0)
		return B_BAD_VALUE;

	FUNCTION_START(("group = %ld, start = %u, maximum = %u, minimum = %u\n", group, start, maximum, minimum));

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

		for (; block < fBlocksPerGroup; block++) {
			if (cached.SetTo(fGroups[group], block) < B_OK)
				RETURN_ERROR(B_ERROR);

			// find a block large enough to hold the allocation
			for (uint32 bit = start % cached.NumBlockBits(); bit < cached.NumBlockBits(); bit++) {
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

				if (fGroups[group].Allocate(transaction, rangeStart, numBlocks) < B_OK)
					RETURN_ERROR(B_IO_ERROR);

				run.allocation_group = HOST_ENDIAN_TO_BFS_INT32(group);
				run.start = HOST_ENDIAN_TO_BFS_INT16(rangeStart);
				run.length = HOST_ENDIAN_TO_BFS_INT16(numBlocks);

				fVolume->SuperBlock().used_blocks =
					HOST_ENDIAN_TO_BFS_INT64(fVolume->UsedBlocks() + numBlocks);
					// We are not writing back the disk's superblock - it's
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
BlockAllocator::AllocateForInode(Transaction *transaction, const block_run *parent,
	mode_t type, block_run &run)
{
	// apply some allocation policies here (AllocateBlocks() will break them
	// if necessary) - we will start with those described in Dominic Giampaolo's
	// "Practical File System Design", and see how good they work

	// files are going in the same allocation group as its parent, sub-directories
	// will be inserted 8 allocation groups after the one of the parent
	uint16 group = parent->AllocationGroup();
	if ((type & (S_DIRECTORY | S_INDEX_DIR | S_ATTR_DIR)) == S_DIRECTORY)
		group += 8;

	return AllocateBlocks(transaction, group, 0, 1, 1, run);
}


status_t 
BlockAllocator::Allocate(Transaction *transaction, const Inode *inode, off_t numBlocks,
	block_run &run, uint16 minimum)
{
	if (numBlocks <= 0)
		return B_ERROR;

	// one block_run can't hold more data than there is in one allocation group
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
	if (numBlocks > MAX_BLOCK_RUN_LENGTH)
		numBlocks = MAX_BLOCK_RUN_LENGTH;

	// apply some allocation policies here (AllocateBlocks() will break them
	// if necessary)
	uint16 group = inode->BlockRun().AllocationGroup();
	uint16 start = 0;

	// are there already allocated blocks? (then just try to allocate near the last one)
	if (inode->Size() > 0) {
		data_stream *data = &inode->Node()->data;
		// ToDo: we currently don't care for when the data stream
		// is already grown into the indirect ranges
		if (data->max_double_indirect_range == 0
			&& data->max_indirect_range == 0) {
			// Since size > 0, there must be a valid block run in this stream
			int32 last = 0;
			for (; last < NUM_DIRECT_BLOCKS - 1; last++)
				if (data->direct[last + 1].IsZero())
					break;

			group = data->direct[last].AllocationGroup();
			start = data->direct[last].Start() + data->direct[last].Length();
		}
	} else if (inode->IsContainer() || inode->IsSymLink()) {
		// directory and symbolic link data will go in the same allocation
		// group as the inode is in but after the inode data
		start = inode->BlockRun().Start();
	} else {
		// file data will start in the next allocation group
		group = inode->BlockRun().AllocationGroup() + 1;
	}

	return AllocateBlocks(transaction, group, start, numBlocks, minimum, run);
}


status_t 
BlockAllocator::Free(Transaction *transaction, block_run run)
{
	Locker lock(fLock);

	int32 group = run.AllocationGroup();
	uint16 start = run.Start();
	uint16 length = run.Length();

	FUNCTION_START(("group = %ld, start = %u, length = %u\n", group, start, length));

	// doesn't use Volume::IsValidBlockRun() here because it can check better
	// against the group size (the last group may have a different length)
	if (group < 0 || group >= fNumGroups
		|| start > fGroups[group].fNumBits
		|| uint32(start + length) > fGroups[group].fNumBits
		|| length == 0) {
		FATAL(("tried to free an invalid block_run (%ld, %u, %u)\n", group, start, length));
		DEBUGGER(("tried to free invalid block_run"));
		return B_BAD_VALUE;
	}
	// check if someone tries to free reserved areas at the beginning of the drive
	if (group == 0 && start < uint32(fVolume->Log().Start() + fVolume->Log().Length())) {
		FATAL(("tried to free a reserved block_run (%ld, %u, %u)\n", group, start, length));
		DEBUGGER(("tried to free reserved block"));
		return B_BAD_VALUE;
	}
#ifdef DEBUG	
	if (CheckBlockRun(run) < B_OK)
		return B_BAD_DATA;
#endif

	if (fGroups[group].Free(transaction, start, length) < B_OK)
		RETURN_ERROR(B_IO_ERROR);

#ifdef DEBUG
	if (CheckBlockRun(run, NULL, NULL, false) < B_OK)
		DEBUGGER(("CheckBlockRun() reports allocated blocks (which were just freed)\n"));
#endif

	fVolume->SuperBlock().used_blocks =
		HOST_ENDIAN_TO_BFS_INT64(fVolume->UsedBlocks() - run.Length());
	return B_OK;
}


size_t 
BlockAllocator::BitmapSize() const
{
	return fVolume->BlockSize() * fNumGroups * fBlocksPerGroup;
}


//	#pragma mark -
//	Functions to check the validity of the bitmap - they are used from
//	the "chkbfs" command


bool 
BlockAllocator::IsValidCheckControl(check_control *control)
{
	if (control == NULL
		|| control->magic != BFS_IOCTL_CHECK_MAGIC) {
		FATAL(("invalid check_control (%p)!\n", control));
		return false;
	}

	return true;
}


status_t 
BlockAllocator::StartChecking(check_control *control)
{
	if (!IsValidCheckControl(control))
		return B_BAD_VALUE;

	status_t status = fLock.Lock();
	if (status < B_OK)
		return status;

	size_t size = BitmapSize();
	fCheckBitmap = (uint32 *)malloc(size);
	if (fCheckBitmap == NULL) {
		fLock.Unlock();
		return B_NO_MEMORY;
	}

	check_cookie *cookie = new check_cookie();
	if (cookie == NULL) {
		free(fCheckBitmap);
		fCheckBitmap = NULL;
		fLock.Unlock();

		return B_NO_MEMORY;
	}

	// initialize bitmap
	memset(fCheckBitmap, 0, size);
	for (int32 block = fVolume->Log().Start() + fVolume->Log().Length(); block-- > 0;)
		SetCheckBitmapAt(block);

	cookie->stack.Push(fVolume->Root());
	cookie->stack.Push(fVolume->Indices());
	cookie->iterator = NULL;
	control->cookie = cookie;

	fCheckCookie = cookie;
		// to be able to restore nicely if "chkbfs" exited abnormally

	// ToDo: check reserved area in bitmap!

	return B_OK;
}


status_t 
BlockAllocator::StopChecking(check_control *control)
{
	check_cookie *cookie;
	if (control == NULL)
		cookie = fCheckCookie;
	else
		cookie = (check_cookie *)control->cookie;

	if (cookie == NULL)
		return B_ERROR;

	if (cookie->iterator != NULL) {
		delete cookie->iterator;
		cookie->iterator = NULL;

		// the current directory inode is still locked in memory			
		put_vnode(fVolume->ID(), fVolume->ToVnode(cookie->current));
	}

	// if CheckNextNode() could completely work through, we can
	// fix any damages of the bitmap	
	if (control != NULL && control->status == B_ENTRY_NOT_FOUND) {
		// calculate the number of used blocks in the check bitmap
		size_t size = fVolume->BlockSize() * fNumGroups * fBlocksPerGroup;
		off_t usedBlocks = 0LL;

		// ToDo: update the allocation groups used blocks info
		for (uint32 i = size >> 2; i-- > 0;) {
			uint32 compare = 1;
			for (int16 j = 0; j < 32; j++, compare <<= 1) {
				if (compare & fCheckBitmap[i])
					usedBlocks++;
			}
		}

		control->stats.freed = fVolume->UsedBlocks() - usedBlocks + control->stats.missing;
		if (control->stats.freed < 0)
			control->stats.freed = 0;

		// Should we fix errors? Were there any errors we can fix?
		if (control->flags & BFS_FIX_BITMAP_ERRORS
			&& (control->stats.freed != 0 || control->stats.missing != 0)) {
			// if so, write the check bitmap back over the original one,
			// and use transactions here to play safe - we even use several
			// transactions, so that we don't blow the maximum log size
			// on large disks; since we don't need to make this atomic
			fVolume->SuperBlock().used_blocks = HOST_ENDIAN_TO_BFS_INT64(usedBlocks);

			int32 blocksInBitmap = fNumGroups * fBlocksPerGroup;
			int32 blockSize = fVolume->BlockSize();

			for (int32 i = 0; i < blocksInBitmap; i += 512) {
				Transaction transaction(fVolume, 1 + i);

				int32 blocksToWrite = 512;
				if (blocksToWrite + i > blocksInBitmap)
					blocksToWrite = blocksInBitmap - i;

				status_t status = transaction.WriteBlocks(1 + i,
					(uint8 *)fCheckBitmap + i * blockSize, blocksToWrite);
				if (status < B_OK) {
					FATAL(("error writing bitmap: %s\n", strerror(status)));
					break;
				}
				transaction.Done();
			}
		}
	} else
		FATAL(("BlockAllocator::CheckNextNode() didn't run through\n"));

	free(fCheckBitmap);
	fCheckBitmap = NULL;
	fCheckCookie = NULL;
	delete cookie;
	fLock.Unlock();

	return B_OK;
}


status_t
BlockAllocator::CheckNextNode(check_control *control)
{
	if (!IsValidCheckControl(control))
		return B_BAD_VALUE;

	check_cookie *cookie = (check_cookie *)control->cookie;

	while (true) {
		if (cookie->iterator == NULL) {
			if (!cookie->stack.Pop(&cookie->current)) {
				// no more runs on the stack, we are obviously finished!
				control->status = B_ENTRY_NOT_FOUND;
				return B_ENTRY_NOT_FOUND;
			}

			// get iterator for the next directory

#ifdef UNSAFE_GET_VNODE
			RecursiveLocker locker(fVolume->Lock());
#endif
			Vnode vnode(fVolume, cookie->current);
			Inode *inode;
			if (vnode.Get(&inode) < B_OK) {
				FATAL(("check: Could not open inode at %Ld\n", fVolume->ToBlock(cookie->current)));
				continue;
			}

			if (!inode->IsContainer()) {
				FATAL(("check: inode at %Ld should have been a directory\n", fVolume->ToBlock(cookie->current)));
				continue;
			}

			BPlusTree *tree;
			if (inode->GetTree(&tree) != B_OK) {
				FATAL(("check: could not open b+tree from inode at %Ld\n", fVolume->ToBlock(cookie->current)));
				continue;
			}

			cookie->parent = inode;
			cookie->parent_mode = inode->Mode();

			cookie->iterator = new TreeIterator(tree);
			if (cookie->iterator == NULL)
				RETURN_ERROR(B_NO_MEMORY);

			// the inode must stay locked in memory until the iterator is freed
			vnode.Keep();

			// check the inode of the directory
			control->errors = 0;
			control->status = CheckInode(inode, control);

			if (inode->GetName(control->name) < B_OK)
				strcpy(control->name, "(node has no name)");

			control->inode = inode->ID();
			control->mode = inode->Mode();

			return B_OK;
		}

		char name[B_FILE_NAME_LENGTH];
		uint16 length;
		vnode_id id;

		status_t status = cookie->iterator->GetNextEntry(name, &length, B_FILE_NAME_LENGTH, &id);
		if (status == B_ENTRY_NOT_FOUND) {
			// there are no more entries in this iterator, free it and go on
			delete cookie->iterator;
			cookie->iterator = NULL;

			// unlock the directory's inode from memory			
			put_vnode(fVolume->ID(), fVolume->ToVnode(cookie->current));

			continue;
		} else if (status == B_OK) {
			// ignore "." and ".." entries		
			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			// fill in the control data as soon as we have them
			strlcpy(control->name, name, B_FILE_NAME_LENGTH);
			control->inode = id;
			control->errors = 0;

			Vnode vnode(fVolume, id);
			Inode *inode;
			if (vnode.Get(&inode) < B_OK) {
				FATAL(("Could not open inode ID %Ld!\n", id));
				control->errors |= BFS_COULD_NOT_OPEN;
				control->status = B_ERROR;
				return B_OK;
			}

			// check if the inode's name is the same as in the b+tree
			if (inode->IsRegularNode()) {
				SimpleLocker locker(inode->SmallDataLock());

				const char *localName = inode->Name();
				if (localName == NULL || strcmp(localName, name)) {
					control->errors |= BFS_NAMES_DONT_MATCH;
					FATAL(("Names differ: tree \"%s\", inode \"%s\"\n", name, localName));
				}
			}

			control->mode = inode->Mode();

			// Check for the correct mode of the node (if the mode of the
			// file don't fit to its parent, there is a serious problem)
			if ((cookie->parent_mode & S_ATTR_DIR && !inode->IsAttribute())
				|| (cookie->parent_mode & S_INDEX_DIR && !inode->IsIndex())
				|| ((cookie->parent_mode & S_DIRECTORY | S_ATTR_DIR | S_INDEX_DIR) == S_DIRECTORY
					&& inode->Mode() & (S_ATTR | S_ATTR_DIR | S_INDEX_DIR))) {
				FATAL(("inode at %Ld is of wrong type: %o (parent %o at %Ld)!\n",
					inode->BlockNumber(), inode->Mode(), cookie->parent_mode, cookie->parent->BlockNumber()));

				// if we are allowed to fix errors, we should remove the file
				if (control->flags & BFS_REMOVE_WRONG_TYPES
					&& control->flags & BFS_FIX_BITMAP_ERRORS) {
#ifdef UNSAFE_GET_VNODE
					RecursiveLocker locker(fVolume->Lock());
#endif
					// it's safe to start a transaction, because Inode::Remove()
					// won't touch the block bitmap (which we hold the lock for)
					// if we set the INODE_DONT_FREE_SPACE flag - since we fix
					// the bitmap anyway
					Transaction transaction(fVolume, cookie->parent->BlockNumber());

					inode->Node()->flags |= INODE_DONT_FREE_SPACE;
					status = cookie->parent->Remove(&transaction, name, NULL, inode->IsContainer());
					if (status == B_OK)
						transaction.Done();
				} else
					status = B_ERROR;

				control->errors |= BFS_WRONG_TYPE;
				control->status = status;
				return B_OK;
			}

			// If the inode has an attribute directory, push it on the stack
			if (!inode->Attributes().IsZero())
				cookie->stack.Push(inode->Attributes());

			// push the directory on the stack so that it will be scanned later
			if (inode->IsContainer() && !inode->IsIndex())
				cookie->stack.Push(inode->BlockRun());
			else {
				// check it now
				control->status = CheckInode(inode, control);

				return B_OK;
			}
		}
	}
	// is never reached
}


bool
BlockAllocator::CheckBitmapIsUsedAt(off_t block) const
{
	size_t size = BitmapSize();
	uint32 index = block / 32;	// 32bit resolution
	if (index > size / 4)
		return false;

	return BFS_ENDIAN_TO_HOST_INT32(fCheckBitmap[index]) & (1UL << (block & 0x1f));
}


void
BlockAllocator::SetCheckBitmapAt(off_t block)
{
	size_t size = BitmapSize();
	uint32 index = block / 32;	// 32bit resolution
	if (index > size / 4)
		return;

	fCheckBitmap[index] |= HOST_ENDIAN_TO_BFS_INT32(1UL << (block & 0x1f));
}


status_t
BlockAllocator::CheckBlockRun(block_run run, const char *type, check_control *control, bool allocated)
{
	if (run.AllocationGroup() < 0 || run.AllocationGroup() >= fNumGroups
		|| run.Start() > fGroups[run.AllocationGroup()].fNumBits
		|| uint32(run.Start() + run.Length()) > fGroups[run.AllocationGroup()].fNumBits
		|| run.length == 0) {
		PRINT(("%s: block_run(%ld, %u, %u) is invalid!\n", type, run.AllocationGroup(), run.Start(), run.Length()));
		if (control == NULL)
			return B_BAD_DATA;

		control->errors |= BFS_INVALID_BLOCK_RUN;
		return B_OK;
	}

	uint32 bitsPerBlock = fVolume->BlockSize() << 3;
	uint32 block = run.Start() / bitsPerBlock;
	uint32 pos = run.Start() % bitsPerBlock;
	int32 length = 0;
	off_t firstMissing = -1, firstSet = -1;
	off_t firstGroupBlock = (off_t)run.AllocationGroup() << fVolume->AllocationGroupShift();

	AllocationBlock cached(fVolume);

	for (; block < fBlocksPerGroup && length < run.Length(); block++, pos = 0) {
		if (cached.SetTo(fGroups[run.AllocationGroup()], block) < B_OK)
			RETURN_ERROR(B_IO_ERROR);

		if (pos >= cached.NumBlockBits()) {
			// something very strange has happened...
			RETURN_ERROR(B_ERROR);
		}

		while (length < run.Length() && pos < cached.NumBlockBits()) {
			if (cached.IsUsed(pos) != allocated) {
				if (control == NULL) {
					PRINT(("%s: block_run(%ld, %u, %u) is only partially allocated (pos = %ld, length = %ld)!\n",
						type, run.AllocationGroup(), run.Start(), run.Length(), pos, length));
					return B_BAD_DATA;
				}
				if (firstMissing == -1) {
					firstMissing = firstGroupBlock + pos + block * bitsPerBlock;
					control->errors |= BFS_MISSING_BLOCKS;
				}
				control->stats.missing++;
			} else if (firstMissing != -1) {
				PRINT(("%s: block_run(%ld, %u, %u): blocks %Ld - %Ld are %sallocated!\n",
					type, run.allocation_group, run.start, run.length, firstMissing,
					firstGroupBlock + pos + block * bitsPerBlock - 1, allocated ? "not " : ""));
				firstMissing = -1;
			}

			if (fCheckBitmap != NULL) {
				// Set the block in the check bitmap as well, but have a look if it
				// is already allocated first
				uint32 offset = pos + block * bitsPerBlock;
				if (CheckBitmapIsUsedAt(firstGroupBlock + offset)) {
					if (firstSet == -1) {
						firstSet = firstGroupBlock + offset;
						control->errors |= BFS_BLOCKS_ALREADY_SET;
					}
					control->stats.already_set++;
				} else {
					if (firstSet != -1) {
						FATAL(("%s: block_run(%ld, %u, %u): blocks %Ld - %Ld are already set!\n",
							type, run.AllocationGroup(), run.Start(), run.Length(), firstSet, firstGroupBlock + offset - 1));
						firstSet = -1;
					}
					SetCheckBitmapAt(firstGroupBlock + offset);
				}
			}
			length++;
			pos++;
		}

		if (block + 1 >= fBlocksPerGroup || length >= run.Length()) {
			if (firstMissing != -1)
				PRINT(("%s: block_run(%ld, %u, %u): blocks %Ld - %Ld are not allocated!\n", type, run.AllocationGroup(), run.Start(), run.Length(), firstMissing, firstGroupBlock + pos + block * bitsPerBlock - 1));
			if (firstSet != -1)
				FATAL(("%s: block_run(%ld, %u, %u): blocks %Ld - %Ld are already set!\n", type, run.AllocationGroup(), run.Start(), run.Length(), firstSet, firstGroupBlock + pos + block * bitsPerBlock - 1));
		}
	}

	return B_OK;
}


status_t
BlockAllocator::CheckInode(Inode *inode, check_control *control)
{
	if (control != NULL && fCheckBitmap == NULL)
		return B_NO_INIT;
	if (inode == NULL)
		return B_BAD_VALUE;

	status_t status = CheckBlockRun(inode->BlockRun(), "inode", control);
	if (status < B_OK)
		return status;

	if (inode->IsSymLink() && (inode->Flags() & INODE_LONG_SYMLINK) == 0) {
		// symlinks may not have a valid data stream
		if (strlen(inode->Node()->short_symlink) >= SHORT_SYMLINK_NAME_LENGTH)
			return B_BAD_DATA;

		return B_OK;
	}

	data_stream *data = &inode->Node()->data;

	// check the direct range

	if (data->max_direct_range) {
		for (int32 i = 0; i < NUM_DIRECT_BLOCKS; i++) {
			if (data->direct[i].IsZero())
				break;

			status = CheckBlockRun(data->direct[i], "direct", control);
			if (status < B_OK)
				return status;
		}
	}

	CachedBlock cached(fVolume);

	// check the indirect range

	if (data->max_indirect_range) {
		status = CheckBlockRun(data->indirect, "indirect", control);
		if (status < B_OK)
			return status;

		off_t block = fVolume->ToBlock(data->indirect);

		for (int32 i = 0; i < data->indirect.Length(); i++) {
			block_run *runs = (block_run *)cached.SetTo(block + i);
			if (runs == NULL)
				RETURN_ERROR(B_IO_ERROR);

			int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
			int32 index = 0;
			for (; index < runsPerBlock; index++) {
				if (runs[index].IsZero())
					break;

				status = CheckBlockRun(runs[index], "indirect->run", control);
				if (status < B_OK)
					return status;
			}
			if (index < runsPerBlock)
				break;
		}
	}

	// check the double indirect range

	if (data->max_double_indirect_range) {
		status = CheckBlockRun(data->double_indirect, "double indirect", control);
		if (status < B_OK)
			return status;

		int32 runsPerBlock = fVolume->BlockSize() / sizeof(block_run);
		int32 runsPerArray = runsPerBlock << ARRAY_BLOCKS_SHIFT;

		CachedBlock cachedDirect(fVolume);
		int32 maxIndirectIndex = (data->double_indirect.Length() << fVolume->BlockShift())
									/ sizeof(block_run);

		for (int32 indirectIndex = 0; indirectIndex < maxIndirectIndex; indirectIndex++) {
			// get the indirect array block
			block_run *array = (block_run *)cached.SetTo(fVolume->ToBlock(data->double_indirect)
				+ indirectIndex / runsPerBlock);
			if (array == NULL)
				return B_IO_ERROR;

			block_run indirect = array[indirectIndex % runsPerBlock];
			// are we finished yet?
			if (indirect.IsZero())
				return B_OK;

			status = CheckBlockRun(indirect, "double indirect->runs", control);
			if (status < B_OK)
				return status;

			int32 maxIndex = (indirect.Length() << fVolume->BlockShift()) / sizeof(block_run);

			for (int32 index = 0; index < maxIndex; ) {
				block_run *runs = (block_run *)cachedDirect.SetTo(fVolume->ToBlock(indirect)
					+ index / runsPerBlock);
				if (runs == NULL)
					return B_IO_ERROR;

				do {
					// are we finished yet?
					if (runs[index % runsPerBlock].IsZero())
						return B_OK;

					status = CheckBlockRun(runs[index % runsPerBlock], "double indirect->runs->run", control);
					if (status < B_OK)
						return status;
				} while ((++index % runsPerArray) != 0);
			}
		}
	}

	return B_OK;
}

