/* Volume - BFS super block, mounting, etc.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include "Debug.h"
#include "Volume.h"
#include "Journal.h"
#include "Inode.h"
#include "Query.h"

#include <kernel_cpp.h>
#include <KernelExport.h>
#include <fs_volume.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


class DeviceOpener {
	public:
		DeviceOpener(const char *device, int mode);
		~DeviceOpener();

		int Open(const char *device, int mode);
		status_t InitCache(off_t numBlocks);
		void RemoveCache(int mode);

		void Keep();

		int Device() const { return fDevice; }

		status_t GetSize(off_t *_size, uint32 *_blockSize = NULL);

	private:
		int		fDevice;
		bool	fCached;
};


DeviceOpener::DeviceOpener(const char *device, int mode)
	:
	fCached(false)
{
	Open(device, mode);
}


DeviceOpener::~DeviceOpener()
{
	if (fDevice >= B_OK) {
		close(fDevice);
		if (fCached)
			remove_cached_device_blocks(fDevice, NO_WRITES);
	}
}


int 
DeviceOpener::Open(const char *device, int mode)
{
	fDevice = open(device, mode);
	return fDevice;
}


status_t
DeviceOpener::InitCache(off_t numBlocks)
{
	if (init_cache_for_device(fDevice, numBlocks) == B_OK) {
		fCached = true;
		return B_OK;
	}

	return B_ERROR;
}


void 
DeviceOpener::RemoveCache(int mode)
{
	if (!fCached)
		return;

	remove_cached_device_blocks(fDevice, mode);
	fCached = false;
}


void 
DeviceOpener::Keep()
{
	fDevice = -1;
}


/** Returns the size of the device in bytes. It uses B_GET_GEOMETRY
 *	to compute the size, or fstat() if that failed.
 */

status_t 
DeviceOpener::GetSize(off_t *_size, uint32 *_blockSize)
{
	device_geometry geometry;
	if (ioctl(fDevice, B_GET_GEOMETRY, &geometry) < 0) {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fDevice, &stat) < 0)
			return B_ERROR;

		if (_size)
			*_size = stat.st_size;
		if (_blockSize)	// that shouldn't cause us any problems
			*_blockSize = 512;

		return B_OK;
	}

	if (_size)
		*_size = geometry.head_count * geometry.cylinder_count * geometry.sectors_per_track;
	if (_blockSize)
		*_blockSize = geometry.bytes_per_sector;

	return B_OK;
}


//	#pragma mark -


bool
disk_super_block::IsValid()
{
	if (Magic1() != (int32)SUPER_BLOCK_MAGIC1
		|| Magic2() != (int32)SUPER_BLOCK_MAGIC2
		|| Magic3() != (int32)SUPER_BLOCK_MAGIC3
		|| (int32)block_size != inode_size
		|| ByteOrder() != SUPER_BLOCK_FS_LENDIAN
		|| (1UL << BlockShift()) != BlockSize()
		|| AllocationGroups() < 1
		|| AllocationGroupShift() < 1
		|| BlocksPerAllocationGroup() < 1
		|| NumBlocks() < 10
		|| AllocationGroups() != divide_roundup(NumBlocks(),
			1L << AllocationGroupShift()))
		return false;

	return true;
}


void
disk_super_block::Initialize(const char *diskName, off_t numBlocks, uint32 blockSize)
{
	memset(this, 0, sizeof(disk_super_block));

	magic1 = HOST_ENDIAN_TO_BFS_INT32(SUPER_BLOCK_MAGIC1);
	magic2 = HOST_ENDIAN_TO_BFS_INT32(SUPER_BLOCK_MAGIC2);
	magic3 = HOST_ENDIAN_TO_BFS_INT32(SUPER_BLOCK_MAGIC3);
	fs_byte_order = SUPER_BLOCK_FS_LENDIAN;
	flags = SUPER_BLOCK_DISK_CLEAN;

	strlcpy(name, diskName, sizeof(name));

	block_size = inode_size = HOST_ENDIAN_TO_BFS_INT32(blockSize);
	for (block_shift = 9; (1UL << block_shift) < blockSize; block_shift++);

	num_blocks = numBlocks;
	used_blocks = 0;

	// ToDo: set allocation group stuff for real (this is hardcoded for a 10 MB file)!!!

	blocks_per_ag = 1;
	num_ags = 2;
	ag_shift = 13;
}


//	#pragma mark -


Volume::Volume(nspace_id id)
	:
	fID(id),
	fBlockAllocator(this),
	fLock("bfs volume"),
	fRootNode(NULL),
	fIndicesNode(NULL),
	fDirtyCachedBlocks(0),
	fUniqueID(0),
	fFlags(0)
{
}


Volume::~Volume()
{
}


bool
Volume::IsValidSuperBlock()
{
	return fSuperBlock.IsValid();
}


void 
Volume::Panic()
{
	FATAL(("we have to panic... switch to read-only mode!\n"));
	fFlags |= VOLUME_READ_ONLY;
#ifdef USER
	debugger("BFS panics!");
#elif defined(DEBUG)
	kernel_debugger("BFS panics!");
#endif
}


status_t
Volume::Mount(const char *deviceName, uint32 flags)
{
	if (flags & B_MOUNT_READ_ONLY)
		fFlags |= VOLUME_READ_ONLY;

	// ToDo: validate the FS in write mode as well!
#if (B_HOST_IS_LENDIAN && defined(BFS_BIG_ENDIAN_ONLY)) \
	|| (B_HOST_IS_BENDIAN && defined(BFS_LITTLE_ENDIAN_ONLY))
	// in big endian mode, we only mount read-only for now
	flags |= B_MOUNT_READ_ONLY;
#endif

	DeviceOpener opener(deviceName, flags & B_MOUNT_READ_ONLY ? O_RDONLY : O_RDWR);

	// if we couldn't open the device, try read-only (don't rely on a specific error code)
	if (opener.Device() < B_OK && (flags & B_MOUNT_READ_ONLY) == 0) {
		opener.Open(deviceName, O_RDONLY);
		fFlags |= VOLUME_READ_ONLY;
	}

	fDevice = opener.Device();
	if (fDevice < B_OK)
		RETURN_ERROR(fDevice);

	// check if it's a regular file, and if so, disable the cache for the
	// underlaying file system
	struct stat stat;
	if (fstat(fDevice, &stat) < 0)
		RETURN_ERROR(B_ERROR);

//#ifndef USER
	if (stat.st_mode & S_FILE && ioctl(fDevice, IOCTL_FILE_UNCACHED_IO, NULL) < 0) {
		// mount read-only if the cache couldn't be disabled
#	ifdef DEBUG
		FATAL(("couldn't disable cache for image file - system may dead-lock!\n"));
#	else
		FATAL(("couldn't disable cache for image file!\n"));
		Panic();
#	endif
	}
//#endif

	// read the super block
	char buffer[1024];
	if (read_pos(fDevice, 0, buffer, sizeof(buffer)) != sizeof(buffer))
		return B_IO_ERROR;

	status_t status = B_OK;

	// Note: that does work only for x86, for PowerPC, the super block
	// is located at offset 0!
	memcpy(&fSuperBlock, buffer + 512, sizeof(disk_super_block));
	if (!IsValidSuperBlock()) {
#ifndef BFS_LITTLE_ENDIAN_ONLY
		memcpy(&fSuperBlock, buffer, sizeof(disk_super_block));
		if (!IsValidSuperBlock())
			return B_BAD_VALUE;
#else
		return B_BAD_VALUE;
#endif
	}

	if (!IsValidSuperBlock()) {
		FATAL(("invalid super block!\n"));
		return B_BAD_VALUE;
	}

	// check if the device size is large enough to hold the file system
	off_t diskSize;
	if (opener.GetSize(&diskSize) < B_OK)
		RETURN_ERROR(B_ERROR);
	if (diskSize < (NumBlocks() << BlockShift()))
		RETURN_ERROR(B_BAD_VALUE);

	// set the current log pointers, so that journaling will work correctly
	fLogStart = fSuperBlock.LogStart();
	fLogEnd = fSuperBlock.LogEnd();

	// initialize short hands to the super block (to save byte swapping)
	fBlockSize = fSuperBlock.BlockSize();
	fBlockShift = fSuperBlock.BlockShift();
	fAllocationGroupShift = fSuperBlock.AllocationGroupShift();

	if (opener.InitCache(NumBlocks()) != B_OK)
		return B_ERROR;

	fJournal = new Journal(this);
	// replaying the log is the first thing we will do on this disk
	if (fJournal && fJournal->InitCheck() < B_OK
		&& fBlockAllocator.Initialize() < B_OK) {
		// ToDo: improve error reporting for a bad journal
		FATAL(("could not initialize journal/block bitmap allocator!\n"));
		return B_NO_MEMORY;
	}

	fRootNode = new Inode(this, ToVnode(Root()));
	if (fRootNode && fRootNode->InitCheck() == B_OK) {
		if (new_vnode(fID, ToVnode(Root()), (void *)fRootNode) == B_OK) {
			// try to get indices root dir

			// question: why doesn't get_vnode() work here??
			// answer: we have not yet backpropagated the pointer to the
			// volume in bfs_mount(), so bfs_read_vnode() can't get it.
			// But it's not needed to do that anyway.

			fIndicesNode = new Inode(this, ToVnode(Indices()));
			if (fIndicesNode == NULL
				|| fIndicesNode->InitCheck() < B_OK
				|| !fIndicesNode->IsContainer()) {
				INFORM(("bfs: volume doesn't have indices!\n"));

				if (fIndicesNode) {
					// if this is the case, the index root node is gone bad, and
					// BFS switch to read-only mode
					fFlags |= VOLUME_READ_ONLY;
					fIndicesNode = NULL;
				}
			}

			// all went fine
			opener.Keep();
			return B_OK;
		} else
			status = B_NO_MEMORY;
	} else
		status = B_BAD_VALUE;

	FATAL(("could not create root node: new_vnode() failed!\n"));

	return status;
}


status_t
Volume::Unmount()
{
	// This will also flush the log & all blocks to disk
	delete fJournal;
	fJournal = NULL;

	delete fIndicesNode;

	remove_cached_device_blocks(fDevice, IsReadOnly() ? NO_WRITES : ALLOW_WRITES);
	close(fDevice);

	return B_OK;
}


status_t 
Volume::Sync()
{
	return fJournal->FlushLogAndBlocks();
}


status_t
Volume::ValidateBlockRun(block_run run)
{
	if (run.AllocationGroup() < 0 || run.AllocationGroup() > (int32)AllocationGroups()
		|| run.Start() > (1UL << AllocationGroupShift())
		|| run.length == 0
		|| uint32(run.Length() + run.Start()) > (1UL << AllocationGroupShift())) {
		Panic();
		FATAL(("*** invalid run(%ld,%d,%d)\n", run.AllocationGroup(), run.Start(), run.Length()));
		return B_BAD_DATA;
	}
	return B_OK;
}


block_run 
Volume::ToBlockRun(off_t block) const
{
	block_run run;
	run.allocation_group = HOST_ENDIAN_TO_BFS_INT32(block >> AllocationGroupShift());
	run.start = HOST_ENDIAN_TO_BFS_INT16(block & ((1LL << AllocationGroupShift()) - 1));
	run.length = HOST_ENDIAN_TO_BFS_INT16(1);
	return run;
}


status_t
Volume::CreateIndicesRoot(Transaction *transaction)
{
	off_t id;
	status_t status = Inode::Create(transaction, NULL, NULL,
		S_INDEX_DIR | S_STR_INDEX | S_DIRECTORY | 0700, 0, 0, &id, &fIndicesNode);
	if (status < B_OK)
		RETURN_ERROR(status);

	fSuperBlock.indices = ToBlockRun(id);
	return WriteSuperBlock();
}


status_t 
Volume::AllocateForInode(Transaction *transaction, const Inode *parent, mode_t type, block_run &run)
{
	return fBlockAllocator.AllocateForInode(transaction, &parent->BlockRun(), type, run);
}


status_t 
Volume::WriteSuperBlock()
{
	if (write_pos(fDevice, 512, &fSuperBlock, sizeof(disk_super_block)) != sizeof(disk_super_block))
		return B_IO_ERROR;

	return B_OK;
}


void
Volume::UpdateLiveQueries(Inode *inode, const char *attribute, int32 type, const uint8 *oldKey,
	size_t oldLength, const uint8 *newKey, size_t newLength)
{
	if (fQueryLock.Lock() < B_OK)
		return;

	Query *query = NULL;
	while ((query = fQueries.Next(query)) != NULL)
		query->LiveUpdate(inode, attribute, type, oldKey, oldLength, newKey, newLength);

	fQueryLock.Unlock();
}


/** Checks if there is a live query whose results depend on the presence
 *	or value of the specified attribute.
 *	Don't use it if you already have all the data together to evaluate
 *	the queries - it wouldn't safe you anything in this case.
 */

bool 
Volume::CheckForLiveQuery(const char *attribute)
{
	// ToDo: check for a live query that depends on the specified attribute
	return true;
}


void 
Volume::AddQuery(Query *query)
{
	if (fQueryLock.Lock() < B_OK)
		return;

	fQueries.Add(query);

	fQueryLock.Unlock();
}


void 
Volume::RemoveQuery(Query *query)
{
	if (fQueryLock.Lock() < B_OK)
		return;

	fQueries.Remove(query);

	fQueryLock.Unlock();
}


//	#pragma mark -
//	Disk initialization


status_t
Volume::Initialize(const char *device, const char *name, uint32 blockSize, uint32 flags)
{
	// although there is no really good reason for it, we won't
	// accept '/' in disk names (mkbfs does this, too - and since
	// Tracker names mounted volumes like their name)
	if (strchr(name, '/') != NULL)
		return B_BAD_VALUE;

	if (blockSize != 1024 && blockSize != 2048 && blockSize != 4096 && blockSize != 8192)
		return B_BAD_VALUE;

	DeviceOpener opener(device, O_RDWR);
	if (opener.Device() < B_OK)
		return B_BAD_VALUE;

	fDevice = opener.Device();

	uint32 deviceBlockSize;
	off_t deviceSize;
	if (opener.GetSize(&deviceSize, &deviceBlockSize) < B_OK)
		return B_ERROR;

	off_t numBlocks = deviceSize / blockSize;

	// create valid super block

	fSuperBlock.Initialize(name, numBlocks, blockSize);
	
	// initialize short hands to the super block (to save byte swapping)
	fBlockSize = fSuperBlock.BlockSize();
	fBlockShift = fSuperBlock.BlockShift();
	fAllocationGroupShift = fSuperBlock.AllocationGroupShift();

	// since the allocator has not been initialized yet, we
	// cannot use BlockAllocator::BitmapSize() here
	fSuperBlock.log_blocks = ToBlockRun(AllocationGroups()
		* fSuperBlock.BlocksPerAllocationGroup() + 1);
	fSuperBlock.log_blocks.length = 4096;
		// ToDo: set the log size depending on the disk size
	fSuperBlock.log_start = fSuperBlock.log_end = HOST_ENDIAN_TO_BFS_INT64(ToBlock(Log()));

	// set the current log pointers, so that journaling will work correctly
	fLogStart = fSuperBlock.LogStart();
	fLogEnd = fSuperBlock.LogEnd();

	if (!IsValidSuperBlock())
		RETURN_ERROR(B_ERROR);

	if (opener.InitCache(numBlocks) != B_OK)
		return B_ERROR;

	fJournal = new Journal(this);
	if (fJournal == NULL || fJournal->InitCheck() < B_OK)
		RETURN_ERROR(B_ERROR);

	// ready to write data to disk

	Transaction transaction(this, 0);

	if (fBlockAllocator.InitializeAndClearBitmap(transaction) < B_OK)
		RETURN_ERROR(B_ERROR);

	off_t id;
	status_t status = Inode::Create(&transaction, NULL, NULL,
		S_DIRECTORY | 0755, 0, 0, &id, &fRootNode);
	if (status < B_OK)
		RETURN_ERROR(status);

	fSuperBlock.root_dir = ToBlockRun(id);

	if ((flags & VOLUME_NO_INDICES) == 0) {
		if ((status = CreateIndicesRoot(&transaction)) < B_OK)
			return status;
	}

	WriteSuperBlock();
	transaction.Done();

	put_vnode(ID(), fRootNode->ID());
	if (fIndicesNode != NULL)
		put_vnode(ID(), fIndicesNode->ID());

	Sync();
	opener.RemoveCache(ALLOW_WRITES);
	return B_OK;
}
