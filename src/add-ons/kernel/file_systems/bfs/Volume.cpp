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

Volume::Volume(nspace_id id)
	:
	fID(id),
	fBlockAllocator(this),
	fLock("bfs volume"),
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
	if (fSuperBlock.Magic1() != (int32)SUPER_BLOCK_MAGIC1
		|| fSuperBlock.Magic2() != (int32)SUPER_BLOCK_MAGIC2
		|| fSuperBlock.Magic3() != (int32)SUPER_BLOCK_MAGIC3
		|| (int32)fSuperBlock.block_size != fSuperBlock.inode_size
		|| fSuperBlock.ByteOrder() != SUPER_BLOCK_FS_LENDIAN
		|| (1UL << fSuperBlock.BlockShift()) != fSuperBlock.BlockSize()
		|| fSuperBlock.AllocationGroups() < 1
		|| fSuperBlock.AllocationGroupShift() < 1
		|| fSuperBlock.BlocksPerAllocationGroup() < 1
		|| fSuperBlock.NumBlocks() < 10
		|| fSuperBlock.AllocationGroups() != divide_roundup(fSuperBlock.NumBlocks(),
			1L << fSuperBlock.AllocationGroupShift()))
		return false;

	return true;
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

	fDevice = open(deviceName, flags & B_MOUNT_READ_ONLY ? O_RDONLY : O_RDWR);

	// if we couldn't open the device, try read-only (don't rely on a specific error code)
	if (fDevice < B_OK && (flags & B_MOUNT_READ_ONLY) == 0) {
		fDevice = open(deviceName, O_RDONLY);
		fFlags |= VOLUME_READ_ONLY;
	}

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
		if (!IsValidSuperBlock()) {
			close(fDevice);
			return B_BAD_VALUE;
		}
#else		
		close(fDevice);
		return B_BAD_VALUE;
#endif
	}

	if (IsValidSuperBlock()) {
		// set the current log pointers, so that journaling will work correctly
		fLogStart = fSuperBlock.LogStart();
		fLogEnd = fSuperBlock.LogEnd();

		// initialize short hands to the super block (to save byte swapping)
		fBlockSize = fSuperBlock.BlockSize();
		fBlockShift = fSuperBlock.BlockShift();
		fAllocationGroupShift = fSuperBlock.AllocationGroupShift();

		if (init_cache_for_device(fDevice, NumBlocks()) == B_OK) {
			fJournal = new Journal(this);
			// replaying the log is the first thing we will do on this disk
			if (fJournal && fJournal->InitCheck() == B_OK
				&& fBlockAllocator.Initialize() == B_OK) {
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
						return B_OK;
					} else
						status = B_NO_MEMORY;
				} else
					status = B_BAD_VALUE;

				FATAL(("could not create root node: new_vnode() failed!\n"));
			} else {
				// ToDo: improve error reporting for a bad journal
				status = B_NO_MEMORY;
				FATAL(("could not initialize journal/block bitmap allocator!\n"));
			}

			remove_cached_device_blocks(fDevice, NO_WRITES);
		} else {
			FATAL(("could not initialize cache!\n"));
			status = B_IO_ERROR;
		}
		FATAL(("invalid super block!\n"));
	}
	else
		status = B_BAD_VALUE;

	close(fDevice);

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
	run.start = HOST_ENDIAN_TO_BFS_INT16(block & ~((1LL << AllocationGroupShift()) - 1));
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

