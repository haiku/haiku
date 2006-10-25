/* kernel_interface - file system interface to BeOS' vnode layer
 *
 * Copyright 2001-2006, Axel Dörfler, axeld@pinc-software.de
 * This file may be used under the terms of the MIT License.
 */


#ifndef COMPILE_FOR_R5
#	define COMPILE_FOR_R5
#endif

#include "Debug.h"
#include "Volume.h"
#include "Inode.h"
#include "Index.h"
#include "BPlusTree.h"
#include "Query.h"
#include "bfs_control.h"

#include <util/kernel_cpp.h>

#include <string.h>
#include <stdio.h>

// BeOS vnode layer stuff
#include <KernelExport.h>
#ifndef _IMPEXP_KERNEL
#	define _IMPEXP_KERNEL
#endif

#include "fsproto.h"
#include "cache.h"

#include <fs_index.h>
#include <fs_query.h>

// needed for bfs_initialize() in the fs_shell only
#ifdef USER
#	include "boot_block.h"
#endif


#ifdef USER
#	define dprintf printf
#endif

// file system API
static int bfs_mount(nspace_id nsid, const char *device, ulong flags,
				void *parms, size_t len, void **data, vnode_id *vnid);
static int bfs_unmount(void *_ns);
static int bfs_read_fs_stat(void *_ns, struct fs_info *);
static int bfs_write_fs_stat(void *ns, struct fs_info *, long mode);
static int bfs_initialize(const char *devname, void *parms, size_t len);

static int bfs_sync(void *ns);

static int bfs_read_vnode(void *_ns, vnode_id vnid, char r, void **node);
static int bfs_release_vnode(void *_ns, void *_node, char r);
static int bfs_remove_vnode(void *ns, void *node, char r);

static int bfs_walk(void *_ns, void *_base, const char *file,
				char **newpath, vnode_id *vnid);
	
static int bfs_ioctl(void *ns, void *node, void *cookie, int cmd, void *buf, size_t len);
static int bfs_setflags(void *ns, void *node, void *cookie, int flags);

static int bfs_select(void *ns, void *node, void *cookie, uint8 event,
				uint32 ref, selectsync *sync);
static int bfs_deselect(void *ns, void *node, void *cookie, uint8 event,
				selectsync *sync);
static int bfs_fsync(void *ns, void *node);

static int bfs_create(void *ns, void *dir, const char *name,
				int perms, int omode, vnode_id *vnid, void **cookie);
static int bfs_symlink(void *ns, void *dir, const char *name,
				const char *path);
static int bfs_link(void *ns, void *dir, const char *name, void *node);
static int bfs_unlink(void *ns, void *dir, const char *name);
static int bfs_rename(void *ns, void *oldDir, const char *oldName, void *newDir, const char *newName);

static int bfs_read_stat(void *_ns, void *_node, struct stat *st);
static int bfs_write_stat(void *ns, void *node, struct stat *st, long mask);

static int bfs_open(void *_ns, void *_node, int omode, void **cookie);
static int bfs_read(void *_ns, void *_node, void *cookie, off_t pos,
				void *buf, size_t *len);
static int bfs_write(void *ns, void *node, void *cookie, off_t pos,
				const void *buf, size_t *len);
static int bfs_free_cookie(void *ns, void *node, void *cookie);
static int bfs_close(void *ns, void *node, void *cookie);
	
static int bfs_access(void *_ns, void *_node, int mode);
static int bfs_read_link(void *_ns, void *_node, char *buffer, size_t *bufferSize);

// directory functions	
static int bfs_mkdir(void *ns, void *dir, const char *name, int perms);
static int bfs_rmdir(void *ns, void *dir, const char *name);
static int bfs_open_dir(void *_ns, void *_node, void **cookie);
static int bfs_read_dir(void *_ns, void *_node, void *cookie,
				long *num, struct dirent *dirent, size_t bufferSize);
static int bfs_rewind_dir(void *_ns, void *_node, void *cookie);
static int bfs_close_dir(void *_ns, void *_node, void *cookie);
static int bfs_free_dir_cookie(void *_ns, void *_node, void *cookie);

// attribute support
static int bfs_open_attrdir(void *ns, void *node, void **cookie);
static int bfs_close_attrdir(void *ns, void *node, void *cookie);
static int bfs_free_attrdir_cookie(void *ns, void *node, void *cookie);
static int bfs_rewind_attrdir(void *ns, void *node, void *cookie);
static int bfs_read_attrdir(void *ns, void *node, void *cookie, long *num,
				struct dirent *buf, size_t bufferSize);
static int bfs_remove_attr(void *ns, void *node, const char *name);
static int bfs_rename_attr(void *ns, void *node, const char *oldname,
				const char *newname);
static int bfs_stat_attr(void *ns, void *node, const char *name,
				struct attr_info *buf);
static int bfs_write_attr(void *ns, void *node, const char *name, int type,
				const void *buf, size_t *len, off_t pos);
static int bfs_read_attr(void *ns, void *node, const char *name, int type,
				void *buf, size_t *len, off_t pos);

// index support
static int bfs_open_indexdir(void *ns, void **cookie);
static int bfs_close_indexdir(void *ns, void *cookie);
static int bfs_free_indexdir_cookie(void *ns, void *node, void *cookie);
static int bfs_rewind_indexdir(void *ns, void *cookie);
static int bfs_read_indexdir(void *ns, void *cookie, long *num, struct dirent *dirent,
				size_t bufferSize);
static int bfs_create_index(void *ns, const char *name, int type, int flags);
static int bfs_remove_index(void *ns, const char *name);
static int bfs_rename_index(void *ns, const char *oldname, const char *newname);
static int bfs_stat_index(void *ns, const char *name, struct index_info *indexInfo);

// query support
static int bfs_open_query(void *ns, const char *query, ulong flags,
				port_id port, long token, void **cookie);
static int bfs_close_query(void *ns, void *cookie);
static int bfs_free_query_cookie(void *ns, void *node, void *cookie);
static int bfs_read_query(void *ns, void *cookie, long *num,
				struct dirent *buf, size_t bufsize);

// Dano compatibility (required for booting)
static int bfs_wake_vnode(void *ns, void *node);
static int bfs_suspend_vnode(void *ns, void *node);


/* vnode_ops struct. Fill this in to tell the kernel how to call
	functions in your driver.
*/

vnode_ops fs_entry =  {
	&bfs_read_vnode,			// read_vnode
	&bfs_release_vnode,			// write_vnode
	&bfs_remove_vnode,			// remove_vnode
	NULL,						// secure_vnode (not needed)
	&bfs_walk,					// walk
	&bfs_access,				// access
	&bfs_create,				// create
	&bfs_mkdir,					// mkdir
	&bfs_symlink,				// symlink
	&bfs_link,					// link
	&bfs_rename,				// rename
	&bfs_unlink,				// unlink
	&bfs_rmdir,					// rmdir
	&bfs_read_link,				// readlink
	&bfs_open_dir,				// opendir
	&bfs_close_dir,				// closedir
	&bfs_free_dir_cookie,		// free_dircookie
	&bfs_rewind_dir,			// rewinddir
	&bfs_read_dir,				// readdir
	&bfs_open,					// open file
	&bfs_close,					// close file
	&bfs_free_cookie,			// free cookie
	&bfs_read,					// read file
	&bfs_write,					// write file
	NULL,						// readv
	NULL,						// writev
	&bfs_ioctl,					// ioctl
	&bfs_setflags,				// setflags file
	&bfs_read_stat,				// read stat
	&bfs_write_stat,			// write stat
	&bfs_fsync,					// fsync
	&bfs_initialize,			// initialize
	&bfs_mount,					// mount
	&bfs_unmount,				// unmount
	&bfs_sync,					// sync
	&bfs_read_fs_stat,			// read fs stat
	&bfs_write_fs_stat,			// write fs stat
	&bfs_select,				// select
	&bfs_deselect,				// deselect

	&bfs_open_indexdir,			// open index dir
	&bfs_close_indexdir,		// close index dir
	&bfs_free_indexdir_cookie,	// free index dir cookie
	&bfs_rewind_indexdir,		// rewind index dir
	&bfs_read_indexdir,			// read index dir
	&bfs_create_index,			// create index
	&bfs_remove_index,			// remove index
	&bfs_rename_index,			// rename index
	&bfs_stat_index,			// stat index

	&bfs_open_attrdir,			// open attr dir
	&bfs_close_attrdir,			// close attr dir
	&bfs_free_attrdir_cookie,	// free attr dir cookie
	&bfs_rewind_attrdir,		// rewind attr dir
	&bfs_read_attrdir,			// read attr dir
	&bfs_write_attr,			// write attr
	&bfs_read_attr,				// read attr
	&bfs_remove_attr,			// remove attr
	&bfs_rename_attr,			// rename attr
	&bfs_stat_attr,				// stat attr

	&bfs_open_query,			// open query
	&bfs_close_query,			// close query
	&bfs_free_query_cookie,		// free query cookie
	&bfs_read_query,			// read query
	
	&bfs_wake_vnode,			// these two are added for Dano compatibility;
	&bfs_suspend_vnode			// they do nothing.
};

#define BFS_IO_SIZE	65536

// ToDo: has to change to "bfs" later
#ifdef BFS_REPLACEMENT 
#	define BFS_NAME	"bfs"
#else
#	define BFS_NAME "obfs"
#endif

int32	api_version = B_CUR_FS_API_VERSION;


static int
bfs_mount(nspace_id nsid, const char *device, ulong flags, void *parms,
		size_t len, void **data, vnode_id *rootID)
{
	FUNCTION();

	Volume *volume = new Volume(nsid);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status;
	if ((status = volume->Mount(device, flags)) == B_OK) {
		*data = volume;
		*rootID = volume->ToVnode(volume->Root());
		INFORM(("mounted \"%s\" (root node at %Ld, device = %s)\n",
			volume->Name(), *rootID, device));

#ifdef DEBUG
		add_debugger_commands();
#endif
	}
	else
		delete volume;

	RETURN_ERROR(status);
}


static int
bfs_unmount(void *ns)
{
	FUNCTION();
	Volume* volume = (Volume *)ns;

	status_t status = volume->Unmount();
	delete volume;

#ifdef DEBUG
	remove_debugger_commands();
#endif
	RETURN_ERROR(status);
}


/**	Fill in bfs_info struct for device.
 */

static int
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
	strlcpy(info->fsh_name, BFS_NAME, sizeof(info->fsh_name));

	return B_OK;
}


static int
bfs_write_fs_stat(void *_ns, struct fs_info *info, long mask)
{
	FUNCTION_START(("mask = %ld\n", mask));

	Volume *volume = (Volume *)_ns;
	disk_super_block &superBlock = volume->SuperBlock();

	RecursiveLocker locker(volume->Lock());

	status_t status = B_BAD_VALUE;

	if (mask & WFSSTAT_NAME) {
		strncpy(superBlock.name, info->volume_name, sizeof(superBlock.name) - 1);
		superBlock.name[sizeof(superBlock.name) - 1] = '\0';

		status = volume->WriteSuperBlock();
	}
	return status;
}


static int
bfs_initialize(const char *deviceName, void *parms, size_t len)
{
	FUNCTION_START(("deviceName = %s, parameter len = %ld\n", deviceName, len));

	// This function is not available from the outside in BeOS
	// It will be similarly implemented in OpenBeOS, though - the
	// backend (to create the file system) is already done; just
	// call Volume::Initialize().

	// When compiling the userland bfs_shell, we provide a useful
	// implementation for this function anyway, so that the bfs_shell can
	// initialize images.

#ifndef USER
	
	return B_ERROR;

#else // USER

	if (deviceName == NULL)
		return B_BAD_VALUE;

	char **argv = (char**)parms;
	
	uint32 blockSize = 1024;
	uint32 flags = 0;
	bool verbose = false;

	while (argv && *++argv) {
		char *arg = *argv;
		if (*arg == '-') {
			if (arg[1] == '-') {
				if (!strcmp(arg, "--noindex"))
					flags |= VOLUME_NO_INDICES;
				else
					return B_BAD_VALUE;
			}

			while (*++arg && *arg >= 'a' && *arg <= 'z') {
				switch (*arg) {
					case 'v':
						verbose = true;
						break;
					case 'b':
						if (*++argv == NULL || !(**argv >= '0' && **argv <= '9'))
							return B_BAD_VALUE;

						blockSize = atol(*argv);
						break;
					case 'n':
						flags |= VOLUME_NO_INDICES;
						break;
				}
			}
		}
		else
			break;
	}

	char *volumeName = "Unnamed";
	if (argv[0] != NULL)
		volumeName = argv[0];

	if (blockSize != 1024 && blockSize != 2048 && blockSize != 4096 && blockSize != 8192) {
		FATAL(("valid block sizes are: 1024, 2048, 4096, and 8192\n"));
		return -1;
	}

	Volume volume(2);
		// we just happen to know it's 2...
	status_t status = volume.Initialize(deviceName, volumeName, blockSize, flags);
	if (status < B_OK) {
		FATAL(("Initializing volume failed: %s\n", strerror(status)));
		return -1;
	}

	if (verbose) {
		disk_super_block super = volume.SuperBlock();

		INFORM(("Disk was initialized successfully.\n"));
		INFORM(("\tname: \"%s\"\n", super.name));
		INFORM(("\tnum blocks: %Ld\n", super.NumBlocks()));
		INFORM(("\tused blocks: %Ld\n", super.UsedBlocks()));
		INFORM(("\tblock size: %lu bytes\n", super.BlockSize()));
		INFORM(("\tnum allocation groups: %ld\n", super.AllocationGroups()));
		INFORM(("\tallocation group size: %ld blocks\n", 1L << super.AllocationGroupShift()));
		INFORM(("\tlog size: %u blocks\n", super.log_blocks.Length()));
	}

	// make the disk image bootable

	int device = open(deviceName, O_RDWR);
	if (device < 0) {
		FATAL(("Could not make image bootable: Failed to open \"%s\"\n",
			deviceName));
		return B_FILE_ERROR;
	}

	// change BIOS drive and partition offset
	// ToDo: for now, this will only work for images only

	sBootBlockData1[kBIOSDriveOffset] = 0x80;
		// for now, this should be replaced by the real thing
	uint32 *offset = (uint32 *)&sBootBlockData1[kPartitionDataOffset];
	*offset = 0;

	write_pos(device, 0, sBootBlockData1, kBootBlockData1Size);
	write_pos(device, kBootBlockData2Offset, sBootBlockData2, kBootBlockData2Size);

	close(device);

	return B_OK;

#endif	// USER
}


static int
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
 *	Has to be cautious if the node in question is currently under
 *	construction, in which case it waits for that action to be completed,
 *	and uses the inode object from the construction instead of creating
 *	a new one.
 *	ToDo: Must not be called without the volume lock being held. Actually,
 *	this might even happen with the BDirectory(node_ref *) constructor
 *	(at least I think so, I haven't tested it yet), so we should better
 *	test this. Fortunately, we can easily solve the issue with our kernel.
 */

static int
bfs_read_vnode(void *_ns, vnode_id id, char reenter, void **_node)
{
	FUNCTION_START(("vnode_id = %Ld\n", id));
	Volume *volume = (Volume *)_ns;

	if (id < 0 || id > volume->NumBlocks()) {
		FATAL(("inode at %Ld requested!\n", id));
		return B_ERROR;
	}

	CachedBlock cached(volume, id);

	bfs_inode *node = (bfs_inode *)cached.Block();
	Inode *inode = NULL;
	int32 tries = 0;

restartIfBusy:
	status_t status = node->InitCheck(volume);

	if (status == B_BUSY) {
		inode = (Inode *)node->etc;
			// We have to use the "etc" field to get the inode object
			// (the inode is currently being constructed)
			// We would need to call something like get_vnode() again here
			// to get rid of the "etc" field - which would be nice, especially
			// for other file systems which don't have this messy field.

		// let us wait a bit and try again later
		if (tries++ < 200) {
			// wait for one second at maximum
			snooze(5000);
			goto restartIfBusy;
		}
		FATAL(("inode is not becoming unbusy (id = %Ld)\n", id));
		return status;
	} else if (status < B_OK) {
		FATAL(("inode at %Ld is corrupt!\n", id));
		return status;
	}

	// If the inode is currently being constructed, we already have an inode
	// pointer (taken from the node's etc field).
	// If not, we create a new one here

	if (inode == NULL) {
		inode = new Inode(&cached);
		if (inode == NULL)
			return B_NO_MEMORY;

		status = inode->InitCheck(false);
		if (status < B_OK)
			delete inode;
	} else
		status = inode->InitCheck(false);

	if (status == B_OK)
		*_node = inode;

	return status;
}


static int
bfs_release_vnode(void *ns, void *_node, char reenter)
{
	//FUNCTION_START(("node = %p\n", _node));
	Inode *inode = (Inode *)_node;

	delete inode;

	return B_NO_ERROR;
}


static int
bfs_remove_vnode(void *_ns, void *_node, char reenter)
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
	Transaction localTransaction, *transaction = NULL;

	Journal *journal = volume->GetJournal(volume->ToBlock(inode->Parent()));
	if (journal != NULL)
		transaction = journal->CurrentTransaction();

	if (transaction == NULL) {
		transaction = &localTransaction;
		localTransaction.Start(volume, inode->BlockNumber());
	}

	status_t status = inode->Free(transaction);
	if (status == B_OK) {
		if (transaction == &localTransaction)
			localTransaction.Done();

		delete inode;
	}

	return status;
}


static int
bfs_wake_vnode(void *_ns, void *_node)
{
	return B_OK;
}


static int
bfs_suspend_vnode(void *_ns, void *_node)
{
	return B_OK;
}


//	#pragma mark -


/**	the walk function just "walks" through a directory looking for the
 *	specified file. It calls get_vnode() on its vnode-id to init it
 *	for the kernel.
 */

static int
bfs_walk(void *_ns, void *_directory, const char *file, char **_resolvedPath, vnode_id *_vnodeID)
{
	//FUNCTION_START(("file = %s\n", file));
	if (_ns == NULL || _directory == NULL || file == NULL || _vnodeID == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	// check access permissions
	status_t status = directory->CheckPermissions(R_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	BPlusTree *tree;
	if (directory->GetTree(&tree) != B_OK)
		RETURN_ERROR(B_BAD_VALUE);

	if ((status = tree->Find((uint8 *)file, (uint16)strlen(file), _vnodeID)) < B_OK) {
		PRINT(("bfs_walk() could not find %Ld:\"%s\": %s\n", directory->BlockNumber(), file, strerror(status)));
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

	// Is inode a symlink? Then resolve it, if we should

	if (inode->IsSymLink() && _resolvedPath != NULL) {
		status_t status = B_OK;
		char *newPath = NULL;
		
		// Symbolic links can store their target in the data stream (for links
		// that take more than 144 bytes of storage [the size of the data_stream
		// structure]), or directly instead of the data_stream class
		// So we have to deal with both cases here.
		
		// Note: we would naturally call bfs_read_link() here, but the API of the
		// vnode layer would require us to always reserve a large chunk of memory
		// for the path, so we're not going to do that

		if (inode->Flags() & INODE_LONG_SYMLINK) {
			size_t readBytes = inode->Size();
			char *data = (char *)malloc(readBytes);
			if (data != NULL) {
				status = inode->ReadAt(0, (uint8 *)data, &readBytes);
				if (status == B_OK && readBytes == inode->Size())
					status = new_path(data, &newPath);

				free(data);
			} else
				status = B_NO_MEMORY;
		} else
			status = new_path((char *)&inode->Node()->short_symlink, &newPath);

		put_vnode(volume->ID(), inode->ID());
		if (status == B_OK)
			*_resolvedPath = newPath;

		RETURN_ERROR(status);
	}

	return B_OK;
}


static int
bfs_ioctl(void *_ns, void *_node, void *_cookie, int cmd, void *buffer, size_t bufferLength)
{
	FUNCTION_START(("node = %p, cmd = %d, buf = %p, len = %ld\n", _node, cmd, buffer, bufferLength));

	if (_ns == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	switch (cmd) {
		case IOCTL_FILE_UNCACHED_IO:
		{
			if (inode == NULL)
				return B_BAD_VALUE;

			// if the inode is already set up for uncached access, bail out
			if (inode->Flags() & INODE_NO_CACHE) {
				FATAL(("File %Ld is already uncached\n", inode->ID()));
				return B_ERROR;
			}

			PRINT(("uncached access to inode %Ld\n", inode->ID()));

			// ToDo: sync the cache for this file!
			// Unfortunately, we can't remove any blocks from the cache in BeOS,
			// that means we can't guarantee consistency for the file contents
			// spanning over both access modes.

			// request buffers for being able to access the file without
			// using the cache or allocating memory
			status_t status = volume->Pool().RequestBuffers(volume->BlockSize());
			if (status == B_OK)
				inode->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_NO_CACHE);
			return status;
		}
		case IOCTL_CREATE_TIME:
		{
			if (inode == NULL || buffer == NULL)
				return B_BAD_VALUE;

			off_t *creationTime = (off_t *)buffer;
			*creationTime = inode->Node()->CreateTime();
			return B_OK;
		}
		case IOCTL_MODIFIED_TIME:
		{
			if (inode == NULL || buffer == NULL)
				return B_BAD_VALUE;

			off_t *modifiedTime = (off_t *)buffer;
			*modifiedTime = inode->LastModified();
			return B_OK;
		}
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
				inode->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_CHKBFS_RUNNING);

			return status;
		}
		case BFS_IOCTL_STOP_CHECKING:
		{
			// stop checking
			BlockAllocator &allocator = volume->Allocator();
			check_control *control = (check_control *)buffer;

			status_t status = allocator.StopChecking(control);
			if (status == B_OK && inode != NULL)
				inode->Node()->flags &= HOST_ENDIAN_TO_BFS_INT32(~INODE_CHKBFS_RUNNING);

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
			while (allocator.AllocateBlocks(&transaction, 8, 0, 64, 1, run) == B_OK) {
				PRINT(("write block_run(%ld, %d, %d)\n", run.allocation_group, run.start, run.length));
				for (int32 i = 0;i < run.length;i++) {
					uint8 *block = cached.SetTo(run);
					if (block != NULL) {
						memset(block, 0, volume->BlockSize());
						cached.WriteBack(&transaction);
					}
				}
			}
			return B_OK;
		}
		case 56743:
			dump_super_block(&volume->SuperBlock());
			return B_OK;
		case 56744:
			if (inode != NULL)
				dump_inode(inode->Node());
			return B_OK;
		case 56745:
			if (inode != NULL)
				dump_block((const char *)inode->Node(), volume->BlockSize());
			return B_OK;
#endif
	}
	return B_BAD_VALUE;
}


/** Sets the open-mode flags for the open file cookie - only
 *	supports O_APPEND currently, but that should be sufficient
 *	for a file system.
 */

static int
bfs_setflags(void *_ns, void *_node, void *_cookie, int flags)
{
	FUNCTION_START(("node = %p, flags = %d", _node, flags));

	file_cookie *cookie = (file_cookie *)_cookie;
	cookie->open_mode = (cookie->open_mode & ~O_APPEND) | (flags & O_APPEND);

	return B_OK;
}


int 
bfs_select(void *ns, void *node, void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	FUNCTION_START(("event = %d, ref = %lu, sync = %p\n", event, ref, sync));
	notify_select_event(sync, ref);

	return B_OK;
}


static int
bfs_deselect(void *ns, void *node, void *cookie, uint8 event, selectsync *sync)
{
	FUNCTION();
	return B_OK;
}


static int
bfs_fsync(void *_ns, void *_node)
{
	FUNCTION();
	if (_node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	return inode->Sync();
}


/**	Fills in the stat struct for a node
 */

static int
bfs_read_stat(void *_ns, void *_node, struct stat *st)
{
	FUNCTION();

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;
	bfs_inode *node = inode->Node();

	st->st_dev = volume->ID();
	st->st_ino = inode->ID();
	st->st_nlink = 1;
	st->st_blksize = BFS_IO_SIZE;

	st->st_uid = node->UserID();
	st->st_gid = node->GroupID();
	st->st_mode = node->Mode();
	st->st_size = node->data.Size();

	st->st_atime = time(NULL);
	st->st_mtime = st->st_ctime = (time_t)(node->LastModifiedTime() >> INODE_TIME_SHIFT);
	st->st_crtime = (time_t)(node->CreateTime() >> INODE_TIME_SHIFT);

	return B_NO_ERROR;
}


static int
bfs_write_stat(void *_ns, void *_node, struct stat *stat, long mask)
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

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif

	WriteLocked locked(inode->Lock());
	if (locked.IsLocked() < B_OK)
		RETURN_ERROR(B_ERROR);

	Transaction transaction(volume, inode->BlockNumber());

	bfs_inode *node = inode->Node();

	if (mask & WSTAT_SIZE) {
		// Since WSTAT_SIZE is the only thing that can fail directly, we
		// do it first, so that the inode state will still be consistent
		// with the on-disk version
		if (inode->IsDirectory())
			return B_IS_A_DIRECTORY;

		if (inode->Size() != stat->st_size) {
			status = inode->SetFileSize(&transaction, stat->st_size);
			if (status < B_OK)
				return status;

			// fill the new blocks (if any) with zeros
			inode->FillGapWithZeros(inode->OldSize(), inode->Size());

			Index index(volume);
			index.UpdateSize(&transaction, inode);

			if ((mask & WSTAT_MTIME) == 0)
				index.UpdateLastModified(&transaction, inode);
		}
	}

	if (mask & WSTAT_MODE) {
		PRINT(("original mode = %ld, stat->st_mode = %d\n", node->mode, stat->st_mode));
		node->mode = node->mode & ~S_IUMSK | stat->st_mode & S_IUMSK;
	}

	if (mask & WSTAT_UID)
		node->uid = stat->st_uid;
	if (mask & WSTAT_GID)
		node->gid = stat->st_gid;

	if (mask & WSTAT_MTIME) {
		if (inode->IsDirectory()) {
			// directory modification times are not part of the index
			node->last_modified_time = HOST_ENDIAN_TO_BFS_INT64(
				(bigtime_t)stat->st_mtime << INODE_TIME_SHIFT);
		} else if (!inode->IsDeleted()) {
			// Index::UpdateLastModified() will set the new time in the inode
			Index index(volume);
			index.UpdateLastModified(&transaction, inode,
				(bigtime_t)stat->st_mtime << INODE_TIME_SHIFT);
		}
	}
	if (mask & WSTAT_CRTIME) {
		node->create_time = HOST_ENDIAN_TO_BFS_INT64(
			(bigtime_t)stat->st_crtime << INODE_TIME_SHIFT);
	}

	if ((status = inode->WriteBack(&transaction)) == B_OK)
		transaction.Done();

	notify_listener(B_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL);

	return status;
}


int 
bfs_create(void *_ns, void *_directory, const char *name, int omode, int mode,
	vnode_id *vnodeID, void **_cookie)
{
	FUNCTION_START(("name = \"%s\", perms = %d, omode = %d\n", name, mode, omode));

	if (_ns == NULL || _directory == NULL || _cookie == NULL
		|| name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

	if (!directory->IsDirectory())
		RETURN_ERROR(B_BAD_TYPE);

	status_t status = directory->CheckPermissions(W_OK);
	if (status < B_OK)
		RETURN_ERROR(status);

	// We are creating the cookie at this point, so that we don't have
	// to remove the inode if we don't have enough free memory later...
	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		RETURN_ERROR(B_NO_MEMORY); 

	// initialize the cookie
	cookie->open_mode = omode;
	cookie->last_size = 0;
	cookie->last_notification = system_time();

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	status = Inode::Create(&transaction, directory, name, S_FILE | (mode & S_IUMSK),
		omode, 0, vnodeID);

	if (status >= B_OK) {
		transaction.Done();

		// register the cookie
		*_cookie = cookie;

		notify_listener(B_ENTRY_CREATED, volume->ID(), directory->ID(), 0, *vnodeID, name);
	} else
		free(cookie);

	return status;
}


int 
bfs_symlink(void *_ns, void *_directory, const char *name, const char *path)
{
	FUNCTION();

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

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	Inode *link;
	off_t id;
	status = Inode::Create(&transaction, directory, name, S_SYMLINK | 0777, 0, 0, &id, &link);
	if (status < B_OK)
		RETURN_ERROR(status);

	size_t length = strlen(path);
	if (length < SHORT_SYMLINK_NAME_LENGTH) {
		strcpy(link->Node()->short_symlink, path);
		status = link->WriteBack(&transaction);
	} else {
		link->Node()->flags |= HOST_ENDIAN_TO_BFS_INT32(INODE_LONG_SYMLINK | INODE_LOGGED);
		// The following call will have to write the inode back, so
		// we don't have to do that here...
		status = link->WriteAt(&transaction, 0, (const uint8 *)path, &length);
	}
	// ToDo: would be nice if Inode::Create() would let the INODE_NOT_READY
	//	flag set until here, so that it can be accessed directly

	// Inode::Create() left the inode locked in memory
	put_vnode(volume->ID(), id);

	if (status == B_OK) {
		transaction.Done();

		notify_listener(B_ENTRY_CREATED, volume->ID(), directory->ID(), 0, id, name);
	}

	return status;
}


int 
bfs_link(void *ns, void *dir, const char *name, void *node)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	// This one won't be implemented in a binary compatible BFS

	return B_ERROR;
}


int 
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

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	if ((status = directory->Remove(&transaction, name, &id)) == B_OK) {
		transaction.Done();

		notify_listener(B_ENTRY_REMOVED, volume->ID(), directory->ID(), 0, id, NULL);
	}
	return status;
}


int 
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

	status = newTree->Insert(&transaction, (const uint8 *)newName, strlen(newName), id);
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

		status = newDirectory->Remove(&transaction, newName, NULL, other->IsDirectory());
		if (status < B_OK)
			return status;

		notify_listener(B_ENTRY_REMOVED, volume->ID(), newDirectory->ID(), 0, clobber, NULL);

		status = newTree->Insert(&transaction, (const uint8 *)newName, strlen(newName), id);
	}
	if (status < B_OK)
		return status;

	// If anything fails now, we have to remove the inode from the
	// new directory in any case to restore the previous state
	status_t bailStatus = B_OK;

	// update the name only when they differ
	bool nameUpdated = false;
	if (strcmp(oldName, newName)) {
		status = inode->SetName(&transaction, newName);
		if (status == B_OK) {
			Index index(volume);
			index.UpdateName(&transaction, oldName, newName, inode);
			nameUpdated = true;
		}
	}

	if (status == B_OK) {
		status = tree->Remove(&transaction, (const uint8 *)oldName, strlen(oldName), id);
		if (status == B_OK) {
			inode->Node()->parent = newDirectory->BlockRun();

			// if it's a directory, update the parent directory pointer
			// in its tree if necessary
			BPlusTree *movedTree = NULL;
			if (oldDirectory != newDirectory
				&& inode->IsDirectory()
				&& (status = inode->GetTree(&movedTree)) == B_OK)
				status = movedTree->Replace(&transaction, (const uint8 *)"..", 2, newDirectory->ID());

			if (status == B_OK) {
				status = inode->WriteBack(&transaction);
				if (status == B_OK)	{
					transaction.Done();

					notify_listener(B_ENTRY_MOVED, volume->ID(), oldDirectory->ID(),
						newDirectory->ID(), id, newName);
					return B_OK;
				}
			}
			// If we get here, something has gone wrong already!

			// Those better don't fail, or we switch to a read-only
			// device for safety reasons (Volume::Panic() does this
			// for us)
			// Anyway, if we overwrote a file in the target directory
			// this is lost now (only in-memory, not on-disk)...
			bailStatus = tree->Insert(&transaction, (const uint8 *)oldName, strlen(oldName), id);
			if (movedTree != NULL)
				movedTree->Replace(&transaction, (const uint8 *)"..", 2, oldDirectory->ID());
		}
	}

	if (bailStatus == B_OK && nameUpdated) {
		bailStatus = inode->SetName(&transaction, oldName);
		if (status == B_OK) {
			// update inode and index
			inode->WriteBack(&transaction);

			Index index(volume);
			index.UpdateName(&transaction, newName, oldName, inode);
		}
	}

	if (bailStatus == B_OK)
		bailStatus = newTree->Remove(&transaction, (const uint8 *)newName, strlen(newName), id);

	if (bailStatus < B_OK)
		volume->Panic();

	return status;
}


/**	Opens the file with the specified mode.
 */

static int
bfs_open(void *_ns, void *_node, int omode, void **_cookie)
{
	FUNCTION();
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	// we can't open a file which uses uncached access twice
	if (inode->Flags() & INODE_NO_CACHE)
		return B_BUSY;

	// opening a directory read-only is allowed, although you can't read
	// any data from it.
	if (inode->IsDirectory() && omode & O_RWMASK) {
		omode = omode & ~O_RWMASK;
		// ToDo: for compatibility reasons, we don't return an error here...
		// e.g. "copyattr" tries to do that
		//return B_IS_A_DIRECTORY;
	}

	status_t status = inode->CheckPermissions(oModeToAccess(omode));
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
	cookie->open_mode = omode;
		// needed by e.g. bfs_write() for O_APPEND
	cookie->last_size = inode->Size();
	cookie->last_notification = system_time();

	// Should we truncate the file?
	if (omode & O_TRUNC) {
		WriteLocked locked(inode->Lock());
		Transaction transaction(volume, inode->BlockNumber());

		status_t status = inode->SetFileSize(&transaction, 0);
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

static int
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


int 
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

	status_t status = inode->WriteAt(&transaction, pos, (const uint8 *)buffer, _length);

	if (status == B_OK)
		transaction.Done();

	if (status == B_OK && (inode->Flags() & INODE_NO_CACHE) == 0) {
		// uncached files don't cause notifications during access, and
		// never want to write back any cached blocks

		// periodically notify if the file size has changed
		// ToDo: should we better test for a change in the last_modified time only?
		if (cookie->last_size != inode->Size()
			&& system_time() > cookie->last_notification + INODE_NOTIFICATION_INTERVAL) {
			notify_listener(B_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL);
			cookie->last_size = inode->Size();
			cookie->last_notification = system_time();
		}

		// This will flush the dirty blocks to disk from time to time.
		// It's done here and not in Inode::WriteAt() so that it won't
		// add to the duration of a transaction - it might even be a
		// good idea to offload those calls to another thread
		volume->WriteCachedBlocksIfNecessary();
	}

	return status;
}


/**	Do whatever is necessary to close a file, EXCEPT for freeing
 *	the cookie!
 */

static int
bfs_close(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;

	return B_OK;
}


static int
bfs_free_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();

	if (_ns == NULL || _node == NULL || _cookie == NULL)
		return B_BAD_VALUE;

	file_cookie *cookie = (file_cookie *)_cookie;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	if (cookie->open_mode & O_RWMASK) {
#ifdef UNSAFE_GET_VNODE
		RecursiveLocker locker(volume->Lock());
#endif
		ReadLocked locked(inode->Lock());

		// trim the preallocated blocks and update the size,
		// and last_modified indices if needed

		Transaction transaction(volume, inode->BlockNumber());
		status_t status = B_OK;
		bool changed = false;
		Index index(volume);

		if (inode->OldSize() != inode->Size()) {
			status = inode->Trim(&transaction);
			if (status < B_OK)
				FATAL(("Could not trim preallocated blocks!"));
	
			index.UpdateSize(&transaction, inode);
			changed = true;
		}
		if (inode->OldLastModified() != inode->LastModified()) {
			index.UpdateLastModified(&transaction, inode, inode->LastModified());
			changed = true;
			
			// updating the index doesn't write back the inode
			inode->WriteBack(&transaction);
		}

		if (status == B_OK)
			transaction.Done();

		if (changed)
			notify_listener(B_STAT_CHANGED, volume->ID(), 0, 0, inode->ID(), NULL);
	}

	if (inode->Flags() & INODE_NO_CACHE) {
		volume->Pool().ReleaseBuffers();
		inode->Node()->flags &= HOST_ENDIAN_TO_BFS_INT32(~INODE_NO_CACHE);
			// We don't need to save the inode, because INODE_NO_CACHE is a
			// non-permanent flag which will be removed when the inode is loaded
			// into memory.
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

static int
bfs_access(void *_ns, void *_node, int accessMode)
{
	FUNCTION();
	
	if (_ns == NULL || _node == NULL)
		return B_BAD_VALUE;

	Inode *inode = (Inode *)_node;
	status_t status = inode->CheckPermissions(accessMode);
	if (status < B_OK)
		RETURN_ERROR(status);

	return B_OK;
}


static int
bfs_read_link(void *_ns, void *_node, char *buffer, size_t *bufferSize)
{
	FUNCTION();

	Inode *inode = (Inode *)_node;
	
	if (!inode->IsSymLink())
		RETURN_ERROR(B_BAD_VALUE);

	if (inode->Flags() & INODE_LONG_SYMLINK) {
		status_t status = inode->ReadAt(0, (uint8 *)buffer, bufferSize);
		if (status < B_OK)
			RETURN_ERROR(status);
		
		*bufferSize = inode->Size();
		return B_OK;
	}

	size_t numBytes = strlen((char *)&inode->Node()->short_symlink);
	uint32 bytes = numBytes;
	if (bytes > *bufferSize)
		bytes = *bufferSize;

	memcpy(buffer, inode->Node()->short_symlink, bytes);
	*bufferSize = numBytes;

	return B_OK;
}


//	#pragma mark -
//	Directory functions


static int
bfs_mkdir(void *_ns, void *_directory, const char *name, int mode)
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

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	// Inode::Create() locks the inode if we pass the "id" parameter, but we
	// need it anyway
	off_t id;
	status = Inode::Create(&transaction, directory, name, S_DIRECTORY | (mode & S_IUMSK), 0, 0, &id);
	if (status == B_OK) {
		put_vnode(volume->ID(), id);
		transaction.Done();

		notify_listener(B_ENTRY_CREATED, volume->ID(), directory->ID(), 0, id, name);
	}

	return status;
}


static int
bfs_rmdir(void *_ns, void *_directory, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	
	if (_ns == NULL || _directory == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *directory = (Inode *)_directory;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, directory->BlockNumber());

	off_t id;
	status_t status = directory->Remove(&transaction, name, &id, true);
	if (status == B_OK) {
		transaction.Done();

		notify_listener(B_ENTRY_REMOVED, volume->ID(), directory->ID(), 0, id, NULL);
	}

	return status;
}


/**	creates fs-specific "cookie" struct that keeps track of where
 *	you are at in reading through directory entries in bfs_readdir.
 */

static int
bfs_open_dir(void *_ns, void *_node, void **_cookie)
{
	FUNCTION();
	
	if (_ns == NULL || _node == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	Inode *inode = (Inode *)_node;

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


static int
bfs_read_dir(void *_ns, void *_node, void *_cookie, long *num, 
	struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();

	TreeIterator *iterator = (TreeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	uint16 length;
	vnode_id id;
	status_t status = iterator->GetNextEntry(dirent->d_name, &length, bufferSize, &id);
	if (status == B_ENTRY_NOT_FOUND) {
		*num = 0;
		return B_OK;
	} else if (status != B_OK)
		RETURN_ERROR(status);

	Volume *volume = (Volume *)_ns;

	dirent->d_dev = volume->ID();
	dirent->d_ino = id;

#ifdef KEEP_WRONG_DIRENT_RECLEN
	dirent->d_reclen = length;
#else
	dirent->d_reclen = sizeof(struct dirent) + length;
#endif

	*num = 1;
	return B_OK;
}


/** Sets the TreeIterator back to the beginning of the directory
 */

static int
bfs_rewind_dir(void * /*ns*/, void * /*node*/, void *_cookie)
{
	FUNCTION();
	TreeIterator *iterator = (TreeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);
	
	return iterator->Rewind();
}


static int
bfs_close_dir(void * /*ns*/, void * /*node*/, void * /*_cookie*/)
{
	FUNCTION();
	// Do whatever you need to to close a directory, but DON'T free the cookie!
	return B_OK;
}


static int
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


static int
bfs_open_attrdir(void *_ns, void *_node, void **cookie)
{
	FUNCTION();
	
	Inode *inode = (Inode *)_node;
	if (inode == NULL || inode->Node() == NULL)
		RETURN_ERROR(B_ERROR);

	AttributeIterator *iterator = new AttributeIterator(inode);
	if (iterator == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	*cookie = iterator;
	return B_OK;
}


static int
bfs_close_attrdir(void *ns, void *node, void *cookie)
{
	FUNCTION();
	return B_OK;
}


static int
bfs_free_attrdir_cookie(void *ns, void *node, void *_cookie)
{
	FUNCTION();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	delete iterator;
	return B_OK;
}


static int
bfs_rewind_attrdir(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	
	AttributeIterator *iterator = (AttributeIterator *)_cookie;
	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	RETURN_ERROR(iterator->Rewind());
}


static int
bfs_read_attrdir(void *_ns, void *node, void *_cookie, long *num, struct dirent *dirent, size_t bufsize)
{
	FUNCTION();
	AttributeIterator *iterator = (AttributeIterator *)_cookie;

	if (iterator == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	uint32 type;
	size_t length;
	status_t status = iterator->GetNext(dirent->d_name, &length, &type, &dirent->d_ino);
	if (status == B_ENTRY_NOT_FOUND) {
		*num = 0;
		return B_OK;
	} else if (status != B_OK)
		RETURN_ERROR(status);

	Volume *volume = (Volume *)_ns;

	dirent->d_dev = volume->ID();
#ifdef KEEP_WRONG_DIRENT_RECLEN
	dirent->d_reclen = length;
#else
	dirent->d_reclen = sizeof(struct dirent) + length;
#endif

	*num = 1;
	return B_OK;
}


static int
bfs_remove_attr(void *_ns, void *_node, const char *name)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	if (_ns == NULL || _node == NULL || name == NULL)
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, inode->BlockNumber());

	status = inode->RemoveAttribute(&transaction, name);
	if (status == B_OK) {
		transaction.Done();

		notify_listener(B_ATTR_CHANGED, volume->ID(), 0, 0, inode->ID(), name);
	}

	RETURN_ERROR(status);
}


static int
bfs_rename_attr(void *ns, void *node, const char *oldname, const char *newname)
{
	FUNCTION_START(("name = \"%s\", to = \"%s\"\n", oldname, newname));

	// ToDo: implement bfs_rename_attr()!
	// I'll skip the implementation here, and will do it for OpenBeOS - at least
	// there will be an API to move one attribute to another file, making that
	// function much more complicated - oh joy ;-)

	RETURN_ERROR(B_ENTRY_NOT_FOUND);
}


static int
bfs_stat_attr(void *ns, void *_node, const char *name, struct attr_info *attrInfo)
{
	FUNCTION_START(("name = \"%s\"\n", name));

	Inode *inode = (Inode *)_node;
	if (inode == NULL || inode->Node() == NULL)
		RETURN_ERROR(B_ERROR);

	// first, try to find it in the small data region
	small_data *smallData = NULL;
	if (inode->SmallDataLock().Lock() == B_OK) {
		if ((smallData = inode->FindSmallData((const char *)name)) != NULL) {
			attrInfo->type = smallData->Type();
			attrInfo->size = smallData->DataSize();
		}
		inode->SmallDataLock().Unlock();
	}
	if (smallData != NULL)
		return B_OK;

	// then, search in the attribute directory
	Inode *attribute;
	status_t status = inode->GetAttribute(name, &attribute);
	if (status == B_OK) {
		attrInfo->type = attribute->Type();
		attrInfo->size = attribute->Size();

		inode->ReleaseAttribute(attribute);
		return B_OK;
	}

	RETURN_ERROR(status);
}


static int
bfs_write_attr(void *_ns, void *_node, const char *name, int type, const void *buffer,
	size_t *_length, off_t pos)
{
	FUNCTION_START(("name = \"%s\"\n", name));
	if (_ns == NULL || _node == NULL || name == NULL || *name == '\0')
		RETURN_ERROR(B_BAD_VALUE);

	// Writing the name attribute using this function is not allowed,
	// also using the reserved indices name, last_modified, and size
	// shouldn't be allowed.
	// ToDo: we might think about allowing to update those values, but
	//	really change their corresponding values in the bfs_inode structure
	if (name[0] == FILE_NAME_NAME && name[1] == '\0'
		|| !strcmp(name, "name")
		|| !strcmp(name, "last_modified")
		|| !strcmp(name, "size"))
		RETURN_ERROR(B_NOT_ALLOWED);

	Volume *volume = (Volume *)_ns;
	Inode *inode = (Inode *)_node;

	status_t status = inode->CheckPermissions(W_OK);
	if (status < B_OK)
		return status;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, inode->BlockNumber());

	status = inode->WriteAttribute(&transaction, name, type, pos, (const uint8 *)buffer, _length);
	if (status == B_OK) {
		transaction.Done();

		notify_listener(B_ATTR_CHANGED, volume->ID(), 0, 0, inode->ID(), name);
	}

	return status;
}


static int
bfs_read_attr(void *_ns, void *_node, const char *name, int type, void *buffer,
	size_t *_length, off_t pos)
{
	FUNCTION();
	Inode *inode = (Inode *)_node;

	if (inode == NULL || name == NULL || *name == '\0' || buffer == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status_t status = inode->CheckPermissions(R_OK);
	if (status < B_OK)
		return status;

	return inode->ReadAttribute(name, type, pos, (uint8 *)buffer, _length);
}


//	#pragma mark -
//	Index functions


static int
bfs_open_indexdir(void *_ns, void **_cookie)
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


static int
bfs_close_indexdir(void *_ns, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	RETURN_ERROR(bfs_close_dir(_ns, volume->IndicesNode(), _cookie));
}


static int
bfs_free_indexdir_cookie(void *_ns, void *_node, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	RETURN_ERROR(bfs_free_dir_cookie(_ns, volume->IndicesNode(), _cookie));
}


static int
bfs_rewind_indexdir(void *_ns, void *_cookie)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	RETURN_ERROR(bfs_rewind_dir(_ns, volume->IndicesNode(), _cookie));
}


static int
bfs_read_indexdir(void *_ns, void *_cookie, long *num, struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();
	if (_ns == NULL || _cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	RETURN_ERROR(bfs_read_dir(_ns, volume->IndicesNode(), _cookie, num, dirent, bufferSize));
}


static int
bfs_create_index(void *_ns, const char *name, int type, int flags)
{
	FUNCTION_START(("name = \"%s\", type = %d, flags = %d\n", name, type, flags));
	if (_ns == NULL || name == NULL || *name == '\0')
		return B_BAD_VALUE;

	Volume *volume = (Volume *)_ns;

	if (volume->IsReadOnly())
		return B_READ_ONLY_DEVICE;

	// only root users are allowed to create indices
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, volume->Indices());

	Index index(volume);
	status_t status = index.Create(&transaction, name, type);

	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static int
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

#ifdef UNSAFE_GET_VNODE
	RecursiveLocker locker(volume->Lock());
#endif
	Transaction transaction(volume, volume->Indices());

	status_t status = indices->Remove(&transaction, name);
	if (status == B_OK)
		transaction.Done();

	RETURN_ERROR(status);
}


static int
bfs_rename_index(void *ns, const char *oldname, const char *newname)
{
	FUNCTION_START(("from = %s, to = %s\n", oldname, newname));

	// Well, renaming an index doesn't make that much sense, as you
	// would also need to remove every file in it (or the index
	// would contain wrong data)
	// But in that case, you can better remove the old one, and
	// create a new one...
	// There is also no way to call this function from a userland
	// application.

	RETURN_ERROR(B_ENTRY_NOT_FOUND);
}


static int
bfs_stat_index(void *_ns, const char *name, struct index_info *indexInfo)
{
	FUNCTION_START(("name = %s\n", name));
	if (_ns == NULL || name == NULL || indexInfo == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	Volume *volume = (Volume *)_ns;
	Index index(volume);
	status_t status = index.SetTo(name);
	if (status < B_OK)
		RETURN_ERROR(status);

	bfs_inode *node = index.Node()->Node();

	indexInfo->type = index.Type();
	indexInfo->size = node->data.Size();
	indexInfo->modification_time = (time_t)(node->LastModifiedTime() >> INODE_TIME_SHIFT);
	indexInfo->creation_time = (time_t)(node->CreateTime() >> INODE_TIME_SHIFT);
	indexInfo->uid = node->UserID();
	indexInfo->gid = node->GroupID();

	return B_OK;
}


//	#pragma mark -
//	Query functions


static int
bfs_open_query(void *_ns, const char *queryString, ulong flags, port_id port,
	long token, void **cookie)
{
	FUNCTION();
	if (_ns == NULL || queryString == NULL || cookie == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	PRINT(("query = \"%s\", flags = %lu, port_id = %ld, token = %ld\n", queryString, flags, port, token));

	Volume *volume = (Volume *)_ns;

	Expression *expression = new Expression((char *)queryString);
	if (expression == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	if (expression->InitCheck() < B_OK) {
		FATAL(("Could not parse query, stopped at: \"%s\"\n", expression->Position()));
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

	*cookie = (void *)query;

	return B_OK;
}


static int
bfs_close_query(void *ns, void *cookie)
{
	FUNCTION();
	return B_OK;
}


static int
bfs_free_query_cookie(void *ns, void *node, void *cookie)
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


static int
bfs_read_query(void */*ns*/, void *cookie, long *num, struct dirent *dirent, size_t bufferSize)
{
	FUNCTION();
	Query *query = (Query *)cookie;
	if (query == NULL)
		RETURN_ERROR(B_BAD_VALUE);

	status_t status = query->GetNextEntry(dirent, bufferSize);
	if (status == B_OK)
		*num = 1;
	else if (status == B_ENTRY_NOT_FOUND)
		*num = 0;
	else
		return status;

	return B_OK;
}

