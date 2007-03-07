/* kernel_interface - file system interface to Haiku's vnode layer
 *
 * Copyright 2001-2007, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Debug.h"
#include "Volume.h"
#include "Inode.h"
#include "Index.h"
#include "BPlusTree.h"
#include "Query.h"
#include "Attribute.h"
#include "bfs_control.h"

#include <util/kernel_cpp.h>

#include <string.h>
#include <stdio.h>

#include <KernelExport.h>
#include <NodeMonitor.h>
#include <fs_interface.h>
#include <fs_cache.h>

#include <fs_attr.h>
#include <fs_info.h>
#include <fs_index.h>
#include <fs_query.h>
#include <fs_volume.h>

#define BFS_IO_SIZE	65536


struct identify_cookie {
	disk_super_block super_block;
};


extern void fill_stat_buffer(Inode *inode, struct stat &stat);


void
fill_stat_buffer(Inode *inode, struct stat &stat)
{
	const bfs_inode &node = inode->Node();

	stat.st_dev = inode->GetVolume()->ID();
	stat.st_ino = inode->ID();
	stat.st_nlink = 1;
	stat.st_blksize = BFS_IO_SIZE;

	stat.st_uid = node.UserID();
	stat.st_gid = node.GroupID();
	stat.st_mode = node.Mode();
	stat.st_type = node.Type();

	stat.st_atime = time(NULL);
	stat.st_mtime = stat.st_ctime = (time_t)(node.LastModifiedTime() >> INODE_TIME_SHIFT);
	stat.st_crtime = (time_t)(node.CreateTime() >> INODE_TIME_SHIFT);

	if (inode->IsSymLink() && (node.Flags() & INODE_LONG_SYMLINK) == 0) {
		// symlinks report the size of the link here
		stat.st_size = strlen(node.short_symlink) + 1;
	} else
		stat.st_size = inode->Size();
}


//	#pragma mark - Scanning


static float
bfs_identify_partition(int fd, partition_data *partition, void **_cookie)
{
	disk_super_block superBlock;
	status_t status = Volume::Identify(fd, &superBlock);
	if (status != B_OK)
		return status;

	identify_cookie *cookie = new identify_cookie;
	memcpy(&cookie->super_block, &superBlock, sizeof(disk_super_block));

	*_cookie = cookie;
	return 0.8f;
}


static status_t
bfs_scan_partition(int fd, partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_FILE_SYSTEM;
	partition->content_size = cookie->super_block.NumBlocks() * cookie->super_block.BlockSize();
	partition->block_size = cookie->super_block.BlockSize();
	partition->content_name = strdup(cookie->super_block.name);
	if (partition->content_name == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


static void
bfs_free_identify_partition_cookie(partition_data *partition, void *_cookie)
{
	identify_cookie *cookie = (identify_cookie *)_cookie;

	delete cookie;
}


//	#pragma mark -


static status_t
bfs_mount(mount_id mountID, const char *device, uint32 flags, const char *args,
	void **_data, vnode_id *_rootID)
{
	FUNCTION();

	Volume *volume = new Volume(mountID);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status;
	if ((status = volume->Mount(device, flags)) == B_OK) {
		*_data = volume;
		*_rootID = volume->ToVnode(volume->Root());
		INFORM(("mounted \"%s\" (root node at %Ld, device = %s)\n",
			volume->Name(), *_rootID, device));
	}
	else
		delete volume;

	RETURN_ERROR(status);
}


static status_t
bfs_unmount(void *ns)
{
	FUNCTION();
	Volume* volume = (Volume *)ns;

	status_t status = volume->Unmount();
	delete volume;

	RETURN_ERROR(status);
}


/**	Fill in bfs_info struct for device.
 */

static status_t
bfs_read_fs_stat(void *_ns, struct fs_info *info)
{
	FUNCTION();
	if (_ns == NULL || info == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	RecursiveLocker locker(volume->Lock());

	// File system flags.
	info->flags = B_FS_IS_PERSISTENT | B_FS_HAS_ATTR | B_FS_HAS_MIME | B_FS_HAS_QUERY |
			(volume->IsReadOnly() ? B_FS_IS_READONLY : 0);

	info->io_size = BFS_IO_SIZE;
		// whatever is appropriate here? Just use the same value as BFS (and iso9660) for now

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
bfs_write_fs_stat(void *_ns, const struct fs_info *info, uint32 mask)
{
	FUNCTION_START(("mask = %ld\n", mask));

	Volume *volume = (Volume *)_ns;
	disk_super_block &superBlock = volume->SuperBlock();

	RecursiveLocker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & FS_WRITE_FSINFO_NAME) {
		strncpy(superBlock.name, info->volume_name, sizeof(superBlock.name) - 1);
		superBlock.name[sizeof(superBlock.name) - 1] = '\0';

		status = volume->WriteSuperBlock();
	}
	return status;
}

#if 0
static status_t
bfs_initialize(const char *deviceName, void *parms, size_t len)
{
	FUNCTION_START(("deviceName = %s, parameter len = %ld\n", deviceName, len));

	// The backend (to create the file system) is already done; just
	// call Volume::Initialize().

	return B_ERROR;
}
#endif

static status_t
bfs_sync(void *_ns)
{
	FUNCTION();
	if (_ns == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	return volume->Sync();
}


//	#pragma mark -


/**	Reads in the node from disk and creates an inode object from it.
 */

static status_t
bfs_read_vnode(void *_ns, vnode_id id, void **_node, bool reenter)
{
	//FUNCTION_START(("vnode_id = %Ld\n", id));
	Volume *volume = (Volume *)_ns;

	// first inode may be after the log area, we don't go through
	// the hassle and try to load an earlier block from disk
	if (id < volume->ToBlock(volume->Log()) + volume->Log().Length()
		|| id > volume->NumBlocks()) {
		INFORM(("inode at %Ld requested!\n", id));
		return B_ERROR;
	}

	CachedBlock cached(volume, id);
	bfs_inode *node = (bfs_inode *)cached.Block();
	if (node == NULL) {
		FATAL(("could not read inode: %Ld\n", id));
		return B_IO_ERROR;
	}

	status_t status = node->InitCheck(volume);
	if (status < B_OK) {
		FATAL(("inode at %Ld is corrupt!\n", id));
		return status;
	}

	Inode *inode = new Inode(volume, id);
	if (inode == NULL)
		return B_NO_MEMORY;

	status = inode->InitCheck(false);
	if (status < B_OK)
		delete inode;

	if (status == B_OK)
		*_node = inode;

	return status;
}


static status_t
bfs_release_vnode(void *_ns, void *_node, bool reenter)
{
	//FUNCTION_START(("node = %p\n", _node));

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	// since a directory's size can be changed without having it opened,
	// we need to take care about their preallocated blocks here
	if (!volume->IsReadOnly() && inode->NeedsTrimming()) {
		Transaction transaction(volume, inode->BlockNumber());

		if (inode->TrimPreallocation(transaction) == B_OK)
			transaction.Done();
		else if (transaction.HasParent()) {
			// ToDo: for now, we don't let sub-transactions fail
			transaction.Done();
		}
	}

	delete inode;

	return B_NO_ERROR;
}


static status_t
bfs_remove_vnode(void *_ns, void *_node, bool reenter)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	// The "chkbfs" functionality uses this flag to prevent the space used
	// up by the inode from being freed - this flag is set only in situations
	// where this is a good idea... (the block bitmap will get fixed anyway
	// in this case).
	if (inode->Flags() & INODE_DONT_FREE_SPACE) {
		delete inode;
		return B_OK;
	}

	// If the inode isn't in use anymore, we were called before
	// bfs_unlink() returns - in this case, we can just use the
	// transaction which has already deleted the inode.
	Transaction transaction(volume, volume->ToBlock(inode->Parent()));

	status_t status = inode->Free(transaction);
	if (status == B_OK) {
		transaction.Done();

		delete inode;
	} else if (transaction.HasParent()) {
		// ToDo: for now, we don't let sub-transactions fail
		transaction.Done();
	}

	return status;
}


static bool
bfs_can_page(fs_volume _fs, fs_vnode _v, fs_cookie _cookie)
{
	// ToDo: we're obviously not even asked...
	return false;
}


static status_t
bfs_read_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	Inode *inode = (Inode *)_node;

	if (inode->FileCache() == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	if (!reenter)
		inode->Lock().Lock();

	status_t status = file_cache_read_pages(inode->FileCache(), pos, vecs, count,
		_numBytes);

	if (!reenter)
		inode->Lock().Unlock();

	return status;
}


static status_t
bfs_write_pages(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	Inode *inode = (Inode *)_node;

	if (inode->FileCache() == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	if (!reenter)
		inode->Lock().Lock();

	status_t status = file_cache_write_pages(inode->FileCache(), pos, vecs, count,
		_numBytes);

	if (!reenter)
		inode->Lock().Unlock();

	return status;
}


static status_t
bfs_get_file_map(fs_volume _fs, fs_vnode _node, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *_count)
{
	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	int32 blockShift = volume->BlockShift();
	size_t index = 0, max = *_count;
	block_run run;
	off_t fileOffset;

	//FUNCTION_START(("offset = %Ld, size = %lu\n", offset, size));

	while (true) {
		status_t status = inode->FindBlockRun(offset, run, fileOffset);
		if (status != B_OK)
			return status;

		vecs[index].offset = volume->ToOffset(run) + offset - fileOffset;
		vecs[index].length = (run.Length() << blockShift) - offset + fileOffset;

		offset += vecs[index].length;

		// are we already done?
		if (size <= vecs[index].length
			|| offset >= inode->Size()) {
			if (offset > inode->Size()) {
				// make sure the extent ends with the last official file
				// block (without taking any preallocations into account)
				vecs[index].length = (inode->Size() - fileOffset + volume->BlockSize() - 1)
					& ~(volume->BlockSize() - 1);
			}
			*_count = index + 1;
			return B_OK;
		}

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


/**	the walk function just "walks" through a directory looking for the
 *	specified file. It calls get_vnode() on its vnode-id to init it
 *	for the kernel.
 */

static status_t
bfs_lookup(void *_ns, void *_directory, const char *file, vnode_id *_vnodeID, int *_type)
{
	//FUNCTION_START(("file = %s\n", file));
	if (_ns == NULL || _directory == NULL || file == NULL || _vnodeID == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	// check access permissions
	status_t status = directory->CheckPermissions(X_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	BPlusTree *tree;
	if (directory->GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	if ((status = tree->Find((uint8 *)file, (uint16)strlen(file), _vnodeID)) < B_OK) {
		//PRINT(("bfs_walk() could not find %Ld:\"%s\": %s\n", directory->BlockNumber(), file, strerror(status)));
		return status;
	}

	RecursiveLocker locker(volume->Lock());
		// we have to hold the volume lock in order to not
		// interfere with new_vnode() here

	Inode *inode;
	if ((status = get_vnode(volume->ID(), *_vnodeID, (void **)&inode)) != B_OK) {
		REPORT_ERROR(status);
		return B_ENTRY_NOT_FOUND;
	}

	*_type = inode->Mode();

	return B_OK;
}


static status_t
bfs_get_vnode_name(fs_volume _fs, fs_vnode _node, char *buffer, size_t bufferSize)
{
	Inode *inode = (Inode *)_node;

	return inode->GetName(buffer, bufferSize);
}


static status_t
bfs_ioctl(void *_fs, void *_node, void *_cookie, ulong cmd, void *buffer, size_t bufferLength)
{
	FUNCTION_START(("node = %p, cmd = %lu, buf = %p, len = %ld\n", _node, cmd, buffer, bufferLength));

	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	switch (cmd) {
		case BFS_IOCTL_VERSION:
		{
			uint32 *version = (uint32 *)buffer;

			*version = 0x10000;
			return B_OK;
		}
		case BFS_IOCTL_START_CHECKING:
		{
			// start checking
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			status_t status = allocator.StartChecking(control);
			if (status == B_OK && inode != NULL)
				inode->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_CHKBFS_RUNNING);

			return status;
		}
		case BFS_IOCTL_STOP_CHECKING:
		{
			// stop checking
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			status_t status = allocator.StopChecking(control);
			if (status == B_OK && inode != NULL)
				inode->Node().flags &= HOST_ENDIAN_TO_BFS_INT32(~INODE_CHKBFS_RUNNING);

			return status;
		}
		case BFS_IOCTL_CHECK_NEXT_NODE:
		{
			// check next
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			return allocator.CheckNextNode(control);
		}
#ifdef DEBUG
		case 56742:
		{
			// allocate all free blocks and zero them out (a test for the BlockAllocator)!
			BlockAllocator &allocator = volume->Allocator();
			Transaction transaction(volume, 0);
			CachedBlock cached(volume);
			block_run run;
			while (allocator.AllocateBlocks(transaction, 8, 0, 64, 1, run) == B_OK) {
				PRINT(("write block_run(%ld, %d, %d)\n", run.allocation_group, run.start, run.length));
				for (int32 i = 0;i < run.length;i++) {
					uint8 *block = cached.SetToWritable(transaction, run);
					if (block != NULL)
						memset(block, 0, volume->BlockSize());
				}
			}
			return B_OK;
		}
		case 56743:
			dump_super_block(&volume->SuperBlock());
			return B_OK;
		case 56744:
			if (inode != NULL)
				dump_inode(&inode->Node());
			return B_OK;
		case 56745:
			if (inode != NULL) {
				NodeGetter node(volume, inode);
				dump_block((const char *)node.Node(), volume->BlockSize());
			}
			return B_OK;
#endif
	}
	return B_BAD_VALUE;
}


/** Sets the open-mode flags for the open file cookie - only
 *	supports O_APPEND currently, but that should be sufficient
 *	for a file system.
 */

static status_t
bfs_set_flags(void *_ns, void *_node, void *_cookie, int flags)
{
	FUNCTION_START(("node = %p, flags = %d", _node, flags));

	file_cookie *cookie = (file_cookie *)_cookie;
	cookie->open_mode = (cookie->open_mode & ~O_APPEND) | (flags & O_APPEND);

	return B_OK;
}


#if 0
static status_t
bfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	FUNCTION_START(("event = %d, ref = %lu, sync = %p\n", event, ref, sync));
	notify_select_event(sync, ref);

	return B_OK;
}


static status_t
bfs_deselect(void *ns, void *node, void *cookie, uint8 event, selectsync *sync)
{
	FUNCTION();
	return B_OK;
}
#endif

static status_t
bfs_fsync(void *_ns, void *_node)
{
	FUNCTION();
	if (_node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	ReadLocked locked(inode->Lock());

	status_t status = locked.IsLocked();
	if (status < B_OK)
		RETURN_ERROR(status);

	return inode->Sync();
}


/**	Fills in the stat struct for a node
 */

static status_t
bfs_read_stat(void *_ns, void *_node, struct stat *stat)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;

	fill_stat_buffer(inode, *stat);
	return B_OK;
}


static status_t
bfs_write_stat(void *_ns, void *_node, const struct stat *stat, uint32 mask)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL || stat == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	// that may be incorrect here - I don't think we need write access to
	// change most of the stat...
	// we should definitely check a bit more if the new stats are correct and valid...
	
	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	WriteLocked locked(inode->Lock());
	if (locked.IsLocked() < B_OK)
		RETURN_ERROR(B_ERROR);

	Transaction transaction(volume, inode->BlockNumber());

	bfs_inode &node = inode->Node();

	if (mask & B_STAT_SIZE) {
		// Since WSTAT_SIZE is the only thing that can fail directly, we
		// do it first, so that the inode state will still be consistent
		// with the on-disk version
		if (inode->IsDirectory())
			return B_IS_A_DIRECTORY;

		if (inode->Size() != stat->st_size) {
			status = inode->SetFileSize(transaction, stat->st_size);
			if (status < B_OK)
				return status;

			// fill the new blocks (if any) with zeros
			inode->FillGapWithZeros(inode->OldSize(), inode->Size());

			if (!inode->IsDeleted()) {
				Index index(volume);
				index.UpdateSize(transaction, inode);

				if ((mask & B_STAT_MODIFICATION_TIME) == 0)
					index.UpdateLastModified(transaction, inode);
			}
		}
	}

	if (mask & B_STAT_MODE) {
		PRINT(("original mode = %ld, stat->st_mode = %d\n", node.Mode(), stat->st_mode));
		node.mode = HOST_ENDIAN_TO_BFS_INT32(node.Mode() & ~S_IUMSK | stat->st_mode & S_IUMSK);
	}

	if (mask & B_STAT_UID)
		node.uid = HOST_ENDIAN_TO_BFS_INT32(stat->st_uid);
	if (mask & B_STAT_GID)
		node.gid = HOST_ENDIAN_TO_BFS_INT32(stat->st_gid);

	if (mask & B_STAT_MODIFICATION_TIME) {
		if (inode->IsDirectory()) {
			// directory modification times are not part of the index
			node.last_modified_time = HOST_ENDIAN_TO_BFS_INT64(
				(bigtime_t)stat->st_mtime << INODE_TIME_SHIFT);
		} else if (!inode->IsDeleted()) {
			// Index::UpdateLastModified() will set the new time in the inode
			Index index(volume);
			index.UpdateLastModified(transaction, inode,
				(bigtime_t)stat->st_mtime << INODE_TIME_SHIFT);
		}
	}
	if (mask & B_STAT_CREATION_TIME) {
		node.create_time = HOST_ENDIAN_TO_BFS_INT64(
			(bigtime_t)stat->st_crtime << INODE_TIME_SHIFT);
	}

	if ((status = inode->WriteBack(transaction)) == B_OK)
		transaction.Done();

	notify_stat_changed(volume->ID(), inode->ID(), mask);

	return status;
}


status_t 
bfs_create(void *_ns, void *_directory, const char *name, int openMode, int mode,
	void **_cookie, vnode_id *vnodeID)
{
	FUNCTION_START(("name = \"%s\", perms = %d, openMode = %d\n", name, mode, openMode));

	if (_ns == NULL || _directory == NULL || _cookie == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	// We are creating the cookie at this point, so that we don't have
	// to remove the inode if we don't have enough free memory later...
	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY); 

	// initialize the cookie
	cookie->open_mode = openMode;
	cookie->last_size = 0;
	cookie->last_notification = system_time();

	Transaction transaction(volume, directory->BlockNumber());

	status_t status = Inode::Create(transaction, directory, name, S_FILE | (mode & S_IUMSK),
		openMode, 0, vnodeID);

	if (status >= B_OK) {
		transaction.Done();

		// register the cookie
		*_cookie = cookie;

		notify_entry_created(volume->ID(), directory->ID(), name, *vnodeID);
	} else
		free(cookie);

	return status;
}


static status_t 
bfs_create_symlink(void *_ns, void *_directory, const char *name, const char *path, int mode)
{
	FUNCTION_START(("name = \"%s\", path = \"%s\"\n", name, path));

	if (_ns == NULL || _directory == NULL || path == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	Transaction transaction(volume, directory->BlockNumber());

	Inode *link;
	off_t id;
	status = Inode::Create(transaction, directory, name, S_SYMLINK | 0777, 0, 0, &id, &link);
	if (status < B_OK)
		RETURN_ERROR(status);

	size_t length = strlen(path);
	if (length < SHORT_SYMLINK_NAME_LENGTH) {
		strcpy(link->Node().short_symlink, path);
	} else {
		link->Node().flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_LONG_SYMLINK | INODE_LOGGED);
		
		// links usually don't have a file cache attached - but we now need one
		link->SetFileCache(file_cache_create(volume->ID(), link->ID(), 0, volume->Device()));

		// The following call will have to write the inode back, so
		// we don't have to do that here...
		status = link->WriteAt(transaction, 0, (const uint8 *)path, &length);
	}

	if (status == B_OK)
		status = link->WriteBack(transaction);

	// Inode::Create() left the inode locked in memory, and also doesn't publish links
	publish_vnode(volume->ID(), id, link);
	put_vnode(volume->ID(), id);

	if (status == B_OK) {
		transaction.Done();

		notify_entry_created(volume->ID(), directory->ID(), name, id);
	}

	return status;
}


status_t 
bfs_link(void *ns, void *dir, const char *name, void *node)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	// This one won't be implemented in a binary compatible BFS

	return B_ERROR;
}


status_t 
bfs_unlink(void *_ns, void *_directory, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (_ns == NULL || _directory == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;
	if (!strcmp(name, "..") || !strcmp(name, "."))
		return B_NOT_ALLOWED;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	if ((status = directory->Remove(transaction, name, &id)) == B_OK) {
		transaction.Done();

		notify_entry_removed(volume->ID(), directory->ID(), name, id);
	}
	return status;
}


status_t 
bfs_rename(void *_ns, void *_oldDir, const char *oldName, void *_newDir, const char *newName)
{
	FUNCTION_START(("oldDir = %p, oldName = \"%s\", newDir = %p, newName = \"%s\"\n", _oldDir, oldName, _newDir, newName));

	// there might be some more tests needed?!
	if (_ns == NULL || _oldDir == NULL || _newDir == NULL
		|| oldName == NULL || *oldName == '\0'
		|| newName == NULL || *newName == '\0'
		|| !strcmp(oldName, ".") || !strcmp(oldName, "..")
		|| !strcmp(newName, ".") || !strcmp(newName, "..")
		|| strchr(newName, '/') != NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *oldDirectory = (Inode *)_oldDir;
	Inode *newDirectory = (Inode *)_newDir;

	// are we already done?
	if (oldDirectory == newDirectory && !strcmp(oldName, newName))
		return B_OK;

	// are we allowed to do what we've been told?
	status_t status = oldDirectory->CheckPermissions(W_OK);
	if (status == B_OK)
		status = newDirectory->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

	RecursiveLocker locker(volume->Lock());

	// get the directory's tree, and a pointer to the inode which should be changed
	BPlusTree *tree;
	status = oldDirectory->GetTree(&tree);
	if (status < B_OK)
		RETURN_ERROR(status);

	off_t id;
	status = tree->Find((const uint8 *)oldName, strlen(oldName), &id);
	if (status < B_OK)
		RETURN_ERROR(status);

	Vnode vnode(volume, id);
	Inode *inode;
	if (vnode.Get(&inode) < B_OK)
		return B_IO_ERROR;

	// Don't move a directory into one of its children - we soar up
	// from the newDirectory to either the root node or the old
	// directory, whichever comes first.
	// If we meet our inode on that way, we have to bail out.

	if (oldDirectory != newDirectory) {
		vnode_id parent = volume->ToVnode(newDirectory->Parent());
		vnode_id root = volume->RootNode()->ID();

		while (true) {
			if (parent == id)
				return B_BAD_VALUE;
			else if (parent == root || parent == oldDirectory->ID())
				break;

			Vnode vnode(volume, parent);
			Inode *parentNode;
			if (vnode.Get(&parentNode) < B_OK)
				return B_ERROR;

			parent = volume->ToVnode(parentNode->Parent());
		}
	}

	// Everything okay? Then lets get to work...

	Transaction transaction(volume, oldDirectory->BlockNumber());

	// First, try to make sure there is nothing that will stop us in
	// the target directory - since this is the only non-critical
	// failure, we will test this case first
	BPlusTree *newTree = tree;
	if (newDirectory != oldDirectory) {
		status = newDirectory->GetTree(&newTree);
		if (status < B_OK)
			RETURN_ERROR(status);
	}

	status = newTree->Insert(transaction, (const uint8 *)newName, strlen(newName), id);
	if (status == B_NAME_IN_USE) {
		// If there is already a file with that name, we have to remove
		// it, as long it's not a directory with files in it
		off_t clobber;
		if (newTree->Find((const uint8 *)newName, strlen(newName), &clobber) < B_OK)
			return B_NAME_IN_USE;
		if (clobber == id)
			return B_BAD_VALUE;

		Vnode vnode(volume, clobber);
		Inode *other;
		if (vnode.Get(&other) < B_OK)
			return B_NAME_IN_USE;

		status = newDirectory->Remove(transaction, newName, NULL, other->IsDirectory());
		if (status < B_OK)
			return status;

		notify_entry_removed(volume->ID(), newDirectory->ID(), newName, clobber);

		status = newTree->Insert(transaction, (const uint8 *)newName, strlen(newName), id);
	}
	if (status < B_OK)
		return status;

	// If anything fails now, we have to remove the inode from the
	// new directory in any case to restore the previous state
	status_t bailStatus = B_OK;

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
		status = tree->Remove(transaction, (const uint8 *)oldName, strlen(oldName), id);
		if (status == B_OK) {
			inode->Parent() = newDirectory->BlockRun();

			// if it's a directory, update the parent directory pointer
			// in its tree if necessary
			BPlusTree *movedTree = NULL;
			if (oldDirectory != newDirectory
				&& inode->IsDirectory()
				&& (status = inode->GetTree(&movedTree)) == B_OK)
				status = movedTree->Replace(transaction, (const uint8 *)"..", 2, newDirectory->ID());

			if (status == B_OK)
				status = inode->WriteBack(transaction);

			if (status == B_OK) {
				transaction.Done();

				notify_entry_moved(volume->ID(), oldDirectory->ID(), oldName,
					newDirectory->ID(), newName, id);
				return B_OK;
			}
			// If we get here, something has gone wrong already!

			// Those better don't fail, or we switch to a read-only
			// device for safety reasons (Volume::Panic() does this
			// for us)
			// Anyway, if we overwrote a file in the target directory
			// this is lost now (only in-memory, not on-disk)...
			bailStatus = tree->Insert(transaction, (const uint8 *)oldName, strlen(oldName), id);
			if (movedTree != NULL)
				movedTree->Replace(transaction, (const uint8 *)"..", 2, oldDirectory->ID());
		}
	}

	if (bailStatus == B_OK && nameUpdated) {
		bailStatus = inode->SetName(transaction, oldName);
		if (status == B_OK) {
			// update inode and index
			inode->WriteBack(transaction);

			Index index(volume);
			index.UpdateName(transaction, newName, oldName, inode);
		}
	}

	if (bailStatus == B_OK)
		bailStatus = newTree->Remove(transaction, (const uint8 *)newName, strlen(newName), id);

	if (bailStatus < B_OK)
		volume->Panic();

	return status;
}


/**	Opens the file with the specified mode.
 */

static status_t
bfs_open(void *_fs, void *_node, int openMode, void **_cookie)
{
	FUNCTION();

	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && openMode & O_RWMASK) {
		openMode = openMode & ~O_RWMASK;
		// ToDo: for compatibility reasons, we don't return an error here...
		// e.g. "copyattr" tries to do that
		//return B_IS_A_DIRECTORY;
	}

	status_t status = inode->CheckPermissions(openModeToAccess(openMode)
		| (openMode & O_TRUNC ? W_OK : 0));
	if (status < B_OK)
		RETURN_ERROR(status);

	// we could actually use the cookie to keep track of:
	//	- the last block_run
	//	- the location in the data_stream (indirect, double indirect,
	//	  position in block_run array)
	//
	// This could greatly speed up continuous reads of big files, especially
	// in the indirect block section.

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY); 

	// initialize the cookie
	cookie->open_mode = openMode;
		// needed by e.g. bfs_write() for O_APPEND
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	// Should we truncate the file?
	if (openMode & O_TRUNC) {
		WriteLocked locked(inode->Lock());
		Transaction transaction(volume, inode->BlockNumber());

		status_t status = inode->SetFileSize(transaction, 0);
		if (status >= B_OK)
			status = inode->WriteBack(transaction);

		if (status < B_OK) {
			// bfs_free_cookie() is only called if this function is successful
			free(cookie);
			return status;
		}

		transaction.Done();
	}

	*_cookie = cookie;
	return B_OK;
}


/**	Read a file specified by node, using information in cookie
 *	and at offset specified by pos. read len bytes into buffer buf.
 */

static status_t
bfs_read(void *_ns, void *_node, void *_cookie, off_t pos, void *buffer, size_t *_length)
{
	//FUNCTION();
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream()) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	ReadLocked locked(inode->Lock());
	return inode->ReadAt(pos, (uint8 *)buffer, _length);
}


static status_t
bfs_write(void *_ns, void *_node, void *_cookie, off_t pos, const void *buffer, size_t *_length)
{
	//FUNCTION();
	// uncomment to be more robust against a buggy vnode layer ;-)
	//if (_ns == NULL || _node == NULL || _cookie == NULL)
	//	return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	if (!inode->HasUserAccessableStream()) {
		*_length = 0;
		RETURN_ERROR(B_BAD_VALUE);
	}

	file_cookie *cookie = (file_cookie *)_cookie;

	if (cookie->open_mode & O_APPEND)
		pos = inode->Size();

	WriteLocked locked(inode->Lock());
	if (locked.IsLocked() < B_OK)
		RETURN_ERROR(B_ERROR);

	Transaction transaction;
		// We are not starting the transaction here, since
		// it might not be needed at all (the contents of
		// regular files aren't logged)

	status_t status = inode->WriteAt(transaction, pos, (const uint8 *)buffer, _length);

	if (status == B_OK)
		transaction.Done();

	if (status == B_OK) {
		// periodically notify if the file size has changed
		// ToDo: should we better test for a change in the last_modified time only?
		if (!inode->IsDeleted() && cookie->last_size != inode->Size()
			&& system_time() > cookie->last_notification + INODE_NOTIFICATION_INTERVAL) {
			notify_stat_changed(volume->ID(), inode->ID(),
				B_STAT_MODIFICATION_TIME | B_STAT_SIZE);
			cookie->last_size = inode->Size();
			cookie->last_notification = system_time();
		}
	}

	return status;
}


/**	Do whatever is necessary to close a file, EXCEPT for freeing
 *	the cookie!
 */

static status_t
bfs_close(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


static status_t
bfs_free_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;

	file_cookie *cookie = (file_cookie *)_cookie;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	WriteLocked locked(inode->Lock());

	bool needsTrimming = inode->NeedsTrimming();

	if ((cookie->open_mode & O_RWMASK) != 0
		&& !inode->IsDeleted()
		&& (needsTrimming
			|| inode->OldLastModified() != inode->LastModified()
			|| inode->OldSize() != inode->Size())) {

		// trim the preallocated blocks and update the size,
		// and last_modified indices if needed

		Transaction transaction(volume, inode->BlockNumber());
		status_t status = B_OK;
		bool changedSize = false, changedTime = false;
		Index index(volume);

		if (needsTrimming) {
			status = inode->TrimPreallocation(transaction);
			if (status < B_OK) {
				FATAL(("Could not trim preallocated blocks: inode %Ld, transaction %ld: %s!\n",
					inode->ID(), transaction.ID(), strerror(status)));

				// we still want this transaction to succeed
				status = B_OK;
			}
		}
		if (needsTrimming || inode->OldSize() != inode->Size()) {
			index.UpdateSize(transaction, inode);
			changedSize = true;
		}
		if (inode->OldLastModified() != inode->LastModified()) {
			index.UpdateLastModified(transaction, inode, inode->LastModified());
			changedTime = true;

			// updating the index doesn't write back the inode
			inode->WriteBack(transaction);
		}

		if (status == B_OK)
			transaction.Done();

		if (changedSize || changedTime) {
			notify_stat_changed(volume->ID(), inode->ID(),
				(changedTime ? B_STAT_MODIFICATION_TIME : 0) | (changedSize ? B_STAT_SIZE : 0));
		}
	}

	if (inode->Flags() & INODE_CHKBFS_RUNNING) {
		// "chkbfs" exited abnormally, so we have to stop it here...
		FATAL(("check process was aborted!\n"));
		volume->Allocator().StopChecking(NULL);
	}

	return B_OK;
}


/**	Checks access permissions, return B_NOT_ALLOWED if the action
 *	is not allowed.
 */

static status_t
bfs_access(void *_ns, void *_node, int accessMode)
{
	//FUNCTION();
	
	if (_ns == NULL || _node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	status_t status = inode->CheckPermissions(accessMode);
	if (status < B_OK)
		RETURN_ERROR(status);

	return B_OK;
}


static status_t
bfs_read_link(void *_ns, void *_node, char *buffer, size_t *_bufferSize)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;
	size_t bufferSize = *_bufferSize;

	if (!inode->IsSymLink())
		RETURN_ERROR(B_BAD_VALUE);

	if (inode->Flags() & INODE_LONG_SYMLINK) {
		// we also need space for the terminating null byte
		if (inode->Size() >= bufferSize) {
			*_bufferSize = inode->Size() + 1;
			return B_BUFFER_OVERFLOW;
		}

		status_t status = inode->ReadAt(0, (uint8 *)buffer, _bufferSize);
		if (status < B_OK)
			RETURN_ERROR(status);

		buffer[++*_bufferSize] = '\0';
		return B_OK;
	}

	*_bufferSize = strlcpy(buffer, inode->Node().short_symlink, bufferSize) + 1;
	if (*_bufferSize > bufferSize)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


//	#pragma mark -
//	Directory functions


static status_t
bfs_create_dir(void *_ns, void *_directory, const char *name, int mode, vnode_id *_newVnodeID)
{
	FUNCTION_START(("name = \"%s\", perms = %d\n", name, mode));

	if (_ns == NULL || _directory == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	Transaction transaction(volume, directory->BlockNumber());

	// Inode::Create() locks the inode if we pass the "id" parameter, but we
	// need it anyway
	off_t id;
	status = Inode::Create(transaction, directory, name, S_DIRECTORY | (mode & S_IUMSK), 0, 0, &id);
	if (status == B_OK) {
		*_newVnodeID = id;
		put_vnode(volume->ID(), id);
		transaction.Done();

		notify_entry_created(volume->ID(), directory->ID(), name, id);
	}

	return status;
}


static status_t
bfs_remove_dir(void *_ns, void *_directory, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	
	if (_ns == NULL || _directory == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	status_t status = directory->Remove(transaction, name, &id, true);
	if (status == B_OK) {
		transaction.Done();

		notify_entry_removed(volume->ID(), directory->ID(), name, id);
	}

	return status;
}


/**	Opens a directory ready to be traversed.
 *	bfs_open_dir() is also used by bfs_open_index_dir().
 */

static status_t
bfs_open_dir(void *_ns, void *_node, void **_cookie)
{
	FUNCTION();
	
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	// we don't ask here for directories only, because the bfs_open_index_dir()
	// function utilizes us (so we must be able to open indices as well)
	if (!inode->IsContainer())
		RETURN_ERROR(B_BAD_VALUE);

	BPlusTree *tree;
	if (inode->GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	TreeIterator *iterator = new TreeIterator(tree);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = iterator;
	return B_OK;
}


static status_t
bfs_read_dir(void *_ns, void *_node, void *_cookie, struct dirent *dirent, 
	size_t bufferSize, uint32 *_num)
{
	FUNCTION();

	TreeIterator *iterator = (TreeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	uint16 length;
	vnode_id id;
	status_t status = iterator->GetNextEntry(dirent->d_name, &length, bufferSize, &id);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK)
		RETURN_ERROR(status);

	Volume *volume = (Volume *)_ns;

	dirent->d_dev = volume->ID();
	dirent->d_ino = id;

	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	return B_OK;
}


/** Sets the TreeIterator back to the beginning of the directory
 */

static status_t
bfs_rewind_dir(void * /*ns*/, void * /*node*/, void *_cookie)
{
	FUNCTION();
	TreeIterator *iterator = (TreeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	return iterator->Rewind();
}


static status_t
bfs_close_dir(void * /*ns*/, void * /*node*/, void * /*_cookie*/)
{
	FUNCTION();
	// Do whatever you need to to close a directory, but DON'T free the cookie!
	return B_OK;
}


static status_t
bfs_free_dir_cookie(void *ns, void *node, void *_cookie)
{
	TreeIterator *iterator = (TreeIterator *)_cookie;
	
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	delete iterator;
	return B_OK;
}


//	#pragma mark -
//	Attribute functions


static status_t
bfs_open_attr_dir(void *_ns, void *_node, void **_cookie)
{
	Inode *inode = (Inode *)_node;

	FUNCTION();

	AttributeIterator *iterator = new AttributeIterator(inode);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*_cookie = iterator;
	return B_OK;
}


static status_t
bfs_close_attr_dir(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_attr_dir_cookie(void *ns, void *node, void *_cookie)
{
	FUNCTION();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	delete iterator;
	return B_OK;
}


static status_t
bfs_rewind_attr_dir(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	
	AttributeIterator *iterator = (AttributeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	RETURN_ERROR(iterator->Rewind());
}


static status_t
bfs_read_attr_dir(void *_ns, void *node, void *_cookie, struct dirent *dirent,
	size_t bufferSize, uint32 *_num)
{
	FUNCTION();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	uint32 type;
	size_t length;
	status_t status = iterator->GetNext(dirent->d_name, &length, &type, &dirent->d_ino);
	if (status == B_ENTRY_NOT_FOUND) {
		*_num = 0;
		return B_OK;
	} else if (status != B_OK) {
		RETURN_ERROR(status);
	}

	Volume *volume = (Volume *)_ns;

	dirent->d_dev = volume->ID();
	dirent->d_reclen = sizeof(struct dirent) + length;

	*_num = 1;
	return B_OK;
}


static status_t
bfs_create_attr(fs_volume _fs, fs_vnode _node, const char *name, uint32 type,
	int openMode, fs_cookie *_cookie)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;
	Attribute attribute(inode);

	return attribute.Create(name, type, openMode, (attr_cookie **)_cookie);
}


static status_t
bfs_open_attr(fs_volume _fs, fs_vnode _node, const char *name, int openMode,
	fs_cookie *_cookie)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;
	Attribute attribute(inode);

	return attribute.Open(name, openMode, (attr_cookie **)_cookie);
}


static status_t
bfs_close_attr(fs_volume _fs, fs_vnode _file, fs_cookie cookie)
{
	return B_OK;
}


static status_t
bfs_free_attr_cookie(fs_volume _fs, fs_vnode _file, fs_cookie cookie)
{
	free(cookie);
	return B_OK;
}


static status_t
bfs_read_attr(fs_volume _fs, fs_vnode _file, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	FUNCTION();

	attr_cookie *cookie = (attr_cookie *)_cookie;
	Inode *inode = (Inode *)_file;

	Attribute attribute(inode, cookie);

	return attribute.Read(cookie, pos, (uint8 *)buffer, _length);
}


static status_t
bfs_write_attr(fs_volume _fs, fs_vnode _file, fs_cookie _cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	FUNCTION();

	attr_cookie *cookie = (attr_cookie *)_cookie;
	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_file;

	Transaction transaction(volume, inode->BlockNumber());
	Attribute attribute(inode, cookie);

	status_t status = attribute.Write(transaction, cookie, pos, (const uint8 *)buffer, _length);
	if (status == B_OK) {
		transaction.Done();

		notify_attribute_changed(volume->ID(), inode->ID(), cookie->name, B_ATTR_CHANGED);
			// ToDo: B_ATTR_CREATED is not yet taken into account
			// (we don't know what Attribute::Write() does exactly)
	}

	return status;
}


static status_t
bfs_read_attr_stat(fs_volume _fs, fs_vnode _file, fs_cookie _cookie, struct stat *stat)
{
	FUNCTION();

	attr_cookie *cookie = (attr_cookie *)_cookie;
	Inode *inode = (Inode *)_file;

	Attribute attribute(inode, cookie);

	return attribute.Stat(*stat);
}


static status_t
bfs_write_attr_stat(fs_volume fs, fs_vnode file, fs_cookie cookie,
	const struct stat *stat, int statMask)
{
	return EOPNOTSUPP;
}


static status_t
bfs_rename_attr(fs_volume _fs, fs_vnode fromFile, const char *fromName,
	fs_vnode toFile, const char *toName)
{
	FUNCTION_START(("name = \"%s\", to = \"%s\"\n", fromName, toName));

	// ToDo: implement bfs_rename_attr()!
	// I'll skip the implementation here, and will do it for Haiku - at least
	// there will be an API to move one attribute to another file, making that
	// function much more complicated - oh joy ;-)

	RETURN_ERROR(B_ENTRY_NOT_FOUND);
}


static status_t
bfs_remove_attr(fs_volume _fs, fs_vnode _node, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (name == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_fs;
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

	Transaction transaction(volume, inode->BlockNumber());

	status = inode->RemoveAttribute(transaction, name);
	if (status == B_OK) {
		transaction.Done();

		notify_attribute_changed(volume->ID(), inode->ID(), name, B_ATTR_REMOVED);
	}

	RETURN_ERROR(status);
}


//	#pragma mark -
//	Index functions


static status_t
bfs_open_index_dir(void *_ns, void **_cookie)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;

	if (volume->IndicesNode() == NULL)
		RETURN_ERROR(B_ENTRY_NOT_FOUND);

	// Since the indices root node is just a directory, and we are storing
	// a pointer to it in our Volume object, we can just use the directory
	// traversal functions.
	// In fact we're storing it in the Volume object for that reason.

	RETURN_ERROR(bfs_open_dir(_ns, volume->IndicesNode(), _cookie));
}


static status_t
bfs_close_index_dir(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_close_dir(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_free_index_dir_cookie(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_free_dir_cookie(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_rewind_index_dir(fs_volume _fs, fs_cookie _cookie)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_rewind_dir(_fs, volume->IndicesNode(), _cookie));
}


static status_t
bfs_read_index_dir(fs_volume _fs, fs_cookie _cookie, struct dirent *dirent,
	size_t bufferSize, uint32 *_num)
{
	FUNCTION();
	if (_fs == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	RETURN_ERROR(bfs_read_dir(_fs, volume->IndicesNode(), _cookie, dirent, bufferSize, _num));
}


static status_t
bfs_create_index(fs_volume _fs, const char *name, uint32 type, uint32 flags)
{
	FUNCTION_START(("name = \"%s\", type = %ld, flags = %ld\n", name, type, flags));
	if (_fs == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_fs;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to create indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	Transaction transaction(volume, volume->Indices());

	Index index(volume);
	status_t status = index.Create(transaction, name, type);

	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_remove_index(void *_ns, const char *name)
{
	FUNCTION();
	if (_ns == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to remove indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	Inode *indices;
	if ((indices = volume->IndicesNode()) == NULL)
		return B_ENTRY_NOT_FOUND;

	Transaction transaction(volume, volume->Indices());

	status_t status = indices->Remove(transaction, name);
	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static status_t
bfs_stat_index(fs_volume _fs, const char *name, struct stat *stat)
{
	FUNCTION_START(("name = %s\n", name));
	if (_fs == NULL || name == NULL || stat == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_fs;
	Index index(volume);
	status_t status = index.SetTo(name);
	if (status < B_OK)
		RETURN_ERROR(status);

	bfs_inode &node = index.Node()->Node();

	stat->st_type = index.Type();
	stat->st_size = node.data.Size();
	stat->st_mode = node.Mode();

	stat->st_nlink = 1;
	stat->st_blksize = 65536;

	stat->st_uid = node.UserID();
	stat->st_gid = node.GroupID();

	stat->st_atime = time(NULL);
	stat->st_mtime = stat->st_ctime = (time_t)(node.LastModifiedTime() >> INODE_TIME_SHIFT);
	stat->st_crtime = (time_t)(node.CreateTime() >> INODE_TIME_SHIFT);

	return B_OK;
}


//	#pragma mark -
//	Query functions


static status_t
bfs_open_query(void *_fs, const char *queryString, uint32 flags, port_id port,
	uint32 token, void **_cookie)
{
	FUNCTION_START(("bfs_open_query(\"%s\", flags = %lu, port_id = %ld, token = %ld)\n",
		queryString, flags, port, token));

	Volume *volume = (Volume *)_fs;

	Expression *expression = new Expression((char *)queryString);
	if (expression == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	if (expression->InitCheck() < B_OK) {
		INFORM(("Could not parse query \"%s\", stopped at: \"%s\"\n",
			queryString, expression->Position()));

		delete expression;
		RETURN_ERROR(B_BAD_VALUE);
	}

	Query *query = new Query(volume, expression, flags);
	if (query == NULL) {
		delete expression;
		RETURN_ERROR(B_NO_MEMORY);
	}

	if (flags & B_LIVE_QUERY)
		query->SetLiveMode(port, token);

	*_cookie = (void *)query;

	return B_OK;
}


static status_t
bfs_close_query(void *fs, void *cookie)
{
	FUNCTION();
	return B_OK;
}


static status_t
bfs_free_query_cookie(void *fs, void *cookie)
{
	FUNCTION();
	if (cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Query *query = (Query *)cookie;
	Expression *expression = query->GetExpression();
	delete query;
	delete expression;

	return B_OK;
}


static status_t
bfs_read_query(void */*fs*/, void *cookie, struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	FUNCTION();
	Query *query = (Query *)cookie;
	if (query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

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
bfs_rewind_query(void */*fs*/, void *cookie)
{
	FUNCTION();
	Query *query = (Query *)cookie;
	if (query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	return query->Rewind();
}


//	#pragma mark -


static status_t
bfs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
#ifdef DEBUG
			add_debugger_commands();
#endif
			return B_OK;
		case B_MODULE_UNINIT:
#ifdef DEBUG
			remove_debugger_commands();
#endif
			return B_OK;

		default:
			return B_ERROR;
	}
}


static file_system_module_info sBeFileSystem = {
	{
		"file_systems/bfs" B_CURRENT_FS_API_VERSION,
		0,
		bfs_std_ops,
	},

	"Be File System",

	// scanning
	bfs_identify_partition,
	bfs_scan_partition,
	bfs_free_identify_partition_cookie,
	NULL,	// free_partition_content_cookie()

	&bfs_mount,
	&bfs_unmount,
	&bfs_read_fs_stat,
	&bfs_write_fs_stat,
	&bfs_sync,

	/* vnode operations */
	&bfs_lookup,
	&bfs_get_vnode_name,
	&bfs_read_vnode,
	&bfs_release_vnode,
	&bfs_remove_vnode,

	/* VM file access */
	&bfs_can_page,
	&bfs_read_pages,
	&bfs_write_pages,

	&bfs_get_file_map,

	&bfs_ioctl,
	&bfs_set_flags,
	NULL,	// &bfs_select
	NULL,	// &bfs_deselect
	&bfs_fsync,

	&bfs_read_link,
	&bfs_create_symlink,

	&bfs_link,
	&bfs_unlink,
	&bfs_rename,

	&bfs_access,
	&bfs_read_stat,
	&bfs_write_stat,

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

module_info *modules[] = {
	(module_info *)&sBeFileSystem,
	NULL,
};
