/*
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


//!	file system interface to Haiku's vnode layer


#include "Debug.h"
#include "Volume.h"
#include "Inode.h"
#include "Index.h"
#include "BPlusTree.h"
#include "Query.h"
#include "Attribute.h"
#include "bfs_control.h"
#include "bfs_disk_system.h"

// TODO: temporary solution as long as there is no public I/O requests API
#ifndef BFS_SHELL
#	include <io_requests.h>
#endif

#define BFS_IO_SIZE	65536


struct identify_cookie {
	disk_super_block super_block;
};

extern void fill_stat_buffer(Inode* inode, struct stat& stat);


static void
fill_stat_time(const bfs_inode& node, struct stat& stat)
{
	bigtime_t now = real_time_clock_usecs();
	stat.st_atim.tv_sec = now / 1000000LL;
	stat.st_atim.tv_nsec = (now % 1000000LL) * 1000;

	stat.st_mtim.tv_sec = bfs_inode::ToSecs(node.LastModifiedTime());
	stat.st_mtim.tv_nsec = bfs_inode::ToNsecs(node.LastModifiedTime());
	stat.st_crtim.tv_sec = bfs_inode::ToSecs(node.CreateTime());
	stat.st_crtim.tv_nsec = bfs_inode::ToNsecs(node.CreateTime());

	// For BeOS compatibility, if on-disk ctime is invalid, fall back to mtime:
	bigtime_t changeTime = node.StatusChangeTime();
	if (changeTime < node.LastModifiedTime())
		stat.st_ctim = stat.st_mtim;
	else {
		stat.st_ctim.tv_sec = bfs_inode::ToSecs(changeTime);
		stat.st_ctim.tv_nsec = bfs_inode::ToNsecs(changeTime);
	}
}


void
fill_stat_buffer(Inode* inode, struct stat& stat)
{
	const bfs_inode& node = inode->Node();

	stat.st_dev = inode->GetVolume()->ID();
	stat.st_ino = inode->ID();
	stat.st_nlink = 1;
	stat.st_blksize = BFS_IO_SIZE;

	stat.st_uid = node.UserID();
	stat.st_gid = node.GroupID();
	stat.st_mode = node.Mode();
	stat.st_type = node.Type();

	fill_stat_time(node, stat);

	if (inode->IsSymLink() && (inode->Flags() & INODE_LONG_SYMLINK) == 0) {
		// symlinks report the size of the link here
		stat.st_size = strlen(node.short_symlink);
	} else
		stat.st_size = inode->Size();

	stat.st_blocks = inode->AllocatedSize() / 512;
}


//!	bfs_io() callback hook
static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Inode* inode = (Inode*)cookie;

	return file_map_translate(inode->Map(), offset, size, vecs, _count,
		inode->GetVolume()->BlockSize());
}


//!	bfs_io() callback hook
static status_t
iterative_io_finished_hook(void* cookie, io_request* request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	Inode* inode = (Inode*)cookie;
	rw_lock_read_unlock(&inode->Lock());
	return B_OK;
}


//	#pragma mark - Scanning


static float
bfs_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	disk_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return -1;

	identify_cookie* cookie = new(std::nothrow) identify_cookie;
	if (cookie == NULL)
		return -1;

	memcpy(&cookie->super_block, &superBlock, sizeof(disk_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
bfs_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks()
		* cookie->super_block.BlockSize();
	partition->block_size = cookie->super_block.BlockSize();
	partition->content_name = strdup(cookie->super_block.name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
bfs_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	identify_cookie* cookie = (identify_cookie*)_cookie;
	delete cookie;
}


//	#pragma mark -


static status_t
bfs_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootID)
{
	FUNCTION();

	Volume* volume = new(std::nothrow) Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status = volume->Mount(device, flags);
	if (status != B_OK) {
		delete volume;
		RETURN_ERROR(status);
	}

	_volume->private_volume = volume;
	_volume->ops = &gBFSVolumeOps;
	*_rootID = volume->ToVnode(volume->Root());

	INFORM(("mounted \"%s\" (root node at %" B_PRIdINO ", device = %s)\n",
		volume->Name(), *_rootID, device));
	return B_OK;
}


static status_t
bfs_unmount(fs_volume* _volume)
{
	FUNCTION();
	Volume* volume = (Volume*)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	RETURN_ERROR(status);
}


static status_t
bfs_read_fs_stat(fs_volume* _volume, struct fs_info* info)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	MutexLocker locker(volume->Lock());

	// File system flags.
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME
		| (volume->IndicesNode() != NULL ? B_FS_HAS_QUERY : 0)
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);

	info->io_size = BFS_IO_SIZE;
		// whatever is appropriate here?

	info->block_size = volume->BlockSize();
	info->total_blocks = volume->NumBlocks();
	info->free_blocks = volume->FreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	strlcpy(info->fsh_name, "bfs", sizeof(info->fsh_name));

	return B_OK;
}


static status_t
bfs_write_fs_stat(fs_volume* _volume, const struct fs_info* info, uint32 mask)
{
	FUNCTION_START(("mask = %ld\n", mask));

	Volume* volume = (Volume*)_volume->private_volume;
	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	MutexLocker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & FS_WRITE_FSINFO_NAME) {
		disk_super_block& superBlock = volume->SuperBlock();

		strncpy(superBlock.name, info->volume_name,
			sizeof(superBlock.name) - 1);
		superBlock.name[sizeof(superBlock.name) - 1] = '\0';

		status = volume->WriteSuperBlock();
	}
	return status;
}


static status_t
bfs_sync(fs_volume* _volume)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	return volume->Sync();
}


//	#pragma mark -


/*!	Reads in the node from disk and creates an inode object from it.
*/
static status_t
bfs_get_vnode(fs_volume* _volume, ino_t id, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	//FUNCTION_START(("ino_t = %Ld\n", id));
	Volume* volume = (Volume*)_volume->private_volume;

	// first inode may be after the log area, we don't go through
	// the hassle and try to load an earlier block from disk
	if (id < volume->ToBlock(volume->Log()) + volume->Log().Length()
		|| id > volume->NumBlocks()) {
		INFORM(("inode at %" B_PRIdINO " requested!\n", id));
		return B_ERROR;
	}

	CachedBlock cached(volume, id);
	bfs_inode* node = (bfs_inode*)cached.Block();
	if (node == NULL) {
		FATAL(("could not read inode: %" B_PRIdINO "\n", id));
		return B_IO_ERROR;
	}

	status_t status = node->InitCheck(volume);
	if (status != B_OK) {
		if ((node->Flags() & INODE_DELETED) != 0) {
			INFORM(("inode at %" B_PRIdINO " is already deleted!\n", id));
		} else {
			FATAL(("inode at %" B_PRIdINO " could not be read: %s!\n", id,
				strerror(status)));
		}
		return status;
	}

	Inode* inode = new(std::nothrow) Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status = inode->InitCheck(false);
	if (status != B_OK)
		delete inode;

	if (status == B_OK) {
		_node->private_node = inode;
		_node->ops = &gBFSVnodeOps;
		*_type = inode->Mode();
		*_flags = 0;
	}

	return status;
}


static status_t
bfs_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	// since a directory's size can be changed without having it opened,
	// we need to take care about their preallocated blocks here
	if (!volume->IsReadOnly() && !volume->IsCheckingThread()
		&& inode->NeedsTrimming()) {
		Transaction transaction(volume, inode->BlockNumber());

		if (inode->TrimPreallocation(transaction) == B_OK)
			transaction.Done();
		else if (transaction.HasParent()) {
			// TODO: for now, we don't let sub-transactions fail
			transaction.Done();
		}
	}

	delete inode;
	return B_OK;
}


static status_t
bfs_remove_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	// If the inode isn't in use anymore, we were called before
	// bfs_unlink() returns - in this case, we can just use the
	// transaction which has already deleted the inode.
	Transaction transaction(volume, volume->ToBlock(inode->Parent()));

	// The file system check functionality uses this flag to prevent the space
	// used up by the inode from being freed - this flag is set only in
	// situations where this does not cause any harm as the block bitmap will
	// get fixed anyway in this case).
	if ((inode->Flags() & INODE_DONT_FREE_SPACE) != 0) {
		delete inode;
		return B_OK;
	}

	ASSERT((inode->Flags() & INODE_DELETED) != 0);

	status_t status = inode->Free(transaction);
	if (status == B_OK) {
		status = transaction.Done();
	} else if (transaction.HasParent()) {
		// TODO: for now, we don't let sub-transactions fail
		status = transaction.Done();
	}

	volume->RemovedInodes().Remove(inode);

	// TODO: the VFS currently does not allow this to fail
	delete inode;

	return status;
}


static bool
bfs_can_page(fs_volume* _volume, fs_vnode* _v, void* _cookie)
{
	// TODO: we're obviously not even asked...
	return false;
}


static status_t
bfs_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->FileCache() == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	InodeReadLocker _(inode);

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	while (true) {
		file_io_vec fileVecs[8];
		size_t fileVecCount = 8;

		status = file_map_translate(inode->Map(), pos, bytesLeft, fileVecs,
			&fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bool bufferOverflow = status == B_BUFFER_OVERFLOW;

		size_t bytes = bytesLeft;
		status = read_file_io_vec_pages(volume->Device(), fileVecs,
			fileVecCount, vecs, count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	return status;
}


static status_t
bfs_write_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (inode->FileCache() == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	InodeReadLocker _(inode);

	uint32 vecIndex = 0;
	size_t vecOffset = 0;
	size_t bytesLeft = *_numBytes;
	status_t status;

	while (true) {
		file_io_vec fileVecs[8];
		size_t fileVecCount = 8;

		status = file_map_translate(inode->Map(), pos, bytesLeft, fileVecs,
			&fileVecCount, 0);
		if (status != B_OK && status != B_BUFFER_OVERFLOW)
			break;

		bool bufferOverflow = status == B_BUFFER_OVERFLOW;

		size_t bytes = bytesLeft;
		status = write_file_io_vec_pages(volume->Device(), fileVecs,
			fileVecCount, vecs, count, &vecIndex, &vecOffset, &bytes);
		if (status != B_OK || !bufferOverflow)
			break;

		pos += bytes;
		bytesLeft -= bytes;
	}

	return status;
}


static status_t
bfs_io(fs_volume* _volume, fs_vnode* _node, void* _cookie, io_request* request)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

#ifndef BFS_SHELL
	if (io_request_is_write(request) && volume->IsReadOnly()) {
		notify_io_request(request, B_READ_ONLY_DEVICE);
		return B_READ_ONLY_DEVICE;
	}
#endif

	if (inode->FileCache() == NULL) {
#ifndef BFS_SHELL
		notify_io_request(request, B_BAD_VALUE);
#endif
		RETURN_ERROR(B_BAD_VALUE);
	}

	// We lock the node here and will unlock it in the "finished" hook.
	rw_lock_read_lock(&inode->Lock());

	return do_iterative_fd_io(volume->Device(), request,
		iterative_io_get_vecs_hook, iterative_io_finished_hook, inode);
}


static status_t
bfs_get_file_map(fs_volume* _volume, fs_vnode* _node, off_t offset, size_t size,
	struct file_io_vec* vecs, size_t* _count)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	int32 blockShift = volume->BlockShift();
	uint32 index = 0, max = *_count;
	block_run run;
	off_t fileOffset;

	//FUNCTION_START(("offset = %Ld, size = %lu\n", offset, size));

	while (true) {
		status_t status = inode->FindBlockRun(offset, run, fileOffset);
		if (status != B_OK)
			return status;

		vecs[index].offset = volume->ToOffset(run) + offset - fileOffset;
		vecs[index].length = (run.Length() << blockShift) - offset + fileOffset;

		// are we already done?
		if ((uint64)size <= (uint64)vecs[index].length
			|| (uint64)offset + (uint64)vecs[index].length
				>= (uint64)inode->Size()) {
			if ((uint64)offset + (uint64)vecs[index].length
					> (uint64)inode->Size()) {
				// make sure the extent ends with the last official file
				// block (without taking any preallocations into account)
				vecs[index].length = round_up(inode->Size() - offset,
					volume->BlockSize());
			}
			*_count = index + 1;
			return B_OK;
		}

		offset += vecs[index].length;
		size -= vecs[index].length;
		index++;

		if (index >= max) {
			// we're out of file_io_vecs; let's bail out
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}
	}

	// can never get here
	return B_ERROR;
}


//	#pragma mark -


static status_t
bfs_lookup(fs_volume* _volume, fs_vnode* _directory, const char* file,
	ino_t* _vnodeID)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	InodeReadLocker locker(directory);

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status != B_OK)
		RETURN_ERROR(status);

	BPlusTree* tree = directory->Tree();
	if (tree == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status = tree->Find((uint8*)file, (uint16)strlen(file), _vnodeID);
	if (status != B_OK) {
		//PRINT(("bfs_walk() could not find %Ld:\"%s\": %s\n", directory->BlockNumber(), file, strerror(status)));
		return status;
	}

	entry_cache_add(volume->ID(), directory->ID(), file, *_vnodeID);

	locker.Unlock();

	Inode* inode;
	status = get_vnode(volume->FSVolume(), *_vnodeID, (void**)&inode);
	if (status != B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}

	return B_OK;
}


static status_t
bfs_get_vnode_name(fs_volume* _volume, fs_vnode* _node, char* buffer,
	size_t bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;

	return inode->GetName(buffer, bufferSize);
}


static status_t
bfs_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 cmd,
	void* buffer, size_t bufferLength)
{
	FUNCTION_START(("node = %p, cmd = %lu, buf = %p, len = %ld\n", _node, cmd,
		buffer, bufferLength));

	Volume* volume = (Volume*)_volume->private_volume;

	switch (cmd) {
		case BFS_IOCTL_VERSION:
		{
			uint32 version = 0x10000;
			return user_memcpy(buffer, &version, sizeof(uint32));
		}
		case BFS_IOCTL_START_CHECKING:
		{
			// start checking
			BlockAllocator& allocator = volume->Allocator();
			check_control control;
			if (user_memcpy(&control, buffer, sizeof(check_control)) != B_OK)
				return B_BAD_ADDRESS;

			status_t status = allocator.StartChecking(&control);
			if (status == B_OK) {
				file_cookie* cookie = (file_cookie*)_cookie;
				cookie->open_mode |= BFS_OPEN_MODE_CHECKING;
			}

			return status;
		}
		case BFS_IOCTL_STOP_CHECKING:
		{
			// stop checking
			BlockAllocator& allocator = volume->Allocator();
			check_control control;

			status_t status = allocator.StopChecking(&control);
			if (status == B_OK) {
				file_cookie* cookie = (file_cookie*)_cookie;
				cookie->open_mode &= ~BFS_OPEN_MODE_CHECKING;
			}
			if (status == B_OK)
				status = user_memcpy(buffer, &control, sizeof(check_control));

			return status;
		}
		case BFS_IOCTL_CHECK_NEXT_NODE:
		{
			// check next
			BlockAllocator& allocator = volume->Allocator();
			check_control control;

			status_t status = allocator.CheckNextNode(&control);
			if (status == B_OK)
				status = user_memcpy(buffer, &control, sizeof(check_control));

			return status;
		}
		case BFS_IOCTL_UPDATE_BOOT_BLOCK:
		{
			// let's makebootable (or anyone else) update the boot block
			// while BFS is mounted
			update_boot_block update;
			if (bufferLength != sizeof(update_boot_block))
				return B_BAD_VALUE;
			if (user_memcpy(&update, buffer, sizeof(update_boot_block)) != B_OK)
				return B_BAD_ADDRESS;
			if (update.offset < offsetof(disk_super_block, pad_to_block)
				|| update.length + update.offset > 512)
				return B_BAD_VALUE;
			if (user_memcpy((uint8*)&volume->SuperBlock() + update.offset,
					update.data, update.length) != B_OK)
				return B_BAD_ADDRESS;

			return volume->WriteSuperBlock();
		}

#ifdef DEBUG_FRAGMENTER
		case 56741:
		{
			BlockAllocator& allocator = volume->Allocator();
			allocator.Fragment();
			return B_OK;
		}
#endif

#ifdef DEBUG
		case 56742:
		{
			// allocate all free blocks and zero them out
			// (a test for the BlockAllocator)!
			BlockAllocator& allocator = volume->Allocator();
			Transaction transaction(volume, 0);
			CachedBlock cached(volume);
			block_run run;
			while (allocator.AllocateBlocks(transaction, 8, 0, 64, 1, run)
					== B_OK) {
				PRINT(("write block_run(%ld, %d, %d)\n", run.allocation_group,
					run.start, run.length));
				for (int32 i = 0;i < run.length;i++) {
					uint8* block = cached.SetToWritable(transaction, run);
					if (block != NULL)
						memset(block, 0, volume->BlockSize());
				}
			}
			return B_OK;
		}
#endif
	}
	return B_DEV_INVALID_IOCTL;
}


/*!	Sets the open-mode flags for the open file cookie - only
	supports O_APPEND currently, but that should be sufficient
	for a file system.
*/
static status_t
bfs_set_flags(fs_volume* _volume, fs_vnode* _node, void* _cookie, int flags)
{
	FUNCTION_START(("node = %p, flags = %d", _node, flags));

	file_cookie* cookie = (file_cookie*)_cookie;
	cookie->open_mode = (cookie->open_mode & ~O_APPEND) | (flags & O_APPEND);

	return B_OK;
}


static status_t
bfs_fsync(fs_volume* _volume, fs_vnode* _node)
{
	FUNCTION();

	Inode* inode = (Inode*)_node->private_node;
	return inode->Sync();
}


static status_t
bfs_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	FUNCTION();

	Inode* inode = (Inode*)_node->private_node;
	fill_stat_buffer(inode, *stat);
	return B_OK;
}


static status_t
bfs_write_stat(fs_volume* _volume, fs_vnode* _node, const struct stat* stat,
	uint32 mask)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// TODO: we should definitely check a bit more if the new stats are
	//	valid - or even better, the VFS should check this before calling us

	bfs_inode& node = inode->Node();
	bool updateTime = false;
	uid_t uid = geteuid();

	bool isOwnerOrRoot = uid == 0 || uid == (uid_t)node.UserID();
	bool hasWriteAccess = inode->CheckPermissions(W_OK) == B_OK;

	Transaction transaction(volume, inode->BlockNumber());
	inode->WriteLockInTransaction(transaction);

	if ((mask & B_STAT_SIZE) != 0 && inode->Size() != stat->st_size) {
		// Since B_STAT_SIZE is the only thing that can fail directly, we
		// do it first, so that the inode state will still be consistent
		// with the on-disk version
		if (inode->IsDirectory())
			return B_IS_A_DIRECTORY;
		if (!inode->IsFile())
			return B_BAD_VALUE;
		if (!hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);

		off_t oldSize = inode->Size();

		status_t status = inode->SetFileSize(transaction, stat->st_size);
		if (status != B_OK)
			return status;

		// fill the new blocks (if any) with zeros
		if ((mask & B_STAT_SIZE_INSECURE) == 0) {
			// We must not keep the inode locked during a write operation,
			// or else we might deadlock.
			rw_lock_write_unlock(&inode->Lock());
			inode->FillGapWithZeros(oldSize, inode->Size());
			rw_lock_write_lock(&inode->Lock());
		}

		if (!inode->IsDeleted()) {
			Index index(volume);
			index.UpdateSize(transaction, inode);

			updateTime = true;
		}
	}

	if ((mask & B_STAT_UID) != 0) {
		// only root should be allowed
		if (uid != 0)
			RETURN_ERROR(B_NOT_ALLOWED);
		node.uid = HOST_ENDIAN_TO_BFS_INT32(stat->st_uid);
		updateTime = true;
	}

	if ((mask & B_STAT_GID) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			RETURN_ERROR(B_NOT_ALLOWED);
		node.gid = HOST_ENDIAN_TO_BFS_INT32(stat->st_gid);
		updateTime = true;
	}

	if ((mask & B_STAT_MODE) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			RETURN_ERROR(B_NOT_ALLOWED);
		PRINT(("original mode = %ld, stat->st_mode = %d\n", node.Mode(),
			stat->st_mode));
		node.mode = HOST_ENDIAN_TO_BFS_INT32((node.Mode() & ~S_IUMSK)
			| (stat->st_mode & S_IUMSK));
		updateTime = true;
	}

	if ((mask & B_STAT_CREATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);
		node.create_time
			= HOST_ENDIAN_TO_BFS_INT64(bfs_inode::ToInode(stat->st_crtim));
	}

	if ((mask & B_STAT_MODIFICATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);
		if (!inode->InLastModifiedIndex()) {
			// directory modification times are not part of the index
			node.last_modified_time
				= HOST_ENDIAN_TO_BFS_INT64(bfs_inode::ToInode(stat->st_mtim));
		} else if (!inode->IsDeleted()) {
			// Index::UpdateLastModified() will set the new time in the inode
			Index index(volume);
			index.UpdateLastModified(transaction, inode,
				bfs_inode::ToInode(stat->st_mtim));
		}
	}

	if ((mask & B_STAT_CHANGE_TIME) != 0 || updateTime) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			RETURN_ERROR(B_NOT_ALLOWED);
		bigtime_t newTime;
		if ((mask & B_STAT_CHANGE_TIME) == 0)
			newTime = bfs_inode::ToInode(real_time_clock_usecs());
		else
			newTime = bfs_inode::ToInode(stat->st_ctim);

		node.status_change_time = HOST_ENDIAN_TO_BFS_INT64(newTime);
	}

	status_t status = inode->WriteBack(transaction);
	if (status == B_OK)
		status = transaction.Done();
	if (status == B_OK)
		notify_stat_changed(volume->ID(), inode->ID(), mask);

	return status;
}


status_t
bfs_create(fs_volume* _volume, fs_vnode* _directory, const char* name,
	int openMode, int mode, void** _cookie, ino_t* _vnodeID)
{
	FUNCTION_START(("name = \"%s\", perms = %d, openMode = %d\n", name, mode,
		openMode));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	// We are creating the cookie at this point, so that we don't have
	// to remove the inode if we don't have enough free memory later...
	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	// initialize the cookie
	cookie->open_mode = openMode;
	cookie->last_size = 0;
	cookie->last_notification = system_time();

	Transaction transaction(volume, directory->BlockNumber());

	Inode* inode;
	bool created;
	status_t status = Inode::Create(transaction, directory, name,
		S_FILE | (mode & S_IUMSK), openMode, 0, &created, _vnodeID, &inode);

	// Disable the file cache, if requested?
	if (status == B_OK && (openMode & O_NOCACHE) != 0
		&& inode->FileCache() != NULL) {
		status = file_cache_disable(inode->FileCache());
	}

	entry_cache_add(volume->ID(), directory->ID(), name, *_vnodeID);

	if (status == B_OK)
		status = transaction.Done();

	if (status == B_OK) {
		// register the cookie
		*_cookie = cookie;

		if (created) {
			notify_entry_created(volume->ID(), directory->ID(), name,
				*_vnodeID);
		}
	} else {
		entry_cache_remove(volume->ID(), directory->ID(), name);
		delete cookie;
	}

	return status;
}


static status_t
bfs_create_symlink(fs_volume* _volume, fs_vnode* _directory, const char* name,
	const char* path, int mode)
{
	FUNCTION_START(("name = \"%s\", path = \"%s\"\n", name, path));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	Transaction transaction(volume, directory->BlockNumber());

	Inode* link;
	off_t id;
	status = Inode::Create(transaction, directory, name, S_SYMLINK | 0777,
		0, 0, NULL, &id, &link);
	if (status < B_OK)
		RETURN_ERROR(status);

	size_t length = strlen(path);
	if (length < SHORT_SYMLINK_NAME_LENGTH) {
		strcpy(link->Node().short_symlink, path);
	} else {
		link->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_LONG_SYMLINK
			| INODE_LOGGED);

		// links usually don't have a file cache attached - but we now need one
		link->SetFileCache(file_cache_create(volume->ID(), link->ID(), 0));
		link->SetMap(file_map_create(volume->ID(), link->ID(), 0));

		// The following call will have to write the inode back, so
		// we don't have to do that here...
		status = link->WriteAt(transaction, 0, (const uint8*)path, &length);
	}

	if (status == B_OK)
		status = link->WriteBack(transaction);

	// Inode::Create() left the inode locked in memory, and also doesn't
	// publish links
	publish_vnode(volume->FSVolume(), id, link, &gBFSVnodeOps, link->Mode(), 0);
	put_vnode(volume->FSVolume(), id);

	if (status == B_OK) {
		entry_cache_add(volume->ID(), directory->ID(), name, id);

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_created(volume->ID(), directory->ID(), name, id);
		else
			entry_cache_remove(volume->ID(), directory->ID(), name);
	}

	return status;
}


status_t
bfs_link(fs_volume* _volume, fs_vnode* dir, const char* name, fs_vnode* node)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	// This one won't be implemented in a binary compatible BFS
	return B_NOT_SUPPORTED;
}


status_t
bfs_unlink(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (!strcmp(name, "..") || !strcmp(name, "."))
		return B_NOT_ALLOWED;

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	status = directory->Remove(transaction, name, &id);
	if (status == B_OK) {
		entry_cache_remove(volume->ID(), directory->ID(), name);

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_removed(volume->ID(), directory->ID(), name, id);
		else
			entry_cache_add(volume->ID(), directory->ID(), name, id);
	}
	return status;
}


status_t
bfs_rename(fs_volume* _volume, fs_vnode* _oldDir, const char* oldName,
	fs_vnode* _newDir, const char* newName)
{
	FUNCTION_START(("oldDir = %p, oldName = \"%s\", newDir = %p, newName = "
		"\"%s\"\n", _oldDir, oldName, _newDir, newName));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* oldDirectory = (Inode*)_oldDir->private_node;
	Inode* newDirectory = (Inode*)_newDir->private_node;

	// are we already done?
	if (oldDirectory == newDirectory && !strcmp(oldName, newName))
		return B_OK;

	Transaction transaction(volume, oldDirectory->BlockNumber());

	oldDirectory->WriteLockInTransaction(transaction);
	if (oldDirectory != newDirectory)
		newDirectory->WriteLockInTransaction(transaction);

	// are we allowed to do what we've been told?
	status_t status = oldDirectory->CheckPermissions(W_OK);
	if (status == B_OK)
		status = newDirectory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	// Get the directory's tree, and a pointer to the inode which should be
	// changed
	BPlusTree* tree = oldDirectory->Tree();
	if (tree == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	off_t id;
	status = tree->Find((const uint8*)oldName, strlen(oldName), &id);
	if (status != B_OK)
		RETURN_ERROR(status);

	Vnode vnode(volume, id);
	Inode* inode;
	if (vnode.Get(&inode) != B_OK)
		return B_IO_ERROR;

	// Don't move a directory into one of its children - we soar up
	// from the newDirectory to either the root node or the old
	// directory, whichever comes first.
	// If we meet our inode on that way, we have to bail out.

	if (oldDirectory != newDirectory) {
		ino_t parent = newDirectory->ID();
		ino_t root = volume->RootNode()->ID();

		while (true) {
			if (parent == id)
				return B_BAD_VALUE;
			else if (parent == root || parent == oldDirectory->ID())
				break;

			Vnode vnode(volume, parent);
			Inode* parentNode;
			if (vnode.Get(&parentNode) != B_OK)
				return B_ERROR;

			parent = volume->ToVnode(parentNode->Parent());
		}
	}

	// Everything okay? Then lets get to work...

	// First, try to make sure there is nothing that will stop us in
	// the target directory - since this is the only non-critical
	// failure, we will test this case first
	BPlusTree* newTree = tree;
	if (newDirectory != oldDirectory) {
		newTree = newDirectory->Tree();
		if (newTree == NULL)
			RETURN_ERROR(B_BAD_VALUE);
	}

	status = newTree->Insert(transaction, (const uint8*)newName,
		strlen(newName), id);
	if (status == B_NAME_IN_USE) {
		// If there is already a file with that name, we have to remove
		// it, as long it's not a directory with files in it
		off_t clobber;
		if (newTree->Find((const uint8*)newName, strlen(newName), &clobber)
				< B_OK)
			return B_NAME_IN_USE;
		if (clobber == id)
			return B_BAD_VALUE;

		Vnode vnode(volume, clobber);
		Inode* other;
		if (vnode.Get(&other) < B_OK)
			return B_NAME_IN_USE;

		// only allowed, if either both nodes are directories or neither is
		if (inode->IsDirectory() != other->IsDirectory())
			return other->IsDirectory() ? B_IS_A_DIRECTORY : B_NOT_A_DIRECTORY;

		status = newDirectory->Remove(transaction, newName, NULL,
			other->IsDirectory());
		if (status < B_OK)
			return status;

		entry_cache_remove(volume->ID(), newDirectory->ID(), newName);

		notify_entry_removed(volume->ID(), newDirectory->ID(), newName,
			clobber);

		status = newTree->Insert(transaction, (const uint8*)newName,
			strlen(newName), id);
	}
	if (status != B_OK)
		return status;

	inode->WriteLockInTransaction(transaction);

	volume->UpdateLiveQueriesRenameMove(inode, oldDirectory->ID(), oldName,
		newDirectory->ID(), newName);

	// update the name only when they differ
	bool nameUpdated = false;
	if (strcmp(oldName, newName)) {
		status = inode->SetName(transaction, newName);
		if (status == B_OK) {
			Index index(volume);
			index.UpdateName(transaction, oldName, newName, inode);
			nameUpdated = true;
		}
	}

	if (status == B_OK) {
		status = tree->Remove(transaction, (const uint8*)oldName,
			strlen(oldName), id);
		if (status == B_OK) {
			inode->Parent() = newDirectory->BlockRun();

			// if it's a directory, update the parent directory pointer
			// in its tree if necessary
			BPlusTree* movedTree = inode->Tree();
			if (oldDirectory != newDirectory
				&& inode->IsDirectory()
				&& movedTree != NULL) {
				status = movedTree->Replace(transaction, (const uint8*)"..",
					2, newDirectory->ID());

				if (status == B_OK) {
					// update/add the cache entry for the parent
					entry_cache_add(volume->ID(), id, "..", newDirectory->ID());
				}
			}

			if (status == B_OK && newDirectory != oldDirectory)
				status = oldDirectory->ContainerContentsChanged(transaction);
			if (status == B_OK)
				status = newDirectory->ContainerContentsChanged(transaction);

			if (status == B_OK)
				status = inode->WriteBack(transaction);

			if (status == B_OK) {
				entry_cache_remove(volume->ID(), oldDirectory->ID(), oldName);
				entry_cache_add(volume->ID(), newDirectory->ID(), newName, id);

				status = transaction.Done();
				if (status == B_OK) {
					notify_entry_moved(volume->ID(), oldDirectory->ID(),
						oldName, newDirectory->ID(), newName, id);
					return B_OK;
				}

				entry_cache_remove(volume->ID(), newDirectory->ID(), newName);
				entry_cache_add(volume->ID(), oldDirectory->ID(), oldName, id);
			}
		}
	}

	return status;
}


static status_t
bfs_open(fs_volume* _volume, fs_vnode* _node, int openMode, void** _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	// Opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && (openMode & O_RWMASK) != O_RDONLY)
		return B_IS_A_DIRECTORY;
	if ((openMode & O_DIRECTORY) != 0 && !inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	status_t status = inode->CheckPermissions(open_mode_to_access(openMode)
		| ((openMode & O_TRUNC) != 0 ? W_OK : 0));
	if (status != B_OK)
		RETURN_ERROR(status);

	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY);
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	// initialize the cookie
	cookie->open_mode = openMode & BFS_OPEN_MODE_USER_MASK;
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	// Disable the file cache, if requested?
	CObjectDeleter<void> fileCacheEnabler(file_cache_enable);
	if ((openMode & O_NOCACHE) != 0 && inode->FileCache() != NULL) {
		status = file_cache_disable(inode->FileCache());
		if (status != B_OK)
			return status;
		fileCacheEnabler.SetTo(inode->FileCache());
	}

	// Should we truncate the file?
	if ((openMode & O_TRUNC) != 0) {
		if ((openMode & O_RWMASK) == O_RDONLY)
			return B_NOT_ALLOWED;

		Transaction transaction(volume, inode->BlockNumber());
		inode->WriteLockInTransaction(transaction);

		status_t status = inode->SetFileSize(transaction, 0);
		if (status == B_OK)
			status = inode->WriteBack(transaction);
		if (status == B_OK)
			status = transaction.Done();
		if (status != B_OK)
			return status;
	}

	fileCacheEnabler.Detach();
	cookieDeleter.Detach();
	*_cookie = cookie;
	return B_OK;
}


static status_t
bfs_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	//FUNCTION();
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->HasUserAccessableStream()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8*)buffer, _length);
}


static status_t
bfs_write(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	const void* buffer, size_t* _length)
{
	//FUNCTION();
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!inode->HasUserAccessableStream()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	file_cookie* cookie = (file_cookie*)_cookie;

	if (cookie->open_mode & O_APPEND)
		pos = inode->Size();

	Transaction transaction;
		// We are not starting the transaction here, since
		// it might not be needed at all (the contents of
		// regular files aren't logged)

	status_t status = inode->WriteAt(transaction, pos, (const uint8*)buffer,
		_length);
	if (status == B_OK)
		status = transaction.Done();
	if (status == B_OK) {
		InodeReadLocker locker(inode);

		// periodically notify if the file size has changed
		// TODO: should we better test for a change in the last_modified time only?
		if (!inode->IsDeleted() && cookie->last_size != inode->Size()
			&& system_time() > cookie->last_notification
					+ INODE_NOTIFICATION_INTERVAL) {
			notify_stat_changed(volume->ID(), inode->ID(),
				B_STAT_MODIFICATION_TIME | B_STAT_SIZE | B_STAT_INTERIM_UPDATE);
			cookie->last_size = inode->Size();
			cookie->last_notification = system_time();
		}
	}

	return status;
}


static status_t
bfs_close(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	FUNCTION();

	file_cookie* cookie = (file_cookie*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	Transaction transaction;
	bool needsTrimming = false;

	if (!volume->IsReadOnly() && !volume->IsCheckingThread()) {
		InodeReadLocker locker(inode);
		needsTrimming = inode->NeedsTrimming();

		if ((cookie->open_mode & O_RWMASK) != 0
			&& !inode->IsDeleted()
			&& (needsTrimming
				|| inode->OldLastModified() != inode->LastModified()
				|| (inode->InSizeIndex()
					// TODO: this can prevent the size update notification
					// for nodes not in the index!
					&& inode->OldSize() != inode->Size()))) {
			locker.Unlock();
			transaction.Start(volume, inode->BlockNumber());
		}
	}

	status_t status = transaction.IsStarted() ? B_OK : B_ERROR;

	if (status == B_OK) {
		inode->WriteLockInTransaction(transaction);

		// trim the preallocated blocks and update the size,
		// and last_modified indices if needed
		bool changedSize = false, changedTime = false;
		Index index(volume);

		if (needsTrimming) {
			status = inode->TrimPreallocation(transaction);
			if (status < B_OK) {
				FATAL(("Could not trim preallocated blocks: inode %" B_PRIdINO
					", transaction %d: %s!\n", inode->ID(),
					(int)transaction.ID(), strerror(status)));

				// we still want this transaction to succeed
				status = B_OK;
			}
		}
		if (inode->OldSize() != inode->Size()) {
			if (inode->InSizeIndex())
				index.UpdateSize(transaction, inode);
			changedSize = true;
		}
		if (inode->OldLastModified() != inode->LastModified()) {
			if (inode->InLastModifiedIndex()) {
				index.UpdateLastModified(transaction, inode,
					inode->LastModified());
			}
			changedTime = true;

			// updating the index doesn't write back the inode
			inode->WriteBack(transaction);
		}

		if (changedSize || changedTime) {
			notify_stat_changed(volume->ID(), inode->ID(),
				(changedTime ? B_STAT_MODIFICATION_TIME : 0)
				| (changedSize ? B_STAT_SIZE : 0));
		}
	}
	if (status == B_OK)
		transaction.Done();

	if ((cookie->open_mode & BFS_OPEN_MODE_CHECKING) != 0) {
		// "chkbfs" exited abnormally, so we have to stop it here...
		FATAL(("check process was aborted!\n"));
		volume->Allocator().StopChecking(NULL);
	}

	if ((cookie->open_mode & O_NOCACHE) != 0 && inode->FileCache() != NULL)
		file_cache_enable(inode->FileCache());

	delete cookie;
	return B_OK;
}


/*!	Checks access permissions, return B_NOT_ALLOWED if the action
	is not allowed.
*/
static status_t
bfs_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	//FUNCTION();

	Inode* inode = (Inode*)_node->private_node;
	status_t status = inode->CheckPermissions(accessMode);
	if (status < B_OK)
		RETURN_ERROR(status);

	return B_OK;
}


static status_t
bfs_read_link(fs_volume* _volume, fs_vnode* _node, char* buffer,
	size_t* _bufferSize)
{
	FUNCTION();

	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsSymLink())
		RETURN_ERROR(B_BAD_VALUE);

	if ((inode->Flags() & INODE_LONG_SYMLINK) != 0) {
		if ((uint64)inode->Size() < (uint64)*_bufferSize)
			*_bufferSize = inode->Size();

		status_t status = inode->ReadAt(0, (uint8*)buffer, _bufferSize);
		if (status < B_OK)
			RETURN_ERROR(status);

		return B_OK;
	}

	size_t linkLen = strlen(inode->Node().short_symlink);
	if (linkLen < *_bufferSize)
		*_bufferSize = linkLen;

	return user_memcpy(buffer, inode->Node().short_symlink, *_bufferSize);
}


//	#pragma mark - Directory functions


static status_t
bfs_create_dir(fs_volume* _volume, fs_vnode* _directory, const char* name,
	int mode)
{
	FUNCTION_START(("name = \"%s\", perms = %d\n", name, mode));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	Transaction transaction(volume, directory->BlockNumber());

	// Inode::Create() locks the inode if we pass the "id" parameter, but we
	// need it anyway
	off_t id;
	status = Inode::Create(transaction, directory, name,
		S_DIRECTORY | (mode & S_IUMSK), 0, 0, NULL, &id);
	if (status == B_OK) {
		put_vnode(volume->FSVolume(), id);

		entry_cache_add(volume->ID(), directory->ID(), name, id);

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_created(volume->ID(), directory->ID(), name, id);
		else
			entry_cache_remove(volume->ID(), directory->ID(), name);
	}

	return status;
}


static status_t
bfs_remove_dir(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	status_t status = directory->Remove(transaction, name, &id, true);
	if (status == B_OK) {
		// Remove the cache entry for the directory and potentially also
		// the parent entry still belonging to the directory
		entry_cache_remove(volume->ID(), directory->ID(), name);
		entry_cache_remove(volume->ID(), id, "..");

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_removed(volume->ID(), directory->ID(), name, id);
		else {
			entry_cache_add(volume->ID(), directory->ID(), name, id);
			entry_cache_add(volume->ID(), id, "..", id);
		}
	}

	return status;
}


/*!	Opens a directory ready to be traversed.
	bfs_open_dir() is also used by bfs_open_index_dir().
*/
static status_t
bfs_open_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	FUNCTION();

	Inode* inode = (Inode*)_node->private_node;
	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	// we don't ask here for directories only, because the bfs_open_index_dir()
	// function utilizes us (so we must be able to open indices as well)
	if (!inode->IsContainer())
		RETURN_ERROR(B_NOT_A_DIRECTORY);

	BPlusTree* tree = inode->Tree();
	if (tree == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	TreeIterator* iterator = new(std::nothrow) TreeIterator(tree);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = iterator;
	return B_OK;
}


static status_t
bfs_read_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	struct dirent* dirent, size_t bufferSize, uint32* _num)
{
	FUNCTION();

	TreeIterator* iterator = (TreeIterator*)_cookie;

	uint16 length;
	ino_t id;
	status_t status = iterator->GetNextEntry(dirent->d_name, &length,
		bufferSize, &id);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK)
		RETURN_ERROR(status);

	Volume* volume = (Volume*)_volume->private_volume;

	dirent->d_dev = volume->ID();
	dirent->d_ino = id;

	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	return B_OK;
}


/*!	Sets the TreeIterator back to the beginning of the directory. */
static status_t
bfs_rewind_dir(fs_volume* /*_volume*/, fs_vnode* /*node*/, void* _cookie)
{
	FUNCTION();
	TreeIterator* iterator = (TreeIterator*)_cookie;

	return iterator->Rewind();
}


static status_t
bfs_close_dir(fs_volume* /*_volume*/, fs_vnode* /*node*/, void* /*_cookie*/)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_dir_cookie(fs_volume* _volume, fs_vnode* node, void* _cookie)
{
	delete (TreeIterator*)_cookie;
	return B_OK;
}


//	#pragma mark - Attribute functions


static status_t
bfs_open_attr_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;

	FUNCTION();

	AttributeIterator* iterator = new(std::nothrow) AttributeIterator(inode);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = iterator;
	return B_OK;
}


static status_t
bfs_close_attr_dir(fs_volume* _volume, fs_vnode* node, void* cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_attr_dir_cookie(fs_volume* _volume, fs_vnode* node, void* _cookie)
{
	FUNCTION();
	AttributeIterator* iterator = (AttributeIterator*)_cookie;

	delete iterator;
	return B_OK;
}


static status_t
bfs_rewind_attr_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	FUNCTION();

	AttributeIterator* iterator = (AttributeIterator*)_cookie;
	RETURN_ERROR(iterator->Rewind());
}


static status_t
bfs_read_attr_dir(fs_volume* _volume, fs_vnode* node, void* _cookie,
	struct dirent* dirent, size_t bufferSize, uint32* _num)
{
	FUNCTION();
	AttributeIterator* iterator = (AttributeIterator*)_cookie;

	uint32 type;
	size_t length;
	status_t status = iterator->GetNext(dirent->d_name, &length, &type,
		&dirent->d_ino);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK) {
		RETURN_ERROR(status);
	}

	Volume* volume = (Volume*)_volume->private_volume;

	dirent->d_dev = volume->ID();
	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	return B_OK;
}


static status_t
bfs_create_attr(fs_volume* _volume, fs_vnode* _node, const char* name,
	uint32 type, int openMode, void** _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;
	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	Inode* inode = (Inode*)_node->private_node;
	Attribute attribute(inode);

	return attribute.Create(name, type, openMode, (attr_cookie**)_cookie);
}


static status_t
bfs_open_attr(fs_volume* _volume, fs_vnode* _node, const char* name,
	int openMode, void** _cookie)
{
	FUNCTION();

	Inode* inode = (Inode*)_node->private_node;
	Attribute attribute(inode);

	return attribute.Open(name, openMode, (attr_cookie**)_cookie);
}


static status_t
bfs_close_attr(fs_volume* _volume, fs_vnode* _file, void* cookie)
{
	return B_OK;
}


static status_t
bfs_free_attr_cookie(fs_volume* _volume, fs_vnode* _file, void* cookie)
{
	delete (attr_cookie*)cookie;
	return B_OK;
}


static status_t
bfs_read_attr(fs_volume* _volume, fs_vnode* _file, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	FUNCTION();

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_file->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Read(cookie, pos, (uint8*)buffer, _length);
}


static status_t
bfs_write_attr(fs_volume* _volume, fs_vnode* _file, void* _cookie,
	off_t pos, const void* buffer, size_t* _length)
{
	FUNCTION();

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_file->private_node;

	Transaction transaction(volume, inode->BlockNumber());
	Attribute attribute(inode, cookie);

	bool created;
	status_t status = attribute.Write(transaction, cookie, pos,
		(const uint8*)buffer, _length, &created);
	if (status == B_OK) {
		status = transaction.Done();
		if (status == B_OK) {
			notify_attribute_changed(volume->ID(), inode->ID(), cookie->name,
				created ? B_ATTR_CREATED : B_ATTR_CHANGED);
			notify_stat_changed(volume->ID(), inode->ID(), B_STAT_CHANGE_TIME);
		}
	}

	return status;
}


static status_t
bfs_read_attr_stat(fs_volume* _volume, fs_vnode* _file, void* _cookie,
	struct stat* stat)
{
	FUNCTION();

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_file->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Stat(*stat);
}


static status_t
bfs_write_attr_stat(fs_volume* _volume, fs_vnode* file, void* cookie,
	const struct stat* stat, int statMask)
{
	// TODO: Implement (at least setting the size)!
	return EOPNOTSUPP;
}


static status_t
bfs_rename_attr(fs_volume* _volume, fs_vnode* fromFile, const char* fromName,
	fs_vnode* toFile, const char* toName)
{
	FUNCTION_START(("name = \"%s\", to = \"%s\"\n", fromName, toName));

	// TODO: implement bfs_rename_attr()!
	// There will probably be an API to move one attribute to another file,
	// making that function much more complicated - oh joy ;-)

	return EOPNOTSUPP;
}


static status_t
bfs_remove_attr(fs_volume* _volume, fs_vnode* _node, const char* name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	Transaction transaction(volume, inode->BlockNumber());

	status = inode->RemoveAttribute(transaction, name);
	if (status == B_OK)
		status = transaction.Done();
	if (status == B_OK) {
		notify_attribute_changed(volume->ID(), inode->ID(), name,
			B_ATTR_REMOVED);
	}

	return status;
}


//	#pragma mark - Special Nodes


status_t
bfs_create_special_node(fs_volume* _volume, fs_vnode* _directory,
	const char* name, fs_vnode* subVnode, mode_t mode, uint32 flags,
	fs_vnode* _superVnode, ino_t* _nodeID)
{
	// no need to support entry-less nodes
	if (name == NULL)
		return B_UNSUPPORTED;

	FUNCTION_START(("name = \"%s\", mode = %d, flags = 0x%lx, subVnode: %p\n",
		name, mode, flags, subVnode));

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	Inode* inode;
	status = Inode::Create(transaction, directory, name, mode, O_EXCL, 0, NULL,
		&id, &inode, subVnode ? subVnode->ops : NULL, flags);
	if (status == B_OK) {
		_superVnode->private_node = inode;
		_superVnode->ops = &gBFSVnodeOps;
		*_nodeID = id;

		entry_cache_add(volume->ID(), directory->ID(), name, id);

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_created(volume->ID(), directory->ID(), name, id);
		else
			entry_cache_remove(volume->ID(), directory->ID(), name);
	}

	return status;
}


//	#pragma mark - Index functions


static status_t
bfs_open_index_dir(fs_volume* _volume, void** _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	if (volume->IndicesNode() == NULL) {
		// This volume does not have any indices
		RETURN_ERROR(B_ENTRY_NOT_FOUND);
	}

	// Since the indices root node is just a directory, and we are storing
	// a pointer to it in our Volume object, we can just use the directory
	// traversal functions.
	// In fact we're storing it in the Volume object for that reason.

	fs_vnode indicesNode;
	indicesNode.private_node = volume->IndicesNode();

	RETURN_ERROR(bfs_open_dir(_volume, &indicesNode, _cookie));
}


static status_t
bfs_close_index_dir(fs_volume* _volume, void* _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	fs_vnode indicesNode;
	indicesNode.private_node = volume->IndicesNode();

	RETURN_ERROR(bfs_close_dir(_volume, &indicesNode, _cookie));
}


static status_t
bfs_free_index_dir_cookie(fs_volume* _volume, void* _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	fs_vnode indicesNode;
	indicesNode.private_node = volume->IndicesNode();

	RETURN_ERROR(bfs_free_dir_cookie(_volume, &indicesNode, _cookie));
}


static status_t
bfs_rewind_index_dir(fs_volume* _volume, void* _cookie)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	fs_vnode indicesNode;
	indicesNode.private_node = volume->IndicesNode();

	RETURN_ERROR(bfs_rewind_dir(_volume, &indicesNode, _cookie));
}


static status_t
bfs_read_index_dir(fs_volume* _volume, void* _cookie, struct dirent* dirent,
	size_t bufferSize, uint32* _num)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	fs_vnode indicesNode;
	indicesNode.private_node = volume->IndicesNode();

	RETURN_ERROR(bfs_read_dir(_volume, &indicesNode, _cookie, dirent,
		bufferSize, _num));
}


static status_t
bfs_create_index(fs_volume* _volume, const char* name, uint32 type,
	uint32 flags)
{
	FUNCTION_START(("name = \"%s\", type = %ld, flags = %ld\n", name, type, flags));

	Volume* volume = (Volume*)_volume->private_volume;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to create indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	Transaction transaction(volume, volume->Indices());

	Index index(volume);
	status_t status = index.Create(transaction, name, type);

	if (status == B_OK)
		status = transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_remove_index(fs_volume* _volume, const char* name)
{
	FUNCTION();

	Volume* volume = (Volume*)_volume->private_volume;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to remove indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	Inode* indices = volume->IndicesNode();
	if (indices == NULL)
		return B_ENTRY_NOT_FOUND;

	Transaction transaction(volume, volume->Indices());

	status_t status = indices->Remove(transaction, name);
	if (status == B_OK)
		status = transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_stat_index(fs_volume* _volume, const char* name, struct stat* stat)
{
	FUNCTION_START(("name = %s\n", name));

	Volume* volume = (Volume*)_volume->private_volume;

	Index index(volume);
	status_t status = index.SetTo(name);
	if (status < B_OK)
		RETURN_ERROR(status);

	bfs_inode& node = index.Node()->Node();

	stat->st_type = index.Type();
	stat->st_mode = node.Mode();

	stat->st_size = node.data.Size();
	stat->st_blocks = index.Node()->AllocatedSize() / 512;

	stat->st_nlink = 1;
	stat->st_blksize = 65536;

	stat->st_uid = node.UserID();
	stat->st_gid = node.GroupID();

	fill_stat_time(node, *stat);

	return B_OK;
}


//	#pragma mark - Query functions


static status_t
bfs_open_query(fs_volume* _volume, const char* queryString, uint32 flags,
	port_id port, uint32 token, void** _cookie)
{
	FUNCTION_START(("bfs_open_query(\"%s\", flags = %lu, port_id = %ld, token = %ld)\n",
		queryString, flags, port, token));

	Volume* volume = (Volume*)_volume->private_volume;

	Expression* expression = new(std::nothrow) Expression((char*)queryString);
	if (expression == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	if (expression->InitCheck() < B_OK) {
		INFORM(("Could not parse query \"%s\", stopped at: \"%s\"\n",
			queryString, expression->Position()));

		delete expression;
		RETURN_ERROR(B_BAD_VALUE);
	}

	Query* query = new(std::nothrow) Query(volume, expression, flags);
	if (query == NULL) {
		delete expression;
		RETURN_ERROR(B_NO_MEMORY);
	}

	if (flags & B_LIVE_QUERY)
		query->SetLiveMode(port, token);

	*_cookie = (void*)query;

	return B_OK;
}


static status_t
bfs_close_query(fs_volume* _volume, void* cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_query_cookie(fs_volume* _volume, void* cookie)
{
	FUNCTION();

	Query* query = (Query*)cookie;
	Expression* expression = query->GetExpression();
	delete query;
	delete expression;

	return B_OK;
}


static status_t
bfs_read_query(fs_volume* /*_volume*/, void* cookie, struct dirent* dirent,
	size_t bufferSize, uint32* _num)
{
	FUNCTION();
	Query* query = (Query*)cookie;
	status_t status = query->GetNextEntry(dirent, bufferSize);
	if (status == B_OK)
		*_num = 1;
	else if (status == B_ENTRY_NOT_FOUND)
		*_num = 0;
	else
		return status;

	return B_OK;
}


static status_t
bfs_rewind_query(fs_volume* /*_volume*/, void* cookie)
{
	FUNCTION();

	Query* query = (Query*)cookie;
	return query->Rewind();
}


//	#pragma mark -


static uint32
bfs_get_supported_operations(partition_data* partition, uint32 mask)
{
	// TODO: We should at least check the partition size.
	return B_DISK_SYSTEM_SUPPORTS_INITIALIZING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
		| B_DISK_SYSTEM_SUPPORTS_WRITING;
}


static status_t
bfs_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameterString, off_t /*partitionSize*/, disk_job_id job)
{
	// check name
	status_t status = check_volume_name(name);
	if (status != B_OK)
		return status;

	// parse parameters
	initialize_parameters parameters;
	status = parse_initialize_parameters(parameterString, parameters);
	if (status != B_OK)
		return status;

	update_disk_device_job_progress(job, 0);

	// initialize the volume
	Volume volume(NULL);
	status = volume.Initialize(fd, name, parameters.blockSize,
		parameters.flags);
	if (status < B_OK) {
		INFORM(("Initializing volume failed: %s\n", strerror(status)));
		return status;
	}

	// rescan partition
	status = scan_partition(partitionID);
	if (status != B_OK)
		return status;

	update_disk_device_job_progress(job, 1);

	// print some info, if desired
	if (parameters.verbose) {
		disk_super_block super = volume.SuperBlock();

		INFORM(("Disk was initialized successfully.\n"));
		INFORM(("\tname: \"%s\"\n", super.name));
		INFORM(("\tnum blocks: %" B_PRIdOFF "\n", super.NumBlocks()));
		INFORM(("\tused blocks: %" B_PRIdOFF "\n", super.UsedBlocks()));
		INFORM(("\tblock size: %u bytes\n", (unsigned)super.BlockSize()));
		INFORM(("\tnum allocation groups: %d\n",
			(int)super.AllocationGroups()));
		INFORM(("\tallocation group size: %ld blocks\n",
			1L << super.AllocationGroupShift()));
		INFORM(("\tlog size: %u blocks\n", super.log_blocks.Length()));
	}

	return B_OK;
}


static status_t
bfs_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
	uint32 blockSize, disk_job_id job)
{
	if (blockSize == 0)
		return B_BAD_VALUE;

	update_disk_device_job_progress(job, 0.0);

	// just overwrite the superblock
	disk_super_block superBlock;
	memset(&superBlock, 0, sizeof(superBlock));

	if (write_pos(fd, 512, &superBlock, sizeof(superBlock)) < 0)
		return errno;

	update_disk_device_job_progress(job, 1.0);

	return B_OK;
}


//	#pragma mark -


static status_t
bfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
#ifdef BFS_DEBUGGER_COMMANDS
			add_debugger_commands();
#endif
			return B_OK;
		case B_MODULE_UNINIT:
#ifdef BFS_DEBUGGER_COMMANDS
			remove_debugger_commands();
#endif
			return B_OK;

		default:
			return B_ERROR;
	}
}

fs_volume_ops gBFSVolumeOps = {
	&bfs_unmount,
	&bfs_read_fs_stat,
	&bfs_write_fs_stat,
	&bfs_sync,
	&bfs_get_vnode,

	/* index directory & index operations */
	&bfs_open_index_dir,
	&bfs_close_index_dir,
	&bfs_free_index_dir_cookie,
	&bfs_read_index_dir,
	&bfs_rewind_index_dir,

	&bfs_create_index,
	&bfs_remove_index,
	&bfs_stat_index,

	/* query operations */
	&bfs_open_query,
	&bfs_close_query,
	&bfs_free_query_cookie,
	&bfs_read_query,
	&bfs_rewind_query,
};

fs_vnode_ops gBFSVnodeOps = {
	/* vnode operations */
	&bfs_lookup,
	&bfs_get_vnode_name,
	&bfs_put_vnode,
	&bfs_remove_vnode,

	/* VM file access */
	&bfs_can_page,
	&bfs_read_pages,
	&bfs_write_pages,

	&bfs_io,
	NULL,	// cancel_io()

	&bfs_get_file_map,

	&bfs_ioctl,
	&bfs_set_flags,
	NULL,	// fs_select
	NULL,	// fs_deselect
	&bfs_fsync,

	&bfs_read_link,
	&bfs_create_symlink,

	&bfs_link,
	&bfs_unlink,
	&bfs_rename,

	&bfs_access,
	&bfs_read_stat,
	&bfs_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	&bfs_create,
	&bfs_open,
	&bfs_close,
	&bfs_free_cookie,
	&bfs_read,
	&bfs_write,

	/* directory operations */
	&bfs_create_dir,
	&bfs_remove_dir,
	&bfs_open_dir,
	&bfs_close_dir,
	&bfs_free_dir_cookie,
	&bfs_read_dir,
	&bfs_rewind_dir,

	/* attribute directory operations */
	&bfs_open_attr_dir,
	&bfs_close_attr_dir,
	&bfs_free_attr_dir_cookie,
	&bfs_read_attr_dir,
	&bfs_rewind_attr_dir,

	/* attribute operations */
	&bfs_create_attr,
	&bfs_open_attr,
	&bfs_close_attr,
	&bfs_free_attr_cookie,
	&bfs_read_attr,
	&bfs_write_attr,

	&bfs_read_attr_stat,
	&bfs_write_attr_stat,
	&bfs_rename_attr,
	&bfs_remove_attr,

	/* special nodes */
	&bfs_create_special_node
};

static file_system_module_info sBeFileSystem = {
	{
		"file_systems/bfs" B_CURRENT_FS_API_VERSION,
		0,
		bfs_std_ops,
	},

	"bfs",						// short_name
	"Be File System",			// pretty_name

	// DDM flags
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
//	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
	| B_DISK_SYSTEM_SUPPORTS_WRITING
	,

	// scanning
	bfs_identify_partition,
	bfs_scan_partition,
	bfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&bfs_mount,

	/* capability querying operations */
	&bfs_get_supported_operations,

	NULL,	// validate_resize
	NULL,	// validate_move
	NULL,	// validate_set_content_name
	NULL,	// validate_set_content_parameters
	NULL,	// validate_initialize,

	/* shadow partition modification */
	NULL,	// shadow_changed

	/* writing */
	NULL,	// defragment
	NULL,	// repair
	NULL,	// resize
	NULL,	// move
	NULL,	// set_content_name
	NULL,	// set_content_parameters
	bfs_initialize,
	bfs_uninitialize
};

module_info* modules[] = {
	(module_info*)&sBeFileSystem,
	NULL,
};
