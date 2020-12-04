/*
 * Copyright 2010, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <algorithm>
#include <dirent.h>
#include <sys/ioctl.h>
#include <util/kernel_cpp.h>
#include <string.h>

#include <AutoDeleter.h>
#include <fs_cache.h>
#include <fs_info.h>
#include <io_requests.h>
#include <NodeMonitor.h>
#include <util/AutoLock.h>

#include "Attribute.h"
#include "CachedBlock.h"
#include "DirectoryIterator.h"
#include "ext2.h"
#include "HTree.h"
#include "Inode.h"
#include "Journal.h"
#include "Utility.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


#define EXT2_IO_SIZE	65536


struct identify_cookie {
	ext2_super_block super_block;
};


//!	ext2_io() callback hook
static status_t
iterative_io_get_vecs_hook(void* cookie, io_request* request, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	Inode* inode = (Inode*)cookie;

	return file_map_translate(inode->Map(), offset, size, vecs, _count,
		inode->GetVolume()->BlockSize());
}


//!	ext2_io() callback hook
static status_t
iterative_io_finished_hook(void* cookie, io_request* request, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	Inode* inode = (Inode*)cookie;
	rw_lock_read_unlock(inode->Lock());
	return B_OK;
}


//	#pragma mark - Scanning


static float
ext2_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	STATIC_ASSERT(sizeof(struct ext2_super_block) == 1024);
	STATIC_ASSERT(sizeof(struct ext2_block_group) == 64);

	ext2_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return -1;

	identify_cookie *cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(ext2_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
ext2_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks(
		(cookie->super_block.CompatibleFeatures()
			& EXT2_INCOMPATIBLE_FEATURE_64BIT) != 0)
			<< cookie->super_block.BlockShift();
	partition->block_size = 1UL << cookie->super_block.BlockShift();
	partition->content_name = strdup(cookie->super_block.name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
ext2_free_identify_partition_cookie(partition_data* partition, void* _cookie)
{
	delete (identify_cookie*)_cookie;
}


//	#pragma mark -


static status_t
ext2_mount(fs_volume* _volume, const char* device, uint32 flags,
	const char* args, ino_t* _rootID)
{
	Volume* volume = new(std::nothrow) Volume(_volume);
	if (volume == NULL)
		return B_NO_MEMORY;

	// TODO: this is a bit hacky: we can't use publish_vnode() to publish
	// the root node, or else its file cache cannot be created (we could
	// create it later, though). Therefore we're using get_vnode() in Mount(),
	// but that requires us to export our volume data before calling it.
	_volume->private_volume = volume;
	_volume->ops = &gExt2VolumeOps;

	status_t status = volume->Mount(device, flags);
	if (status != B_OK) {
		ERROR("Failed mounting the volume. Error: %s\n", strerror(status));
		delete volume;
		return status;
	}

	*_rootID = volume->RootNode()->ID();
	return B_OK;
}


static status_t
ext2_unmount(fs_volume *_volume)
{
	Volume* volume = (Volume *)_volume->private_volume;

	status_t status = volume->Unmount();
	delete volume;

	return status;
}


static status_t
ext2_read_fs_info(fs_volume* _volume, struct fs_info* info)
{
	Volume* volume = (Volume*)_volume->private_volume;

	// File system flags
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR
		| (volume->IsReadOnly() ? B_FS_IS_READONLY : 0);
	info->io_size = EXT2_IO_SIZE;
	info->block_size = volume->BlockSize();
	info->total_blocks = volume->NumBlocks();
	info->free_blocks = volume->NumFreeBlocks();

	// Volume name
	strlcpy(info->volume_name, volume->Name(), sizeof(info->volume_name));

	// File system name
	if (volume->HasExtentsFeature())
		strlcpy(info->fsh_name, "ext4", sizeof(info->fsh_name));
	else if (volume->HasJournalFeature())
		strlcpy(info->fsh_name, "ext3", sizeof(info->fsh_name));
	else
		strlcpy(info->fsh_name, "ext2", sizeof(info->fsh_name));

	return B_OK;
}


static status_t
ext2_write_fs_info(fs_volume* _volume, const struct fs_info* info, uint32 mask)
{
	Volume* volume = (Volume*)_volume->private_volume;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	MutexLocker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & FS_WRITE_FSINFO_NAME) {
		Transaction transaction(volume->GetJournal());
		volume->SetName(info->volume_name);
		status = volume->WriteSuperBlock(transaction);
		transaction.Done();
	}
	return status;
}


static status_t
ext2_sync(fs_volume* _volume)
{
	Volume* volume = (Volume*)_volume->private_volume;
	return volume->Sync();
}


//	#pragma mark -


static status_t
ext2_get_vnode(fs_volume* _volume, ino_t id, fs_vnode* _node, int* _type,
	uint32* _flags, bool reenter)
{
	Volume* volume = (Volume*)_volume->private_volume;

	if (id < 2 || id > volume->NumInodes()) {
		ERROR("invalid inode id %" B_PRIdINO " requested!\n", id);
		return B_BAD_VALUE;
	}

	Inode* inode = new(std::nothrow) Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status_t status = inode->InitCheck();
	if (status != B_OK)
		delete inode;

	if (status == B_OK) {
		_node->private_node = inode;
		_node->ops = &gExt2VnodeOps;
		*_type = inode->Mode();
		*_flags = 0;
	} else
		ERROR("get_vnode: InitCheck() failed. Error: %s\n", strerror(status));

	return status;
}


static status_t
ext2_put_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	delete (Inode*)_node->private_node;
	return B_OK;
}


static status_t
ext2_remove_vnode(fs_volume* _volume, fs_vnode* _node, bool reenter)
{
	TRACE("ext2_remove_vnode()\n");
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	ObjectDeleter<Inode> inodeDeleter(inode);

	if (!inode->IsDeleted())
		return B_OK;

	TRACE("ext2_remove_vnode(): Starting transaction\n");
	Transaction transaction(volume->GetJournal());

	if (!inode->IsSymLink() || inode->Size() >= EXT2_SHORT_SYMLINK_LENGTH) {
		TRACE("ext2_remove_vnode(): Truncating\n");
		status_t status = inode->Resize(transaction, 0);
		if (status != B_OK)
			return status;
	}

	TRACE("ext2_remove_vnode(): Removing from orphan list\n");
	status_t status = volume->RemoveOrphan(transaction, inode->ID());
	if (status != B_OK)
		return status;

	TRACE("ext2_remove_vnode(): Setting deletion time\n");
	inode->Node().SetDeletionTime(real_time_clock());

	status = inode->WriteBack(transaction);
	if (status != B_OK)
		return status;

	TRACE("ext2_remove_vnode(): Freeing inode\n");
	status = volume->FreeInode(transaction, inode->ID(), inode->IsDirectory());

	// TODO: When Transaction::Done() fails, do we have to re-add the vnode?
	if (status == B_OK)
		status = transaction.Done();

	return status;
}


static bool
ext2_can_page(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	return true;
}


static status_t
ext2_read_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->FileCache() == NULL)
		return B_BAD_VALUE;

	rw_lock_read_lock(inode->Lock());

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

	rw_lock_read_unlock(inode->Lock());

	return status;
}


static status_t
ext2_write_pages(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (inode->FileCache() == NULL)
		return B_BAD_VALUE;

	rw_lock_read_lock(inode->Lock());

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

	rw_lock_read_unlock(inode->Lock());

	return status;
}


static status_t
ext2_io(fs_volume* _volume, fs_vnode* _node, void* _cookie, io_request* request)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

#ifndef EXT2_SHELL
	if (io_request_is_write(request) && volume->IsReadOnly()) {
		notify_io_request(request, B_READ_ONLY_DEVICE);
		return B_READ_ONLY_DEVICE;
	}
#endif

	if (inode->FileCache() == NULL) {
#ifndef EXT2_SHELL
		notify_io_request(request, B_BAD_VALUE);
#endif
		return B_BAD_VALUE;
	}

	// We lock the node here and will unlock it in the "finished" hook.
	rw_lock_read_lock(inode->Lock());

	return do_iterative_fd_io(volume->Device(), request,
		iterative_io_get_vecs_hook, iterative_io_finished_hook, inode);
}


static status_t
ext2_get_file_map(fs_volume* _volume, fs_vnode* _node, off_t offset,
	size_t size, struct file_io_vec* vecs, size_t* _count)
{
	TRACE("ext2_get_file_map()\n");
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	size_t index = 0, max = *_count;

	while (true) {
		fsblock_t block;
		uint32 count = 1;
		status_t status = inode->FindBlock(offset, block, &count);
		if (status != B_OK)
			return status;

		if (block > volume->NumBlocks()) {
			panic("ext2_get_file_map() found block %" B_PRIu64 " for offset %"
				B_PRIdOFF "\n", block, offset);
		}

		off_t blockOffset = block << volume->BlockShift();
		uint32 blockLength = volume->BlockSize() * count;

		if (index > 0 && (vecs[index - 1].offset
				== blockOffset - vecs[index - 1].length
				|| (vecs[index - 1].offset == -1 && block == 0))) {
			vecs[index - 1].length += blockLength;
		} else {
			if (index >= max) {
				// we're out of file_io_vecs; let's bail out
				*_count = index;
				return B_BUFFER_OVERFLOW;
			}

			// 'block' is 0 for sparse blocks
			if (block != 0)
				vecs[index].offset = blockOffset;
			else
				vecs[index].offset = -1;

			vecs[index].length = blockLength;
			index++;
		}

		offset += blockLength;

		if (offset >= inode->Size() || size <= blockLength) {
			// We're done!
			*_count = index;
			TRACE("ext2_get_file_map for inode %" B_PRIdINO "\n", inode->ID());
			return B_OK;
		}

		size -= blockLength;
	}

	// can never get here
	return B_ERROR;
}


//	#pragma mark -


static status_t
ext2_lookup(fs_volume* _volume, fs_vnode* _directory, const char* name,
	ino_t* _vnodeID)
{
	TRACE("ext2_lookup: name address: %p\n", name);
	TRACE("ext2_lookup: name: %s\n", name);
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		return status;

	HTree htree(volume, directory);
	DirectoryIterator* iterator;

	status = htree.Lookup(name, &iterator);
	if (status != B_OK)
		return status;

	ObjectDeleter<DirectoryIterator> iteratorDeleter(iterator);

	status = iterator->FindEntry(name, _vnodeID);
	if (status != B_OK)
		return status;

	return get_vnode(volume->FSVolume(), *_vnodeID, NULL);
}


static status_t
ext2_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 cmd,
	void* buffer, size_t bufferLength)
{
	TRACE("ioctl: %" B_PRIu32 "\n", cmd);

	Volume* volume = (Volume*)_volume->private_volume;
	switch (cmd) {
		case 56742:
		{
			TRACE("ioctl: Test the block allocator\n");
			// Test the block allocator
			TRACE("ioctl: Creating transaction\n");
			Transaction transaction(volume->GetJournal());
			TRACE("ioctl: Creating cached block\n");
			CachedBlock cached(volume);
			uint32 blocksPerGroup = volume->BlocksPerGroup();
			uint32 blockSize  = volume->BlockSize();
			uint32 firstBlock = volume->FirstDataBlock();
			fsblock_t start = 0;
			uint32 group = 0;
			uint32 length;

			TRACE("ioctl: blocks per group: %" B_PRIu32 ", block size: %"
				B_PRIu32 ", first block: %" B_PRIu32 ", start: %" B_PRIu64
				", group: %" B_PRIu32 "\n", blocksPerGroup,
				blockSize, firstBlock, start, group);

			while (volume->AllocateBlocks(transaction, 1, 2048, group, start,
					length) == B_OK) {
				TRACE("ioctl: Allocated blocks in group %" B_PRIu32 ": %"
					B_PRIu64 "-%" B_PRIu64 "\n", group, start, start + length);
				off_t blockNum = start + group * blocksPerGroup - firstBlock;

				for (uint32 i = 0; i < length; ++i) {
					uint8* block = cached.SetToWritable(transaction, blockNum);
					memset(block, 0, blockSize);
					blockNum++;
				}

				TRACE("ioctl: Blocks cleared\n");

				transaction.Done();
				transaction.Start(volume->GetJournal());
			}

			TRACE("ioctl: Done\n");

			return B_OK;
		}

		case FIOSEEKDATA:
		case FIOSEEKHOLE:
		{
			off_t* offset = (off_t*)buffer;
			Inode* inode = (Inode*)_node->private_node;

			if (*offset >= inode->Size())
				return ENXIO;

			while (*offset < inode->Size()) {
				fsblock_t block;
				uint32 count = 1;
				status_t status = inode->FindBlock(*offset, block, &count);
				if (status != B_OK)
					return status;
				if ((block != 0 && cmd == FIOSEEKDATA)
					|| (block == 0 && cmd == FIOSEEKHOLE)) {
					return B_OK;
				}
				*offset += count * volume->BlockSize();
			}

			if (*offset > inode->Size())
				*offset = inode->Size();
			return cmd == FIOSEEKDATA ? ENXIO : B_OK;
		}
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
ext2_fsync(fs_volume* _volume, fs_vnode* _node)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->Sync();
}


static status_t
ext2_read_stat(fs_volume* _volume, fs_vnode* _node, struct stat* stat)
{
	Inode* inode = (Inode*)_node->private_node;
	const ext2_inode& node = inode->Node();

	stat->st_dev = inode->GetVolume()->ID();
	stat->st_ino = inode->ID();
	stat->st_nlink = node.NumLinks();
	stat->st_blksize = EXT2_IO_SIZE;

	stat->st_uid = node.UserID();
	stat->st_gid = node.GroupID();
	stat->st_mode = node.Mode();
	stat->st_type = 0;

	inode->GetAccessTime(&stat->st_atim);
	inode->GetModificationTime(&stat->st_mtim);
	inode->GetChangeTime(&stat->st_ctim);
	inode->GetCreationTime(&stat->st_crtim);

	stat->st_size = inode->Size();
	stat->st_blocks = (inode->Size() + 511) / 512;

	return B_OK;
}


status_t
ext2_write_stat(fs_volume* _volume, fs_vnode* _node, const struct stat* stat,
	uint32 mask)
{
	TRACE("ext2_write_stat\n");
	Volume* volume = (Volume*)_volume->private_volume;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	Inode* inode = (Inode*)_node->private_node;

	ext2_inode& node = inode->Node();
	bool updateTime = false;
	uid_t uid = geteuid();

	bool isOwnerOrRoot = uid == 0 || uid == (uid_t)node.UserID();
	bool hasWriteAccess = inode->CheckPermissions(W_OK) == B_OK;

	TRACE("ext2_write_stat: Starting transaction\n");
	Transaction transaction(volume->GetJournal());
	inode->WriteLockInTransaction(transaction);

	if ((mask & B_STAT_SIZE) != 0 && inode->Size() != stat->st_size) {
		if (inode->IsDirectory())
			return B_IS_A_DIRECTORY;
		if (!inode->IsFile())
			return B_BAD_VALUE;
		if (!hasWriteAccess)
			return B_NOT_ALLOWED;

		TRACE("ext2_write_stat: Old size: %ld, new size: %ld\n",
			(long)inode->Size(), (long)stat->st_size);

		off_t oldSize = inode->Size();

		status_t status = inode->Resize(transaction, stat->st_size);
		if (status != B_OK)
			return status;

		if ((mask & B_STAT_SIZE_INSECURE) == 0) {
			rw_lock_write_unlock(inode->Lock());
			inode->FillGapWithZeros(oldSize, inode->Size());
			rw_lock_write_lock(inode->Lock());
		}

		updateTime = true;
	}

	if ((mask & B_STAT_MODE) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			return B_NOT_ALLOWED;
		node.UpdateMode(stat->st_mode, S_IUMSK);
		updateTime = true;
	}

	if ((mask & B_STAT_UID) != 0) {
		// only root should be allowed
		if (uid != 0)
			return B_NOT_ALLOWED;
		node.SetUserID(stat->st_uid);
		updateTime = true;
	}

	if ((mask & B_STAT_GID) != 0) {
		// only the user or root can do that
		if (!isOwnerOrRoot)
			return B_NOT_ALLOWED;
		node.SetGroupID(stat->st_gid);
		updateTime = true;
	}

	if ((mask & B_STAT_MODIFICATION_TIME) != 0 || updateTime
		|| (mask & B_STAT_CHANGE_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;
		struct timespec newTimespec = { 0, 0};

		if ((mask & B_STAT_MODIFICATION_TIME) != 0)
			newTimespec = stat->st_mtim;

		if ((mask & B_STAT_CHANGE_TIME) != 0
			&& stat->st_ctim.tv_sec > newTimespec.tv_sec)
			newTimespec = stat->st_ctim;

		if (newTimespec.tv_sec == 0)
			Inode::_BigtimeToTimespec(real_time_clock_usecs(), &newTimespec);

		inode->SetModificationTime(&newTimespec);
	}
	if ((mask & B_STAT_CREATION_TIME) != 0) {
		// the user or root can do that or any user with write access
		if (!isOwnerOrRoot && !hasWriteAccess)
			return B_NOT_ALLOWED;
		inode->SetCreationTime(&stat->st_crtim);
	}

	status_t status = inode->WriteBack(transaction);
	if (status == B_OK)
		status = transaction.Done();
	if (status == B_OK)
		notify_stat_changed(volume->ID(), -1, inode->ID(), mask);

	return status;
}


static status_t
ext2_create(fs_volume* _volume, fs_vnode* _directory, const char* name,
	int openMode, int mode, void** _cookie, ino_t* _vnodeID)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	TRACE("ext2_create()\n");

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		return B_BAD_TYPE;

	TRACE("ext2_create(): Creating cookie\n");

	// Allocate cookie
	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->open_mode = openMode;
	cookie->last_size = 0;
	cookie->last_notification = system_time();

	TRACE("ext2_create(): Starting transaction\n");

	Transaction transaction(volume->GetJournal());

	TRACE("ext2_create(): Creating inode\n");

	Inode* inode;
	bool created;
	status_t status = Inode::Create(transaction, directory, name,
		S_FILE | (mode & S_IUMSK), openMode, EXT2_TYPE_FILE, &created, _vnodeID,
		&inode, &gExt2VnodeOps);
	if (status != B_OK)
		return status;

	TRACE("ext2_create(): Created inode\n");

	if ((openMode & O_NOCACHE) != 0) {
		status = inode->DisableFileCache();
		if (status != B_OK)
			return status;
	}

	entry_cache_add(volume->ID(), directory->ID(), name, *_vnodeID);

	status = transaction.Done();
	if (status != B_OK) {
		entry_cache_remove(volume->ID(), directory->ID(), name);
		return status;
	}

	*_cookie = cookie;
	cookieDeleter.Detach();

	if (created)
		notify_entry_created(volume->ID(), directory->ID(), name, *_vnodeID);

	return B_OK;
}


static status_t
ext2_create_symlink(fs_volume* _volume, fs_vnode* _directory, const char* name,
	const char* path, int mode)
{
	TRACE("ext2_create_symlink()\n");

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		return B_BAD_TYPE;

	status_t status = directory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	TRACE("ext2_create_symlink(): Starting transaction\n");
	Transaction transaction(volume->GetJournal());

	Inode* link;
	ino_t id;
	status = Inode::Create(transaction, directory, name, S_SYMLINK | 0777,
		0, (uint8)EXT2_TYPE_SYMLINK, NULL, &id, &link);
	if (status != B_OK)
		return status;

	// TODO: We have to prepare the link before publishing?

	size_t length = strlen(path);
	TRACE("ext2_create_symlink(): Path (%s) length: %d\n", path, (int)length);
	if (length < EXT2_SHORT_SYMLINK_LENGTH) {
		strcpy(link->Node().symlink, path);
		link->Node().SetSize((uint32)length);
	} else {
		if (!link->HasFileCache()) {
			status = link->CreateFileCache();
			if (status != B_OK)
				return status;
		}

		size_t written = length;
		status = link->WriteAt(transaction, 0, (const uint8*)path, &written);
		if (status == B_OK && written != length)
			status = B_IO_ERROR;
	}

	if (status == B_OK)
		status = link->WriteBack(transaction);

	TRACE("ext2_create_symlink(): Publishing vnode\n");
	publish_vnode(volume->FSVolume(), id, link, &gExt2VnodeOps,
		link->Mode(), 0);
	put_vnode(volume->FSVolume(), id);

	if (status == B_OK) {
		entry_cache_add(volume->ID(), directory->ID(), name, id);

		status = transaction.Done();
		if (status == B_OK)
			notify_entry_created(volume->ID(), directory->ID(), name, id);
		else
			entry_cache_remove(volume->ID(), directory->ID(), name);
	}
	TRACE("ext2_create_symlink(): Done\n");

	return status;
}


static status_t
ext2_link(fs_volume* volume, fs_vnode* dir, const char* name, fs_vnode* node)
{
	// TODO

	return B_UNSUPPORTED;
}


static status_t
ext2_unlink(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	TRACE("ext2_unlink()\n");
	if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		return B_NOT_ALLOWED;

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	status_t status = directory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	TRACE("ext2_unlink(): Starting transaction\n");
	Transaction transaction(volume->GetJournal());

	directory->WriteLockInTransaction(transaction);

	TRACE("ext2_unlink(): Looking up for directory entry\n");
	HTree htree(volume, directory);
	DirectoryIterator* directoryIterator;

	status = htree.Lookup(name, &directoryIterator);
	if (status != B_OK)
		return status;

	ino_t id;
	status = directoryIterator->FindEntry(name, &id);
	if (status != B_OK)
		return status;

	{
		Vnode vnode(volume, id);
		Inode* inode;
		status = vnode.Get(&inode);
		if (status != B_OK)
			return status;

		inode->WriteLockInTransaction(transaction);

		status = inode->Unlink(transaction);
		if (status != B_OK)
			return status;

		status = directoryIterator->RemoveEntry(transaction);
		if (status != B_OK)
			return status;
	}
	entry_cache_remove(volume->ID(), directory->ID(), name);

	status = transaction.Done();
	if (status != B_OK)
		entry_cache_add(volume->ID(), directory->ID(), name, id);
	else
		notify_entry_removed(volume->ID(), directory->ID(), name, id);

	return status;
}


static status_t
ext2_rename(fs_volume* _volume, fs_vnode* _oldDir, const char* oldName,
	fs_vnode* _newDir, const char* newName)
{
	TRACE("ext2_rename()\n");

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* oldDirectory = (Inode*)_oldDir->private_node;
	Inode* newDirectory = (Inode*)_newDir->private_node;

	if (oldDirectory == newDirectory && strcmp(oldName, newName) == 0)
		return B_OK;

	Transaction transaction(volume->GetJournal());

	oldDirectory->WriteLockInTransaction(transaction);
	if (oldDirectory != newDirectory)
		newDirectory->WriteLockInTransaction(transaction);

	status_t status = oldDirectory->CheckPermissions(W_OK);
	if (status == B_OK)
		status = newDirectory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	HTree oldHTree(volume, oldDirectory);
	DirectoryIterator* oldIterator;

	status = oldHTree.Lookup(oldName, &oldIterator);
	if (status != B_OK)
		return status;

	ObjectDeleter<DirectoryIterator> oldIteratorDeleter(oldIterator);

	ino_t oldID;
	status = oldIterator->FindEntry(oldName, &oldID);
	if (status != B_OK)
		return status;

	TRACE("ext2_rename(): found entry to rename\n");

	if (oldDirectory != newDirectory) {
		TRACE("ext2_rename(): Different parent directories\n");
		CachedBlock cached(volume);

		ino_t parentID = newDirectory->ID();
		ino_t oldDirID = oldDirectory->ID();

		do {
			Vnode vnode(volume, parentID);
			Inode* parent;

			status = vnode.Get(&parent);
			if (status != B_OK)
				return B_IO_ERROR;

			fsblock_t blockNum;
			status = parent->FindBlock(0, blockNum);
			if (status != B_OK)
				return status;

			const HTreeRoot* data = (const HTreeRoot*)cached.SetTo(blockNum);
			parentID = data->dotdot.InodeID();
		} while (parentID != oldID && parentID != oldDirID
			&& parentID != EXT2_ROOT_NODE);

		if (parentID == oldID)
			return B_BAD_VALUE;
	}

	HTree newHTree(volume, newDirectory);
	DirectoryIterator* newIterator;

	status = newHTree.Lookup(newName, &newIterator);
	if (status != B_OK)
		return status;

	TRACE("ext2_rename(): found new directory\n");

	ObjectDeleter<DirectoryIterator> newIteratorDeleter(newIterator);

	Vnode vnode(volume, oldID);
	Inode* inode;

	status = vnode.Get(&inode);
	if (status != B_OK)
		return status;

	uint8 fileType;

	// TODO: Support all file types?
	if (inode->IsDirectory())
		fileType = EXT2_TYPE_DIRECTORY;
	else if (inode->IsSymLink())
		fileType = EXT2_TYPE_SYMLINK;
	else
		fileType = EXT2_TYPE_FILE;

	// Add entry in destination directory
	ino_t existentID;
	status = newIterator->FindEntry(newName, &existentID);
	if (status == B_OK) {
		TRACE("ext2_rename(): found existing new entry\n");
		if (existentID == oldID) {
			// Remove entry in oldID
			// return inode->Unlink();
			return B_BAD_VALUE;
		}

		Vnode vnodeExistent(volume, existentID);
		Inode* existent;

		if (vnodeExistent.Get(&existent) != B_OK)
			return B_NAME_IN_USE;

		if (existent->IsDirectory() != inode->IsDirectory()) {
			return existent->IsDirectory() ? B_IS_A_DIRECTORY
				: B_NOT_A_DIRECTORY;
		}

		// TODO: Perhaps we have to revert this in case of error?
		status = newIterator->ChangeEntry(transaction, oldID, fileType);
		if (status != B_OK)
			return status;

		notify_entry_removed(volume->ID(), newDirectory->ID(), newName,
			existentID);
	} else if (status == B_ENTRY_NOT_FOUND) {
		newIterator->Restart();

		status = newIterator->AddEntry(transaction, newName, strlen(newName),
			oldID, fileType);
		if (status != B_OK)
			return status;

	} else
		return status;

	if (oldDirectory == newDirectory) {
		status = oldHTree.Lookup(oldName, &oldIterator);
		if (status != B_OK)
			return status;

		oldIteratorDeleter.SetTo(oldIterator);
		status = oldIterator->FindEntry(oldName, &oldID);
		if (status != B_OK)
			return status;
	}

	// Remove entry from source folder
	status = oldIterator->RemoveEntry(transaction);
	if (status != B_OK)
		return status;

	inode->WriteLockInTransaction(transaction);

	if (oldDirectory != newDirectory && inode->IsDirectory()) {
		DirectoryIterator inodeIterator(inode);

		status = inodeIterator.FindEntry("..");
		if (status == B_ENTRY_NOT_FOUND) {
			ERROR("Corrupt file system. Missing \"..\" in directory %"
				B_PRIdINO "\n", inode->ID());
			return B_BAD_DATA;
		} else if (status != B_OK)
			return status;

		inodeIterator.ChangeEntry(transaction, newDirectory->ID(),
			(uint8)EXT2_TYPE_DIRECTORY);
		// Decrement hardlink count on the source folder
		status = oldDirectory->Unlink(transaction);
		if (status != B_OK)
			ERROR("Error while decrementing hardlink count on the source folder\n");
		// Increment hardlink count on the destination folder
		newDirectory->IncrementNumLinks(transaction);
		status = newDirectory->WriteBack(transaction);
		if (status != B_OK)
			ERROR("Error while writing back the destination folder inode\n");
	}

	status = inode->WriteBack(transaction);
	if (status != B_OK)
		return status;

	entry_cache_remove(volume->ID(), oldDirectory->ID(), oldName);
	entry_cache_add(volume->ID(), newDirectory->ID(), newName, oldID);

	status = transaction.Done();
	if (status != B_OK) {
		entry_cache_remove(volume->ID(), oldDirectory->ID(), newName);
		entry_cache_add(volume->ID(), newDirectory->ID(), oldName, oldID);

		return status;
	}

	notify_entry_moved(volume->ID(), oldDirectory->ID(), oldName,
		newDirectory->ID(), newName, oldID);

	return B_OK;
}


static status_t
ext2_open(fs_volume* _volume, fs_vnode* _node, int openMode, void** _cookie)
{
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && (openMode & O_RWMASK) != 0)
		return B_IS_A_DIRECTORY;

	status_t status =  inode->CheckPermissions(open_mode_to_access(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
	if (status != B_OK)
		return status;

	// Prepare the cookie
	file_cookie* cookie = new(std::nothrow) file_cookie;
	if (cookie == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<file_cookie> cookieDeleter(cookie);

	cookie->open_mode = openMode & EXT2_OPEN_MODE_USER_MASK;
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	MethodDeleter<Inode, status_t, &Inode::EnableFileCache> fileCacheEnabler;
	if ((openMode & O_NOCACHE) != 0) {
		status = inode->DisableFileCache();
		if (status != B_OK)
			return status;
		fileCacheEnabler.SetTo(inode);
	}

	// Should we truncate the file?
	if ((openMode & O_TRUNC) != 0) {
		if ((openMode & O_RWMASK) == O_RDONLY)
			return B_NOT_ALLOWED;

		Transaction transaction(volume->GetJournal());
		inode->WriteLockInTransaction(transaction);

		status_t status = inode->Resize(transaction, 0);
		if (status == B_OK)
			status = inode->WriteBack(transaction);
		if (status == B_OK)
			status = transaction.Done();
		if (status != B_OK)
			return status;

		// TODO: No need to notify file size changed?
	}

	fileCacheEnabler.Detach();
	cookieDeleter.Detach();
	*_cookie = cookie;

	return B_OK;
}


static status_t
ext2_read(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	void* buffer, size_t* _length)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsFile()) {
		*_length = 0;
		return inode->IsDirectory() ? B_IS_A_DIRECTORY : B_BAD_VALUE;
	}

	return inode->ReadAt(pos, (uint8*)buffer, _length);
}


static status_t
ext2_write(fs_volume* _volume, fs_vnode* _node, void* _cookie, off_t pos,
	const void* buffer, size_t* _length)
{
	TRACE("ext2_write()\n");
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (inode->IsDirectory()) {
		*_length = 0;
		return B_IS_A_DIRECTORY;
	}

	TRACE("ext2_write(): Preparing cookie\n");

	file_cookie* cookie = (file_cookie*)_cookie;

	if ((cookie->open_mode & O_APPEND) != 0)
		pos = inode->Size();

	TRACE("ext2_write(): Creating transaction\n");
	Transaction transaction;

	status_t status = inode->WriteAt(transaction, pos, (const uint8*)buffer,
		_length);
	if (status == B_OK)
		status = transaction.Done();
	if (status == B_OK) {
		TRACE("ext2_write(): Finalizing\n");
		ReadLocker lock(*inode->Lock());

		if (cookie->last_size != inode->Size()
			&& system_time() > cookie->last_notification
				+ INODE_NOTIFICATION_INTERVAL) {
			notify_stat_changed(volume->ID(), -1, inode->ID(),
				B_STAT_MODIFICATION_TIME | B_STAT_SIZE | B_STAT_INTERIM_UPDATE);
			cookie->last_size = inode->Size();
			cookie->last_notification = system_time();
		}
	}

	TRACE("ext2_write(): Done\n");

	return status;
}


static status_t
ext2_close(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	return B_OK;
}


static status_t
ext2_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;

	if (inode->Size() != cookie->last_size)
		notify_stat_changed(volume->ID(), -1, inode->ID(), B_STAT_SIZE);

	if ((cookie->open_mode & O_NOCACHE) != 0)
		inode->EnableFileCache();

	delete cookie;
	return B_OK;
}


static status_t
ext2_access(fs_volume* _volume, fs_vnode* _node, int accessMode)
{
	Inode* inode = (Inode*)_node->private_node;
	return inode->CheckPermissions(accessMode);
}


static status_t
ext2_read_link(fs_volume *_volume, fs_vnode *_node, char *buffer,
	size_t *_bufferSize)
{
	Inode* inode = (Inode*)_node->private_node;

	if (!inode->IsSymLink())
		return B_BAD_VALUE;

	if (inode->Size() > EXT2_SHORT_SYMLINK_LENGTH) {
		status_t result = inode->ReadAt(0, reinterpret_cast<uint8*>(buffer),
			_bufferSize);
		if (result != B_OK)
			return result;
	} else {
		size_t bytesToCopy = std::min(static_cast<size_t>(inode->Size()),
			*_bufferSize);

		memcpy(buffer, inode->Node().symlink, bytesToCopy);
	}

	*_bufferSize = inode->Size();
	return B_OK;
}


//	#pragma mark - Directory functions


static status_t
ext2_create_dir(fs_volume* _volume, fs_vnode* _directory, const char* name,
	int mode)
{
	TRACE("ext2_create_dir()\n");
	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	if (!directory->IsDirectory())
		return B_BAD_TYPE;

	status_t status = directory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	TRACE("ext2_create_dir(): Starting transaction\n");
	Transaction transaction(volume->GetJournal());

	ino_t id;
	status = Inode::Create(transaction, directory, name,
		S_DIRECTORY | (mode & S_IUMSK), 0, EXT2_TYPE_DIRECTORY, NULL, &id);
	if (status != B_OK)
		return status;

	put_vnode(volume->FSVolume(), id);

	entry_cache_add(volume->ID(), directory->ID(), name, id);

	status = transaction.Done();
	if (status != B_OK) {
		entry_cache_remove(volume->ID(), directory->ID(), name);
		return status;
	}

	notify_entry_created(volume->ID(), directory->ID(), name, id);

	TRACE("ext2_create_dir(): Done\n");

	return B_OK;
}


static status_t
ext2_remove_dir(fs_volume* _volume, fs_vnode* _directory, const char* name)
{
	TRACE("ext2_remove_dir()\n");

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* directory = (Inode*)_directory->private_node;

	status_t status = directory->CheckPermissions(W_OK);
	if (status != B_OK)
		return status;

	TRACE("ext2_remove_dir(): Starting transaction\n");
	Transaction transaction(volume->GetJournal());

	directory->WriteLockInTransaction(transaction);

	TRACE("ext2_remove_dir(): Looking up for directory entry\n");
	HTree htree(volume, directory);
	DirectoryIterator* directoryIterator;

	status = htree.Lookup(name, &directoryIterator);
	if (status != B_OK)
		return status;

	ino_t id;
	status = directoryIterator->FindEntry(name, &id);
	if (status != B_OK)
		return status;

	Vnode vnode(volume, id);
	Inode* inode;
	status = vnode.Get(&inode);
	if (status != B_OK)
		return status;

	inode->WriteLockInTransaction(transaction);

	status = inode->Unlink(transaction);
	if (status != B_OK)
		return status;

	status = directory->Unlink(transaction);
	if (status != B_OK)
		return status;

	status = directoryIterator->RemoveEntry(transaction);
	if (status != B_OK)
		return status;

	entry_cache_remove(volume->ID(), directory->ID(), name);
	entry_cache_remove(volume->ID(), id, "..");

	status = transaction.Done();
	if (status != B_OK) {
		entry_cache_add(volume->ID(), directory->ID(), name, id);
		entry_cache_add(volume->ID(), id, "..", id);
	} else
		notify_entry_removed(volume->ID(), directory->ID(), name, id);

	return status;
}


static status_t
ext2_open_dir(fs_volume* _volume, fs_vnode* _node, void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;
	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	if (!inode->IsDirectory())
		return B_NOT_A_DIRECTORY;

	DirectoryIterator* iterator = new(std::nothrow) DirectoryIterator(inode);
	if (iterator == NULL)
		return B_NO_MEMORY;

	*_cookie = iterator;
	return B_OK;
}


static status_t
ext2_read_dir(fs_volume *_volume, fs_vnode *_node, void *_cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	DirectoryIterator *iterator = (DirectoryIterator *)_cookie;
	Volume* volume = (Volume*)_volume->private_volume;

	uint32 maxCount = *_num;
	uint32 count = 0;

	while (count < maxCount && bufferSize > sizeof(struct dirent)) {

		size_t length = bufferSize - sizeof(struct dirent) + 1;
		ino_t id;

		status_t status = iterator->GetNext(dirent->d_name, &length, &id);
		if (status == B_ENTRY_NOT_FOUND)
			break;

		if (status == B_BUFFER_OVERFLOW) {
			// the remaining name buffer length was too small
			if (count == 0)
				return B_BUFFER_OVERFLOW;
			break;
		}

		if (status != B_OK)
			return status;

		status = iterator->Next();
		if (status != B_OK && status != B_ENTRY_NOT_FOUND)
			return status;

		dirent->d_dev = volume->ID();
		dirent->d_ino = id;
		dirent->d_reclen = sizeof(struct dirent) + length;

		bufferSize -= dirent->d_reclen;
		dirent = (struct dirent*)((uint8*)dirent + dirent->d_reclen);
		count++;
	}

	*_num = count;
	return B_OK;
}


static status_t
ext2_rewind_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void *_cookie)
{
	DirectoryIterator *iterator = (DirectoryIterator *)_cookie;
	return iterator->Rewind();
}


static status_t
ext2_close_dir(fs_volume * /*_volume*/, fs_vnode * /*node*/, void * /*_cookie*/)
{
	return B_OK;
}


static status_t
ext2_free_dir_cookie(fs_volume *_volume, fs_vnode *_node, void *_cookie)
{
	delete (DirectoryIterator*)_cookie;
	return B_OK;
}


static status_t
ext2_open_attr_dir(fs_volume *_volume, fs_vnode *_node, void **_cookie)
{
	Inode* inode = (Inode*)_node->private_node;
	Volume* volume = (Volume*)_volume->private_volume;
	TRACE("%s()\n", __FUNCTION__);

	if (!(volume->SuperBlock().CompatibleFeatures() & EXT2_FEATURE_EXT_ATTR))
		return ENOSYS;

	// on directories too ?
	if (!inode->IsFile())
		return EINVAL;

	int32 *index = new(std::nothrow) int32;
	if (index == NULL)
		return B_NO_MEMORY;
	*index = 0;
	*(int32**)_cookie = index;
	return B_OK;
}

static status_t
ext2_close_attr_dir(fs_volume* _volume, fs_vnode* _node, void* cookie)
{
	TRACE("%s()\n", __FUNCTION__);
	return B_OK;
}


static status_t
ext2_free_attr_dir_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	TRACE("%s()\n", __FUNCTION__);
	delete (int32 *)_cookie;
	return B_OK;
}


static status_t
ext2_read_attr_dir(fs_volume* _volume, fs_vnode* _node,
				void* _cookie, struct dirent* dirent, size_t bufferSize,
				uint32* _num)
{
	Inode* inode = (Inode*)_node->private_node;
	int32 index = *(int32 *)_cookie;
	Attribute attribute(inode);
	TRACE("%s()\n", __FUNCTION__);

	size_t length = bufferSize;
	status_t status = attribute.Find(index);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK)
		return status;

	status = attribute.GetName(dirent->d_name, &length);
	if (status != B_OK)
		return B_OK;

	Volume* volume = (Volume*)_volume->private_volume;

	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	*(int32*)_cookie = index + 1;
	return B_OK;
}


static status_t
ext2_rewind_attr_dir(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	*(int32*)_cookie = 0;
	TRACE("%s()\n", __FUNCTION__);
	return B_OK;
}


	/* attribute operations */
static status_t
ext2_create_attr(fs_volume* _volume, fs_vnode* _node,
	const char* name, uint32 type, int openMode, void** _cookie)
{
	return EROFS;
}


static status_t
ext2_open_attr(fs_volume* _volume, fs_vnode* _node, const char* name,
	int openMode, void** _cookie)
{
	TRACE("%s()\n", __FUNCTION__);

	Volume* volume = (Volume*)_volume->private_volume;
	Inode* inode = (Inode*)_node->private_node;
	Attribute attribute(inode);

	if (!(volume->SuperBlock().CompatibleFeatures() & EXT2_FEATURE_EXT_ATTR))
		return ENOSYS;

	return attribute.Open(name, openMode, (attr_cookie**)_cookie);
}


static status_t
ext2_close_attr(fs_volume* _volume, fs_vnode* _node,
	void* cookie)
{
	return B_OK;
}


static status_t
ext2_free_attr_cookie(fs_volume* _volume, fs_vnode* _node,
	void* cookie)
{
	delete (attr_cookie*)cookie;
	return B_OK;
}


static status_t
ext2_read_attr(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t pos, void* buffer, size_t* _length)
{
	TRACE("%s()\n", __FUNCTION__);

	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Read(cookie, pos, (uint8*)buffer, _length);
}


static status_t
ext2_write_attr(fs_volume* _volume, fs_vnode* _node, void* cookie,
	off_t pos, const void* buffer, size_t* length)
{
	return EROFS;
}



static status_t
ext2_read_attr_stat(fs_volume* _volume, fs_vnode* _node,
	void* _cookie, struct stat* stat)
{
	attr_cookie* cookie = (attr_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	Attribute attribute(inode, cookie);

	return attribute.Stat(*stat);
}


static status_t
ext2_write_attr_stat(fs_volume* _volume, fs_vnode* _node,
	void* cookie, const struct stat* stat, int statMask)
{
	return EROFS;
}


static status_t
ext2_rename_attr(fs_volume* _volume, fs_vnode* fromVnode,
	const char* fromName, fs_vnode* toVnode, const char* toName)
{
	return ENOSYS;
}


static status_t
ext2_remove_attr(fs_volume* _volume, fs_vnode* vnode,
	const char* name)
{
	return ENOSYS;
}


fs_volume_ops gExt2VolumeOps = {
	&ext2_unmount,
	&ext2_read_fs_info,
	&ext2_write_fs_info,
	&ext2_sync,
	&ext2_get_vnode,
};


fs_vnode_ops gExt2VnodeOps = {
	/* vnode operations */
	&ext2_lookup,
	NULL,
	&ext2_put_vnode,
	&ext2_remove_vnode,

	/* VM file access */
	&ext2_can_page,
	&ext2_read_pages,
	&ext2_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	&ext2_get_file_map,

	&ext2_ioctl,
	NULL,
	NULL,	// fs_select
	NULL,	// fs_deselect
	&ext2_fsync,

	&ext2_read_link,
	&ext2_create_symlink,

	&ext2_link,
	&ext2_unlink,
	&ext2_rename,

	&ext2_access,
	&ext2_read_stat,
	&ext2_write_stat,
	NULL,	// fs_preallocate

	/* file operations */
	&ext2_create,
	&ext2_open,
	&ext2_close,
	&ext2_free_cookie,
	&ext2_read,
	&ext2_write,

	/* directory operations */
	&ext2_create_dir,
	&ext2_remove_dir,
	&ext2_open_dir,
	&ext2_close_dir,
	&ext2_free_dir_cookie,
	&ext2_read_dir,
	&ext2_rewind_dir,

	/* attribute directory operations */
	&ext2_open_attr_dir,
	&ext2_close_attr_dir,
	&ext2_free_attr_dir_cookie,
	&ext2_read_attr_dir,
	&ext2_rewind_attr_dir,

	/* attribute operations */
	NULL, //&ext2_create_attr,
	&ext2_open_attr,
	&ext2_close_attr,
	&ext2_free_attr_cookie,
	&ext2_read_attr,
	NULL, //&ext2_write_attr,
	&ext2_read_attr_stat,
	NULL, //&ext2_write_attr_stat,
	NULL, //&ext2_rename_attr,
	NULL, //&ext2_remove_attr,
};


static file_system_module_info sExt2FileSystem = {
	{
		"file_systems/ext2" B_CURRENT_FS_API_VERSION,
		0,
		NULL,
	},

	"ext2",						// short_name
	"Ext2 File System",			// pretty_name
	B_DISK_SYSTEM_SUPPORTS_WRITING
		| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME,	// DDM flags

	// scanning
	ext2_identify_partition,
	ext2_scan_partition,
	ext2_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&ext2_mount,

	NULL,
};


module_info *modules[] = {
	(module_info *)&sExt2FileSystem,
	NULL,
};
