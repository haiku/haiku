/*
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Virtual File System and File System Interface Layer */


#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fs_attr.h>
#include <fs_info.h>
#include <fs_interface.h>
#include <fs_volume.h>
#include <OS.h>
#include <StorageDefs.h>

#include <AutoDeleter.h>
#include <block_cache.h>
#include <boot/kernel_args.h>
#include <debug_heap.h>
#include <disk_device_manager/KDiskDevice.h>
#include <disk_device_manager/KDiskDeviceManager.h>
#include <disk_device_manager/KDiskDeviceUtils.h>
#include <disk_device_manager/KDiskSystem.h>
#include <fd.h>
#include <file_cache.h>
#include <fs/node_monitor.h>
#include <khash.h>
#include <KPath.h>
#include <lock.h>
#include <low_resource_manager.h>
#include <syscalls.h>
#include <syscall_restart.h>
#include <tracing.h>
#include <util/atomic.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/VMCache.h>

#include "EntryCache.h"
#include "fifo.h"
#include "IORequest.h"
#include "unused_vnodes.h"
#include "vfs_tracing.h"
#include "Vnode.h"
#include "../cache/vnode_store.h"


//#define TRACE_VFS
#ifdef TRACE_VFS
#	define TRACE(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define TRACE(x) ;
#	define FUNCTION(x) ;
#endif

#define ADD_DEBUGGER_COMMANDS


#define HAS_FS_CALL(vnode, op)			(vnode->ops->op != NULL)
#define HAS_FS_MOUNT_CALL(mount, op)	(mount->volume->ops->op != NULL)

#if KDEBUG
#	define FS_CALL(vnode, op, params...) \
		( HAS_FS_CALL(vnode, op) ? \
			vnode->ops->op(vnode->mount->volume, vnode, params) \
			: (panic("FS_CALL op " #op " is NULL"), 0))
#	define FS_CALL_NO_PARAMS(vnode, op) \
		( HAS_FS_CALL(vnode, op) ? \
			vnode->ops->op(vnode->mount->volume, vnode) \
			: (panic("FS_CALL_NO_PARAMS op " #op " is NULL"), 0))
#	define FS_MOUNT_CALL(mount, op, params...) \
		( HAS_FS_MOUNT_CALL(mount, op) ? \
			mount->volume->ops->op(mount->volume, params) \
			: (panic("FS_MOUNT_CALL op " #op " is NULL"), 0))
#	define FS_MOUNT_CALL_NO_PARAMS(mount, op) \
		( HAS_FS_MOUNT_CALL(mount, op) ? \
			mount->volume->ops->op(mount->volume) \
			: (panic("FS_MOUNT_CALL_NO_PARAMS op " #op " is NULL"), 0))
#else
#	define FS_CALL(vnode, op, params...) \
			vnode->ops->op(vnode->mount->volume, vnode, params)
#	define FS_CALL_NO_PARAMS(vnode, op) \
			vnode->ops->op(vnode->mount->volume, vnode)
#	define FS_MOUNT_CALL(mount, op, params...) \
			mount->volume->ops->op(mount->volume, params)
#	define FS_MOUNT_CALL_NO_PARAMS(mount, op) \
			mount->volume->ops->op(mount->volume)
#endif


const static size_t kMaxPathLength = 65536;
	// The absolute maximum path length (for getcwd() - this is not depending
	// on PATH_MAX


struct vnode_hash_key {
	dev_t	device;
	ino_t	vnode;
};

typedef DoublyLinkedList<vnode> VnodeList;

/*!	\brief Structure to manage a mounted file system

	Note: The root_vnode and root_vnode->covers fields (what others?) are
	initialized in fs_mount() and not changed afterwards. That is as soon
	as the mount is mounted and it is made sure it won't be unmounted
	(e.g. by holding a reference to a vnode of that mount) (read) access
	to those fields is always safe, even without additional locking. Morever
	while mounted the mount holds a reference to the root_vnode->covers vnode,
	and thus making the access path vnode->mount->root_vnode->covers->mount->...
	safe if a reference to vnode is held (note that for the root mount
	root_vnode->covers is NULL, though).
*/
struct fs_mount {
	fs_mount()
		:
		volume(NULL),
		device_name(NULL)
	{
		recursive_lock_init(&rlock, "mount rlock");
	}

	~fs_mount()
	{
		recursive_lock_destroy(&rlock);
		free(device_name);

		while (volume) {
			fs_volume* superVolume = volume->super_volume;

			if (volume->file_system != NULL)
				put_module(volume->file_system->info.name);

			free(volume->file_system_name);
			free(volume);
			volume = superVolume;
		}
	}

	struct fs_mount* next;
	dev_t			id;
	fs_volume*		volume;
	char*			device_name;
	recursive_lock	rlock;	// guards the vnodes list
		// TODO: Make this a mutex! It is never used recursively.
	struct vnode*	root_vnode;
	struct vnode*	covers_vnode;	// immutable
	KPartition*		partition;
	VnodeList		vnodes;
	EntryCache		entry_cache;
	bool			unmounting;
	bool			owns_file_device;
};

struct advisory_lock : public DoublyLinkedListLinkImpl<advisory_lock> {
	list_link		link;
	team_id			team;
	pid_t			session;
	off_t			start;
	off_t			end;
	bool			shared;
};

typedef DoublyLinkedList<advisory_lock> LockList;

struct advisory_locking {
	sem_id			lock;
	sem_id			wait_sem;
	LockList		locks;

	advisory_locking()
		:
		lock(-1),
		wait_sem(-1)
	{
	}

	~advisory_locking()
	{
		if (lock >= 0)
			delete_sem(lock);
		if (wait_sem >= 0)
			delete_sem(wait_sem);
	}
};

/*!	\brief Guards sMountsTable.

	The holder is allowed to read/write access the sMountsTable.
	Manipulation of the fs_mount structures themselves
	(and their destruction) requires different locks though.
*/
static mutex sMountMutex = MUTEX_INITIALIZER("vfs_mount_lock");

/*!	\brief Guards mount/unmount operations.

	The fs_mount() and fs_unmount() hold the lock during their whole operation.
	That is locking the lock ensures that no FS is mounted/unmounted. In
	particular this means that
	- sMountsTable will not be modified,
	- the fields immutable after initialization of the fs_mount structures in
	  sMountsTable will not be modified,

	The thread trying to lock the lock must not hold sVnodeLock or
	sMountMutex.
*/
static recursive_lock sMountOpLock;

/*!	\brief Guards sVnodeTable.

	The holder is allowed read/write access to sVnodeTable and to
	any unbusy vnode in that table, save to the immutable fields (device, id,
	private_node, mount) to which only read-only access is allowed.
	The mutable fields advisory_locking, mandatory_locked_by, and ref_count, as
	well as the busy, removed, unused flags, and the vnode's type can also be
	write accessed when holding a read lock to sVnodeLock *and* having the vnode
	locked. Write access to covered_by and covers requires to write lock
	sVnodeLock.

	The thread trying to acquire the lock must not hold sMountMutex.
	You must not hold this lock when calling create_sem(), as this might call
	vfs_free_unused_vnodes() and thus cause a deadlock.
*/
static rw_lock sVnodeLock = RW_LOCK_INITIALIZER("vfs_vnode_lock");

/*!	\brief Guards io_context::root.

	Must be held when setting or getting the io_context::root field.
	The only operation allowed while holding this lock besides getting or
	setting the field is inc_vnode_ref_count() on io_context::root.
*/
static mutex sIOContextRootLock = MUTEX_INITIALIZER("io_context::root lock");


#define VNODE_HASH_TABLE_SIZE 1024
static hash_table* sVnodeTable;
static struct vnode* sRoot;

#define MOUNTS_HASH_TABLE_SIZE 16
static hash_table* sMountsTable;
static dev_t sNextMountID = 1;

#define MAX_TEMP_IO_VECS 8

mode_t __gUmask = 022;

/* function declarations */

static void free_unused_vnodes();

// file descriptor operation prototypes
static status_t file_read(struct file_descriptor* descriptor, off_t pos,
	void* buffer, size_t* _bytes);
static status_t file_write(struct file_descriptor* descriptor, off_t pos,
	const void* buffer, size_t* _bytes);
static off_t file_seek(struct file_descriptor* descriptor, off_t pos,
	int seekType);
static void file_free_fd(struct file_descriptor* descriptor);
static status_t file_close(struct file_descriptor* descriptor);
static status_t file_select(struct file_descriptor* descriptor, uint8 event,
	struct selectsync* sync);
static status_t file_deselect(struct file_descriptor* descriptor, uint8 event,
	struct selectsync* sync);
static status_t dir_read(struct io_context* context,
	struct file_descriptor* descriptor, struct dirent* buffer,
	size_t bufferSize, uint32* _count);
static status_t dir_read(struct io_context* ioContext, struct vnode* vnode,
	void* cookie, struct dirent* buffer, size_t bufferSize, uint32* _count);
static status_t dir_rewind(struct file_descriptor* descriptor);
static void dir_free_fd(struct file_descriptor* descriptor);
static status_t dir_close(struct file_descriptor* descriptor);
static status_t attr_dir_read(struct io_context* context,
	struct file_descriptor* descriptor, struct dirent* buffer,
	size_t bufferSize, uint32* _count);
static status_t attr_dir_rewind(struct file_descriptor* descriptor);
static void attr_dir_free_fd(struct file_descriptor* descriptor);
static status_t attr_dir_close(struct file_descriptor* descriptor);
static status_t attr_read(struct file_descriptor* descriptor, off_t pos,
	void* buffer, size_t* _bytes);
static status_t attr_write(struct file_descriptor* descriptor, off_t pos,
	const void* buffer, size_t* _bytes);
static off_t attr_seek(struct file_descriptor* descriptor, off_t pos,
	int seekType);
static void attr_free_fd(struct file_descriptor* descriptor);
static status_t attr_close(struct file_descriptor* descriptor);
static status_t attr_read_stat(struct file_descriptor* descriptor,
	struct stat* statData);
static status_t attr_write_stat(struct file_descriptor* descriptor,
	const struct stat* stat, int statMask);
static status_t index_dir_read(struct io_context* context,
	struct file_descriptor* descriptor, struct dirent* buffer,
	size_t bufferSize, uint32* _count);
static status_t index_dir_rewind(struct file_descriptor* descriptor);
static void index_dir_free_fd(struct file_descriptor* descriptor);
static status_t index_dir_close(struct file_descriptor* descriptor);
static status_t query_read(struct io_context* context,
	struct file_descriptor* descriptor, struct dirent* buffer,
	size_t bufferSize, uint32* _count);
static status_t query_rewind(struct file_descriptor* descriptor);
static void query_free_fd(struct file_descriptor* descriptor);
static status_t query_close(struct file_descriptor* descriptor);

static status_t common_ioctl(struct file_descriptor* descriptor, uint32 op,
	void* buffer, size_t length);
static status_t common_read_stat(struct file_descriptor* descriptor,
	struct stat* statData);
static status_t common_write_stat(struct file_descriptor* descriptor,
	const struct stat* statData, int statMask);
static status_t common_path_read_stat(int fd, char* path, bool traverseLeafLink,
	struct stat* stat, bool kernel);

static status_t vnode_path_to_vnode(struct vnode* vnode, char* path,
	bool traverseLeafLink, int count, bool kernel,
	struct vnode** _vnode, ino_t* _parentID);
static status_t dir_vnode_to_path(struct vnode* vnode, char* buffer,
	size_t bufferSize, bool kernel);
static status_t fd_and_path_to_vnode(int fd, char* path, bool traverseLeafLink,
	struct vnode** _vnode, ino_t* _parentID, bool kernel);
static void inc_vnode_ref_count(struct vnode* vnode);
static status_t dec_vnode_ref_count(struct vnode* vnode, bool alwaysFree,
	bool reenter);
static inline void put_vnode(struct vnode* vnode);
static status_t fs_unmount(char* path, dev_t mountID, uint32 flags,
	bool kernel);
static int open_vnode(struct vnode* vnode, int openMode, bool kernel);


static struct fd_ops sFileOps = {
	file_read,
	file_write,
	file_seek,
	common_ioctl,
	NULL,		// set_flags
	file_select,
	file_deselect,
	NULL,		// read_dir()
	NULL,		// rewind_dir()
	common_read_stat,
	common_write_stat,
	file_close,
	file_free_fd
};

static struct fd_ops sDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	common_ioctl,
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	dir_read,
	dir_rewind,
	common_read_stat,
	common_write_stat,
	dir_close,
	dir_free_fd
};

static struct fd_ops sAttributeDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	common_ioctl,
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	attr_dir_read,
	attr_dir_rewind,
	common_read_stat,
	common_write_stat,
	attr_dir_close,
	attr_dir_free_fd
};

static struct fd_ops sAttributeOps = {
	attr_read,
	attr_write,
	attr_seek,
	common_ioctl,
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	NULL,		// read_dir()
	NULL,		// rewind_dir()
	attr_read_stat,
	attr_write_stat,
	attr_close,
	attr_free_fd
};

static struct fd_ops sIndexDirectoryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	NULL,		// ioctl()
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	index_dir_read,
	index_dir_rewind,
	NULL,		// read_stat()
	NULL,		// write_stat()
	index_dir_close,
	index_dir_free_fd
};

#if 0
static struct fd_ops sIndexOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	NULL,		// ioctl()
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	NULL,		// dir_read()
	NULL,		// dir_rewind()
	index_read_stat,	// read_stat()
	NULL,		// write_stat()
	NULL,		// dir_close()
	NULL		// free_fd()
};
#endif

static struct fd_ops sQueryOps = {
	NULL,		// read()
	NULL,		// write()
	NULL,		// seek()
	NULL,		// ioctl()
	NULL,		// set_flags
	NULL,		// select()
	NULL,		// deselect()
	query_read,
	query_rewind,
	NULL,		// read_stat()
	NULL,		// write_stat()
	query_close,
	query_free_fd
};


// VNodePutter
class VNodePutter {
public:
	VNodePutter(struct vnode* vnode = NULL) : fVNode(vnode) {}

	~VNodePutter()
	{
		Put();
	}

	void SetTo(struct vnode* vnode)
	{
		Put();
		fVNode = vnode;
	}

	void Put()
	{
		if (fVNode) {
			put_vnode(fVNode);
			fVNode = NULL;
		}
	}

	struct vnode* Detach()
	{
		struct vnode* vnode = fVNode;
		fVNode = NULL;
		return vnode;
	}

private:
	struct vnode* fVNode;
};


class FDCloser {
public:
	FDCloser() : fFD(-1), fKernel(true) {}

	FDCloser(int fd, bool kernel) : fFD(fd), fKernel(kernel) {}

	~FDCloser()
	{
		Close();
	}

	void SetTo(int fd, bool kernel)
	{
		Close();
		fFD = fd;
		fKernel = kernel;
	}

	void Close()
	{
		if (fFD >= 0) {
			if (fKernel)
				_kern_close(fFD);
			else
				_user_close(fFD);
			fFD = -1;
		}
	}

	int Detach()
	{
		int fd = fFD;
		fFD = -1;
		return fd;
	}

private:
	int		fFD;
	bool	fKernel;
};


#if VFS_PAGES_IO_TRACING

namespace VFSPagesIOTracing {

class PagesIOTraceEntry : public AbstractTraceEntry {
protected:
	PagesIOTraceEntry(struct vnode* vnode, void* cookie, off_t pos,
		const generic_io_vec* vecs, uint32 count, uint32 flags, generic_size_t bytesRequested,
		status_t status, generic_size_t bytesTransferred)
		:
		fVnode(vnode),
		fMountID(vnode->mount->id),
		fNodeID(vnode->id),
		fCookie(cookie),
		fPos(pos),
		fCount(count),
		fFlags(flags),
		fBytesRequested(bytesRequested),
		fStatus(status),
		fBytesTransferred(bytesTransferred)
	{
		fVecs = (generic_io_vec*)alloc_tracing_buffer_memcpy(vecs, sizeof(generic_io_vec) * count,
			false);
	}

	void AddDump(TraceOutput& out, const char* mode)
	{
		out.Print("vfs pages io %5s: vnode: %p (%ld, %lld), cookie: %p, "
			"pos: %lld, size: %llu, vecs: {", mode, fVnode, fMountID, fNodeID,
			fCookie, fPos, (uint64)fBytesRequested);

		if (fVecs != NULL) {
			for (uint32 i = 0; i < fCount; i++) {
				if (i > 0)
					out.Print(", ");
				out.Print("(%llx, %llu)", (uint64)fVecs[i].base, (uint64)fVecs[i].length);
			}
		}

		out.Print("}, flags: %#lx -> status: %#lx, transferred: %llu",
			fFlags, fStatus, (uint64)fBytesTransferred);
	}

protected:
	struct vnode*	fVnode;
	dev_t			fMountID;
	ino_t			fNodeID;
	void*			fCookie;
	off_t			fPos;
	generic_io_vec*		fVecs;
	uint32			fCount;
	uint32			fFlags;
	generic_size_t			fBytesRequested;
	status_t		fStatus;
	generic_size_t			fBytesTransferred;
};


class ReadPages : public PagesIOTraceEntry {
public:
	ReadPages(struct vnode* vnode, void* cookie, off_t pos,
		const generic_io_vec* vecs, uint32 count, uint32 flags, generic_size_t bytesRequested,
		status_t status, generic_size_t bytesTransferred)
		:
		PagesIOTraceEntry(vnode, cookie, pos, vecs, count, flags,
			bytesRequested, status, bytesTransferred)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		PagesIOTraceEntry::AddDump(out, "read");
	}
};


class WritePages : public PagesIOTraceEntry {
public:
	WritePages(struct vnode* vnode, void* cookie, off_t pos,
		const generic_io_vec* vecs, uint32 count, uint32 flags, generic_size_t bytesRequested,
		status_t status, generic_size_t bytesTransferred)
		:
		PagesIOTraceEntry(vnode, cookie, pos, vecs, count, flags,
			bytesRequested, status, bytesTransferred)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		PagesIOTraceEntry::AddDump(out, "write");
	}
};

}	// namespace VFSPagesIOTracing

#	define TPIO(x) new(std::nothrow) VFSPagesIOTracing::x;
#else
#	define TPIO(x) ;
#endif	// VFS_PAGES_IO_TRACING


static int
mount_compare(void* _m, const void* _key)
{
	struct fs_mount* mount = (fs_mount*)_m;
	const dev_t* id = (dev_t*)_key;

	if (mount->id == *id)
		return 0;

	return -1;
}


static uint32
mount_hash(void* _m, const void* _key, uint32 range)
{
	struct fs_mount* mount = (fs_mount*)_m;
	const dev_t* id = (dev_t*)_key;

	if (mount)
		return mount->id % range;

	return (uint32)*id % range;
}


/*! Finds the mounted device (the fs_mount structure) with the given ID.
	Note, you must hold the gMountMutex lock when you call this function.
*/
static struct fs_mount*
find_mount(dev_t id)
{
	ASSERT_LOCKED_MUTEX(&sMountMutex);

	return (fs_mount*)hash_lookup(sMountsTable, (void*)&id);
}


static status_t
get_mount(dev_t id, struct fs_mount** _mount)
{
	struct fs_mount* mount;

	ReadLocker nodeLocker(sVnodeLock);
	MutexLocker mountLocker(sMountMutex);

	mount = find_mount(id);
	if (mount == NULL)
		return B_BAD_VALUE;

	struct vnode* rootNode = mount->root_vnode;
	if (rootNode == NULL || rootNode->IsBusy() || rootNode->ref_count == 0) {
		// might have been called during a mount/unmount operation
		return B_BUSY;
	}

	inc_vnode_ref_count(mount->root_vnode);
	*_mount = mount;
	return B_OK;
}


static void
put_mount(struct fs_mount* mount)
{
	if (mount)
		put_vnode(mount->root_vnode);
}


/*!	Tries to open the specified file system module.
	Accepts a file system name of the form "bfs" or "file_systems/bfs/v1".
	Returns a pointer to file system module interface, or NULL if it
	could not open the module.
*/
static file_system_module_info*
get_file_system(const char* fsName)
{
	char name[B_FILE_NAME_LENGTH];
	if (strncmp(fsName, "file_systems/", strlen("file_systems/"))) {
		// construct module name if we didn't get one
		// (we currently support only one API)
		snprintf(name, sizeof(name), "file_systems/%s/v1", fsName);
		fsName = NULL;
	}

	file_system_module_info* info;
	if (get_module(fsName ? fsName : name, (module_info**)&info) != B_OK)
		return NULL;

	return info;
}


/*!	Accepts a file system name of the form "bfs" or "file_systems/bfs/v1"
	and returns a compatible fs_info.fsh_name name ("bfs" in both cases).
	The name is allocated for you, and you have to free() it when you're
	done with it.
	Returns NULL if the required memory is not available.
*/
static char*
get_file_system_name(const char* fsName)
{
	const size_t length = strlen("file_systems/");

	if (strncmp(fsName, "file_systems/", length)) {
		// the name already seems to be the module's file name
		return strdup(fsName);
	}

	fsName += length;
	const char* end = strchr(fsName, '/');
	if (end == NULL) {
		// this doesn't seem to be a valid name, but well...
		return strdup(fsName);
	}

	// cut off the trailing /v1

	char* name = (char*)malloc(end + 1 - fsName);
	if (name == NULL)
		return NULL;

	strlcpy(name, fsName, end + 1 - fsName);
	return name;
}


/*!	Accepts a list of file system names separated by a colon, one for each
	layer and returns the file system name for the specified layer.
	The name is allocated for you, and you have to free() it when you're
	done with it.
	Returns NULL if the required memory is not available or if there is no
	name for the specified layer.
*/
static char*
get_file_system_name_for_layer(const char* fsNames, int32 layer)
{
	while (layer >= 0) {
		const char* end = strchr(fsNames, ':');
		if (end == NULL) {
			if (layer == 0)
				return strdup(fsNames);
			return NULL;
		}

		if (layer == 0) {
			size_t length = end - fsNames + 1;
			char* result = (char*)malloc(length);
			strlcpy(result, fsNames, length);
			return result;
		}

		fsNames = end + 1;
		layer--;
	}

	return NULL;
}


static int
vnode_compare(void* _vnode, const void* _key)
{
	struct vnode* vnode = (struct vnode*)_vnode;
	const struct vnode_hash_key* key = (vnode_hash_key*)_key;

	if (vnode->device == key->device && vnode->id == key->vnode)
		return 0;

	return -1;
}


static uint32
vnode_hash(void* _vnode, const void* _key, uint32 range)
{
	struct vnode* vnode = (struct vnode*)_vnode;
	const struct vnode_hash_key* key = (vnode_hash_key*)_key;

#define VHASH(mountid, vnodeid) \
	(((uint32)((vnodeid) >> 32) + (uint32)(vnodeid)) ^ (uint32)(mountid))

	if (vnode != NULL)
		return VHASH(vnode->device, vnode->id) % range;

	return VHASH(key->device, key->vnode) % range;

#undef VHASH
}


static void
add_vnode_to_mount_list(struct vnode* vnode, struct fs_mount* mount)
{
	RecursiveLocker _(mount->rlock);
	mount->vnodes.Add(vnode);
}


static void
remove_vnode_from_mount_list(struct vnode* vnode, struct fs_mount* mount)
{
	RecursiveLocker _(mount->rlock);
	mount->vnodes.Remove(vnode);
}


/*!	\brief Looks up a vnode by mount and node ID in the sVnodeTable.

	The caller must hold the sVnodeLock (read lock at least).

	\param mountID the mount ID.
	\param vnodeID the node ID.

	\return The vnode structure, if it was found in the hash table, \c NULL
			otherwise.
*/
static struct vnode*
lookup_vnode(dev_t mountID, ino_t vnodeID)
{
	struct vnode_hash_key key;

	key.device = mountID;
	key.vnode = vnodeID;

	return (vnode*)hash_lookup(sVnodeTable, &key);
}


/*!	Creates a new vnode with the given mount and node ID.
	If the node already exists, it is returned instead and no new node is
	created. In either case -- but not, if an error occurs -- the function write
	locks \c sVnodeLock and keeps it locked for the caller when returning. On
	error the lock is not not held on return.

	\param mountID The mount ID.
	\param vnodeID The vnode ID.
	\param _vnode Will be set to the new vnode on success.
	\param _nodeCreated Will be set to \c true when the returned vnode has
		been newly created, \c false when it already existed. Will not be
		changed on error.
	\return \c B_OK, when the vnode was successfully created and inserted or
		a node with the given ID was found, \c B_NO_MEMORY or
		\c B_ENTRY_NOT_FOUND on error.
*/
static status_t
create_new_vnode_and_lock(dev_t mountID, ino_t vnodeID, struct vnode*& _vnode,
	bool& _nodeCreated)
{
	FUNCTION(("create_new_vnode_and_lock()\n"));

	struct vnode* vnode = (struct vnode*)malloc(sizeof(struct vnode));
	if (vnode == NULL)
		return B_NO_MEMORY;

	// initialize basic values
	memset(vnode, 0, sizeof(struct vnode));
	vnode->device = mountID;
	vnode->id = vnodeID;
	vnode->ref_count = 1;
	vnode->SetBusy(true);

	// look up the the node -- it might have been added by someone else in the
	// meantime
	rw_lock_write_lock(&sVnodeLock);
	struct vnode* existingVnode = lookup_vnode(mountID, vnodeID);
	if (existingVnode != NULL) {
		free(vnode);
		_vnode = existingVnode;
		_nodeCreated = false;
		return B_OK;
	}

	// get the mount structure
	mutex_lock(&sMountMutex);
	vnode->mount = find_mount(mountID);
	if (!vnode->mount || vnode->mount->unmounting) {
		mutex_unlock(&sMountMutex);
		rw_lock_write_unlock(&sVnodeLock);
		free(vnode);
		return B_ENTRY_NOT_FOUND;
	}

	// add the vnode to the mount's node list and the hash table
	hash_insert(sVnodeTable, vnode);
	add_vnode_to_mount_list(vnode, vnode->mount);

	mutex_unlock(&sMountMutex);

	_vnode = vnode;
	_nodeCreated = true;

	// keep the vnode lock locked
	return B_OK;
}


/*!	Frees the vnode and all resources it has acquired, and removes
	it from the vnode hash as well as from its mount structure.
	Will also make sure that any cache modifications are written back.
*/
static void
free_vnode(struct vnode* vnode, bool reenter)
{
	ASSERT_PRINT(vnode->ref_count == 0 && vnode->IsBusy(), "vnode: %p\n",
		vnode);

	// write back any changes in this vnode's cache -- but only
	// if the vnode won't be deleted, in which case the changes
	// will be discarded

	if (!vnode->IsRemoved() && HAS_FS_CALL(vnode, fsync))
		FS_CALL_NO_PARAMS(vnode, fsync);

	// Note: If this vnode has a cache attached, there will still be two
	// references to that cache at this point. The last one belongs to the vnode
	// itself (cf. vfs_get_vnode_cache()) and one belongs to the node's file
	// cache. Each but the last reference to a cache also includes a reference
	// to the vnode. The file cache, however, released its reference (cf.
	// file_cache_create()), so that this vnode's ref count has the chance to
	// ever drop to 0. Deleting the file cache now, will cause the next to last
	// cache reference to be released, which will also release a (no longer
	// existing) vnode reference. To avoid problems, we set the vnode's ref
	// count, so that it will neither become negative nor 0.
	vnode->ref_count = 2;

	if (!vnode->IsUnpublished()) {
		if (vnode->IsRemoved())
			FS_CALL(vnode, remove_vnode, reenter);
		else
			FS_CALL(vnode, put_vnode, reenter);
	}

	// If the vnode has a VMCache attached, make sure that it won't try to get
	// another reference via VMVnodeCache::AcquireUnreferencedStoreRef(). As
	// long as the vnode is busy and in the hash, that won't happen, but as
	// soon as we've removed it from the hash, it could reload the vnode -- with
	// a new cache attached!
	if (vnode->cache != NULL)
		((VMVnodeCache*)vnode->cache)->VnodeDeleted();

	// The file system has removed the resources of the vnode now, so we can
	// make it available again (by removing the busy vnode from the hash).
	rw_lock_write_lock(&sVnodeLock);
	hash_remove(sVnodeTable, vnode);
	rw_lock_write_unlock(&sVnodeLock);

	// if we have a VMCache attached, remove it
	if (vnode->cache)
		vnode->cache->ReleaseRef();

	vnode->cache = NULL;

	remove_vnode_from_mount_list(vnode, vnode->mount);

	free(vnode);
}


/*!	\brief Decrements the reference counter of the given vnode and deletes it,
	if the counter dropped to 0.

	The caller must, of course, own a reference to the vnode to call this
	function.
	The caller must not hold the sVnodeLock or the sMountMutex.

	\param vnode the vnode.
	\param alwaysFree don't move this vnode into the unused list, but really
		   delete it if possible.
	\param reenter \c true, if this function is called (indirectly) from within
		   a file system. This will be passed to file system hooks only.
	\return \c B_OK, if everything went fine, an error code otherwise.
*/
static status_t
dec_vnode_ref_count(struct vnode* vnode, bool alwaysFree, bool reenter)
{
	ReadLocker locker(sVnodeLock);
	AutoLocker<Vnode> nodeLocker(vnode);

	int32 oldRefCount = atomic_add(&vnode->ref_count, -1);

	ASSERT_PRINT(oldRefCount > 0, "vnode %p\n", vnode);

	TRACE(("dec_vnode_ref_count: vnode %p, ref now %ld\n", vnode,
		vnode->ref_count));

	if (oldRefCount != 1)
		return B_OK;

	if (vnode->IsBusy())
		panic("dec_vnode_ref_count: called on busy vnode %p\n", vnode);

	bool freeNode = false;
	bool freeUnusedNodes = false;

	// Just insert the vnode into an unused list if we don't need
	// to delete it
	if (vnode->IsRemoved() || alwaysFree) {
		vnode_to_be_freed(vnode);
		vnode->SetBusy(true);
		freeNode = true;
	} else
		freeUnusedNodes = vnode_unused(vnode);

	nodeLocker.Unlock();
	locker.Unlock();

	if (freeNode)
		free_vnode(vnode, reenter);
	else if (freeUnusedNodes)
		free_unused_vnodes();

	return B_OK;
}


/*!	\brief Increments the reference counter of the given vnode.

	The caller must make sure that the node isn't deleted while this function
	is called. This can be done either:
	- by ensuring that a reference to the node exists and remains in existence,
	  or
	- by holding the vnode's lock (which also requires read locking sVnodeLock)
	  or by holding sVnodeLock write locked.

	In the second case the caller is responsible for dealing with the ref count
	0 -> 1 transition. That is 1. this function must not be invoked when the
	node is busy in the first place and 2. vnode_used() must be called for the
	node.

	\param vnode the vnode.
*/
static void
inc_vnode_ref_count(struct vnode* vnode)
{
	atomic_add(&vnode->ref_count, 1);
	TRACE(("inc_vnode_ref_count: vnode %p, ref now %ld\n", vnode,
		vnode->ref_count));
}


static bool
is_special_node_type(int type)
{
	// at the moment only FIFOs are supported
	return S_ISFIFO(type);
}


static status_t
create_special_sub_node(struct vnode* vnode, uint32 flags)
{
	if (S_ISFIFO(vnode->Type()))
		return create_fifo_vnode(vnode->mount->volume, vnode);

	return B_BAD_VALUE;
}


/*!	\brief Retrieves a vnode for a given mount ID, node ID pair.

	If the node is not yet in memory, it will be loaded.

	The caller must not hold the sVnodeLock or the sMountMutex.

	\param mountID the mount ID.
	\param vnodeID the node ID.
	\param _vnode Pointer to a vnode* variable into which the pointer to the
		   retrieved vnode structure shall be written.
	\param reenter \c true, if this function is called (indirectly) from within
		   a file system.
	\return \c B_OK, if everything when fine, an error code otherwise.
*/
static status_t
get_vnode(dev_t mountID, ino_t vnodeID, struct vnode** _vnode, bool canWait,
	int reenter)
{
	FUNCTION(("get_vnode: mountid %ld vnid 0x%Lx %p\n", mountID, vnodeID,
		_vnode));

	rw_lock_read_lock(&sVnodeLock);

	int32 tries = 2000;
		// try for 10 secs
restart:
	struct vnode* vnode = lookup_vnode(mountID, vnodeID);
	AutoLocker<Vnode> nodeLocker(vnode);

	if (vnode && vnode->IsBusy()) {
		nodeLocker.Unlock();
		rw_lock_read_unlock(&sVnodeLock);
		if (!canWait || --tries < 0) {
			// vnode doesn't seem to become unbusy
			dprintf("vnode %ld:%Ld is not becoming unbusy!\n", mountID,
				vnodeID);
			return B_BUSY;
		}
		snooze(5000); // 5 ms
		rw_lock_read_lock(&sVnodeLock);
		goto restart;
	}

	TRACE(("get_vnode: tried to lookup vnode, got %p\n", vnode));

	status_t status;

	if (vnode) {
		if (vnode->ref_count == 0) {
			// this vnode has been unused before
			vnode_used(vnode);
		}
		inc_vnode_ref_count(vnode);

		nodeLocker.Unlock();
		rw_lock_read_unlock(&sVnodeLock);
	} else {
		// we need to create a new vnode and read it in
		rw_lock_read_unlock(&sVnodeLock);
			// unlock -- create_new_vnode_and_lock() write-locks on success
		bool nodeCreated;
		status = create_new_vnode_and_lock(mountID, vnodeID, vnode,
			nodeCreated);
		if (status != B_OK)
			return status;

		if (!nodeCreated) {
			rw_lock_read_lock(&sVnodeLock);
			rw_lock_write_unlock(&sVnodeLock);
			goto restart;
		}

		rw_lock_write_unlock(&sVnodeLock);

		int type;
		uint32 flags;
		status = FS_MOUNT_CALL(vnode->mount, get_vnode, vnodeID, vnode, &type,
			&flags, reenter);
		if (status == B_OK && vnode->private_node == NULL)
			status = B_BAD_VALUE;

		bool gotNode = status == B_OK;
		bool publishSpecialSubNode = false;
		if (gotNode) {
			vnode->SetType(type);
			publishSpecialSubNode = is_special_node_type(type)
				&& (flags & B_VNODE_DONT_CREATE_SPECIAL_SUB_NODE) == 0;
		}

		if (gotNode && publishSpecialSubNode)
			status = create_special_sub_node(vnode, flags);

		if (status != B_OK) {
			if (gotNode)
				FS_CALL(vnode, put_vnode, reenter);

			rw_lock_write_lock(&sVnodeLock);
			hash_remove(sVnodeTable, vnode);
			remove_vnode_from_mount_list(vnode, vnode->mount);
			rw_lock_write_unlock(&sVnodeLock);

			free(vnode);
			return status;
		}

		rw_lock_read_lock(&sVnodeLock);
		vnode->Lock();

		vnode->SetRemoved((flags & B_VNODE_PUBLISH_REMOVED) != 0);
		vnode->SetBusy(false);

		vnode->Unlock();
		rw_lock_read_unlock(&sVnodeLock);
	}

	TRACE(("get_vnode: returning %p\n", vnode));

	*_vnode = vnode;
	return B_OK;
}


/*!	\brief Decrements the reference counter of the given vnode and deletes it,
	if the counter dropped to 0.

	The caller must, of course, own a reference to the vnode to call this
	function.
	The caller must not hold the sVnodeLock or the sMountMutex.

	\param vnode the vnode.
*/
static inline void
put_vnode(struct vnode* vnode)
{
	dec_vnode_ref_count(vnode, false, false);
}


static void
free_unused_vnodes(int32 level)
{
	unused_vnodes_check_started();

	if (level == B_NO_LOW_RESOURCE) {
		unused_vnodes_check_done();
		return;
	}

	flush_hot_vnodes();

	// determine how many nodes to free
	uint32 count = 1;
	{
		MutexLocker unusedVnodesLocker(sUnusedVnodesLock);

		switch (level) {
			case B_LOW_RESOURCE_NOTE:
				count = sUnusedVnodes / 100;
				break;
			case B_LOW_RESOURCE_WARNING:
				count = sUnusedVnodes / 10;
				break;
			case B_LOW_RESOURCE_CRITICAL:
				count = sUnusedVnodes;
				break;
		}

		if (count > sUnusedVnodes)
			count = sUnusedVnodes;
	}

	// Write back the modified pages of some unused vnodes and free them.

	for (uint32 i = 0; i < count; i++) {
		ReadLocker vnodesReadLocker(sVnodeLock);

		// get the first node
		MutexLocker unusedVnodesLocker(sUnusedVnodesLock);
		struct vnode* vnode = (struct vnode*)list_get_first_item(
			&sUnusedVnodeList);
		unusedVnodesLocker.Unlock();

		if (vnode == NULL)
			break;

		// lock the node
		AutoLocker<Vnode> nodeLocker(vnode);

		// Check whether the node is still unused -- since we only append to the
		// the tail of the unused queue, the vnode should still be at its head.
		// Alternatively we could check its ref count for 0 and its busy flag,
		// but if the node is no longer at the head of the queue, it means it
		// has been touched in the meantime, i.e. it is no longer the least
		// recently used unused vnode and we rather don't free it.
		unusedVnodesLocker.Lock();
		if (vnode != list_get_first_item(&sUnusedVnodeList))
			continue;
		unusedVnodesLocker.Unlock();

		ASSERT(!vnode->IsBusy());

		// grab a reference
		inc_vnode_ref_count(vnode);
		vnode_used(vnode);

		// write back changes and free the node
		nodeLocker.Unlock();
		vnodesReadLocker.Unlock();

		if (vnode->cache != NULL)
			vnode->cache->WriteModified();

		dec_vnode_ref_count(vnode, true, false);
			// this should free the vnode when it's still unused
	}

	unused_vnodes_check_done();
}


/*!	Gets the vnode the given vnode is covering.

	The caller must have \c sVnodeLock read-locked at least.

	The function returns a reference to the retrieved vnode (if any), the caller
	is responsible to free.

	\param vnode The vnode whose covered node shall be returned.
	\return The covered vnode, or \c NULL if the given vnode doesn't cover any
		vnode.
*/
static inline Vnode*
get_covered_vnode_locked(Vnode* vnode)
{
	if (Vnode* coveredNode = vnode->covers) {
		while (coveredNode->covers != NULL)
			coveredNode = coveredNode->covers;

		inc_vnode_ref_count(coveredNode);
		return coveredNode;
	}

	return NULL;
}


/*!	Gets the vnode the given vnode is covering.

	The caller must not hold \c sVnodeLock. Note that this implies a race
	condition, since the situation can change at any time.

	The function returns a reference to the retrieved vnode (if any), the caller
	is responsible to free.

	\param vnode The vnode whose covered node shall be returned.
	\return The covered vnode, or \c NULL if the given vnode doesn't cover any
		vnode.
*/
static inline Vnode*
get_covered_vnode(Vnode* vnode)
{
	if (!vnode->IsCovering())
		return NULL;

	ReadLocker vnodeReadLocker(sVnodeLock);
	return get_covered_vnode_locked(vnode);
}


/*!	Gets the vnode the given vnode is covered by.

	The caller must have \c sVnodeLock read-locked at least.

	The function returns a reference to the retrieved vnode (if any), the caller
	is responsible to free.

	\param vnode The vnode whose covering node shall be returned.
	\return The covering vnode, or \c NULL if the given vnode isn't covered by
		any vnode.
*/
static Vnode*
get_covering_vnode_locked(Vnode* vnode)
{
	if (Vnode* coveringNode = vnode->covered_by) {
		while (coveringNode->covered_by != NULL)
			coveringNode = coveringNode->covered_by;

		inc_vnode_ref_count(coveringNode);
		return coveringNode;
	}

	return NULL;
}


/*!	Gets the vnode the given vnode is covered by.

	The caller must not hold \c sVnodeLock. Note that this implies a race
	condition, since the situation can change at any time.

	The function returns a reference to the retrieved vnode (if any), the caller
	is responsible to free.

	\param vnode The vnode whose covering node shall be returned.
	\return The covering vnode, or \c NULL if the given vnode isn't covered by
		any vnode.
*/
static inline Vnode*
get_covering_vnode(Vnode* vnode)
{
	if (!vnode->IsCovered())
		return NULL;

	ReadLocker vnodeReadLocker(sVnodeLock);
	return get_covering_vnode_locked(vnode);
}


static void
free_unused_vnodes()
{
	free_unused_vnodes(
		low_resource_state(B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE));
}


static void
vnode_low_resource_handler(void* /*data*/, uint32 resources, int32 level)
{
	TRACE(("vnode_low_resource_handler(level = %ld)\n", level));

	free_unused_vnodes(level);
}


static inline void
put_advisory_locking(struct advisory_locking* locking)
{
	release_sem(locking->lock);
}


/*!	Returns the advisory_locking object of the \a vnode in case it
	has one, and locks it.
	You have to call put_advisory_locking() when you're done with
	it.
	Note, you must not have the vnode mutex locked when calling
	this function.
*/
static struct advisory_locking*
get_advisory_locking(struct vnode* vnode)
{
	rw_lock_read_lock(&sVnodeLock);
	vnode->Lock();

	struct advisory_locking* locking = vnode->advisory_locking;
	sem_id lock = locking != NULL ? locking->lock : B_ERROR;

	vnode->Unlock();
	rw_lock_read_unlock(&sVnodeLock);

	if (lock >= 0)
		lock = acquire_sem(lock);
	if (lock < 0) {
		// This means the locking has been deleted in the mean time
		// or had never existed in the first place - otherwise, we
		// would get the lock at some point.
		return NULL;
	}

	return locking;
}


/*!	Creates a locked advisory_locking object, and attaches it to the
	given \a vnode.
	Returns B_OK in case of success - also if the vnode got such an
	object from someone else in the mean time, you'll still get this
	one locked then.
*/
static status_t
create_advisory_locking(struct vnode* vnode)
{
	if (vnode == NULL)
		return B_FILE_ERROR;

	ObjectDeleter<advisory_locking> lockingDeleter;
	struct advisory_locking* locking = NULL;

	while (get_advisory_locking(vnode) == NULL) {
		// no locking object set on the vnode yet, create one
		if (locking == NULL) {
			locking = new(std::nothrow) advisory_locking;
			if (locking == NULL)
				return B_NO_MEMORY;
			lockingDeleter.SetTo(locking);

			locking->wait_sem = create_sem(0, "advisory lock");
			if (locking->wait_sem < 0)
				return locking->wait_sem;

			locking->lock = create_sem(0, "advisory locking");
			if (locking->lock < 0)
				return locking->lock;
		}

		// set our newly created locking object
		ReadLocker _(sVnodeLock);
		AutoLocker<Vnode> nodeLocker(vnode);
		if (vnode->advisory_locking == NULL) {
			vnode->advisory_locking = locking;
			lockingDeleter.Detach();
			return B_OK;
		}
	}

	// The vnode already had a locking object. That's just as well.

	return B_OK;
}


/*!	Retrieves the first lock that has been set by the current team.
*/
static status_t
get_advisory_lock(struct vnode* vnode, struct flock* flock)
{
	struct advisory_locking* locking = get_advisory_locking(vnode);
	if (locking == NULL)
		return B_BAD_VALUE;

	// TODO: this should probably get the flock by its file descriptor!
	team_id team = team_get_current_team_id();
	status_t status = B_BAD_VALUE;

	LockList::Iterator iterator = locking->locks.GetIterator();
	while (iterator.HasNext()) {
		struct advisory_lock* lock = iterator.Next();

		if (lock->team == team) {
			flock->l_start = lock->start;
			flock->l_len = lock->end - lock->start + 1;
			status = B_OK;
			break;
		}
	}

	put_advisory_locking(locking);
	return status;
}


/*! Returns \c true when either \a flock is \c NULL or the \a flock intersects
	with the advisory_lock \a lock.
*/
static bool
advisory_lock_intersects(struct advisory_lock* lock, struct flock* flock)
{
	if (flock == NULL)
		return true;

	return lock->start <= flock->l_start - 1 + flock->l_len
		&& lock->end >= flock->l_start;
}


/*!	Removes the specified lock, or all locks of the calling team
	if \a flock is NULL.
*/
static status_t
release_advisory_lock(struct vnode* vnode, struct flock* flock)
{
	FUNCTION(("release_advisory_lock(vnode = %p, flock = %p)\n", vnode, flock));

	struct advisory_locking* locking = get_advisory_locking(vnode);
	if (locking == NULL)
		return B_OK;

	// TODO: use the thread ID instead??
	team_id team = team_get_current_team_id();
	pid_t session = thread_get_current_thread()->team->session_id;

	// find matching lock entries

	LockList::Iterator iterator = locking->locks.GetIterator();
	while (iterator.HasNext()) {
		struct advisory_lock* lock = iterator.Next();
		bool removeLock = false;

		if (lock->session == session)
			removeLock = true;
		else if (lock->team == team && advisory_lock_intersects(lock, flock)) {
			bool endsBeyond = false;
			bool startsBefore = false;
			if (flock != NULL) {
				startsBefore = lock->start < flock->l_start;
				endsBeyond = lock->end > flock->l_start - 1 + flock->l_len;
			}

			if (!startsBefore && !endsBeyond) {
				// lock is completely contained in flock
				removeLock = true;
			} else if (startsBefore && !endsBeyond) {
				// cut the end of the lock
				lock->end = flock->l_start - 1;
			} else if (!startsBefore && endsBeyond) {
				// cut the start of the lock
				lock->start = flock->l_start + flock->l_len;
			} else {
				// divide the lock into two locks
				struct advisory_lock* secondLock = new advisory_lock;
				if (secondLock == NULL) {
					// TODO: we should probably revert the locks we already
					// changed... (ie. allocate upfront)
					put_advisory_locking(locking);
					return B_NO_MEMORY;
				}

				lock->end = flock->l_start - 1;

				secondLock->team = lock->team;
				secondLock->session = lock->session;
				// values must already be normalized when getting here
				secondLock->start = flock->l_start + flock->l_len;
				secondLock->end = lock->end;
				secondLock->shared = lock->shared;

				locking->locks.Add(secondLock);
			}
		}

		if (removeLock) {
			// this lock is no longer used
			iterator.Remove();
			free(lock);
		}
	}

	bool removeLocking = locking->locks.IsEmpty();
	release_sem_etc(locking->wait_sem, 1, B_RELEASE_ALL);

	put_advisory_locking(locking);

	if (removeLocking) {
		// We can remove the whole advisory locking structure; it's no
		// longer used
		locking = get_advisory_locking(vnode);
		if (locking != NULL) {
			ReadLocker locker(sVnodeLock);
			AutoLocker<Vnode> nodeLocker(vnode);

			// the locking could have been changed in the mean time
			if (locking->locks.IsEmpty()) {
				vnode->advisory_locking = NULL;
				nodeLocker.Unlock();
				locker.Unlock();

				// we've detached the locking from the vnode, so we can
				// safely delete it
				delete locking;
			} else {
				// the locking is in use again
				nodeLocker.Unlock();
				locker.Unlock();
				release_sem_etc(locking->lock, 1, B_DO_NOT_RESCHEDULE);
			}
		}
	}

	return B_OK;
}


/*!	Acquires an advisory lock for the \a vnode. If \a wait is \c true, it
	will wait for the lock to become available, if there are any collisions
	(it will return B_PERMISSION_DENIED in this case if \a wait is \c false).

	If \a session is -1, POSIX semantics are used for this lock. Otherwise,
	BSD flock() semantics are used, that is, all children can unlock the file
	in question (we even allow parents to remove the lock, though, but that
	seems to be in line to what the BSD's are doing).
*/
static status_t
acquire_advisory_lock(struct vnode* vnode, pid_t session, struct flock* flock,
	bool wait)
{
	FUNCTION(("acquire_advisory_lock(vnode = %p, flock = %p, wait = %s)\n",
		vnode, flock, wait ? "yes" : "no"));

	bool shared = flock->l_type == F_RDLCK;
	status_t status = B_OK;

	// TODO: do deadlock detection!

	struct advisory_locking* locking;

	while (true) {
		// if this vnode has an advisory_locking structure attached,
		// lock that one and search for any colliding file lock
		status = create_advisory_locking(vnode);
		if (status != B_OK)
			return status;

		locking = vnode->advisory_locking;
		team_id team = team_get_current_team_id();
		sem_id waitForLock = -1;

		// test for collisions
		LockList::Iterator iterator = locking->locks.GetIterator();
		while (iterator.HasNext()) {
			struct advisory_lock* lock = iterator.Next();

			// TODO: locks from the same team might be joinable!
			if (lock->team != team && advisory_lock_intersects(lock, flock)) {
				// locks do overlap
				if (!shared || !lock->shared) {
					// we need to wait
					waitForLock = locking->wait_sem;
					break;
				}
			}
		}

		if (waitForLock < 0)
			break;

		// We need to wait. Do that or fail now, if we've been asked not to.

		if (!wait) {
			put_advisory_locking(locking);
			return session != -1 ? B_WOULD_BLOCK : B_PERMISSION_DENIED;
		}

		status = switch_sem_etc(locking->lock, waitForLock, 1,
			B_CAN_INTERRUPT, 0);
		if (status != B_OK && status != B_BAD_SEM_ID)
			return status;

		// We have been notified, but we need to re-lock the locking object. So
		// go another round...
	}

	// install new lock

	struct advisory_lock* lock = (struct advisory_lock*)malloc(
		sizeof(struct advisory_lock));
	if (lock == NULL) {
		put_advisory_locking(locking);
		return B_NO_MEMORY;
	}

	lock->team = team_get_current_team_id();
	lock->session = session;
	// values must already be normalized when getting here
	lock->start = flock->l_start;
	lock->end = flock->l_start - 1 + flock->l_len;
	lock->shared = shared;

	locking->locks.Add(lock);
	put_advisory_locking(locking);

	return status;
}


/*!	Normalizes the \a flock structure to make it easier to compare the
	structure with others. The l_start and l_len fields are set to absolute
	values according to the l_whence field.
*/
static status_t
normalize_flock(struct file_descriptor* descriptor, struct flock* flock)
{
	switch (flock->l_whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			flock->l_start += descriptor->pos;
			break;
		case SEEK_END:
		{
			struct vnode* vnode = descriptor->u.vnode;
			struct stat stat;
			status_t status;

			if (!HAS_FS_CALL(vnode, read_stat))
				return B_UNSUPPORTED;

			status = FS_CALL(vnode, read_stat, &stat);
			if (status != B_OK)
				return status;

			flock->l_start += stat.st_size;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	if (flock->l_start < 0)
		flock->l_start = 0;
	if (flock->l_len == 0)
		flock->l_len = OFF_MAX;

	// don't let the offset and length overflow
	if (flock->l_start > 0 && OFF_MAX - flock->l_start < flock->l_len)
		flock->l_len = OFF_MAX - flock->l_start;

	if (flock->l_len < 0) {
		// a negative length reverses the region
		flock->l_start += flock->l_len;
		flock->l_len = -flock->l_len;
	}

	return B_OK;
}


static void
replace_vnode_if_disconnected(struct fs_mount* mount,
	struct vnode* vnodeToDisconnect, struct vnode*& vnode,
	struct vnode* fallBack, bool lockRootLock)
{
	struct vnode* givenVnode = vnode;
	bool vnodeReplaced = false;

	ReadLocker vnodeReadLocker(sVnodeLock);

	if (lockRootLock)
		mutex_lock(&sIOContextRootLock);

	while (vnode != NULL && vnode->mount == mount
		&& (vnodeToDisconnect == NULL || vnodeToDisconnect == vnode)) {
		if (vnode->covers != NULL) {
			// redirect the vnode to the covered vnode
			vnode = vnode->covers;
		} else
			vnode = fallBack;

		vnodeReplaced = true;
	}

	// If we've replaced the node, grab a reference for the new one.
	if (vnodeReplaced && vnode != NULL)
		inc_vnode_ref_count(vnode);

	if (lockRootLock)
		mutex_unlock(&sIOContextRootLock);

	vnodeReadLocker.Unlock();

	if (vnodeReplaced)
		put_vnode(givenVnode);
}


/*!	Disconnects all file descriptors that are associated with the
	\a vnodeToDisconnect, or if this is NULL, all vnodes of the specified
	\a mount object.

	Note, after you've called this function, there might still be ongoing
	accesses - they won't be interrupted if they already happened before.
	However, any subsequent access will fail.

	This is not a cheap function and should be used with care and rarely.
	TODO: there is currently no means to stop a blocking read/write!
*/
static void
disconnect_mount_or_vnode_fds(struct fs_mount* mount,
	struct vnode* vnodeToDisconnect)
{
	// iterate over all teams and peek into their file descriptors
	TeamListIterator teamIterator;
	while (Team* team = teamIterator.Next()) {
		BReference<Team> teamReference(team, true);

		// lock the I/O context
		io_context* context = team->io_context;
		MutexLocker contextLocker(context->io_mutex);

		replace_vnode_if_disconnected(mount, vnodeToDisconnect, context->root,
			sRoot, true);
		replace_vnode_if_disconnected(mount, vnodeToDisconnect, context->cwd,
			sRoot, false);

		for (uint32 i = 0; i < context->table_size; i++) {
			if (struct file_descriptor* descriptor = context->fds[i]) {
				inc_fd_ref_count(descriptor);

				// if this descriptor points at this mount, we
				// need to disconnect it to be able to unmount
				struct vnode* vnode = fd_vnode(descriptor);
				if (vnodeToDisconnect != NULL) {
					if (vnode == vnodeToDisconnect)
						disconnect_fd(descriptor);
				} else if ((vnode != NULL && vnode->mount == mount)
					|| (vnode == NULL && descriptor->u.mount == mount))
					disconnect_fd(descriptor);

				put_fd(descriptor);
			}
		}
	}
}


/*!	\brief Gets the root node of the current IO context.
	If \a kernel is \c true, the kernel IO context will be used.
	The caller obtains a reference to the returned node.
*/
struct vnode*
get_root_vnode(bool kernel)
{
	if (!kernel) {
		// Get current working directory from io context
		struct io_context* context = get_current_io_context(kernel);

		mutex_lock(&sIOContextRootLock);

		struct vnode* root = context->root;
		if (root != NULL)
			inc_vnode_ref_count(root);

		mutex_unlock(&sIOContextRootLock);

		if (root != NULL)
			return root;

		// That should never happen.
		dprintf("get_root_vnode(): IO context for team %ld doesn't have a "
			"root\n", team_get_current_team_id());
	}

	inc_vnode_ref_count(sRoot);
	return sRoot;
}


/*!	\brief Resolves a vnode to the vnode it is covered by, if any.

	Given an arbitrary vnode (identified by mount and node ID), the function
	checks, whether the vnode is covered by another vnode. If it is, the
	function returns the mount and node ID of the covering vnode. Otherwise
	it simply returns the supplied mount and node ID.

	In case of error (e.g. the supplied node could not be found) the variables
	for storing the resolved mount and node ID remain untouched and an error
	code is returned.

	\param mountID The mount ID of the vnode in question.
	\param nodeID The node ID of the vnode in question.
	\param resolvedMountID Pointer to storage for the resolved mount ID.
	\param resolvedNodeID Pointer to storage for the resolved node ID.
	\return
	- \c B_OK, if everything went fine,
	- another error code, if something went wrong.
*/
status_t
vfs_resolve_vnode_to_covering_vnode(dev_t mountID, ino_t nodeID,
	dev_t* resolvedMountID, ino_t* resolvedNodeID)
{
	// get the node
	struct vnode* node;
	status_t error = get_vnode(mountID, nodeID, &node, true, false);
	if (error != B_OK)
		return error;

	// resolve the node
	if (Vnode* coveringNode = get_covering_vnode(node)) {
		put_vnode(node);
		node = coveringNode;
	}

	// set the return values
	*resolvedMountID = node->device;
	*resolvedNodeID = node->id;

	put_vnode(node);

	return B_OK;
}


/*!	\brief Gets the directory path and leaf name for a given path.

	The supplied \a path is transformed to refer to the directory part of
	the entry identified by the original path, and into the buffer \a filename
	the leaf name of the original entry is written.
	Neither the returned path nor the leaf name can be expected to be
	canonical.

	\param path The path to be analyzed. Must be able to store at least one
		   additional character.
	\param filename The buffer into which the leaf name will be written.
		   Must be of size B_FILE_NAME_LENGTH at least.
	\return \c B_OK, if everything went fine, \c B_NAME_TOO_LONG, if the leaf
		   name is longer than \c B_FILE_NAME_LENGTH, or \c B_ENTRY_NOT_FOUND,
		   if the given path name is empty.
*/
static status_t
get_dir_path_and_leaf(char* path, char* filename)
{
	if (*path == '\0')
		return B_ENTRY_NOT_FOUND;

	char* last = strrchr(path, '/');
		// '/' are not allowed in file names!

	FUNCTION(("get_dir_path_and_leaf(path = %s)\n", path));

	if (last == NULL) {
		// this path is single segment with no '/' in it
		// ex. "foo"
		if (strlcpy(filename, path, B_FILE_NAME_LENGTH) >= B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		strcpy(path, ".");
	} else {
		last++;
		if (last[0] == '\0') {
			// special case: the path ends in one or more '/' - remove them
			while (*--last == '/' && last != path);
			last[1] = '\0';

			if (last == path && last[0] == '/') {
				// This path points to the root of the file system
				strcpy(filename, ".");
				return B_OK;
			}
			for (; last != path && *(last - 1) != '/'; last--);
				// rewind to the start of the leaf before the '/'
		}

		// normal leaf: replace the leaf portion of the path with a '.'
		if (strlcpy(filename, last, B_FILE_NAME_LENGTH) >= B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		last[0] = '.';
		last[1] = '\0';
	}
	return B_OK;
}


static status_t
entry_ref_to_vnode(dev_t mountID, ino_t directoryID, const char* name,
	bool traverse, bool kernel, struct vnode** _vnode)
{
	char clonedName[B_FILE_NAME_LENGTH + 1];
	if (strlcpy(clonedName, name, B_FILE_NAME_LENGTH) >= B_FILE_NAME_LENGTH)
		return B_NAME_TOO_LONG;

	// get the directory vnode and let vnode_path_to_vnode() do the rest
	struct vnode* directory;

	status_t status = get_vnode(mountID, directoryID, &directory, true, false);
	if (status < 0)
		return status;

	return vnode_path_to_vnode(directory, clonedName, traverse, 0, kernel,
		_vnode, NULL);
}


/*!	Looks up the entry with name \a name in the directory represented by \a dir
	and returns the respective vnode.
	On success a reference to the vnode is acquired for the caller.
*/
static status_t
lookup_dir_entry(struct vnode* dir, const char* name, struct vnode** _vnode)
{
	ino_t id;

	if (dir->mount->entry_cache.Lookup(dir->id, name, id))
		return get_vnode(dir->device, id, _vnode, true, false);

	status_t status = FS_CALL(dir, lookup, name, &id);
	if (status != B_OK)
		return status;

	// The lookup() hook call get_vnode() or publish_vnode(), so we do already
	// have a reference and just need to look the node up.
	rw_lock_read_lock(&sVnodeLock);
	*_vnode = lookup_vnode(dir->device, id);
	rw_lock_read_unlock(&sVnodeLock);

	if (*_vnode == NULL) {
		panic("lookup_dir_entry(): could not lookup vnode (mountid 0x%lx vnid "
			"0x%Lx)\n", dir->device, id);
		return B_ENTRY_NOT_FOUND;
	}

//	ktrace_printf("lookup_dir_entry(): dir: %p (%ld, %lld), name: \"%s\" -> "
//		"%p (%ld, %lld)", dir, dir->mount->id, dir->id, name, *_vnode,
//		(*_vnode)->mount->id, (*_vnode)->id);

	return B_OK;
}


/*!	Returns the vnode for the relative path starting at the specified \a vnode.
	\a path must not be NULL.
	If it returns successfully, \a path contains the name of the last path
	component. This function clobbers the buffer pointed to by \a path only
	if it does contain more than one component.
	Note, this reduces the ref_count of the starting \a vnode, no matter if
	it is successful or not!
*/
static status_t
vnode_path_to_vnode(struct vnode* vnode, char* path, bool traverseLeafLink,
	int count, struct io_context* ioContext, struct vnode** _vnode,
	ino_t* _parentID)
{
	status_t status = B_OK;
	ino_t lastParentID = vnode->id;

	FUNCTION(("vnode_path_to_vnode(vnode = %p, path = %s)\n", vnode, path));

	if (path == NULL) {
		put_vnode(vnode);
		return B_BAD_VALUE;
	}

	if (*path == '\0') {
		put_vnode(vnode);
		return B_ENTRY_NOT_FOUND;
	}

	while (true) {
		struct vnode* nextVnode;
		char* nextPath;

		TRACE(("vnode_path_to_vnode: top of loop. p = %p, p = '%s'\n", path,
			path));

		// done?
		if (path[0] == '\0')
			break;

		// walk to find the next path component ("path" will point to a single
		// path component), and filter out multiple slashes
		for (nextPath = path + 1; *nextPath != '\0' && *nextPath != '/';
				nextPath++);

		if (*nextPath == '/') {
			*nextPath = '\0';
			do
				nextPath++;
			while (*nextPath == '/');
		}

		// See if the '..' is at a covering vnode move to the covered
		// vnode so we pass the '..' path to the underlying filesystem.
		// Also prevent breaking the root of the IO context.
		if (strcmp("..", path) == 0) {
			if (vnode == ioContext->root) {
				// Attempted prison break! Keep it contained.
				path = nextPath;
				continue;
			}

			if (Vnode* coveredVnode = get_covered_vnode(vnode)) {
				nextVnode = coveredVnode;
				put_vnode(vnode);
				vnode = nextVnode;
			}
		}

		// check if vnode is really a directory
		if (status == B_OK && !S_ISDIR(vnode->Type()))
			status = B_NOT_A_DIRECTORY;

		// Check if we have the right to search the current directory vnode.
		// If a file system doesn't have the access() function, we assume that
		// searching a directory is always allowed
		if (status == B_OK && HAS_FS_CALL(vnode, access))
			status = FS_CALL(vnode, access, X_OK);

		// Tell the filesystem to get the vnode of this path component (if we
		// got the permission from the call above)
		if (status == B_OK)
			status = lookup_dir_entry(vnode, path, &nextVnode);

		if (status != B_OK) {
			put_vnode(vnode);
			return status;
		}

		// If the new node is a symbolic link, resolve it (if we've been told
		// to do it)
		if (S_ISLNK(nextVnode->Type())
			&& (traverseLeafLink || nextPath[0] != '\0')) {
			size_t bufferSize;
			char* buffer;

			TRACE(("traverse link\n"));

			// it's not exactly nice style using goto in this way, but hey,
			// it works :-/
			if (count + 1 > B_MAX_SYMLINKS) {
				status = B_LINK_LIMIT;
				goto resolve_link_error;
			}

			buffer = (char*)malloc(bufferSize = B_PATH_NAME_LENGTH);
			if (buffer == NULL) {
				status = B_NO_MEMORY;
				goto resolve_link_error;
			}

			if (HAS_FS_CALL(nextVnode, read_symlink)) {
				bufferSize--;
				status = FS_CALL(nextVnode, read_symlink, buffer, &bufferSize);
				// null-terminate
				if (status >= 0)
					buffer[bufferSize] = '\0';
			} else
				status = B_BAD_VALUE;

			if (status != B_OK) {
				free(buffer);

		resolve_link_error:
				put_vnode(vnode);
				put_vnode(nextVnode);

				return status;
			}
			put_vnode(nextVnode);

			// Check if we start from the root directory or the current
			// directory ("vnode" still points to that one).
			// Cut off all leading slashes if it's the root directory
			path = buffer;
			bool absoluteSymlink = false;
			if (path[0] == '/') {
				// we don't need the old directory anymore
				put_vnode(vnode);

				while (*++path == '/')
					;

				mutex_lock(&sIOContextRootLock);
				vnode = ioContext->root;
				inc_vnode_ref_count(vnode);
				mutex_unlock(&sIOContextRootLock);

				absoluteSymlink = true;
			}

			inc_vnode_ref_count(vnode);
				// balance the next recursion - we will decrement the
				// ref_count of the vnode, no matter if we succeeded or not

			if (absoluteSymlink && *path == '\0') {
				// symlink was just "/"
				nextVnode = vnode;
			} else {
				status = vnode_path_to_vnode(vnode, path, true, count + 1,
					ioContext, &nextVnode, &lastParentID);
			}

			free(buffer);

			if (status != B_OK) {
				put_vnode(vnode);
				return status;
			}
		} else
			lastParentID = vnode->id;

		// decrease the ref count on the old dir we just looked up into
		put_vnode(vnode);

		path = nextPath;
		vnode = nextVnode;

		// see if we hit a covered node
		if (Vnode* coveringNode = get_covering_vnode(vnode)) {
			put_vnode(vnode);
			vnode = coveringNode;
		}
	}

	*_vnode = vnode;
	if (_parentID)
		*_parentID = lastParentID;

	return B_OK;
}


static status_t
vnode_path_to_vnode(struct vnode* vnode, char* path, bool traverseLeafLink,
	int count, bool kernel, struct vnode** _vnode, ino_t* _parentID)
{
	return vnode_path_to_vnode(vnode, path, traverseLeafLink, count,
		get_current_io_context(kernel), _vnode, _parentID);
}


static status_t
path_to_vnode(char* path, bool traverseLink, struct vnode** _vnode,
	ino_t* _parentID, bool kernel)
{
	struct vnode* start = NULL;

	FUNCTION(("path_to_vnode(path = \"%s\")\n", path));

	if (!path)
		return B_BAD_VALUE;

	if (*path == '\0')
		return B_ENTRY_NOT_FOUND;

	// figure out if we need to start at root or at cwd
	if (*path == '/') {
		if (sRoot == NULL) {
			// we're a bit early, aren't we?
			return B_ERROR;
		}

		while (*++path == '/')
			;
		start = get_root_vnode(kernel);

		if (*path == '\0') {
			*_vnode = start;
			return B_OK;
		}

	} else {
		struct io_context* context = get_current_io_context(kernel);

		mutex_lock(&context->io_mutex);
		start = context->cwd;
		if (start != NULL)
			inc_vnode_ref_count(start);
		mutex_unlock(&context->io_mutex);

		if (start == NULL)
			return B_ERROR;
	}

	return vnode_path_to_vnode(start, path, traverseLink, 0, kernel, _vnode,
		_parentID);
}


/*! Returns the vnode in the next to last segment of the path, and returns
	the last portion in filename.
	The path buffer must be able to store at least one additional character.
*/
static status_t
path_to_dir_vnode(char* path, struct vnode** _vnode, char* filename,
	bool kernel)
{
	status_t status = get_dir_path_and_leaf(path, filename);
	if (status != B_OK)
		return status;

	return path_to_vnode(path, true, _vnode, NULL, kernel);
}


/*!	\brief Retrieves the directory vnode and the leaf name of an entry referred
		   to by a FD + path pair.

	\a path must be given in either case. \a fd might be omitted, in which
	case \a path is either an absolute path or one relative to the current
	directory. If both a supplied and \a path is relative it is reckoned off
	of the directory referred to by \a fd. If \a path is absolute \a fd is
	ignored.

	The caller has the responsibility to call put_vnode() on the returned
	directory vnode.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. Must not be \c NULL. The buffer
	       is modified by this function. It must have at least room for a
	       string one character longer than the path it contains.
	\param _vnode A pointer to a variable the directory vnode shall be written
		   into.
	\param filename A buffer of size B_FILE_NAME_LENGTH or larger into which
		   the leaf name of the specified entry will be written.
	\param kernel \c true, if invoked from inside the kernel, \c false if
		   invoked from userland.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
static status_t
fd_and_path_to_dir_vnode(int fd, char* path, struct vnode** _vnode,
	char* filename, bool kernel)
{
	if (!path)
		return B_BAD_VALUE;
	if (*path == '\0')
		return B_ENTRY_NOT_FOUND;
	if (fd < 0)
		return path_to_dir_vnode(path, _vnode, filename, kernel);

	status_t status = get_dir_path_and_leaf(path, filename);
	if (status != B_OK)
		return status;

	return fd_and_path_to_vnode(fd, path, true, _vnode, NULL, kernel);
}


/*!	\brief Retrieves the directory vnode and the leaf name of an entry referred
		   to by a vnode + path pair.

	\a path must be given in either case. \a vnode might be omitted, in which
	case \a path is either an absolute path or one relative to the current
	directory. If both a supplied and \a path is relative it is reckoned off
	of the directory referred to by \a vnode. If \a path is absolute \a vnode is
	ignored.

	The caller has the responsibility to call put_vnode() on the returned
	directory vnode.

	\param vnode The vnode. May be \c NULL.
	\param path The absolute or relative path. Must not be \c NULL. The buffer
	       is modified by this function. It must have at least room for a
	       string one character longer than the path it contains.
	\param _vnode A pointer to a variable the directory vnode shall be written
		   into.
	\param filename A buffer of size B_FILE_NAME_LENGTH or larger into which
		   the leaf name of the specified entry will be written.
	\param kernel \c true, if invoked from inside the kernel, \c false if
		   invoked from userland.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
static status_t
vnode_and_path_to_dir_vnode(struct vnode* vnode, char* path,
	struct vnode** _vnode, char* filename, bool kernel)
{
	if (!path)
		return B_BAD_VALUE;
	if (*path == '\0')
		return B_ENTRY_NOT_FOUND;
	if (vnode == NULL || path[0] == '/')
		return path_to_dir_vnode(path, _vnode, filename, kernel);

	status_t status = get_dir_path_and_leaf(path, filename);
	if (status != B_OK)
		return status;

	inc_vnode_ref_count(vnode);
		// vnode_path_to_vnode() always decrements the ref count

	return vnode_path_to_vnode(vnode, path, true, 0, kernel, _vnode, NULL);
}


/*! Returns a vnode's name in the d_name field of a supplied dirent buffer.
*/
static status_t
get_vnode_name(struct vnode* vnode, struct vnode* parent, struct dirent* buffer,
	size_t bufferSize, struct io_context* ioContext)
{
	if (bufferSize < sizeof(struct dirent))
		return B_BAD_VALUE;

	// See if the vnode is convering another vnode and move to the covered
	// vnode so we get the underlying file system
	VNodePutter vnodePutter;
	if (Vnode* coveredVnode = get_covered_vnode(vnode)) {
		vnode = coveredVnode;
		vnodePutter.SetTo(vnode);
	}

	if (HAS_FS_CALL(vnode, get_vnode_name)) {
		// The FS supports getting the name of a vnode.
		if (FS_CALL(vnode, get_vnode_name, buffer->d_name,
			(char*)buffer + bufferSize - buffer->d_name) == B_OK)
			return B_OK;
	}

	// The FS doesn't support getting the name of a vnode. So we search the
	// parent directory for the vnode, if the caller let us.

	if (parent == NULL || !HAS_FS_CALL(parent, read_dir))
		return B_UNSUPPORTED;

	void* cookie;

	status_t status = FS_CALL(parent, open_dir, &cookie);
	if (status >= B_OK) {
		while (true) {
			uint32 num = 1;
			// We use the FS hook directly instead of dir_read(), since we don't
			// want the entries to be fixed. We have already resolved vnode to
			// the covered node.
			status = FS_CALL(parent, read_dir, cookie, buffer, bufferSize,
				&num);
			if (status != B_OK)
				break;
			if (num == 0) {
				status = B_ENTRY_NOT_FOUND;
				break;
			}

			if (vnode->id == buffer->d_ino) {
				// found correct entry!
				break;
			}
		}

		FS_CALL(vnode, close_dir, cookie);
		FS_CALL(vnode, free_dir_cookie, cookie);
	}
	return status;
}


static status_t
get_vnode_name(struct vnode* vnode, struct vnode* parent, char* name,
	size_t nameSize, bool kernel)
{
	char buffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
	struct dirent* dirent = (struct dirent*)buffer;

	status_t status = get_vnode_name(vnode, parent, dirent, sizeof(buffer),
		get_current_io_context(kernel));
	if (status != B_OK)
		return status;

	if (strlcpy(name, dirent->d_name, nameSize) >= nameSize)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


/*!	Gets the full path to a given directory vnode.
	It uses the fs_get_vnode_name() call to get the name of a vnode; if a
	file system doesn't support this call, it will fall back to iterating
	through the parent directory to get the name of the child.

	To protect against circular loops, it supports a maximum tree depth
	of 256 levels.

	Note that the path may not be correct the time this function returns!
	It doesn't use any locking to prevent returning the correct path, as
	paths aren't safe anyway: the path to a file can change at any time.

	It might be a good idea, though, to check if the returned path exists
	in the calling function (it's not done here because of efficiency)
*/
static status_t
dir_vnode_to_path(struct vnode* vnode, char* buffer, size_t bufferSize,
	bool kernel)
{
	FUNCTION(("dir_vnode_to_path(%p, %p, %lu)\n", vnode, buffer, bufferSize));

	if (vnode == NULL || buffer == NULL || bufferSize == 0)
		return B_BAD_VALUE;

	if (!S_ISDIR(vnode->Type()))
		return B_NOT_A_DIRECTORY;

	char* path = buffer;
	int32 insert = bufferSize;
	int32 maxLevel = 256;
	int32 length;
	status_t status;
	struct io_context* ioContext = get_current_io_context(kernel);

	// we don't use get_vnode() here because this call is more
	// efficient and does all we need from get_vnode()
	inc_vnode_ref_count(vnode);

	if (vnode != ioContext->root) {
		// we don't hit the IO context root
		// resolve a vnode to its covered vnode
		if (Vnode* coveredVnode = get_covered_vnode(vnode)) {
			put_vnode(vnode);
			vnode = coveredVnode;
		}
	}

	path[--insert] = '\0';
		// the path is filled right to left

	while (true) {
		// the name buffer is also used for fs_read_dir()
		char nameBuffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
		char* name = &((struct dirent*)nameBuffer)->d_name[0];
		struct vnode* parentVnode;
		ino_t parentID;

		// lookup the parent vnode
		if (vnode == ioContext->root) {
			// we hit the IO context root
			parentVnode = vnode;
			inc_vnode_ref_count(vnode);
		} else {
			status = lookup_dir_entry(vnode, "..", &parentVnode);
			if (status != B_OK)
				goto out;
		}

		// get the node's name
		status = get_vnode_name(vnode, parentVnode, (struct dirent*)nameBuffer,
			sizeof(nameBuffer), ioContext);

		if (vnode != ioContext->root) {
			// we don't hit the IO context root
			// resolve a vnode to its covered vnode
			if (Vnode* coveredVnode = get_covered_vnode(parentVnode)) {
				put_vnode(parentVnode);
				parentVnode = coveredVnode;
				parentID = parentVnode->id;
			}
		}

		bool hitRoot = (parentVnode == vnode);

		// release the current vnode, we only need its parent from now on
		put_vnode(vnode);
		vnode = parentVnode;

		if (status != B_OK)
			goto out;

		if (hitRoot) {
			// we have reached "/", which means we have constructed the full
			// path
			break;
		}

		// TODO: add an explicit check for loops in about 10 levels to do
		// real loop detection

		// don't go deeper as 'maxLevel' to prevent circular loops
		if (maxLevel-- < 0) {
			status = B_LINK_LIMIT;
			goto out;
		}

		// add the name in front of the current path
		name[B_FILE_NAME_LENGTH - 1] = '\0';
		length = strlen(name);
		insert -= length;
		if (insert <= 0) {
			status = B_RESULT_NOT_REPRESENTABLE;
			goto out;
		}
		memcpy(path + insert, name, length);
		path[--insert] = '/';
	}

	// the root dir will result in an empty path: fix it
	if (path[insert] == '\0')
		path[--insert] = '/';

	TRACE(("  path is: %s\n", path + insert));

	// move the path to the start of the buffer
	length = bufferSize - insert;
	memmove(buffer, path + insert, length);

out:
	put_vnode(vnode);
	return status;
}


/*!	Checks the length of every path component, and adds a '.'
	if the path ends in a slash.
	The given path buffer must be able to store at least one
	additional character.
*/
static status_t
check_path(char* to)
{
	int32 length = 0;

	// check length of every path component

	while (*to) {
		char* begin;
		if (*to == '/')
			to++, length++;

		begin = to;
		while (*to != '/' && *to)
			to++, length++;

		if (to - begin > B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;
	}

	if (length == 0)
		return B_ENTRY_NOT_FOUND;

	// complete path if there is a slash at the end

	if (*(to - 1) == '/') {
		if (length > B_PATH_NAME_LENGTH - 2)
			return B_NAME_TOO_LONG;

		to[0] = '.';
		to[1] = '\0';
	}

	return B_OK;
}


static struct file_descriptor*
get_fd_and_vnode(int fd, struct vnode** _vnode, bool kernel)
{
	struct file_descriptor* descriptor
		= get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	struct vnode* vnode = fd_vnode(descriptor);
	if (vnode == NULL) {
		put_fd(descriptor);
		return NULL;
	}

	// ToDo: when we can close a file descriptor at any point, investigate
	//	if this is still valid to do (accessing the vnode without ref_count
	//	or locking)
	*_vnode = vnode;
	return descriptor;
}


static struct vnode*
get_vnode_from_fd(int fd, bool kernel)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	vnode = fd_vnode(descriptor);
	if (vnode != NULL)
		inc_vnode_ref_count(vnode);

	put_fd(descriptor);
	return vnode;
}


/*!	Gets the vnode from an FD + path combination. If \a fd is lower than zero,
	only the path will be considered. In this case, the \a path must not be
	NULL.
	If \a fd is a valid file descriptor, \a path may be NULL for directories,
	and should be NULL for files.
*/
static status_t
fd_and_path_to_vnode(int fd, char* path, bool traverseLeafLink,
	struct vnode** _vnode, ino_t* _parentID, bool kernel)
{
	if (fd < 0 && !path)
		return B_BAD_VALUE;

	if (path != NULL && *path == '\0')
		return B_ENTRY_NOT_FOUND;

	if (fd < 0 || (path != NULL && path[0] == '/')) {
		// no FD or absolute path
		return path_to_vnode(path, traverseLeafLink, _vnode, _parentID, kernel);
	}

	// FD only, or FD + relative path
	struct vnode* vnode = get_vnode_from_fd(fd, kernel);
	if (!vnode)
		return B_FILE_ERROR;

	if (path != NULL) {
		return vnode_path_to_vnode(vnode, path, traverseLeafLink, 0, kernel,
			_vnode, _parentID);
	}

	// there is no relative path to take into account

	*_vnode = vnode;
	if (_parentID)
		*_parentID = -1;

	return B_OK;
}


static int
get_new_fd(int type, struct fs_mount* mount, struct vnode* vnode,
	void* cookie, int openMode, bool kernel)
{
	struct file_descriptor* descriptor;
	int fd;

	// If the vnode is locked, we don't allow creating a new file/directory
	// file_descriptor for it
	if (vnode && vnode->mandatory_locked_by != NULL
		&& (type == FDTYPE_FILE || type == FDTYPE_DIR))
		return B_BUSY;

	descriptor = alloc_fd();
	if (!descriptor)
		return B_NO_MEMORY;

	if (vnode)
		descriptor->u.vnode = vnode;
	else
		descriptor->u.mount = mount;
	descriptor->cookie = cookie;

	switch (type) {
		// vnode types
		case FDTYPE_FILE:
			descriptor->ops = &sFileOps;
			break;
		case FDTYPE_DIR:
			descriptor->ops = &sDirectoryOps;
			break;
		case FDTYPE_ATTR:
			descriptor->ops = &sAttributeOps;
			break;
		case FDTYPE_ATTR_DIR:
			descriptor->ops = &sAttributeDirectoryOps;
			break;

		// mount types
		case FDTYPE_INDEX_DIR:
			descriptor->ops = &sIndexDirectoryOps;
			break;
		case FDTYPE_QUERY:
			descriptor->ops = &sQueryOps;
			break;

		default:
			panic("get_new_fd() called with unknown type %d\n", type);
			break;
	}
	descriptor->type = type;
	descriptor->open_mode = openMode;

	io_context* context = get_current_io_context(kernel);
	fd = new_fd(context, descriptor);
	if (fd < 0) {
		free(descriptor);
		return B_NO_MORE_FDS;
	}

	mutex_lock(&context->io_mutex);
	fd_set_close_on_exec(context, fd, (openMode & O_CLOEXEC) != 0);
	mutex_unlock(&context->io_mutex);

	return fd;
}


/*!	In-place normalizes \a path. It's otherwise semantically equivalent to
	vfs_normalize_path(). See there for more documentation.
*/
static status_t
normalize_path(char* path, size_t pathSize, bool traverseLink, bool kernel)
{
	VNodePutter dirPutter;
	struct vnode* dir = NULL;
	status_t error;

	for (int i = 0; i < B_MAX_SYMLINKS; i++) {
		// get dir vnode + leaf name
		struct vnode* nextDir;
		char leaf[B_FILE_NAME_LENGTH];
		error = vnode_and_path_to_dir_vnode(dir, path, &nextDir, leaf, kernel);
		if (error != B_OK)
			return error;

		dir = nextDir;
		strcpy(path, leaf);
		dirPutter.SetTo(dir);

		// get file vnode, if we shall resolve links
		bool fileExists = false;
		struct vnode* fileVnode;
		VNodePutter fileVnodePutter;
		if (traverseLink) {
			inc_vnode_ref_count(dir);
			if (vnode_path_to_vnode(dir, path, false, 0, kernel, &fileVnode,
					NULL) == B_OK) {
				fileVnodePutter.SetTo(fileVnode);
				fileExists = true;
			}
		}

		if (!fileExists || !traverseLink || !S_ISLNK(fileVnode->Type())) {
			// we're done -- construct the path
			bool hasLeaf = true;
			if (strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0) {
				// special cases "." and ".." -- get the dir, forget the leaf
				inc_vnode_ref_count(dir);
				error = vnode_path_to_vnode(dir, leaf, false, 0, kernel,
					&nextDir, NULL);
				if (error != B_OK)
					return error;
				dir = nextDir;
				dirPutter.SetTo(dir);
				hasLeaf = false;
			}

			// get the directory path
			error = dir_vnode_to_path(dir, path, B_PATH_NAME_LENGTH, kernel);
			if (error != B_OK)
				return error;

			// append the leaf name
			if (hasLeaf) {
				// insert a directory separator if this is not the file system
				// root
				if ((strcmp(path, "/") != 0
					&& strlcat(path, "/", pathSize) >= pathSize)
					|| strlcat(path, leaf, pathSize) >= pathSize) {
					return B_NAME_TOO_LONG;
				}
			}

			return B_OK;
		}

		// read link
		if (HAS_FS_CALL(fileVnode, read_symlink)) {
			size_t bufferSize = B_PATH_NAME_LENGTH - 1;
			error = FS_CALL(fileVnode, read_symlink, path, &bufferSize);
			if (error != B_OK)
				return error;
			path[bufferSize] = '\0';
		} else
			return B_BAD_VALUE;
	}

	return B_LINK_LIMIT;
}


#ifdef ADD_DEBUGGER_COMMANDS


static void
_dump_advisory_locking(advisory_locking* locking)
{
	if (locking == NULL)
		return;

	kprintf("   lock:        %ld", locking->lock);
	kprintf("   wait_sem:    %ld", locking->wait_sem);

	int32 index = 0;
	LockList::Iterator iterator = locking->locks.GetIterator();
	while (iterator.HasNext()) {
		struct advisory_lock* lock = iterator.Next();

		kprintf("   [%2ld] team:   %ld\n", index++, lock->team);
		kprintf("        start:  %Ld\n", lock->start);
		kprintf("        end:    %Ld\n", lock->end);
		kprintf("        shared? %s\n", lock->shared ? "yes" : "no");
	}
}


static void
_dump_mount(struct fs_mount* mount)
{
	kprintf("MOUNT: %p\n", mount);
	kprintf(" id:            %ld\n", mount->id);
	kprintf(" device_name:   %s\n", mount->device_name);
	kprintf(" root_vnode:    %p\n", mount->root_vnode);
	kprintf(" covers:        %p\n", mount->root_vnode->covers);
	kprintf(" partition:     %p\n", mount->partition);
	kprintf(" lock:          %p\n", &mount->rlock);
	kprintf(" flags:        %s%s\n", mount->unmounting ? " unmounting" : "",
		mount->owns_file_device ? " owns_file_device" : "");

	fs_volume* volume = mount->volume;
	while (volume != NULL) {
		kprintf(" volume %p:\n", volume);
		kprintf("  layer:            %ld\n", volume->layer);
		kprintf("  private_volume:   %p\n", volume->private_volume);
		kprintf("  ops:              %p\n", volume->ops);
		kprintf("  file_system:      %p\n", volume->file_system);
		kprintf("  file_system_name: %s\n", volume->file_system_name);
		volume = volume->super_volume;
	}

	set_debug_variable("_volume", (addr_t)mount->volume->private_volume);
	set_debug_variable("_root", (addr_t)mount->root_vnode);
	set_debug_variable("_covers", (addr_t)mount->root_vnode->covers);
	set_debug_variable("_partition", (addr_t)mount->partition);
}


static bool
debug_prepend_vnode_name_to_path(char* buffer, size_t& bufferSize,
	const char* name)
{
	bool insertSlash = buffer[bufferSize] != '\0';
	size_t nameLength = strlen(name);

	if (bufferSize < nameLength + (insertSlash ? 1 : 0))
		return false;

	if (insertSlash)
		buffer[--bufferSize] = '/';

	bufferSize -= nameLength;
	memcpy(buffer + bufferSize, name, nameLength);

	return true;
}


static bool
debug_prepend_vnode_id_to_path(char* buffer, size_t& bufferSize, dev_t devID,
	ino_t nodeID)
{
	if (bufferSize == 0)
		return false;

	bool insertSlash = buffer[bufferSize] != '\0';
	if (insertSlash)
		buffer[--bufferSize] = '/';

	size_t size = snprintf(buffer, bufferSize,
		"<%" B_PRIdDEV ",%" B_PRIdINO ">", devID, nodeID);
	if (size > bufferSize) {
		if (insertSlash)
			bufferSize++;
		return false;
	}

	if (size < bufferSize)
		memmove(buffer + bufferSize - size, buffer, size);

	bufferSize -= size;
	return true;
}


static char*
debug_resolve_vnode_path(struct vnode* vnode, char* buffer, size_t bufferSize,
	bool& _truncated)
{
	// null-terminate the path
	buffer[--bufferSize] = '\0';

	while (true) {
		while (vnode->covers != NULL)
			vnode = vnode->covers;

		if (vnode == sRoot) {
			_truncated = bufferSize == 0;
			if (!_truncated)
				buffer[--bufferSize] = '/';
			return buffer + bufferSize;
		}

		// resolve the name
		ino_t dirID;
		const char* name = vnode->mount->entry_cache.DebugReverseLookup(
			vnode->id, dirID);
		if (name == NULL) {
			// Failed to resolve the name -- prepend "<dev,node>/".
			_truncated = !debug_prepend_vnode_id_to_path(buffer, bufferSize,
				vnode->mount->id, vnode->id);
			return buffer + bufferSize;
		}

		// prepend the name
		if (!debug_prepend_vnode_name_to_path(buffer, bufferSize, name)) {
			_truncated = true;
			return buffer + bufferSize;
		}

		// resolve the directory node
		struct vnode* nextVnode = lookup_vnode(vnode->mount->id, dirID);
		if (nextVnode == NULL) {
			_truncated = !debug_prepend_vnode_id_to_path(buffer, bufferSize,
				vnode->mount->id, dirID);
			return buffer + bufferSize;
		}

		vnode = nextVnode;
	}
}


static void
_dump_vnode(struct vnode* vnode, bool printPath)
{
	kprintf("VNODE: %p\n", vnode);
	kprintf(" device:        %ld\n", vnode->device);
	kprintf(" id:            %Ld\n", vnode->id);
	kprintf(" ref_count:     %ld\n", vnode->ref_count);
	kprintf(" private_node:  %p\n", vnode->private_node);
	kprintf(" mount:         %p\n", vnode->mount);
	kprintf(" covered_by:    %p\n", vnode->covered_by);
	kprintf(" covers:        %p\n", vnode->covers);
	kprintf(" cache:         %p\n", vnode->cache);
	kprintf(" type:          %#" B_PRIx32 "\n", vnode->Type());
	kprintf(" flags:         %s%s%s\n", vnode->IsRemoved() ? "r" : "-",
		vnode->IsBusy() ? "b" : "-", vnode->IsUnpublished() ? "u" : "-");
	kprintf(" advisory_lock: %p\n", vnode->advisory_locking);

	_dump_advisory_locking(vnode->advisory_locking);

	if (printPath) {
		void* buffer = debug_malloc(B_PATH_NAME_LENGTH);
		if (buffer != NULL) {
			bool truncated;
			char* path = debug_resolve_vnode_path(vnode, (char*)buffer,
				B_PATH_NAME_LENGTH, truncated);
			if (path != NULL) {
				kprintf(" path:          ");
				if (truncated)
					kputs("<truncated>/");
				kputs(path);
				kputs("\n");
			} else
				kprintf("Failed to resolve vnode path.\n");

			debug_free(buffer);
		} else
			kprintf("Failed to allocate memory for constructing the path.\n");
	}

	set_debug_variable("_node", (addr_t)vnode->private_node);
	set_debug_variable("_mount", (addr_t)vnode->mount);
	set_debug_variable("_covered_by", (addr_t)vnode->covered_by);
	set_debug_variable("_covers", (addr_t)vnode->covers);
	set_debug_variable("_adv_lock", (addr_t)vnode->advisory_locking);
}


static int
dump_mount(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [id|address]\n", argv[0]);
		return 0;
	}

	uint32 id = parse_expression(argv[1]);
	struct fs_mount* mount = NULL;

	mount = (fs_mount*)hash_lookup(sMountsTable, (void*)&id);
	if (mount == NULL) {
		if (IS_USER_ADDRESS(id)) {
			kprintf("fs_mount not found\n");
			return 0;
		}
		mount = (fs_mount*)id;
	}

	_dump_mount(mount);
	return 0;
}


static int
dump_mounts(int argc, char** argv)
{
	if (argc != 1) {
		kprintf("usage: %s\n", argv[0]);
		return 0;
	}

	kprintf("address     id root       covers     cookie     fs_name\n");

	struct hash_iterator iterator;
	struct fs_mount* mount;

	hash_open(sMountsTable, &iterator);
	while ((mount = (struct fs_mount*)hash_next(sMountsTable, &iterator))
			!= NULL) {
		kprintf("%p%4ld %p %p %p %s\n", mount, mount->id, mount->root_vnode,
			mount->root_vnode->covers, mount->volume->private_volume,
			mount->volume->file_system_name);

		fs_volume* volume = mount->volume;
		while (volume->super_volume != NULL) {
			volume = volume->super_volume;
			kprintf("                                     %p %s\n",
				volume->private_volume, volume->file_system_name);
		}
	}

	hash_close(sMountsTable, &iterator, false);
	return 0;
}


static int
dump_vnode(int argc, char** argv)
{
	bool printPath = false;
	int argi = 1;
	if (argc >= 2 && strcmp(argv[argi], "-p") == 0) {
		printPath = true;
		argi++;
	}

	if (argi >= argc || argi + 2 < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	struct vnode* vnode = NULL;

	if (argi + 1 == argc) {
		vnode = (struct vnode*)parse_expression(argv[argi]);
		if (IS_USER_ADDRESS(vnode)) {
			kprintf("invalid vnode address\n");
			return 0;
		}
		_dump_vnode(vnode, printPath);
		return 0;
	}

	struct hash_iterator iterator;
	dev_t device = parse_expression(argv[argi]);
	ino_t id = parse_expression(argv[argi + 1]);

	hash_open(sVnodeTable, &iterator);
	while ((vnode = (struct vnode*)hash_next(sVnodeTable, &iterator)) != NULL) {
		if (vnode->id != id || vnode->device != device)
			continue;

		_dump_vnode(vnode, printPath);
	}

	hash_close(sVnodeTable, &iterator, false);
	return 0;
}


static int
dump_vnodes(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [device]\n", argv[0]);
		return 0;
	}

	// restrict dumped nodes to a certain device if requested
	dev_t device = parse_expression(argv[1]);

	struct hash_iterator iterator;
	struct vnode* vnode;

	kprintf("address    dev     inode  ref cache      fs-node    locking    "
		"flags\n");

	hash_open(sVnodeTable, &iterator);
	while ((vnode = (struct vnode*)hash_next(sVnodeTable, &iterator)) != NULL) {
		if (vnode->device != device)
			continue;

		kprintf("%p%4ld%10Ld%5ld %p %p %p %s%s%s\n", vnode, vnode->device,
			vnode->id, vnode->ref_count, vnode->cache, vnode->private_node,
			vnode->advisory_locking, vnode->IsRemoved() ? "r" : "-",
			vnode->IsBusy() ? "b" : "-", vnode->IsUnpublished() ? "u" : "-");
	}

	hash_close(sVnodeTable, &iterator, false);
	return 0;
}


static int
dump_vnode_caches(int argc, char** argv)
{
	struct hash_iterator iterator;
	struct vnode* vnode;

	if (argc > 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [device]\n", argv[0]);
		return 0;
	}

	// restrict dumped nodes to a certain device if requested
	dev_t device = -1;
	if (argc > 1)
		device = parse_expression(argv[1]);

	kprintf("address    dev     inode cache          size   pages\n");

	hash_open(sVnodeTable, &iterator);
	while ((vnode = (struct vnode*)hash_next(sVnodeTable, &iterator)) != NULL) {
		if (vnode->cache == NULL)
			continue;
		if (device != -1 && vnode->device != device)
			continue;

		kprintf("%p%4ld%10Ld %p %8Ld%8ld\n", vnode, vnode->device, vnode->id,
			vnode->cache, (vnode->cache->virtual_end + B_PAGE_SIZE - 1)
				/ B_PAGE_SIZE, vnode->cache->page_count);
	}

	hash_close(sVnodeTable, &iterator, false);
	return 0;
}


int
dump_io_context(int argc, char** argv)
{
	if (argc > 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s [team-id|address]\n", argv[0]);
		return 0;
	}

	struct io_context* context = NULL;

	if (argc > 1) {
		uint32 num = parse_expression(argv[1]);
		if (IS_KERNEL_ADDRESS(num))
			context = (struct io_context*)num;
		else {
			Team* team = team_get_team_struct_locked(num);
			if (team == NULL) {
				kprintf("could not find team with ID %ld\n", num);
				return 0;
			}
			context = (struct io_context*)team->io_context;
		}
	} else
		context = get_current_io_context(true);

	kprintf("I/O CONTEXT: %p\n", context);
	kprintf(" root vnode:\t%p\n", context->root);
	kprintf(" cwd vnode:\t%p\n", context->cwd);
	kprintf(" used fds:\t%lu\n", context->num_used_fds);
	kprintf(" max fds:\t%lu\n", context->table_size);

	if (context->num_used_fds)
		kprintf("   no.  type         ops  ref  open  mode         pos"
			"      cookie\n");

	for (uint32 i = 0; i < context->table_size; i++) {
		struct file_descriptor* fd = context->fds[i];
		if (fd == NULL)
			continue;

		kprintf("  %3" B_PRIu32 ":  %4" B_PRId32 "  %p  %3" B_PRId32 "  %4"
			B_PRIu32 "  %4" B_PRIx32 "  %10" B_PRIdOFF "  %p  %s %p\n", i,
			fd->type, fd->ops, fd->ref_count, fd->open_count, fd->open_mode,
			fd->pos, fd->cookie,
			fd->type >= FDTYPE_INDEX && fd->type <= FDTYPE_QUERY
				? "mount" : "vnode",
			fd->u.vnode);
	}

	kprintf(" used monitors:\t%lu\n", context->num_monitors);
	kprintf(" max monitors:\t%lu\n", context->max_monitors);

	set_debug_variable("_cwd", (addr_t)context->cwd);

	return 0;
}


int
dump_vnode_usage(int argc, char** argv)
{
	if (argc != 1) {
		kprintf("usage: %s\n", argv[0]);
		return 0;
	}

	kprintf("Unused vnodes: %ld (max unused %ld)\n", sUnusedVnodes,
		kMaxUnusedVnodes);

	struct hash_iterator iterator;
	hash_open(sVnodeTable, &iterator);

	uint32 count = 0;
	struct vnode* vnode;
	while ((vnode = (struct vnode*)hash_next(sVnodeTable, &iterator)) != NULL) {
		count++;
	}

	hash_close(sVnodeTable, &iterator, false);

	kprintf("%lu vnodes total (%ld in use).\n", count, count - sUnusedVnodes);
	return 0;
}

#endif	// ADD_DEBUGGER_COMMANDS

/*!	Clears an iovec array of physical pages.
	Returns in \a _bytes the number of bytes successfully cleared.
*/
static status_t
zero_pages(const iovec* vecs, size_t vecCount, size_t* _bytes)
{
	size_t bytes = *_bytes;
	size_t index = 0;

	while (bytes > 0) {
		size_t length = min_c(vecs[index].iov_len, bytes);

		status_t status = vm_memset_physical((addr_t)vecs[index].iov_base, 0,
			length);
		if (status != B_OK) {
			*_bytes -= bytes;
			return status;
		}

		bytes -= length;
	}

	return B_OK;
}


/*!	Does the dirty work of combining the file_io_vecs with the iovecs
	and calls the file system hooks to read/write the request to disk.
*/
static status_t
common_file_io_vec_pages(struct vnode* vnode, void* cookie,
	const file_io_vec* fileVecs, size_t fileVecCount, const iovec* vecs,
	size_t vecCount, uint32* _vecIndex, size_t* _vecOffset, size_t* _numBytes,
	bool doWrite)
{
	if (fileVecCount == 0) {
		// There are no file vecs at this offset, so we're obviously trying
		// to access the file outside of its bounds
		return B_BAD_VALUE;
	}

	size_t numBytes = *_numBytes;
	uint32 fileVecIndex;
	size_t vecOffset = *_vecOffset;
	uint32 vecIndex = *_vecIndex;
	status_t status;
	size_t size;

	if (!doWrite && vecOffset == 0) {
		// now directly read the data from the device
		// the first file_io_vec can be read directly

		if (fileVecs[0].length < numBytes)
			size = fileVecs[0].length;
		else
			size = numBytes;

		if (fileVecs[0].offset >= 0) {
			status = FS_CALL(vnode, read_pages, cookie, fileVecs[0].offset,
				&vecs[vecIndex], vecCount - vecIndex, &size);
		} else {
			// sparse read
			status = zero_pages(&vecs[vecIndex], vecCount - vecIndex, &size);
		}
		if (status != B_OK)
			return status;

		// TODO: this is a work-around for buggy device drivers!
		//	When our own drivers honour the length, we can:
		//	a) also use this direct I/O for writes (otherwise, it would
		//	   overwrite precious data)
		//	b) panic if the term below is true (at least for writes)
		if (size > fileVecs[0].length) {
			//dprintf("warning: device driver %p doesn't respect total length "
			//	"in read_pages() call!\n", ref->device);
			size = fileVecs[0].length;
		}

		ASSERT(size <= fileVecs[0].length);

		// If the file portion was contiguous, we're already done now
		if (size == numBytes)
			return B_OK;

		// if we reached the end of the file, we can return as well
		if (size != fileVecs[0].length) {
			*_numBytes = size;
			return B_OK;
		}

		fileVecIndex = 1;

		// first, find out where we have to continue in our iovecs
		for (; vecIndex < vecCount; vecIndex++) {
			if (size < vecs[vecIndex].iov_len)
				break;

			size -= vecs[vecIndex].iov_len;
		}

		vecOffset = size;
	} else {
		fileVecIndex = 0;
		size = 0;
	}

	// Too bad, let's process the rest of the file_io_vecs

	size_t totalSize = size;
	size_t bytesLeft = numBytes - size;

	for (; fileVecIndex < fileVecCount; fileVecIndex++) {
		const file_io_vec &fileVec = fileVecs[fileVecIndex];
		off_t fileOffset = fileVec.offset;
		off_t fileLeft = min_c(fileVec.length, bytesLeft);

		TRACE(("FILE VEC [%lu] length %Ld\n", fileVecIndex, fileLeft));

		// process the complete fileVec
		while (fileLeft > 0) {
			iovec tempVecs[MAX_TEMP_IO_VECS];
			uint32 tempCount = 0;

			// size tracks how much of what is left of the current fileVec
			// (fileLeft) has been assigned to tempVecs
			size = 0;

			// assign what is left of the current fileVec to the tempVecs
			for (size = 0; size < fileLeft && vecIndex < vecCount
					&& tempCount < MAX_TEMP_IO_VECS;) {
				// try to satisfy one iovec per iteration (or as much as
				// possible)

				// bytes left of the current iovec
				size_t vecLeft = vecs[vecIndex].iov_len - vecOffset;
				if (vecLeft == 0) {
					vecOffset = 0;
					vecIndex++;
					continue;
				}

				TRACE(("fill vec %ld, offset = %lu, size = %lu\n",
					vecIndex, vecOffset, size));

				// actually available bytes
				size_t tempVecSize = min_c(vecLeft, fileLeft - size);

				tempVecs[tempCount].iov_base
					= (void*)((addr_t)vecs[vecIndex].iov_base + vecOffset);
				tempVecs[tempCount].iov_len = tempVecSize;
				tempCount++;

				size += tempVecSize;
				vecOffset += tempVecSize;
			}

			size_t bytes = size;

			if (fileOffset == -1) {
				if (doWrite) {
					panic("sparse write attempt: vnode %p", vnode);
					status = B_IO_ERROR;
				} else {
					// sparse read
					status = zero_pages(tempVecs, tempCount, &bytes);
				}
			} else if (doWrite) {
				status = FS_CALL(vnode, write_pages, cookie, fileOffset,
					tempVecs, tempCount, &bytes);
			} else {
				status = FS_CALL(vnode, read_pages, cookie, fileOffset,
					tempVecs, tempCount, &bytes);
			}
			if (status != B_OK)
				return status;

			totalSize += bytes;
			bytesLeft -= size;
			if (fileOffset >= 0)
				fileOffset += size;
			fileLeft -= size;
			//dprintf("-> file left = %Lu\n", fileLeft);

			if (size != bytes || vecIndex >= vecCount) {
				// there are no more bytes or iovecs, let's bail out
				*_numBytes = totalSize;
				return B_OK;
			}
		}
	}

	*_vecIndex = vecIndex;
	*_vecOffset = vecOffset;
	*_numBytes = totalSize;
	return B_OK;
}


//	#pragma mark - public API for file systems


extern "C" status_t
new_vnode(fs_volume* volume, ino_t vnodeID, void* privateNode,
	fs_vnode_ops* ops)
{
	FUNCTION(("new_vnode(volume = %p (%ld), vnodeID = %Ld, node = %p)\n",
		volume, volume->id, vnodeID, privateNode));

	if (privateNode == NULL)
		return B_BAD_VALUE;

	// create the node
	bool nodeCreated;
	struct vnode* vnode;
	status_t status = create_new_vnode_and_lock(volume->id, vnodeID, vnode,
		nodeCreated);
	if (status != B_OK)
		return status;

	WriteLocker nodeLocker(sVnodeLock, true);
		// create_new_vnode_and_lock() has locked for us

	// file system integrity check:
	// test if the vnode already exists and bail out if this is the case!
	if (!nodeCreated) {
		panic("vnode %ld:%Ld already exists (node = %p, vnode->node = %p)!",
			volume->id, vnodeID, privateNode, vnode->private_node);
		return B_ERROR;
	}

	vnode->private_node = privateNode;
	vnode->ops = ops;
	vnode->SetUnpublished(true);

	TRACE(("returns: %s\n", strerror(status)));

	return status;
}


extern "C" status_t
publish_vnode(fs_volume* volume, ino_t vnodeID, void* privateNode,
	fs_vnode_ops* ops, int type, uint32 flags)
{
	FUNCTION(("publish_vnode()\n"));

	WriteLocker locker(sVnodeLock);

	struct vnode* vnode = lookup_vnode(volume->id, vnodeID);

	bool nodeCreated = false;
	if (vnode == NULL) {
		if (privateNode == NULL)
			return B_BAD_VALUE;

		// create the node
		locker.Unlock();
			// create_new_vnode_and_lock() will re-lock for us on success
		status_t status = create_new_vnode_and_lock(volume->id, vnodeID, vnode,
			nodeCreated);
		if (status != B_OK)
			return status;

		locker.SetTo(sVnodeLock, true);
	}

	if (nodeCreated) {
		vnode->private_node = privateNode;
		vnode->ops = ops;
		vnode->SetUnpublished(true);
	} else if (vnode->IsBusy() && vnode->IsUnpublished()
		&& vnode->private_node == privateNode && vnode->ops == ops) {
		// already known, but not published
	} else
		return B_BAD_VALUE;

	bool publishSpecialSubNode = false;

	vnode->SetType(type);
	vnode->SetRemoved((flags & B_VNODE_PUBLISH_REMOVED) != 0);
	publishSpecialSubNode = is_special_node_type(type)
		&& (flags & B_VNODE_DONT_CREATE_SPECIAL_SUB_NODE) == 0;

	status_t status = B_OK;

	// create sub vnodes, if necessary
	if (volume->sub_volume != NULL || publishSpecialSubNode) {
		locker.Unlock();

		fs_volume* subVolume = volume;
		if (volume->sub_volume != NULL) {
			while (status == B_OK && subVolume->sub_volume != NULL) {
				subVolume = subVolume->sub_volume;
				status = subVolume->ops->create_sub_vnode(subVolume, vnodeID,
					vnode);
			}
		}

		if (status == B_OK && publishSpecialSubNode)
			status = create_special_sub_node(vnode, flags);

		if (status != B_OK) {
			// error -- clean up the created sub vnodes
			while (subVolume->super_volume != volume) {
				subVolume = subVolume->super_volume;
				subVolume->ops->delete_sub_vnode(subVolume, vnode);
			}
		}

		if (status == B_OK) {
			ReadLocker vnodesReadLocker(sVnodeLock);
			AutoLocker<Vnode> nodeLocker(vnode);
			vnode->SetBusy(false);
			vnode->SetUnpublished(false);
		} else {
			locker.Lock();
			hash_remove(sVnodeTable, vnode);
			remove_vnode_from_mount_list(vnode, vnode->mount);
			free(vnode);
		}
	} else {
		// we still hold the write lock -- mark the node unbusy and published
		vnode->SetBusy(false);
		vnode->SetUnpublished(false);
	}

	TRACE(("returns: %s\n", strerror(status)));

	return status;
}


extern "C" status_t
get_vnode(fs_volume* volume, ino_t vnodeID, void** _privateNode)
{
	struct vnode* vnode;

	if (volume == NULL)
		return B_BAD_VALUE;

	status_t status = get_vnode(volume->id, vnodeID, &vnode, true, true);
	if (status != B_OK)
		return status;

	// If this is a layered FS, we need to get the node cookie for the requested
	// layer.
	if (HAS_FS_CALL(vnode, get_super_vnode)) {
		fs_vnode resolvedNode;
		status_t status = FS_CALL(vnode, get_super_vnode, volume,
			&resolvedNode);
		if (status != B_OK) {
			panic("get_vnode(): Failed to get super node for vnode %p, "
				"volume: %p", vnode, volume);
			put_vnode(vnode);
			return status;
		}

		if (_privateNode != NULL)
			*_privateNode = resolvedNode.private_node;
	} else if (_privateNode != NULL)
		*_privateNode = vnode->private_node;

	return B_OK;
}


extern "C" status_t
acquire_vnode(fs_volume* volume, ino_t vnodeID)
{
	struct vnode* vnode;

	rw_lock_read_lock(&sVnodeLock);
	vnode = lookup_vnode(volume->id, vnodeID);
	rw_lock_read_unlock(&sVnodeLock);

	if (vnode == NULL)
		return B_BAD_VALUE;

	inc_vnode_ref_count(vnode);
	return B_OK;
}


extern "C" status_t
put_vnode(fs_volume* volume, ino_t vnodeID)
{
	struct vnode* vnode;

	rw_lock_read_lock(&sVnodeLock);
	vnode = lookup_vnode(volume->id, vnodeID);
	rw_lock_read_unlock(&sVnodeLock);

	if (vnode == NULL)
		return B_BAD_VALUE;

	dec_vnode_ref_count(vnode, false, true);
	return B_OK;
}


extern "C" status_t
remove_vnode(fs_volume* volume, ino_t vnodeID)
{
	ReadLocker locker(sVnodeLock);

	struct vnode* vnode = lookup_vnode(volume->id, vnodeID);
	if (vnode == NULL)
		return B_ENTRY_NOT_FOUND;

	if (vnode->covered_by != NULL || vnode->covers != NULL) {
		// this vnode is in use
		return B_BUSY;
	}

	vnode->Lock();

	vnode->SetRemoved(true);
	bool removeUnpublished = false;

	if (vnode->IsUnpublished()) {
		// prepare the vnode for deletion
		removeUnpublished = true;
		vnode->SetBusy(true);
	}

	vnode->Unlock();
	locker.Unlock();

	if (removeUnpublished) {
		// If the vnode hasn't been published yet, we delete it here
		atomic_add(&vnode->ref_count, -1);
		free_vnode(vnode, true);
	}

	return B_OK;
}


extern "C" status_t
unremove_vnode(fs_volume* volume, ino_t vnodeID)
{
	struct vnode* vnode;

	rw_lock_read_lock(&sVnodeLock);

	vnode = lookup_vnode(volume->id, vnodeID);
	if (vnode) {
		AutoLocker<Vnode> nodeLocker(vnode);
		vnode->SetRemoved(false);
	}

	rw_lock_read_unlock(&sVnodeLock);
	return B_OK;
}


extern "C" status_t
get_vnode_removed(fs_volume* volume, ino_t vnodeID, bool* _removed)
{
	ReadLocker _(sVnodeLock);

	if (struct vnode* vnode = lookup_vnode(volume->id, vnodeID)) {
		if (_removed != NULL)
			*_removed = vnode->IsRemoved();
		return B_OK;
	}

	return B_BAD_VALUE;
}


extern "C" fs_volume*
volume_for_vnode(fs_vnode* _vnode)
{
	if (_vnode == NULL)
		return NULL;

	struct vnode* vnode = static_cast<struct vnode*>(_vnode);
	return vnode->mount->volume;
}


#if 0
extern "C" status_t
read_pages(int fd, off_t pos, const iovec* vecs, size_t count,
	size_t* _numBytes)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = vfs_read_pages(vnode, descriptor->cookie, pos, vecs,
		count, 0, _numBytes);

	put_fd(descriptor);
	return status;
}


extern "C" status_t
write_pages(int fd, off_t pos, const iovec* vecs, size_t count,
	size_t* _numBytes)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = vfs_write_pages(vnode, descriptor->cookie, pos, vecs,
		count, 0, _numBytes);

	put_fd(descriptor);
	return status;
}
#endif


extern "C" status_t
read_file_io_vec_pages(int fd, const file_io_vec* fileVecs, size_t fileVecCount,
	const iovec* vecs, size_t vecCount, uint32* _vecIndex, size_t* _vecOffset,
	size_t* _bytes)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = common_file_io_vec_pages(vnode, descriptor->cookie,
		fileVecs, fileVecCount, vecs, vecCount, _vecIndex, _vecOffset, _bytes,
		false);

	put_fd(descriptor);
	return status;
}


extern "C" status_t
write_file_io_vec_pages(int fd, const file_io_vec* fileVecs, size_t fileVecCount,
	const iovec* vecs, size_t vecCount, uint32* _vecIndex, size_t* _vecOffset,
	size_t* _bytes)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = common_file_io_vec_pages(vnode, descriptor->cookie,
		fileVecs, fileVecCount, vecs, vecCount, _vecIndex, _vecOffset, _bytes,
		true);

	put_fd(descriptor);
	return status;
}


extern "C" status_t
entry_cache_add(dev_t mountID, ino_t dirID, const char* name, ino_t nodeID)
{
	// lookup mount -- the caller is required to make sure that the mount
	// won't go away
	MutexLocker locker(sMountMutex);
	struct fs_mount* mount = find_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;
	locker.Unlock();

	return mount->entry_cache.Add(dirID, name, nodeID);
}


extern "C" status_t
entry_cache_remove(dev_t mountID, ino_t dirID, const char* name)
{
	// lookup mount -- the caller is required to make sure that the mount
	// won't go away
	MutexLocker locker(sMountMutex);
	struct fs_mount* mount = find_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;
	locker.Unlock();

	return mount->entry_cache.Remove(dirID, name);
}


//	#pragma mark - private VFS API
//	Functions the VFS exports for other parts of the kernel


/*! Acquires another reference to the vnode that has to be released
	by calling vfs_put_vnode().
*/
void
vfs_acquire_vnode(struct vnode* vnode)
{
	inc_vnode_ref_count(vnode);
}


/*! This is currently called from file_cache_create() only.
	It's probably a temporary solution as long as devfs requires that
	fs_read_pages()/fs_write_pages() are called with the standard
	open cookie and not with a device cookie.
	If that's done differently, remove this call; it has no other
	purpose.
*/
extern "C" status_t
vfs_get_cookie_from_fd(int fd, void** _cookie)
{
	struct file_descriptor* descriptor;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	*_cookie = descriptor->cookie;
	return B_OK;
}


extern "C" status_t
vfs_get_vnode_from_fd(int fd, bool kernel, struct vnode** vnode)
{
	*vnode = get_vnode_from_fd(fd, kernel);

	if (*vnode == NULL)
		return B_FILE_ERROR;

	return B_NO_ERROR;
}


extern "C" status_t
vfs_get_vnode_from_path(const char* path, bool kernel, struct vnode** _vnode)
{
	TRACE(("vfs_get_vnode_from_path: entry. path = '%s', kernel %d\n",
		path, kernel));

	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* buffer = pathBuffer.LockBuffer();
	strlcpy(buffer, path, pathBuffer.BufferSize());

	struct vnode* vnode;
	status_t status = path_to_vnode(buffer, true, &vnode, NULL, kernel);
	if (status != B_OK)
		return status;

	*_vnode = vnode;
	return B_OK;
}


extern "C" status_t
vfs_get_vnode(dev_t mountID, ino_t vnodeID, bool canWait, struct vnode** _vnode)
{
	struct vnode* vnode;

	status_t status = get_vnode(mountID, vnodeID, &vnode, canWait, false);
	if (status != B_OK)
		return status;

	*_vnode = vnode;
	return B_OK;
}


extern "C" status_t
vfs_entry_ref_to_vnode(dev_t mountID, ino_t directoryID,
	const char* name, struct vnode** _vnode)
{
	return entry_ref_to_vnode(mountID, directoryID, name, false, true, _vnode);
}


extern "C" void
vfs_vnode_to_node_ref(struct vnode* vnode, dev_t* _mountID, ino_t* _vnodeID)
{
	*_mountID = vnode->device;
	*_vnodeID = vnode->id;
}


/*!
	Helper function abstracting the process of "converting" a given
	vnode-pointer to a fs_vnode-pointer.
	Currently only used in bindfs.
*/
extern "C" fs_vnode*
vfs_fsnode_for_vnode(struct vnode* vnode)
{
	return vnode;
}


/*!
	Calls fs_open() on the given vnode and returns a new
	file descriptor for it
*/
int
vfs_open_vnode(struct vnode* vnode, int openMode, bool kernel)
{
	return open_vnode(vnode, openMode, kernel);
}


/*!	Looks up a vnode with the given mount and vnode ID.
	Must only be used with "in-use" vnodes as it doesn't grab a reference
	to the node.
	It's currently only be used by file_cache_create().
*/
extern "C" status_t
vfs_lookup_vnode(dev_t mountID, ino_t vnodeID, struct vnode** _vnode)
{
	rw_lock_read_lock(&sVnodeLock);
	struct vnode* vnode = lookup_vnode(mountID, vnodeID);
	rw_lock_read_unlock(&sVnodeLock);

	if (vnode == NULL)
		return B_ERROR;

	*_vnode = vnode;
	return B_OK;
}


extern "C" status_t
vfs_get_fs_node_from_path(fs_volume* volume, const char* path,
	bool traverseLeafLink, bool kernel, void** _node)
{
	TRACE(("vfs_get_fs_node_from_path(volume = %p, path = \"%s\", kernel %d)\n",
		volume, path, kernel));

	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	fs_mount* mount;
	status_t status = get_mount(volume->id, &mount);
	if (status != B_OK)
		return status;

	char* buffer = pathBuffer.LockBuffer();
	strlcpy(buffer, path, pathBuffer.BufferSize());

	struct vnode* vnode = mount->root_vnode;

	if (buffer[0] == '/')
		status = path_to_vnode(buffer, traverseLeafLink, &vnode, NULL, kernel);
	else {
		inc_vnode_ref_count(vnode);
			// vnode_path_to_vnode() releases a reference to the starting vnode
		status = vnode_path_to_vnode(vnode, buffer, traverseLeafLink, 0,
			kernel, &vnode, NULL);
	}

	put_mount(mount);

	if (status != B_OK)
		return status;

	if (vnode->device != volume->id) {
		// wrong mount ID - must not gain access on foreign file system nodes
		put_vnode(vnode);
		return B_BAD_VALUE;
	}

	// Use get_vnode() to resolve the cookie for the right layer.
	status = get_vnode(volume, vnode->id, _node);
	put_vnode(vnode);

	return status;
}


status_t
vfs_read_stat(int fd, const char* path, bool traverseLeafLink,
	struct stat* stat, bool kernel)
{
	status_t status;

	if (path) {
		// path given: get the stat of the node referred to by (fd, path)
		KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		status = common_path_read_stat(fd, pathBuffer.LockBuffer(),
			traverseLeafLink, stat, kernel);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor* descriptor
			= get_fd(get_current_io_context(kernel), fd);
		if (descriptor == NULL)
			return B_FILE_ERROR;

		if (descriptor->ops->fd_read_stat)
			status = descriptor->ops->fd_read_stat(descriptor, stat);
		else
			status = B_UNSUPPORTED;

		put_fd(descriptor);
	}

	return status;
}


/*!	Finds the full path to the file that contains the module \a moduleName,
	puts it into \a pathBuffer, and returns B_OK for success.
	If \a pathBuffer was too small, it returns \c B_BUFFER_OVERFLOW,
	\c B_ENTRY_NOT_FOUNT if no file could be found.
	\a pathBuffer is clobbered in any case and must not be relied on if this
	functions returns unsuccessfully.
	\a basePath and \a pathBuffer must not point to the same space.
*/
status_t
vfs_get_module_path(const char* basePath, const char* moduleName,
	char* pathBuffer, size_t bufferSize)
{
	struct vnode* dir;
	struct vnode* file;
	status_t status;
	size_t length;
	char* path;

	if (bufferSize == 0
		|| strlcpy(pathBuffer, basePath, bufferSize) >= bufferSize)
		return B_BUFFER_OVERFLOW;

	status = path_to_vnode(pathBuffer, true, &dir, NULL, true);
	if (status != B_OK)
		return status;

	// the path buffer had been clobbered by the above call
	length = strlcpy(pathBuffer, basePath, bufferSize);
	if (pathBuffer[length - 1] != '/')
		pathBuffer[length++] = '/';

	path = pathBuffer + length;
	bufferSize -= length;

	while (moduleName) {
		char* nextPath = strchr(moduleName, '/');
		if (nextPath == NULL)
			length = strlen(moduleName);
		else {
			length = nextPath - moduleName;
			nextPath++;
		}

		if (length + 1 >= bufferSize) {
			status = B_BUFFER_OVERFLOW;
			goto err;
		}

		memcpy(path, moduleName, length);
		path[length] = '\0';
		moduleName = nextPath;

		status = vnode_path_to_vnode(dir, path, true, 0, true, &file, NULL);
		if (status != B_OK) {
			// vnode_path_to_vnode() has already released the reference to dir
			return status;
		}

		if (S_ISDIR(file->Type())) {
			// goto the next directory
			path[length] = '/';
			path[length + 1] = '\0';
			path += length + 1;
			bufferSize -= length + 1;

			dir = file;
		} else if (S_ISREG(file->Type())) {
			// it's a file so it should be what we've searched for
			put_vnode(file);

			return B_OK;
		} else {
			TRACE(("vfs_get_module_path(): something is strange here: "
				"0x%08lx...\n", file->Type()));
			status = B_ERROR;
			dir = file;
			goto err;
		}
	}

	// if we got here, the moduleName just pointed to a directory, not to
	// a real module - what should we do in this case?
	status = B_ENTRY_NOT_FOUND;

err:
	put_vnode(dir);
	return status;
}


/*!	\brief Normalizes a given path.

	The path must refer to an existing or non-existing entry in an existing
	directory, that is chopping off the leaf component the remaining path must
	refer to an existing directory.

	The returned will be canonical in that it will be absolute, will not
	contain any "." or ".." components or duplicate occurrences of '/'s,
	and none of the directory components will by symbolic links.

	Any two paths referring to the same entry, will result in the same
	normalized path (well, that is pretty much the definition of `normalized',
	isn't it :-).

	\param path The path to be normalized.
	\param buffer The buffer into which the normalized path will be written.
		   May be the same one as \a path.
	\param bufferSize The size of \a buffer.
	\param traverseLink If \c true, the function also resolves leaf symlinks.
	\param kernel \c true, if the IO context of the kernel shall be used,
		   otherwise that of the team this thread belongs to. Only relevant,
		   if the path is relative (to get the CWD).
	\return \c B_OK if everything went fine, another error code otherwise.
*/
status_t
vfs_normalize_path(const char* path, char* buffer, size_t bufferSize,
	bool traverseLink, bool kernel)
{
	if (!path || !buffer || bufferSize < 1)
		return B_BAD_VALUE;

	if (path != buffer) {
		if (strlcpy(buffer, path, bufferSize) >= bufferSize)
			return B_BUFFER_OVERFLOW;
	}

	return normalize_path(buffer, bufferSize, traverseLink, kernel);
}


/*!	\brief Creates a special node in the file system.

	The caller gets a reference to the newly created node (which is passed
	back through \a _createdVnode) and is responsible for releasing it.

	\param path The path where to create the entry for the node. Can be \c NULL,
		in which case the node is created without an entry in the root FS -- it
		will automatically be deleted when the last reference has been released.
	\param subVnode The definition of the subnode. Can be \c NULL, in which case
		the target file system will just create the node with its standard
		operations. Depending on the type of the node a subnode might be created
		automatically, though.
	\param mode The type and permissions for the node to be created.
	\param flags Flags to be passed to the creating FS.
	\param kernel \c true, if called in the kernel context (relevant only if
		\a path is not \c NULL and not absolute).
	\param _superVnode Pointer to a pre-allocated structure to be filled by the
		file system creating the node, with the private data pointer and
		operations for the super node. Can be \c NULL.
	\param _createVnode Pointer to pre-allocated storage where to store the
		pointer to the newly created node.
	\return \c B_OK, if everything went fine, another error code otherwise.
*/
status_t
vfs_create_special_node(const char* path, fs_vnode* subVnode, mode_t mode,
	uint32 flags, bool kernel, fs_vnode* _superVnode,
	struct vnode** _createdVnode)
{
	struct vnode* dirNode;
	char _leaf[B_FILE_NAME_LENGTH];
	char* leaf = NULL;

	if (path) {
		// We've got a path. Get the dir vnode and the leaf name.
		KPath tmpPathBuffer(B_PATH_NAME_LENGTH + 1);
		if (tmpPathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		char* tmpPath = tmpPathBuffer.LockBuffer();
		if (strlcpy(tmpPath, path, B_PATH_NAME_LENGTH) >= B_PATH_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		// get the dir vnode and the leaf name
		leaf = _leaf;
		status_t error = path_to_dir_vnode(tmpPath, &dirNode, leaf, kernel);
		if (error != B_OK)
			return error;
	} else {
		// No path. Create the node in the root FS.
		dirNode = sRoot;
		inc_vnode_ref_count(dirNode);
	}

	VNodePutter _(dirNode);

	// check support for creating special nodes
	if (!HAS_FS_CALL(dirNode, create_special_node))
		return B_UNSUPPORTED;

	// create the node
	fs_vnode superVnode;
	ino_t nodeID;
	status_t status = FS_CALL(dirNode, create_special_node, leaf, subVnode,
		mode, flags, _superVnode != NULL ? _superVnode : &superVnode, &nodeID);
	if (status != B_OK)
		return status;

	// lookup the node
	rw_lock_read_lock(&sVnodeLock);
	*_createdVnode = lookup_vnode(dirNode->mount->id, nodeID);
	rw_lock_read_unlock(&sVnodeLock);

	if (*_createdVnode == NULL) {
		panic("vfs_create_special_node(): lookup of node failed");
		return B_ERROR;
	}

	return B_OK;
}


extern "C" void
vfs_put_vnode(struct vnode* vnode)
{
	put_vnode(vnode);
}


extern "C" status_t
vfs_get_cwd(dev_t* _mountID, ino_t* _vnodeID)
{
	// Get current working directory from io context
	struct io_context* context = get_current_io_context(false);
	status_t status = B_OK;

	mutex_lock(&context->io_mutex);

	if (context->cwd != NULL) {
		*_mountID = context->cwd->device;
		*_vnodeID = context->cwd->id;
	} else
		status = B_ERROR;

	mutex_unlock(&context->io_mutex);
	return status;
}


status_t
vfs_unmount(dev_t mountID, uint32 flags)
{
	return fs_unmount(NULL, mountID, flags, true);
}


extern "C" status_t
vfs_disconnect_vnode(dev_t mountID, ino_t vnodeID)
{
	struct vnode* vnode;

	status_t status = get_vnode(mountID, vnodeID, &vnode, true, true);
	if (status != B_OK)
		return status;

	disconnect_mount_or_vnode_fds(vnode->mount, vnode);
	put_vnode(vnode);
	return B_OK;
}


extern "C" void
vfs_free_unused_vnodes(int32 level)
{
	vnode_low_resource_handler(NULL,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE,
		level);
}


extern "C" bool
vfs_can_page(struct vnode* vnode, void* cookie)
{
	FUNCTION(("vfs_canpage: vnode 0x%p\n", vnode));

	if (HAS_FS_CALL(vnode, can_page))
		return FS_CALL(vnode, can_page, cookie);
	return false;
}


extern "C" status_t
vfs_read_pages(struct vnode* vnode, void* cookie, off_t pos,
	const generic_io_vec* vecs, size_t count, uint32 flags,
	generic_size_t* _numBytes)
{
	FUNCTION(("vfs_read_pages: vnode %p, vecs %p, pos %Ld\n", vnode, vecs,
		pos));

#if VFS_PAGES_IO_TRACING
	generic_size_t bytesRequested = *_numBytes;
#endif

	IORequest request;
	status_t status = request.Init(pos, vecs, count, *_numBytes, false, flags);
	if (status == B_OK) {
		status = vfs_vnode_io(vnode, cookie, &request);
		if (status == B_OK)
			status = request.Wait();
		*_numBytes = request.TransferredBytes();
	}

	TPIO(ReadPages(vnode, cookie, pos, vecs, count, flags, bytesRequested,
		status, *_numBytes));

	return status;
}


extern "C" status_t
vfs_write_pages(struct vnode* vnode, void* cookie, off_t pos,
	const generic_io_vec* vecs, size_t count, uint32 flags,
	generic_size_t* _numBytes)
{
	FUNCTION(("vfs_write_pages: vnode %p, vecs %p, pos %Ld\n", vnode, vecs,
		pos));

#if VFS_PAGES_IO_TRACING
	generic_size_t bytesRequested = *_numBytes;
#endif

	IORequest request;
	status_t status = request.Init(pos, vecs, count, *_numBytes, true, flags);
	if (status == B_OK) {
		status = vfs_vnode_io(vnode, cookie, &request);
		if (status == B_OK)
			status = request.Wait();
		*_numBytes = request.TransferredBytes();
	}

	TPIO(WritePages(vnode, cookie, pos, vecs, count, flags, bytesRequested,
		status, *_numBytes));

	return status;
}


/*!	Gets the vnode's VMCache object. If it didn't have one, it will be
	created if \a allocate is \c true.
	In case it's successful, it will also grab a reference to the cache
	it returns.
*/
extern "C" status_t
vfs_get_vnode_cache(struct vnode* vnode, VMCache** _cache, bool allocate)
{
	if (vnode->cache != NULL) {
		vnode->cache->AcquireRef();
		*_cache = vnode->cache;
		return B_OK;
	}

	rw_lock_read_lock(&sVnodeLock);
	vnode->Lock();

	status_t status = B_OK;

	// The cache could have been created in the meantime
	if (vnode->cache == NULL) {
		if (allocate) {
			// TODO: actually the vnode needs to be busy already here, or
			//	else this won't work...
			bool wasBusy = vnode->IsBusy();
			vnode->SetBusy(true);

			vnode->Unlock();
			rw_lock_read_unlock(&sVnodeLock);

			status = vm_create_vnode_cache(vnode, &vnode->cache);

			rw_lock_read_lock(&sVnodeLock);
			vnode->Lock();
			vnode->SetBusy(wasBusy);
		} else
			status = B_BAD_VALUE;
	}

	vnode->Unlock();
	rw_lock_read_unlock(&sVnodeLock);

	if (status == B_OK) {
		vnode->cache->AcquireRef();
		*_cache = vnode->cache;
	}

	return status;
}


status_t
vfs_get_file_map(struct vnode* vnode, off_t offset, size_t size,
	file_io_vec* vecs, size_t* _count)
{
	FUNCTION(("vfs_get_file_map: vnode %p, vecs %p, offset %Ld, size = %lu\n",
		vnode, vecs, offset, size));

	return FS_CALL(vnode, get_file_map, offset, size, vecs, _count);
}


status_t
vfs_stat_vnode(struct vnode* vnode, struct stat* stat)
{
	status_t status = FS_CALL(vnode, read_stat, stat);

	// fill in the st_dev and st_ino fields
	if (status == B_OK) {
		stat->st_dev = vnode->device;
		stat->st_ino = vnode->id;
		stat->st_rdev = -1;
	}

	return status;
}


status_t
vfs_stat_node_ref(dev_t device, ino_t inode, struct stat* stat)
{
	struct vnode* vnode;
	status_t status = get_vnode(device, inode, &vnode, true, false);
	if (status != B_OK)
		return status;

	status = FS_CALL(vnode, read_stat, stat);

	// fill in the st_dev and st_ino fields
	if (status == B_OK) {
		stat->st_dev = vnode->device;
		stat->st_ino = vnode->id;
		stat->st_rdev = -1;
	}

	put_vnode(vnode);
	return status;
}


status_t
vfs_get_vnode_name(struct vnode* vnode, char* name, size_t nameSize)
{
	return get_vnode_name(vnode, NULL, name, nameSize, true);
}


status_t
vfs_entry_ref_to_path(dev_t device, ino_t inode, const char* leaf,
	char* path, size_t pathLength)
{
	struct vnode* vnode;
	status_t status;

	// filter invalid leaf names
	if (leaf != NULL && (leaf[0] == '\0' || strchr(leaf, '/')))
		return B_BAD_VALUE;

	// get the vnode matching the dir's node_ref
	if (leaf && (strcmp(leaf, ".") == 0 || strcmp(leaf, "..") == 0)) {
		// special cases "." and "..": we can directly get the vnode of the
		// referenced directory
		status = entry_ref_to_vnode(device, inode, leaf, false, true, &vnode);
		leaf = NULL;
	} else
		status = get_vnode(device, inode, &vnode, true, false);
	if (status != B_OK)
		return status;

	// get the directory path
	status = dir_vnode_to_path(vnode, path, pathLength, true);
	put_vnode(vnode);
		// we don't need the vnode anymore
	if (status != B_OK)
		return status;

	// append the leaf name
	if (leaf) {
		// insert a directory separator if this is not the file system root
		if ((strcmp(path, "/") && strlcat(path, "/", pathLength)
				>= pathLength)
			|| strlcat(path, leaf, pathLength) >= pathLength) {
			return B_NAME_TOO_LONG;
		}
	}

	return B_OK;
}


/*!	If the given descriptor locked its vnode, that lock will be released. */
void
vfs_unlock_vnode_if_locked(struct file_descriptor* descriptor)
{
	struct vnode* vnode = fd_vnode(descriptor);

	if (vnode != NULL && vnode->mandatory_locked_by == descriptor)
		vnode->mandatory_locked_by = NULL;
}


/*!	Closes all file descriptors of the specified I/O context that
	have the O_CLOEXEC flag set.
*/
void
vfs_exec_io_context(io_context* context)
{
	uint32 i;

	for (i = 0; i < context->table_size; i++) {
		mutex_lock(&context->io_mutex);

		struct file_descriptor* descriptor = context->fds[i];
		bool remove = false;

		if (descriptor != NULL && fd_close_on_exec(context, i)) {
			context->fds[i] = NULL;
			context->num_used_fds--;

			remove = true;
		}

		mutex_unlock(&context->io_mutex);

		if (remove) {
			close_fd(descriptor);
			put_fd(descriptor);
		}
	}
}


/*! Sets up a new io_control structure, and inherits the properties
	of the parent io_control if it is given.
*/
io_context*
vfs_new_io_context(io_context* parentContext, bool purgeCloseOnExec)
{
	io_context* context = (io_context*)malloc(sizeof(io_context));
	if (context == NULL)
		return NULL;

	TIOC(NewIOContext(context, parentContext));

	memset(context, 0, sizeof(io_context));
	context->ref_count = 1;

	MutexLocker parentLocker;

	size_t tableSize;
	if (parentContext) {
		parentLocker.SetTo(parentContext->io_mutex, false);
		tableSize = parentContext->table_size;
	} else
		tableSize = DEFAULT_FD_TABLE_SIZE;

	// allocate space for FDs and their close-on-exec flag
	context->fds = (file_descriptor**)malloc(
		sizeof(struct file_descriptor*) * tableSize
		+ sizeof(struct select_sync*) * tableSize
		+ (tableSize + 7) / 8);
	if (context->fds == NULL) {
		free(context);
		return NULL;
	}

	context->select_infos = (select_info**)(context->fds + tableSize);
	context->fds_close_on_exec = (uint8*)(context->select_infos + tableSize);

	memset(context->fds, 0, sizeof(struct file_descriptor*) * tableSize
		+ sizeof(struct select_sync*) * tableSize
		+ (tableSize + 7) / 8);

	mutex_init(&context->io_mutex, "I/O context");

	// Copy all parent file descriptors

	if (parentContext) {
		size_t i;

		mutex_lock(&sIOContextRootLock);
		context->root = parentContext->root;
		if (context->root)
			inc_vnode_ref_count(context->root);
		mutex_unlock(&sIOContextRootLock);

		context->cwd = parentContext->cwd;
		if (context->cwd)
			inc_vnode_ref_count(context->cwd);

		for (i = 0; i < tableSize; i++) {
			struct file_descriptor* descriptor = parentContext->fds[i];

			if (descriptor != NULL) {
				bool closeOnExec = fd_close_on_exec(parentContext, i);
				if (closeOnExec && purgeCloseOnExec)
					continue;

				TFD(InheritFD(context, i, descriptor, parentContext));

				context->fds[i] = descriptor;
				context->num_used_fds++;
				atomic_add(&descriptor->ref_count, 1);
				atomic_add(&descriptor->open_count, 1);

				if (closeOnExec)
					fd_set_close_on_exec(context, i, true);
			}
		}

		parentLocker.Unlock();
	} else {
		context->root = sRoot;
		context->cwd = sRoot;

		if (context->root)
			inc_vnode_ref_count(context->root);

		if (context->cwd)
			inc_vnode_ref_count(context->cwd);
	}

	context->table_size = tableSize;

	list_init(&context->node_monitors);
	context->max_monitors = DEFAULT_NODE_MONITORS;

	return context;
}


static status_t
vfs_free_io_context(io_context* context)
{
	uint32 i;

	TIOC(FreeIOContext(context));

	if (context->root)
		put_vnode(context->root);

	if (context->cwd)
		put_vnode(context->cwd);

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (struct file_descriptor* descriptor = context->fds[i]) {
			close_fd(descriptor);
			put_fd(descriptor);
		}
	}

	mutex_destroy(&context->io_mutex);

	remove_node_monitors(context);
	free(context->fds);
	free(context);

	return B_OK;
}


void
vfs_get_io_context(io_context* context)
{
	atomic_add(&context->ref_count, 1);
}


void
vfs_put_io_context(io_context* context)
{
	if (atomic_add(&context->ref_count, -1) == 1)
		vfs_free_io_context(context);
}


static status_t
vfs_resize_fd_table(struct io_context* context, const int newSize)
{
	if (newSize <= 0 || newSize > MAX_FD_TABLE_SIZE)
		return B_BAD_VALUE;

	TIOC(ResizeIOContext(context, newSize));

	MutexLocker _(context->io_mutex);

	int oldSize = context->table_size;
	int oldCloseOnExitBitmapSize = (oldSize + 7) / 8;
	int newCloseOnExitBitmapSize = (newSize + 7) / 8;

	// If the tables shrink, make sure none of the fds being dropped are in use.
	if (newSize < oldSize) {
		for (int i = oldSize; i-- > newSize;) {
			if (context->fds[i])
				return B_BUSY;
		}
	}

	// store pointers to the old tables
	file_descriptor** oldFDs = context->fds;
	select_info** oldSelectInfos = context->select_infos;
	uint8* oldCloseOnExecTable = context->fds_close_on_exec;

	// allocate new tables
	file_descriptor** newFDs = (file_descriptor**)malloc(
		sizeof(struct file_descriptor*) * newSize
		+ sizeof(struct select_sync*) * newSize
		+ newCloseOnExitBitmapSize);
	if (newFDs == NULL)
		return B_NO_MEMORY;

	context->fds = newFDs;
	context->select_infos = (select_info**)(context->fds + newSize);
	context->fds_close_on_exec = (uint8*)(context->select_infos + newSize);
	context->table_size = newSize;

	// copy entries from old tables
	int toCopy = min_c(oldSize, newSize);

	memcpy(context->fds, oldFDs, sizeof(void*) * toCopy);
	memcpy(context->select_infos, oldSelectInfos, sizeof(void*) * toCopy);
	memcpy(context->fds_close_on_exec, oldCloseOnExecTable,
		min_c(oldCloseOnExitBitmapSize, newCloseOnExitBitmapSize));

	// clear additional entries, if the tables grow
	if (newSize > oldSize) {
		memset(context->fds + oldSize, 0, sizeof(void*) * (newSize - oldSize));
		memset(context->select_infos + oldSize, 0,
			sizeof(void*) * (newSize - oldSize));
		memset(context->fds_close_on_exec + oldCloseOnExitBitmapSize, 0,
			newCloseOnExitBitmapSize - oldCloseOnExitBitmapSize);
	}

	free(oldFDs);

	return B_OK;
}


static status_t
vfs_resize_monitor_table(struct io_context* context, const int newSize)
{
	int	status = B_OK;

	if (newSize <= 0 || newSize > MAX_NODE_MONITORS)
		return B_BAD_VALUE;

	mutex_lock(&context->io_mutex);

	if ((size_t)newSize < context->num_monitors) {
		status = B_BUSY;
		goto out;
	}
	context->max_monitors = newSize;

out:
	mutex_unlock(&context->io_mutex);
	return status;
}


status_t
vfs_get_mount_point(dev_t mountID, dev_t* _mountPointMountID,
	ino_t* _mountPointNodeID)
{
	ReadLocker nodeLocker(sVnodeLock);
	MutexLocker mountLocker(sMountMutex);

	struct fs_mount* mount = find_mount(mountID);
	if (mount == NULL)
		return B_BAD_VALUE;

	Vnode* mountPoint = mount->covers_vnode;

	*_mountPointMountID = mountPoint->device;
	*_mountPointNodeID = mountPoint->id;

	return B_OK;
}


status_t
vfs_bind_mount_directory(dev_t mountID, ino_t nodeID, dev_t coveredMountID,
	ino_t coveredNodeID)
{
	// get the vnodes
	Vnode* vnode;
	status_t error = get_vnode(mountID, nodeID, &vnode, true, false);
	if (error != B_OK)
		return B_BAD_VALUE;
	VNodePutter vnodePutter(vnode);

	Vnode* coveredVnode;
	error = get_vnode(coveredMountID, coveredNodeID, &coveredVnode, true,
		false);
	if (error != B_OK)
		return B_BAD_VALUE;
	VNodePutter coveredVnodePutter(coveredVnode);

	// establish the covered/covering links
	WriteLocker locker(sVnodeLock);

	if (vnode->covers != NULL || coveredVnode->covered_by != NULL)
		return B_BUSY;

	vnode->covers = coveredVnode;
	vnode->SetCovering(true);

	coveredVnode->covered_by = vnode;
	coveredVnode->SetCovered(true);

	// the vnodes do now reference each other
	inc_vnode_ref_count(vnode);
	inc_vnode_ref_count(coveredVnode);

// TODO: This relationship must be tracked, so it can be dissolved on unmount.

	return B_OK;
}


int
vfs_getrlimit(int resource, struct rlimit* rlp)
{
	if (!rlp)
		return B_BAD_ADDRESS;

	switch (resource) {
		case RLIMIT_NOFILE:
		{
			struct io_context* context = get_current_io_context(false);
			MutexLocker _(context->io_mutex);

			rlp->rlim_cur = context->table_size;
			rlp->rlim_max = MAX_FD_TABLE_SIZE;
			return 0;
		}

		case RLIMIT_NOVMON:
		{
			struct io_context* context = get_current_io_context(false);
			MutexLocker _(context->io_mutex);

			rlp->rlim_cur = context->max_monitors;
			rlp->rlim_max = MAX_NODE_MONITORS;
			return 0;
		}

		default:
			return B_BAD_VALUE;
	}
}


int
vfs_setrlimit(int resource, const struct rlimit* rlp)
{
	if (!rlp)
		return B_BAD_ADDRESS;

	switch (resource) {
		case RLIMIT_NOFILE:
			/* TODO: check getuid() */
			if (rlp->rlim_max != RLIM_SAVED_MAX
				&& rlp->rlim_max != MAX_FD_TABLE_SIZE)
				return B_NOT_ALLOWED;

			return vfs_resize_fd_table(get_current_io_context(false),
				rlp->rlim_cur);

		case RLIMIT_NOVMON:
			/* TODO: check getuid() */
			if (rlp->rlim_max != RLIM_SAVED_MAX
				&& rlp->rlim_max != MAX_NODE_MONITORS)
				return B_NOT_ALLOWED;

			return vfs_resize_monitor_table(get_current_io_context(false),
				rlp->rlim_cur);

		default:
			return B_BAD_VALUE;
	}
}


status_t
vfs_init(kernel_args* args)
{
	vnode::StaticInit();

	struct vnode dummyVnode;
	sVnodeTable = hash_init(VNODE_HASH_TABLE_SIZE,
		offset_of_member(dummyVnode, next), &vnode_compare, &vnode_hash);
	if (sVnodeTable == NULL)
		panic("vfs_init: error creating vnode hash table\n");

	list_init_etc(&sUnusedVnodeList, offset_of_member(dummyVnode, unused_link));

	struct fs_mount dummyMount;
	sMountsTable = hash_init(MOUNTS_HASH_TABLE_SIZE,
		offset_of_member(dummyMount, next), &mount_compare, &mount_hash);
	if (sMountsTable == NULL)
		panic("vfs_init: error creating mounts hash table\n");

	node_monitor_init();

	sRoot = NULL;

	recursive_lock_init(&sMountOpLock, "vfs_mount_op_lock");

	if (block_cache_init() != B_OK)
		return B_ERROR;

#ifdef ADD_DEBUGGER_COMMANDS
	// add some debugger commands
	add_debugger_command_etc("vnode", &dump_vnode,
		"Print info about the specified vnode",
		"[ \"-p\" ] ( <vnode> | <devID> <nodeID> )\n"
		"Prints information about the vnode specified by address <vnode> or\n"
		"<devID>, <vnodeID> pair. If \"-p\" is given, a path of the vnode is\n"
		"constructed and printed. It might not be possible to construct a\n"
		"complete path, though.\n",
		0);
	add_debugger_command("vnodes", &dump_vnodes,
		"list all vnodes (from the specified device)");
	add_debugger_command("vnode_caches", &dump_vnode_caches,
		"list all vnode caches");
	add_debugger_command("mount", &dump_mount,
		"info about the specified fs_mount");
	add_debugger_command("mounts", &dump_mounts, "list all fs_mounts");
	add_debugger_command("io_context", &dump_io_context,
		"info about the I/O context");
	add_debugger_command("vnode_usage", &dump_vnode_usage,
		"info about vnode usage");
#endif

	register_low_resource_handler(&vnode_low_resource_handler, NULL,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE,
		0);

	file_map_init();

	return file_cache_init();
}


//	#pragma mark - fd_ops implementations


/*!
	Calls fs_open() on the given vnode and returns a new
	file descriptor for it
*/
static int
open_vnode(struct vnode* vnode, int openMode, bool kernel)
{
	void* cookie;
	status_t status = FS_CALL(vnode, open, openMode, &cookie);
	if (status != B_OK)
		return status;

	int fd = get_new_fd(FDTYPE_FILE, NULL, vnode, cookie, openMode, kernel);
	if (fd < 0) {
		FS_CALL(vnode, close, cookie);
		FS_CALL(vnode, free_cookie, cookie);
	}
	return fd;
}


/*!
	Calls fs_open() on the given vnode and returns a new
	file descriptor for it
*/
static int
create_vnode(struct vnode* directory, const char* name, int openMode,
	int perms, bool kernel)
{
	bool traverse = ((openMode & (O_NOTRAVERSE | O_NOFOLLOW)) == 0);
	status_t status = B_ERROR;
	struct vnode* vnode;
	void* cookie;
	ino_t newID;

	// This is somewhat tricky: If the entry already exists, the FS responsible
	// for the directory might not necessarily also be the one responsible for
	// the node the entry refers to (e.g. in case of mount points or FIFOs). So
	// we can actually never call the create() hook without O_EXCL. Instead we
	// try to look the entry up first. If it already exists, we just open the
	// node (unless O_EXCL), otherwise we call create() with O_EXCL. This
	// introduces a race condition, since someone else might have created the
	// entry in the meantime. We hope the respective FS returns the correct
	// error code and retry (up to 3 times) again.

	for (int i = 0; i < 3 && status != B_OK; i++) {
		// look the node up
		status = lookup_dir_entry(directory, name, &vnode);
		if (status == B_OK) {
			VNodePutter putter(vnode);

			if ((openMode & O_EXCL) != 0)
				return B_FILE_EXISTS;

			// If the node is a symlink, we have to follow it, unless
			// O_NOTRAVERSE is set.
			if (S_ISLNK(vnode->Type()) && traverse) {
				putter.Put();
				char clonedName[B_FILE_NAME_LENGTH + 1];
				if (strlcpy(clonedName, name, B_FILE_NAME_LENGTH)
						>= B_FILE_NAME_LENGTH) {
					return B_NAME_TOO_LONG;
				}

				inc_vnode_ref_count(directory);
				status = vnode_path_to_vnode(directory, clonedName, true, 0,
					kernel, &vnode, NULL);
				if (status != B_OK)
					return status;

				putter.SetTo(vnode);
			}

			if ((openMode & O_NOFOLLOW) != 0 && S_ISLNK(vnode->Type())) {
				put_vnode(vnode);
				return B_LINK_LIMIT;
			}

			int fd = open_vnode(vnode, openMode & ~O_CREAT, kernel);
			// on success keep the vnode reference for the FD
			if (fd >= 0)
				putter.Detach();

			return fd;
		}

		// it doesn't exist yet -- try to create it

		if (!HAS_FS_CALL(directory, create))
			return B_READ_ONLY_DEVICE;

		status = FS_CALL(directory, create, name, openMode | O_EXCL, perms,
			&cookie, &newID);
		if (status != B_OK
			&& ((openMode & O_EXCL) != 0 || status != B_FILE_EXISTS)) {
			return status;
		}
	}

	if (status != B_OK)
		return status;

	// the node has been created successfully

	rw_lock_read_lock(&sVnodeLock);
	vnode = lookup_vnode(directory->device, newID);
	rw_lock_read_unlock(&sVnodeLock);

	if (vnode == NULL) {
		panic("vfs: fs_create() returned success but there is no vnode, "
			"mount ID %ld!\n", directory->device);
		return B_BAD_VALUE;
	}

	int fd = get_new_fd(FDTYPE_FILE, NULL, vnode, cookie, openMode, kernel);
	if (fd >= 0)
		return fd;

	status = fd;

	// something went wrong, clean up

	FS_CALL(vnode, close, cookie);
	FS_CALL(vnode, free_cookie, cookie);
	put_vnode(vnode);

	FS_CALL(directory, unlink, name);

	return status;
}


/*! Calls fs open_dir() on the given vnode and returns a new
	file descriptor for it
*/
static int
open_dir_vnode(struct vnode* vnode, bool kernel)
{
	void* cookie;
	status_t status = FS_CALL(vnode, open_dir, &cookie);
	if (status != B_OK)
		return status;

	// directory is opened, create a fd
	status = get_new_fd(FDTYPE_DIR, NULL, vnode, cookie, O_CLOEXEC, kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_dir, cookie);
	FS_CALL(vnode, free_dir_cookie, cookie);

	return status;
}


/*! Calls fs open_attr_dir() on the given vnode and returns a new
	file descriptor for it.
	Used by attr_dir_open(), and attr_dir_open_fd().
*/
static int
open_attr_dir_vnode(struct vnode* vnode, bool kernel)
{
	if (!HAS_FS_CALL(vnode, open_attr_dir))
		return B_UNSUPPORTED;

	void* cookie;
	status_t status = FS_CALL(vnode, open_attr_dir, &cookie);
	if (status != B_OK)
		return status;

	// directory is opened, create a fd
	status = get_new_fd(FDTYPE_ATTR_DIR, NULL, vnode, cookie, O_CLOEXEC,
		kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_attr_dir, cookie);
	FS_CALL(vnode, free_attr_dir_cookie, cookie);

	return status;
}


static int
file_create_entry_ref(dev_t mountID, ino_t directoryID, const char* name,
	int openMode, int perms, bool kernel)
{
	FUNCTION(("file_create_entry_ref: name = '%s', omode %x, perms %d, "
		"kernel %d\n", name, openMode, perms, kernel));

	// get directory to put the new file in
	struct vnode* directory;
	status_t status = get_vnode(mountID, directoryID, &directory, true, false);
	if (status != B_OK)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);
	put_vnode(directory);

	return status;
}


static int
file_create(int fd, char* path, int openMode, int perms, bool kernel)
{
	FUNCTION(("file_create: path '%s', omode %x, perms %d, kernel %d\n", path,
		openMode, perms, kernel));

	// get directory to put the new file in
	char name[B_FILE_NAME_LENGTH];
	struct vnode* directory;
	status_t status = fd_and_path_to_dir_vnode(fd, path, &directory, name,
		kernel);
	if (status < 0)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);

	put_vnode(directory);
	return status;
}


static int
file_open_entry_ref(dev_t mountID, ino_t directoryID, const char* name,
	int openMode, bool kernel)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("file_open_entry_ref(ref = (%ld, %Ld, %s), openMode = %d)\n",
		mountID, directoryID, name, openMode));

	bool traverse = (openMode & (O_NOTRAVERSE | O_NOFOLLOW)) == 0;

	// get the vnode matching the entry_ref
	struct vnode* vnode;
	status_t status = entry_ref_to_vnode(mountID, directoryID, name, traverse,
		kernel, &vnode);
	if (status != B_OK)
		return status;

	if ((openMode & O_NOFOLLOW) != 0 && S_ISLNK(vnode->Type())) {
		put_vnode(vnode);
		return B_LINK_LIMIT;
	}

	int newFD = open_vnode(vnode, openMode, kernel);
	if (newFD >= 0) {
		// The vnode reference has been transferred to the FD
		cache_node_opened(vnode, FDTYPE_FILE, vnode->cache, mountID,
			directoryID, vnode->id, name);
	} else
		put_vnode(vnode);

	return newFD;
}


static int
file_open(int fd, char* path, int openMode, bool kernel)
{
	bool traverse = (openMode & (O_NOTRAVERSE | O_NOFOLLOW)) == 0;

	FUNCTION(("file_open: fd: %d, entry path = '%s', omode %d, kernel %d\n",
		fd, path, openMode, kernel));

	// get the vnode matching the vnode + path combination
	struct vnode* vnode;
	ino_t parentID;
	status_t status = fd_and_path_to_vnode(fd, path, traverse, &vnode,
		&parentID, kernel);
	if (status != B_OK)
		return status;

	if ((openMode & O_NOFOLLOW) != 0 && S_ISLNK(vnode->Type())) {
		put_vnode(vnode);
		return B_LINK_LIMIT;
	}

	// open the vnode
	int newFD = open_vnode(vnode, openMode, kernel);
	if (newFD >= 0) {
		// The vnode reference has been transferred to the FD
		cache_node_opened(vnode, FDTYPE_FILE, vnode->cache,
			vnode->device, parentID, vnode->id, NULL);
	} else
		put_vnode(vnode);

	return newFD;
}


static status_t
file_close(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;
	status_t status = B_OK;

	FUNCTION(("file_close(descriptor = %p)\n", descriptor));

	cache_node_closed(vnode, FDTYPE_FILE, vnode->cache, vnode->device,
		vnode->id);
	if (HAS_FS_CALL(vnode, close)) {
		status = FS_CALL(vnode, close, descriptor->cookie);
	}

	if (status == B_OK) {
		// remove all outstanding locks for this team
		release_advisory_lock(vnode, NULL);
	}
	return status;
}


static void
file_free_fd(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_cookie, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t
file_read(struct file_descriptor* descriptor, off_t pos, void* buffer,
	size_t* length)
{
	struct vnode* vnode = descriptor->u.vnode;
	FUNCTION(("file_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length,
		*length));

	if (S_ISDIR(vnode->Type()))
		return B_IS_A_DIRECTORY;

	return FS_CALL(vnode, read, descriptor->cookie, pos, buffer, length);
}


static status_t
file_write(struct file_descriptor* descriptor, off_t pos, const void* buffer,
	size_t* length)
{
	struct vnode* vnode = descriptor->u.vnode;
	FUNCTION(("file_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));

	if (S_ISDIR(vnode->Type()))
		return B_IS_A_DIRECTORY;
	if (!HAS_FS_CALL(vnode, write))
		return B_READ_ONLY_DEVICE;

	return FS_CALL(vnode, write, descriptor->cookie, pos, buffer, length);
}


static off_t
file_seek(struct file_descriptor* descriptor, off_t pos, int seekType)
{
	struct vnode* vnode = descriptor->u.vnode;
	off_t offset;

	FUNCTION(("file_seek(pos = %Ld, seekType = %d)\n", pos, seekType));

	// some kinds of files are not seekable
	switch (vnode->Type() & S_IFMT) {
		case S_IFIFO:
		case S_IFSOCK:
			return ESPIPE;

		// The Open Group Base Specs don't mention any file types besides pipes,
		// fifos, and sockets specially, so we allow seeking them.
		case S_IFREG:
		case S_IFBLK:
		case S_IFDIR:
		case S_IFLNK:
		case S_IFCHR:
			break;
	}

	switch (seekType) {
		case SEEK_SET:
			offset = 0;
			break;
		case SEEK_CUR:
			offset = descriptor->pos;
			break;
		case SEEK_END:
		{
			// stat() the node
			if (!HAS_FS_CALL(vnode, read_stat))
				return B_UNSUPPORTED;

			struct stat stat;
			status_t status = FS_CALL(vnode, read_stat, &stat);
			if (status != B_OK)
				return status;

			offset = stat.st_size;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	// assumes off_t is 64 bits wide
	if (offset > 0 && LONGLONG_MAX - offset < pos)
		return B_BUFFER_OVERFLOW;

	pos += offset;
	if (pos < 0)
		return B_BAD_VALUE;

	return descriptor->pos = pos;
}


static status_t
file_select(struct file_descriptor* descriptor, uint8 event,
	struct selectsync* sync)
{
	FUNCTION(("file_select(%p, %u, %p)\n", descriptor, event, sync));

	struct vnode* vnode = descriptor->u.vnode;

	// If the FS has no select() hook, notify select() now.
	if (!HAS_FS_CALL(vnode, select))
		return notify_select_event(sync, event);

	return FS_CALL(vnode, select, descriptor->cookie, event, sync);
}


static status_t
file_deselect(struct file_descriptor* descriptor, uint8 event,
	struct selectsync* sync)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (!HAS_FS_CALL(vnode, deselect))
		return B_OK;

	return FS_CALL(vnode, deselect, descriptor->cookie, event, sync);
}


static status_t
dir_create_entry_ref(dev_t mountID, ino_t parentID, const char* name, int perms,
	bool kernel)
{
	struct vnode* vnode;
	status_t status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("dir_create_entry_ref(dev = %ld, ino = %Ld, name = '%s', "
		"perms = %d)\n", mountID, parentID, name, perms));

	status = get_vnode(mountID, parentID, &vnode, true, false);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(vnode, create_dir))
		status = FS_CALL(vnode, create_dir, name, perms);
	else
		status = B_READ_ONLY_DEVICE;

	put_vnode(vnode);
	return status;
}


static status_t
dir_create(int fd, char* path, int perms, bool kernel)
{
	char filename[B_FILE_NAME_LENGTH];
	struct vnode* vnode;
	status_t status;

	FUNCTION(("dir_create: path '%s', perms %d, kernel %d\n", path, perms,
		kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (HAS_FS_CALL(vnode, create_dir)) {
		status = FS_CALL(vnode, create_dir, filename, perms);
	} else
		status = B_READ_ONLY_DEVICE;

	put_vnode(vnode);
	return status;
}


static int
dir_open_entry_ref(dev_t mountID, ino_t parentID, const char* name, bool kernel)
{
	FUNCTION(("dir_open_entry_ref()\n"));

	if (name && name[0] == '\0')
		return B_BAD_VALUE;

	// get the vnode matching the entry_ref/node_ref
	struct vnode* vnode;
	status_t status;
	if (name) {
		status = entry_ref_to_vnode(mountID, parentID, name, true, kernel,
			&vnode);
	} else
		status = get_vnode(mountID, parentID, &vnode, true, false);
	if (status != B_OK)
		return status;

	int newFD = open_dir_vnode(vnode, kernel);
	if (newFD >= 0) {
		// The vnode reference has been transferred to the FD
		cache_node_opened(vnode, FDTYPE_DIR, vnode->cache, mountID, parentID,
			vnode->id, name);
	} else
		put_vnode(vnode);

	return newFD;
}


static int
dir_open(int fd, char* path, bool kernel)
{
	FUNCTION(("dir_open: fd: %d, entry path = '%s', kernel %d\n", fd, path,
		kernel));

	// get the vnode matching the vnode + path combination
	struct vnode* vnode = NULL;
	ino_t parentID;
	status_t status = fd_and_path_to_vnode(fd, path, true, &vnode, &parentID,
		kernel);
	if (status != B_OK)
		return status;

	// open the dir
	int newFD = open_dir_vnode(vnode, kernel);
	if (newFD >= 0) {
		// The vnode reference has been transferred to the FD
		cache_node_opened(vnode, FDTYPE_DIR, vnode->cache, vnode->device,
			parentID, vnode->id, NULL);
	} else
		put_vnode(vnode);

	return newFD;
}


static status_t
dir_close(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("dir_close(descriptor = %p)\n", descriptor));

	cache_node_closed(vnode, FDTYPE_DIR, vnode->cache, vnode->device,
		vnode->id);
	if (HAS_FS_CALL(vnode, close_dir))
		return FS_CALL(vnode, close_dir, descriptor->cookie);

	return B_OK;
}


static void
dir_free_fd(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_dir_cookie, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t
dir_read(struct io_context* ioContext, struct file_descriptor* descriptor,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	return dir_read(ioContext, descriptor->u.vnode, descriptor->cookie, buffer,
		bufferSize, _count);
}


static status_t
fix_dirent(struct vnode* parent, struct dirent* entry,
	struct io_context* ioContext)
{
	// set d_pdev and d_pino
	entry->d_pdev = parent->device;
	entry->d_pino = parent->id;

	// If this is the ".." entry and the directory covering another vnode,
	// we need to replace d_dev and d_ino with the actual values.
	if (strcmp(entry->d_name, "..") == 0 && parent->IsCovering()) {
		// Make sure the IO context root is not bypassed.
		if (parent == ioContext->root) {
			entry->d_dev = parent->device;
			entry->d_ino = parent->id;
		} else {
			inc_vnode_ref_count(parent);
				// vnode_path_to_vnode() puts the node

			// ".." is guaranteed not to be clobbered by this call
			struct vnode* vnode;
			status_t status = vnode_path_to_vnode(parent, (char*)"..", false, 0,
				ioContext, &vnode, NULL);

			if (status == B_OK) {
				entry->d_dev = vnode->device;
				entry->d_ino = vnode->id;
				put_vnode(vnode);
			}
		}
	} else {
		// resolve covered vnodes
		ReadLocker _(&sVnodeLock);

		struct vnode* vnode = lookup_vnode(entry->d_dev, entry->d_ino);
		if (vnode != NULL && vnode->covered_by != NULL) {
			do {
				vnode = vnode->covered_by;
			} while (vnode->covered_by != NULL);

			entry->d_dev = vnode->device;
			entry->d_ino = vnode->id;
		}
	}

	return B_OK;
}


static status_t
dir_read(struct io_context* ioContext, struct vnode* vnode, void* cookie,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	if (!HAS_FS_CALL(vnode, read_dir))
		return B_UNSUPPORTED;

	status_t error = FS_CALL(vnode, read_dir, cookie, buffer, bufferSize,
		_count);
	if (error != B_OK)
		return error;

	// we need to adjust the read dirents
	uint32 count = *_count;
	for (uint32 i = 0; i < count; i++) {
		error = fix_dirent(vnode, buffer, ioContext);
		if (error != B_OK)
			return error;

		buffer = (struct dirent*)((uint8*)buffer + buffer->d_reclen);
	}

	return error;
}


static status_t
dir_rewind(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (HAS_FS_CALL(vnode, rewind_dir)) {
		return FS_CALL(vnode, rewind_dir, descriptor->cookie);
	}

	return B_UNSUPPORTED;
}


static status_t
dir_remove(int fd, char* path, bool kernel)
{
	char name[B_FILE_NAME_LENGTH];
	struct vnode* directory;
	status_t status;

	if (path != NULL) {
		// we need to make sure our path name doesn't stop with "/", ".",
		// or ".."
		char* lastSlash;
		while ((lastSlash = strrchr(path, '/')) != NULL) {
			char* leaf = lastSlash + 1;
			if (!strcmp(leaf, ".."))
				return B_NOT_ALLOWED;

			// omit multiple slashes
			while (lastSlash > path && lastSlash[-1] == '/')
				lastSlash--;

			if (leaf[0]
				&& strcmp(leaf, ".")) {
				break;
			}
			// "name/" -> "name", or "name/." -> "name"
			lastSlash[0] = '\0';
		}

		if (!strcmp(path, ".") || !strcmp(path, ".."))
			return B_NOT_ALLOWED;
	}

	status = fd_and_path_to_dir_vnode(fd, path, &directory, name, kernel);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(directory, remove_dir))
		status = FS_CALL(directory, remove_dir, name);
	else
		status = B_READ_ONLY_DEVICE;

	put_vnode(directory);
	return status;
}


static status_t
common_ioctl(struct file_descriptor* descriptor, uint32 op, void* buffer,
	size_t length)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (HAS_FS_CALL(vnode, ioctl))
		return FS_CALL(vnode, ioctl, descriptor->cookie, op, buffer, length);

	return B_DEV_INVALID_IOCTL;
}


static status_t
common_fcntl(int fd, int op, uint32 argument, bool kernel)
{
	struct flock flock;

	FUNCTION(("common_fcntl(fd = %d, op = %d, argument = %lx, %s)\n",
		fd, op, argument, kernel ? "kernel" : "user"));

	struct file_descriptor* descriptor = get_fd(get_current_io_context(kernel),
		fd);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	struct vnode* vnode = fd_vnode(descriptor);

	status_t status = B_OK;

	if (op == F_SETLK || op == F_SETLKW || op == F_GETLK) {
		if (descriptor->type != FDTYPE_FILE)
			status = B_BAD_VALUE;
		else if (user_memcpy(&flock, (struct flock*)argument,
				sizeof(struct flock)) != B_OK)
			status = B_BAD_ADDRESS;

		if (status != B_OK) {
			put_fd(descriptor);
			return status;
		}
	}

	switch (op) {
		case F_SETFD:
		{
			struct io_context* context = get_current_io_context(kernel);
			// Set file descriptor flags

			// O_CLOEXEC is the only flag available at this time
			mutex_lock(&context->io_mutex);
			fd_set_close_on_exec(context, fd, (argument & FD_CLOEXEC) != 0);
			mutex_unlock(&context->io_mutex);

			status = B_OK;
			break;
		}

		case F_GETFD:
		{
			struct io_context* context = get_current_io_context(kernel);

			// Get file descriptor flags
			mutex_lock(&context->io_mutex);
			status = fd_close_on_exec(context, fd) ? FD_CLOEXEC : 0;
			mutex_unlock(&context->io_mutex);
			break;
		}

		case F_SETFL:
			// Set file descriptor open mode

			// we only accept changes to O_APPEND and O_NONBLOCK
			argument &= O_APPEND | O_NONBLOCK;
			if (descriptor->ops->fd_set_flags != NULL) {
				status = descriptor->ops->fd_set_flags(descriptor, argument);
			} else if (vnode != NULL && HAS_FS_CALL(vnode, set_flags)) {
				status = FS_CALL(vnode, set_flags, descriptor->cookie,
					(int)argument);
			} else
				status = B_UNSUPPORTED;

			if (status == B_OK) {
				// update this descriptor's open_mode field
				descriptor->open_mode = (descriptor->open_mode
					& ~(O_APPEND | O_NONBLOCK)) | argument;
			}

			break;

		case F_GETFL:
			// Get file descriptor open mode
			status = descriptor->open_mode;
			break;

		case F_DUPFD:
		{
			struct io_context* context = get_current_io_context(kernel);

			status = new_fd_etc(context, descriptor, (int)argument);
			if (status >= 0) {
				mutex_lock(&context->io_mutex);
				fd_set_close_on_exec(context, fd, false);
				mutex_unlock(&context->io_mutex);

				atomic_add(&descriptor->ref_count, 1);
			}
			break;
		}

		case F_GETLK:
			if (vnode != NULL) {
				status = get_advisory_lock(vnode, &flock);
				if (status == B_OK) {
					// copy back flock structure
					status = user_memcpy((struct flock*)argument, &flock,
						sizeof(struct flock));
				}
			} else
				status = B_BAD_VALUE;
			break;

		case F_SETLK:
		case F_SETLKW:
			status = normalize_flock(descriptor, &flock);
			if (status != B_OK)
				break;

			if (vnode == NULL) {
				status = B_BAD_VALUE;
			} else if (flock.l_type == F_UNLCK) {
				status = release_advisory_lock(vnode, &flock);
			} else {
				// the open mode must match the lock type
				if (((descriptor->open_mode & O_RWMASK) == O_RDONLY
						&& flock.l_type == F_WRLCK)
					|| ((descriptor->open_mode & O_RWMASK) == O_WRONLY
						&& flock.l_type == F_RDLCK))
					status = B_FILE_ERROR;
				else {
					status = acquire_advisory_lock(vnode, -1,
						&flock, op == F_SETLKW);
				}
			}
			break;

		// ToDo: add support for more ops?

		default:
			status = B_BAD_VALUE;
	}

	put_fd(descriptor);
	return status;
}


static status_t
common_sync(int fd, bool kernel)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;
	status_t status;

	FUNCTION(("common_fsync: entry. fd %d kernel %d\n", fd, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (HAS_FS_CALL(vnode, fsync))
		status = FS_CALL_NO_PARAMS(vnode, fsync);
	else
		status = B_UNSUPPORTED;

	put_fd(descriptor);
	return status;
}


static status_t
common_lock_node(int fd, bool kernel)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = B_OK;

	// We need to set the locking atomically - someone
	// else might set one at the same time
	if (atomic_pointer_test_and_set(&vnode->mandatory_locked_by, descriptor,
			(file_descriptor*)NULL) != NULL)
		status = B_BUSY;

	put_fd(descriptor);
	return status;
}


static status_t
common_unlock_node(int fd, bool kernel)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	status_t status = B_OK;

	// We need to set the locking atomically - someone
	// else might set one at the same time
	if (atomic_pointer_test_and_set(&vnode->mandatory_locked_by,
			(file_descriptor*)NULL, descriptor) != descriptor)
		status = B_BAD_VALUE;

	put_fd(descriptor);
	return status;
}


static status_t
common_read_link(int fd, char* path, char* buffer, size_t* _bufferSize,
	bool kernel)
{
	struct vnode* vnode;
	status_t status;

	status = fd_and_path_to_vnode(fd, path, false, &vnode, NULL, kernel);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(vnode, read_symlink)) {
		status = FS_CALL(vnode, read_symlink, buffer, _bufferSize);
	} else
		status = B_BAD_VALUE;

	put_vnode(vnode);
	return status;
}


static status_t
common_create_symlink(int fd, char* path, const char* toPath, int mode,
	bool kernel)
{
	// path validity checks have to be in the calling function!
	char name[B_FILE_NAME_LENGTH];
	struct vnode* vnode;
	status_t status;

	FUNCTION(("common_create_symlink(fd = %d, path = %s, toPath = %s, "
		"mode = %d, kernel = %d)\n", fd, path, toPath, mode, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, name, kernel);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(vnode, create_symlink))
		status = FS_CALL(vnode, create_symlink, name, toPath, mode);
	else {
		status = HAS_FS_CALL(vnode, write)
			? B_UNSUPPORTED : B_READ_ONLY_DEVICE;
	}

	put_vnode(vnode);

	return status;
}


static status_t
common_create_link(int pathFD, char* path, int toFD, char* toPath,
	bool traverseLeafLink, bool kernel)
{
	// path validity checks have to be in the calling function!

	FUNCTION(("common_create_link(path = %s, toPath = %s, kernel = %d)\n", path,
		toPath, kernel));

	char name[B_FILE_NAME_LENGTH];
	struct vnode* directory;
	status_t status = fd_and_path_to_dir_vnode(pathFD, path, &directory, name,
		kernel);
	if (status != B_OK)
		return status;

	struct vnode* vnode;
	status = fd_and_path_to_vnode(toFD, toPath, traverseLeafLink, &vnode, NULL,
		kernel);
	if (status != B_OK)
		goto err;

	if (directory->mount != vnode->mount) {
		status = B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (HAS_FS_CALL(directory, link))
		status = FS_CALL(directory, link, name, vnode);
	else
		status = B_READ_ONLY_DEVICE;

err1:
	put_vnode(vnode);
err:
	put_vnode(directory);

	return status;
}


static status_t
common_unlink(int fd, char* path, bool kernel)
{
	char filename[B_FILE_NAME_LENGTH];
	struct vnode* vnode;
	status_t status;

	FUNCTION(("common_unlink: fd: %d, path '%s', kernel %d\n", fd, path,
		kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (HAS_FS_CALL(vnode, unlink))
		status = FS_CALL(vnode, unlink, filename);
	else
		status = B_READ_ONLY_DEVICE;

	put_vnode(vnode);

	return status;
}


static status_t
common_access(int fd, char* path, int mode, bool effectiveUserGroup, bool kernel)
{
	struct vnode* vnode;
	status_t status;

	// TODO: honor effectiveUserGroup argument

	status = fd_and_path_to_vnode(fd, path, true, &vnode, NULL, kernel);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(vnode, access))
		status = FS_CALL(vnode, access, mode);
	else
		status = B_OK;

	put_vnode(vnode);

	return status;
}


static status_t
common_rename(int fd, char* path, int newFD, char* newPath, bool kernel)
{
	struct vnode* fromVnode;
	struct vnode* toVnode;
	char fromName[B_FILE_NAME_LENGTH];
	char toName[B_FILE_NAME_LENGTH];
	status_t status;

	FUNCTION(("common_rename(fd = %d, path = %s, newFD = %d, newPath = %s, "
		"kernel = %d)\n", fd, path, newFD, newPath, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &fromVnode, fromName, kernel);
	if (status != B_OK)
		return status;

	status = fd_and_path_to_dir_vnode(newFD, newPath, &toVnode, toName, kernel);
	if (status != B_OK)
		goto err1;

	if (fromVnode->device != toVnode->device) {
		status = B_CROSS_DEVICE_LINK;
		goto err2;
	}

	if (fromName[0] == '\0' || toName[0] == '\0'
		|| !strcmp(fromName, ".") || !strcmp(fromName, "..")
		|| !strcmp(toName, ".") || !strcmp(toName, "..")
		|| (fromVnode == toVnode && !strcmp(fromName, toName))) {
		status = B_BAD_VALUE;
		goto err2;
	}

	if (HAS_FS_CALL(fromVnode, rename))
		status = FS_CALL(fromVnode, rename, fromName, toVnode, toName);
	else
		status = B_READ_ONLY_DEVICE;

err2:
	put_vnode(toVnode);
err1:
	put_vnode(fromVnode);

	return status;
}


static status_t
common_read_stat(struct file_descriptor* descriptor, struct stat* stat)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("common_read_stat: stat %p\n", stat));

	// TODO: remove this once all file systems properly set them!
	stat->st_crtim.tv_nsec = 0;
	stat->st_ctim.tv_nsec = 0;
	stat->st_mtim.tv_nsec = 0;
	stat->st_atim.tv_nsec = 0;

	status_t status = FS_CALL(vnode, read_stat, stat);

	// fill in the st_dev and st_ino fields
	if (status == B_OK) {
		stat->st_dev = vnode->device;
		stat->st_ino = vnode->id;
		stat->st_rdev = -1;
	}

	return status;
}


static status_t
common_write_stat(struct file_descriptor* descriptor, const struct stat* stat,
	int statMask)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("common_write_stat(vnode = %p, stat = %p, statMask = %d)\n",
		vnode, stat, statMask));

	if (!HAS_FS_CALL(vnode, write_stat))
		return B_READ_ONLY_DEVICE;

	return FS_CALL(vnode, write_stat, stat, statMask);
}


static status_t
common_path_read_stat(int fd, char* path, bool traverseLeafLink,
	struct stat* stat, bool kernel)
{
	FUNCTION(("common_path_read_stat: fd: %d, path '%s', stat %p,\n", fd, path,
		stat));

	struct vnode* vnode;
	status_t status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode,
		NULL, kernel);
	if (status != B_OK)
		return status;

	status = FS_CALL(vnode, read_stat, stat);

	// fill in the st_dev and st_ino fields
	if (status == B_OK) {
		stat->st_dev = vnode->device;
		stat->st_ino = vnode->id;
		stat->st_rdev = -1;
	}

	put_vnode(vnode);
	return status;
}


static status_t
common_path_write_stat(int fd, char* path, bool traverseLeafLink,
	const struct stat* stat, int statMask, bool kernel)
{
	FUNCTION(("common_write_stat: fd: %d, path '%s', stat %p, stat_mask %d, "
		"kernel %d\n", fd, path, stat, statMask, kernel));

	struct vnode* vnode;
	status_t status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode,
		NULL, kernel);
	if (status != B_OK)
		return status;

	if (HAS_FS_CALL(vnode, write_stat))
		status = FS_CALL(vnode, write_stat, stat, statMask);
	else
		status = B_READ_ONLY_DEVICE;

	put_vnode(vnode);

	return status;
}


static int
attr_dir_open(int fd, char* path, bool traverseLeafLink, bool kernel)
{
	FUNCTION(("attr_dir_open(fd = %d, path = '%s', kernel = %d)\n", fd, path,
		kernel));

	struct vnode* vnode;
	status_t status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode,
		NULL, kernel);
	if (status != B_OK)
		return status;

	status = open_attr_dir_vnode(vnode, kernel);
	if (status < 0)
		put_vnode(vnode);

	return status;
}


static status_t
attr_dir_close(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_close(descriptor = %p)\n", descriptor));

	if (HAS_FS_CALL(vnode, close_attr_dir))
		return FS_CALL(vnode, close_attr_dir, descriptor->cookie);

	return B_OK;
}


static void
attr_dir_free_fd(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_dir_cookie, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t
attr_dir_read(struct io_context* ioContext, struct file_descriptor* descriptor,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_read(descriptor = %p)\n", descriptor));

	if (HAS_FS_CALL(vnode, read_attr_dir))
		return FS_CALL(vnode, read_attr_dir, descriptor->cookie, buffer,
			bufferSize, _count);

	return B_UNSUPPORTED;
}


static status_t
attr_dir_rewind(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_rewind(descriptor = %p)\n", descriptor));

	if (HAS_FS_CALL(vnode, rewind_attr_dir))
		return FS_CALL(vnode, rewind_attr_dir, descriptor->cookie);

	return B_UNSUPPORTED;
}


static int
attr_create(int fd, char* path, const char* name, uint32 type,
	int openMode, bool kernel)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	bool traverse = (openMode & (O_NOTRAVERSE | O_NOFOLLOW)) == 0;
	struct vnode* vnode;
	status_t status = fd_and_path_to_vnode(fd, path, traverse, &vnode, NULL,
		kernel);
	if (status != B_OK)
		return status;

	if ((openMode & O_NOFOLLOW) != 0 && S_ISLNK(vnode->Type())) {
		status = B_LINK_LIMIT;
		goto err;
	}

	if (!HAS_FS_CALL(vnode, create_attr)) {
		status = B_READ_ONLY_DEVICE;
		goto err;
	}

	void* cookie;
	status = FS_CALL(vnode, create_attr, name, type, openMode, &cookie);
	if (status != B_OK)
		goto err;

	fd = get_new_fd(FDTYPE_ATTR, NULL, vnode, cookie, openMode, kernel);
	if (fd >= 0)
		return fd;

	status = fd;

	FS_CALL(vnode, close_attr, cookie);
	FS_CALL(vnode, free_attr_cookie, cookie);

	FS_CALL(vnode, remove_attr, name);

err:
	put_vnode(vnode);

	return status;
}


static int
attr_open(int fd, char* path, const char* name, int openMode, bool kernel)
{
	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	bool traverse = (openMode & (O_NOTRAVERSE | O_NOFOLLOW)) == 0;
	struct vnode* vnode;
	status_t status = fd_and_path_to_vnode(fd, path, traverse, &vnode, NULL,
		kernel);
	if (status != B_OK)
		return status;

	if ((openMode & O_NOFOLLOW) != 0 && S_ISLNK(vnode->Type())) {
		status = B_LINK_LIMIT;
		goto err;
	}

	if (!HAS_FS_CALL(vnode, open_attr)) {
		status = B_UNSUPPORTED;
		goto err;
	}

	void* cookie;
	status = FS_CALL(vnode, open_attr, name, openMode, &cookie);
	if (status != B_OK)
		goto err;

	// now we only need a file descriptor for this attribute and we're done
	fd = get_new_fd(FDTYPE_ATTR, NULL, vnode, cookie, openMode, kernel);
	if (fd >= 0)
		return fd;

	status = fd;

	FS_CALL(vnode, close_attr, cookie);
	FS_CALL(vnode, free_attr_cookie, cookie);

err:
	put_vnode(vnode);

	return status;
}


static status_t
attr_close(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_close(descriptor = %p)\n", descriptor));

	if (HAS_FS_CALL(vnode, close_attr))
		return FS_CALL(vnode, close_attr, descriptor->cookie);

	return B_OK;
}


static void
attr_free_fd(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_cookie, descriptor->cookie);
		put_vnode(vnode);
	}
}


static status_t
attr_read(struct file_descriptor* descriptor, off_t pos, void* buffer,
	size_t* length)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length,
		*length));

	if (!HAS_FS_CALL(vnode, read_attr))
		return B_UNSUPPORTED;

	return FS_CALL(vnode, read_attr, descriptor->cookie, pos, buffer, length);
}


static status_t
attr_write(struct file_descriptor* descriptor, off_t pos, const void* buffer,
	size_t* length)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	if (!HAS_FS_CALL(vnode, write_attr))
		return B_UNSUPPORTED;

	return FS_CALL(vnode, write_attr, descriptor->cookie, pos, buffer, length);
}


static off_t
attr_seek(struct file_descriptor* descriptor, off_t pos, int seekType)
{
	off_t offset;

	switch (seekType) {
		case SEEK_SET:
			offset = 0;
			break;
		case SEEK_CUR:
			offset = descriptor->pos;
			break;
		case SEEK_END:
		{
			struct vnode* vnode = descriptor->u.vnode;
			if (!HAS_FS_CALL(vnode, read_stat))
				return B_UNSUPPORTED;

			struct stat stat;
			status_t status = FS_CALL(vnode, read_attr_stat, descriptor->cookie,
				&stat);
			if (status != B_OK)
				return status;

			offset = stat.st_size;
			break;
		}
		default:
			return B_BAD_VALUE;
	}

	// assumes off_t is 64 bits wide
	if (offset > 0 && LONGLONG_MAX - offset < pos)
		return B_BUFFER_OVERFLOW;

	pos += offset;
	if (pos < 0)
		return B_BAD_VALUE;

	return descriptor->pos = pos;
}


static status_t
attr_read_stat(struct file_descriptor* descriptor, struct stat* stat)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_read_stat: stat 0x%p\n", stat));

	if (!HAS_FS_CALL(vnode, read_attr_stat))
		return B_UNSUPPORTED;

	return FS_CALL(vnode, read_attr_stat, descriptor->cookie, stat);
}


static status_t
attr_write_stat(struct file_descriptor* descriptor, const struct stat* stat,
	int statMask)
{
	struct vnode* vnode = descriptor->u.vnode;

	FUNCTION(("attr_write_stat: stat = %p, statMask %d\n", stat, statMask));

	if (!HAS_FS_CALL(vnode, write_attr_stat))
		return B_READ_ONLY_DEVICE;

	return FS_CALL(vnode, write_attr_stat, descriptor->cookie, stat, statMask);
}


static status_t
attr_remove(int fd, const char* name, bool kernel)
{
	struct file_descriptor* descriptor;
	struct vnode* vnode;
	status_t status;

	if (name == NULL || *name == '\0')
		return B_BAD_VALUE;

	FUNCTION(("attr_remove: fd = %d, name = \"%s\", kernel %d\n", fd, name,
		kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (HAS_FS_CALL(vnode, remove_attr))
		status = FS_CALL(vnode, remove_attr, name);
	else
		status = B_READ_ONLY_DEVICE;

	put_fd(descriptor);

	return status;
}


static status_t
attr_rename(int fromFD, const char* fromName, int toFD, const char* toName,
	bool kernel)
{
	struct file_descriptor* fromDescriptor;
	struct file_descriptor* toDescriptor;
	struct vnode* fromVnode;
	struct vnode* toVnode;
	status_t status;

	if (fromName == NULL || *fromName == '\0' || toName == NULL
		|| *toName == '\0')
		return B_BAD_VALUE;

	FUNCTION(("attr_rename: from fd = %d, from name = \"%s\", to fd = %d, to "
		"name = \"%s\", kernel %d\n", fromFD, fromName, toFD, toName, kernel));

	fromDescriptor = get_fd_and_vnode(fromFD, &fromVnode, kernel);
	if (fromDescriptor == NULL)
		return B_FILE_ERROR;

	toDescriptor = get_fd_and_vnode(toFD, &toVnode, kernel);
	if (toDescriptor == NULL) {
		status = B_FILE_ERROR;
		goto err;
	}

	// are the files on the same volume?
	if (fromVnode->device != toVnode->device) {
		status = B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (HAS_FS_CALL(fromVnode, rename_attr)) {
		status = FS_CALL(fromVnode, rename_attr, fromName, toVnode, toName);
	} else
		status = B_READ_ONLY_DEVICE;

err1:
	put_fd(toDescriptor);
err:
	put_fd(fromDescriptor);

	return status;
}


static int
index_dir_open(dev_t mountID, bool kernel)
{
	struct fs_mount* mount;
	void* cookie;

	FUNCTION(("index_dir_open(mountID = %ld, kernel = %d)\n", mountID, kernel));

	status_t status = get_mount(mountID, &mount);
	if (status != B_OK)
		return status;

	if (!HAS_FS_MOUNT_CALL(mount, open_index_dir)) {
		status = B_UNSUPPORTED;
		goto error;
	}

	status = FS_MOUNT_CALL(mount, open_index_dir, &cookie);
	if (status != B_OK)
		goto error;

	// get fd for the index directory
	int fd;
	fd = get_new_fd(FDTYPE_INDEX_DIR, mount, NULL, cookie, O_CLOEXEC, kernel);
	if (fd >= 0)
		return fd;

	// something went wrong
	FS_MOUNT_CALL(mount, close_index_dir, cookie);
	FS_MOUNT_CALL(mount, free_index_dir_cookie, cookie);

	status = fd;

error:
	put_mount(mount);
	return status;
}


static status_t
index_dir_close(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	FUNCTION(("index_dir_close(descriptor = %p)\n", descriptor));

	if (HAS_FS_MOUNT_CALL(mount, close_index_dir))
		return FS_MOUNT_CALL(mount, close_index_dir, descriptor->cookie);

	return B_OK;
}


static void
index_dir_free_fd(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (mount != NULL) {
		FS_MOUNT_CALL(mount, free_index_dir_cookie, descriptor->cookie);
		put_mount(mount);
	}
}


static status_t
index_dir_read(struct io_context* ioContext, struct file_descriptor* descriptor,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (HAS_FS_MOUNT_CALL(mount, read_index_dir)) {
		return FS_MOUNT_CALL(mount, read_index_dir, descriptor->cookie, buffer,
			bufferSize, _count);
	}

	return B_UNSUPPORTED;
}


static status_t
index_dir_rewind(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (HAS_FS_MOUNT_CALL(mount, rewind_index_dir))
		return FS_MOUNT_CALL(mount, rewind_index_dir, descriptor->cookie);

	return B_UNSUPPORTED;
}


static status_t
index_create(dev_t mountID, const char* name, uint32 type, uint32 flags,
	bool kernel)
{
	FUNCTION(("index_create(mountID = %ld, name = %s, kernel = %d)\n", mountID,
		name, kernel));

	struct fs_mount* mount;
	status_t status = get_mount(mountID, &mount);
	if (status != B_OK)
		return status;

	if (!HAS_FS_MOUNT_CALL(mount, create_index)) {
		status = B_READ_ONLY_DEVICE;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, create_index, name, type, flags);

out:
	put_mount(mount);
	return status;
}


#if 0
static status_t
index_read_stat(struct file_descriptor* descriptor, struct stat* stat)
{
	struct vnode* vnode = descriptor->u.vnode;

	// ToDo: currently unused!
	FUNCTION(("index_read_stat: stat 0x%p\n", stat));
	if (!HAS_FS_CALL(vnode, read_index_stat))
		return B_UNSUPPORTED;

	return B_UNSUPPORTED;
	//return FS_CALL(vnode, read_index_stat, descriptor->cookie, stat);
}


static void
index_free_fd(struct file_descriptor* descriptor)
{
	struct vnode* vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_index_cookie, descriptor->cookie);
		put_vnode(vnode);
	}
}
#endif


static status_t
index_name_read_stat(dev_t mountID, const char* name, struct stat* stat,
	bool kernel)
{
	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID,
		name, kernel));

	struct fs_mount* mount;
	status_t status = get_mount(mountID, &mount);
	if (status != B_OK)
		return status;

	if (!HAS_FS_MOUNT_CALL(mount, read_index_stat)) {
		status = B_UNSUPPORTED;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, read_index_stat, name, stat);

out:
	put_mount(mount);
	return status;
}


static status_t
index_remove(dev_t mountID, const char* name, bool kernel)
{
	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID,
		name, kernel));

	struct fs_mount* mount;
	status_t status = get_mount(mountID, &mount);
	if (status != B_OK)
		return status;

	if (!HAS_FS_MOUNT_CALL(mount, remove_index)) {
		status = B_READ_ONLY_DEVICE;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, remove_index, name);

out:
	put_mount(mount);
	return status;
}


/*!	TODO: the query FS API is still the pretty much the same as in R5.
		It would be nice if the FS would find some more kernel support
		for them.
		For example, query parsing should be moved into the kernel.
*/
static int
query_open(dev_t device, const char* query, uint32 flags, port_id port,
	int32 token, bool kernel)
{
	struct fs_mount* mount;
	void* cookie;

	FUNCTION(("query_open(device = %ld, query = \"%s\", kernel = %d)\n", device,
		query, kernel));

	status_t status = get_mount(device, &mount);
	if (status != B_OK)
		return status;

	if (!HAS_FS_MOUNT_CALL(mount, open_query)) {
		status = B_UNSUPPORTED;
		goto error;
	}

	status = FS_MOUNT_CALL(mount, open_query, query, flags, port, token,
		&cookie);
	if (status != B_OK)
		goto error;

	// get fd for the index directory
	int fd;
	fd = get_new_fd(FDTYPE_QUERY, mount, NULL, cookie, O_CLOEXEC, kernel);
	if (fd >= 0)
		return fd;

	status = fd;

	// something went wrong
	FS_MOUNT_CALL(mount, close_query, cookie);
	FS_MOUNT_CALL(mount, free_query_cookie, cookie);

error:
	put_mount(mount);
	return status;
}


static status_t
query_close(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	FUNCTION(("query_close(descriptor = %p)\n", descriptor));

	if (HAS_FS_MOUNT_CALL(mount, close_query))
		return FS_MOUNT_CALL(mount, close_query, descriptor->cookie);

	return B_OK;
}


static void
query_free_fd(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (mount != NULL) {
		FS_MOUNT_CALL(mount, free_query_cookie, descriptor->cookie);
		put_mount(mount);
	}
}


static status_t
query_read(struct io_context* ioContext, struct file_descriptor* descriptor,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (HAS_FS_MOUNT_CALL(mount, read_query)) {
		return FS_MOUNT_CALL(mount, read_query, descriptor->cookie, buffer,
			bufferSize, _count);
	}

	return B_UNSUPPORTED;
}


static status_t
query_rewind(struct file_descriptor* descriptor)
{
	struct fs_mount* mount = descriptor->u.mount;

	if (HAS_FS_MOUNT_CALL(mount, rewind_query))
		return FS_MOUNT_CALL(mount, rewind_query, descriptor->cookie);

	return B_UNSUPPORTED;
}


//	#pragma mark - General File System functions


static dev_t
fs_mount(char* path, const char* device, const char* fsName, uint32 flags,
	const char* args, bool kernel)
{
	struct ::fs_mount* mount;
	status_t status = B_OK;
	fs_volume* volume = NULL;
	int32 layer = 0;
	Vnode* coveredNode = NULL;

	FUNCTION(("fs_mount: entry. path = '%s', fs_name = '%s'\n", path, fsName));

	// The path is always safe, we just have to make sure that fsName is
	// almost valid - we can't make any assumptions about args, though.
	// A NULL fsName is OK, if a device was given and the FS is not virtual.
	// We'll get it from the DDM later.
	if (fsName == NULL) {
		if (!device || flags & B_MOUNT_VIRTUAL_DEVICE)
			return B_BAD_VALUE;
	} else if (fsName[0] == '\0')
		return B_BAD_VALUE;

	RecursiveLocker mountOpLocker(sMountOpLock);

	// Helper to delete a newly created file device on failure.
	// Not exactly beautiful, but helps to keep the code below cleaner.
	struct FileDeviceDeleter {
		FileDeviceDeleter() : id(-1) {}
		~FileDeviceDeleter()
		{
			KDiskDeviceManager::Default()->DeleteFileDevice(id);
		}

		partition_id id;
	} fileDeviceDeleter;

	// If the file system is not a "virtual" one, the device argument should
	// point to a real file/device (if given at all).
	// get the partition
	KDiskDeviceManager* ddm = KDiskDeviceManager::Default();
	KPartition* partition = NULL;
	KPath normalizedDevice;
	bool newlyCreatedFileDevice = false;

	if (!(flags & B_MOUNT_VIRTUAL_DEVICE) && device != NULL) {
		// normalize the device path
		status = normalizedDevice.SetTo(device, true);
		if (status != B_OK)
			return status;

		// get a corresponding partition from the DDM
		partition = ddm->RegisterPartition(normalizedDevice.Path());
		if (partition == NULL) {
			// Partition not found: This either means, the user supplied
			// an invalid path, or the path refers to an image file. We try
			// to let the DDM create a file device for the path.
			partition_id deviceID = ddm->CreateFileDevice(
				normalizedDevice.Path(), &newlyCreatedFileDevice);
			if (deviceID >= 0) {
				partition = ddm->RegisterPartition(deviceID);
				if (newlyCreatedFileDevice)
					fileDeviceDeleter.id = deviceID;
			}
		}

		if (!partition) {
			TRACE(("fs_mount(): Partition `%s' not found.\n",
				normalizedDevice.Path()));
			return B_ENTRY_NOT_FOUND;
		}

		device = normalizedDevice.Path();
			// correct path to file device
	}
	PartitionRegistrar partitionRegistrar(partition, true);

	// Write lock the partition's device. For the time being, we keep the lock
	// until we're done mounting -- not nice, but ensure, that no-one is
	// interfering.
	// TODO: Just mark the partition busy while mounting!
	KDiskDevice* diskDevice = NULL;
	if (partition) {
		diskDevice = ddm->WriteLockDevice(partition->Device()->ID());
		if (!diskDevice) {
			TRACE(("fs_mount(): Failed to lock disk device!\n"));
			return B_ERROR;
		}
	}

	DeviceWriteLocker writeLocker(diskDevice, true);
		// this takes over the write lock acquired before

	if (partition != NULL) {
		// make sure, that the partition is not busy
		if (partition->IsBusy()) {
			TRACE(("fs_mount(): Partition is busy.\n"));
			return B_BUSY;
		}

		// if no FS name had been supplied, we get it from the partition
		if (fsName == NULL) {
			KDiskSystem* diskSystem = partition->DiskSystem();
			if (!diskSystem) {
				TRACE(("fs_mount(): No FS name was given, and the DDM didn't "
					"recognize it.\n"));
				return B_BAD_VALUE;
			}

			if (!diskSystem->IsFileSystem()) {
				TRACE(("fs_mount(): No FS name was given, and the DDM found a "
					"partitioning system.\n"));
				return B_BAD_VALUE;
			}

			// The disk system name will not change, and the KDiskSystem
			// object will not go away while the disk device is locked (and
			// the partition has a reference to it), so this is safe.
			fsName = diskSystem->Name();
		}
	}

	mount = new(std::nothrow) (struct ::fs_mount);
	if (mount == NULL)
		return B_NO_MEMORY;

	mount->device_name = strdup(device);
		// "device" can be NULL

	status = mount->entry_cache.Init();
	if (status != B_OK)
		goto err1;

	// initialize structure
	mount->id = sNextMountID++;
	mount->partition = NULL;
	mount->root_vnode = NULL;
	mount->covers_vnode = NULL;
	mount->unmounting = false;
	mount->owns_file_device = false;
	mount->volume = NULL;

	// build up the volume(s)
	while (true) {
		char* layerFSName = get_file_system_name_for_layer(fsName, layer);
		if (layerFSName == NULL) {
			if (layer == 0) {
				status = B_NO_MEMORY;
				goto err1;
			}

			break;
		}

		volume = (fs_volume*)malloc(sizeof(fs_volume));
		if (volume == NULL) {
			status = B_NO_MEMORY;
			free(layerFSName);
			goto err1;
		}

		volume->id = mount->id;
		volume->partition = partition != NULL ? partition->ID() : -1;
		volume->layer = layer++;
		volume->private_volume = NULL;
		volume->ops = NULL;
		volume->sub_volume = NULL;
		volume->super_volume = NULL;
		volume->file_system = NULL;
		volume->file_system_name = NULL;

		volume->file_system_name = get_file_system_name(layerFSName);
		if (volume->file_system_name == NULL) {
			status = B_NO_MEMORY;
			free(layerFSName);
			free(volume);
			goto err1;
		}

		volume->file_system = get_file_system(layerFSName);
		if (volume->file_system == NULL) {
			status = B_DEVICE_NOT_FOUND;
			free(layerFSName);
			free(volume->file_system_name);
			free(volume);
			goto err1;
		}

		if (mount->volume == NULL)
			mount->volume = volume;
		else {
			volume->super_volume = mount->volume;
			mount->volume->sub_volume = volume;
			mount->volume = volume;
		}
	}

	// insert mount struct into list before we call FS's mount() function
	// so that vnodes can be created for this mount
	mutex_lock(&sMountMutex);
	hash_insert(sMountsTable, mount);
	mutex_unlock(&sMountMutex);

	ino_t rootID;

	if (!sRoot) {
		// we haven't mounted anything yet
		if (strcmp(path, "/") != 0) {
			status = B_ERROR;
			goto err2;
		}

		status = mount->volume->file_system->mount(mount->volume, device, flags,
			args, &rootID);
		if (status != 0)
			goto err2;
	} else {
		status = path_to_vnode(path, true, &coveredNode, NULL, kernel);
		if (status != B_OK)
			goto err2;

		mount->covers_vnode = coveredNode;

		// make sure covered_vnode is a directory
		if (!S_ISDIR(coveredNode->Type())) {
			status = B_NOT_A_DIRECTORY;
			goto err3;
		}

		if (coveredNode->IsCovered()) {
			// this is already a covered vnode
			status = B_BUSY;
			goto err3;
		}

		// mount it/them
		fs_volume* volume = mount->volume;
		while (volume) {
			status = volume->file_system->mount(volume, device, flags, args,
				&rootID);
			if (status != B_OK) {
				if (volume->sub_volume)
					goto err4;
				goto err3;
			}

			volume = volume->super_volume;
		}

		volume = mount->volume;
		while (volume) {
			if (volume->ops->all_layers_mounted != NULL)
				volume->ops->all_layers_mounted(volume);
			volume = volume->super_volume;
		}
	}

	// the root node is supposed to be owned by the file system - it must
	// exist at this point
	mount->root_vnode = lookup_vnode(mount->id, rootID);
	if (mount->root_vnode == NULL || mount->root_vnode->ref_count != 1) {
		panic("fs_mount: file system does not own its root node!\n");
		status = B_ERROR;
		goto err4;
	}

	// set up the links between the root vnode and the vnode it covers
	rw_lock_write_lock(&sVnodeLock);
	if (coveredNode != NULL) {
		if (coveredNode->IsCovered()) {
			// the vnode is covered now
			status = B_BUSY;
			rw_lock_write_unlock(&sVnodeLock);
			goto err4;
		}

		mount->root_vnode->covers = coveredNode;
		mount->root_vnode->SetCovering(true);

		coveredNode->covered_by = mount->root_vnode;
		coveredNode->SetCovered(true);
	}
	rw_lock_write_unlock(&sVnodeLock);

	if (!sRoot) {
		sRoot = mount->root_vnode;
		mutex_lock(&sIOContextRootLock);
		get_current_io_context(true)->root = sRoot;
		mutex_unlock(&sIOContextRootLock);
		inc_vnode_ref_count(sRoot);
	}

	// supply the partition (if any) with the mount cookie and mark it mounted
	if (partition) {
		partition->SetMountCookie(mount->volume->private_volume);
		partition->SetVolumeID(mount->id);

		// keep a partition reference as long as the partition is mounted
		partitionRegistrar.Detach();
		mount->partition = partition;
		mount->owns_file_device = newlyCreatedFileDevice;
		fileDeviceDeleter.id = -1;
	}

	notify_mount(mount->id,
		coveredNode != NULL ? coveredNode->device : -1,
		coveredNode ? coveredNode->id : -1);

	return mount->id;

err4:
	FS_MOUNT_CALL_NO_PARAMS(mount, unmount);
err3:
	if (coveredNode != NULL)
		put_vnode(coveredNode);
err2:
	mutex_lock(&sMountMutex);
	hash_remove(sMountsTable, mount);
	mutex_unlock(&sMountMutex);
err1:
	delete mount;

	return status;
}


static status_t
fs_unmount(char* path, dev_t mountID, uint32 flags, bool kernel)
{
	struct fs_mount* mount;
	status_t err;

	FUNCTION(("fs_unmount(path '%s', dev %ld, kernel %d\n", path, mountID,
		kernel));

	struct vnode* pathVnode = NULL;
	if (path != NULL) {
		err = path_to_vnode(path, true, &pathVnode, NULL, kernel);
		if (err != B_OK)
			return B_ENTRY_NOT_FOUND;
	}

	RecursiveLocker mountOpLocker(sMountOpLock);

	// this lock is not strictly necessary, but here in case of KDEBUG
	// to keep the ASSERT in find_mount() working.
	KDEBUG_ONLY(mutex_lock(&sMountMutex));
	mount = find_mount(path != NULL ? pathVnode->device : mountID);
	KDEBUG_ONLY(mutex_unlock(&sMountMutex));
	if (mount == NULL) {
		panic("fs_unmount: find_mount() failed on root vnode @%p of mount\n",
			pathVnode);
	}

	if (path != NULL) {
		put_vnode(pathVnode);

		if (mount->root_vnode != pathVnode) {
			// not mountpoint
			return B_BAD_VALUE;
		}
	}

	// if the volume is associated with a partition, lock the device of the
	// partition as long as we are unmounting
	KDiskDeviceManager* ddm = KDiskDeviceManager::Default();
	KPartition* partition = mount->partition;
	KDiskDevice* diskDevice = NULL;
	if (partition != NULL) {
		if (partition->Device() == NULL) {
			dprintf("fs_unmount(): There is no device!\n");
			return B_ERROR;
		}
		diskDevice = ddm->WriteLockDevice(partition->Device()->ID());
		if (!diskDevice) {
			TRACE(("fs_unmount(): Failed to lock disk device!\n"));
			return B_ERROR;
		}
	}
	DeviceWriteLocker writeLocker(diskDevice, true);

	// make sure, that the partition is not busy
	if (partition != NULL) {
		if ((flags & B_UNMOUNT_BUSY_PARTITION) == 0 && partition->IsBusy()) {
			TRACE(("fs_unmount(): Partition is busy.\n"));
			return B_BUSY;
		}
	}

	// grab the vnode master mutex to keep someone from creating
	// a vnode while we're figuring out if we can continue
	WriteLocker vnodesWriteLocker(&sVnodeLock);

	bool disconnectedDescriptors = false;

	while (true) {
		bool busy = false;

		// cycle through the list of vnodes associated with this mount and
		// make sure all of them are not busy or have refs on them
		VnodeList::Iterator iterator = mount->vnodes.GetIterator();
		while (struct vnode* vnode = iterator.Next()) {
			// The root vnode ref_count needs to be 1 here (the mount has a
			// reference).
			if (vnode->IsBusy()
				|| ((vnode->ref_count != 0 && mount->root_vnode != vnode)
					|| (vnode->ref_count != 1 && mount->root_vnode == vnode))) {
				// there are still vnodes in use on this mount, so we cannot
				// unmount yet
				busy = true;
				break;
			}
		}

		if (!busy)
			break;

		if ((flags & B_FORCE_UNMOUNT) == 0)
			return B_BUSY;

		if (disconnectedDescriptors) {
			// wait a bit until the last access is finished, and then try again
			vnodesWriteLocker.Unlock();
			snooze(100000);
			// TODO: if there is some kind of bug that prevents the ref counts
			// from getting back to zero, this will fall into an endless loop...
			vnodesWriteLocker.Lock();
			continue;
		}

		// the file system is still busy - but we're forced to unmount it,
		// so let's disconnect all open file descriptors

		mount->unmounting = true;
			// prevent new vnodes from being created

		vnodesWriteLocker.Unlock();

		disconnect_mount_or_vnode_fds(mount, NULL);
		disconnectedDescriptors = true;

		vnodesWriteLocker.Lock();
	}

	// we can safely continue, mark all of the vnodes busy and this mount
	// structure in unmounting state
	mount->unmounting = true;

	VnodeList::Iterator iterator = mount->vnodes.GetIterator();
	while (struct vnode* vnode = iterator.Next()) {
		vnode->SetBusy(true);
		vnode_to_be_freed(vnode);
	}

	// The ref_count of the root node is 1 at this point, see above why this is
	mount->root_vnode->ref_count--;
	vnode_to_be_freed(mount->root_vnode);

	Vnode* coveredNode = mount->root_vnode->covers;
	coveredNode->covered_by = mount->root_vnode->covered_by;
	coveredNode->SetCovered(coveredNode->covered_by != NULL);

	mount->root_vnode->covered_by = NULL;
	mount->root_vnode->SetCovered(false);
	mount->root_vnode->SetCovering(false);

	vnodesWriteLocker.Unlock();

	put_vnode(coveredNode);

	// Free all vnodes associated with this mount.
	// They will be removed from the mount list by free_vnode(), so
	// we don't have to do this.
	while (struct vnode* vnode = mount->vnodes.Head())
		free_vnode(vnode, false);

	// remove the mount structure from the hash table
	mutex_lock(&sMountMutex);
	hash_remove(sMountsTable, mount);
	mutex_unlock(&sMountMutex);

	mountOpLocker.Unlock();

	FS_MOUNT_CALL_NO_PARAMS(mount, unmount);
	notify_unmount(mount->id);

	// dereference the partition and mark it unmounted
	if (partition) {
		partition->SetVolumeID(-1);
		partition->SetMountCookie(NULL);

		if (mount->owns_file_device)
			KDiskDeviceManager::Default()->DeleteFileDevice(partition->ID());
		partition->Unregister();
	}

	delete mount;
	return B_OK;
}


static status_t
fs_sync(dev_t device)
{
	struct fs_mount* mount;
	status_t status = get_mount(device, &mount);
	if (status != B_OK)
		return status;

	struct vnode marker;
	memset(&marker, 0, sizeof(marker));
	marker.SetBusy(true);
	marker.SetRemoved(true);

	// First, synchronize all file caches

	while (true) {
		WriteLocker locker(sVnodeLock);
			// Note: That's the easy way. Which is probably OK for sync(),
			// since it's a relatively rare call and doesn't need to allow for
			// a lot of concurrency. Using a read lock would be possible, but
			// also more involved, since we had to lock the individual nodes
			// and take care of the locking order, which we might not want to
			// do while holding fs_mount::rlock.

		// synchronize access to vnode list
		recursive_lock_lock(&mount->rlock);

		struct vnode* vnode;
		if (!marker.IsRemoved()) {
			vnode = mount->vnodes.GetNext(&marker);
			mount->vnodes.Remove(&marker);
			marker.SetRemoved(true);
		} else
			vnode = mount->vnodes.First();

		while (vnode != NULL && (vnode->cache == NULL
			|| vnode->IsRemoved() || vnode->IsBusy())) {
			// TODO: we could track writes (and writable mapped vnodes)
			//	and have a simple flag that we could test for here
			vnode = mount->vnodes.GetNext(vnode);
		}

		if (vnode != NULL) {
			// insert marker vnode again
			mount->vnodes.Insert(mount->vnodes.GetNext(vnode), &marker);
			marker.SetRemoved(false);
		}

		recursive_lock_unlock(&mount->rlock);

		if (vnode == NULL)
			break;

		vnode = lookup_vnode(mount->id, vnode->id);
		if (vnode == NULL || vnode->IsBusy())
			continue;

		if (vnode->ref_count == 0) {
			// this vnode has been unused before
			vnode_used(vnode);
		}
		inc_vnode_ref_count(vnode);

		locker.Unlock();

		if (vnode->cache != NULL && !vnode->IsRemoved())
			vnode->cache->WriteModified();

		put_vnode(vnode);
	}

	// And then, let the file systems do their synchronizing work

	if (HAS_FS_MOUNT_CALL(mount, sync))
		status = FS_MOUNT_CALL_NO_PARAMS(mount, sync);

	put_mount(mount);
	return status;
}


static status_t
fs_read_info(dev_t device, struct fs_info* info)
{
	struct fs_mount* mount;
	status_t status = get_mount(device, &mount);
	if (status != B_OK)
		return status;

	memset(info, 0, sizeof(struct fs_info));

	if (HAS_FS_MOUNT_CALL(mount, read_fs_info))
		status = FS_MOUNT_CALL(mount, read_fs_info, info);

	// fill in info the file system doesn't (have to) know about
	if (status == B_OK) {
		info->dev = mount->id;
		info->root = mount->root_vnode->id;

		fs_volume* volume = mount->volume;
		while (volume->super_volume != NULL)
			volume = volume->super_volume;

		strlcpy(info->fsh_name, volume->file_system_name,
			sizeof(info->fsh_name));
		if (mount->device_name != NULL) {
			strlcpy(info->device_name, mount->device_name,
				sizeof(info->device_name));
		}
	}

	// if the call is not supported by the file system, there are still
	// the parts that we filled out ourselves

	put_mount(mount);
	return status;
}


static status_t
fs_write_info(dev_t device, const struct fs_info* info, int mask)
{
	struct fs_mount* mount;
	status_t status = get_mount(device, &mount);
	if (status != B_OK)
		return status;

	if (HAS_FS_MOUNT_CALL(mount, write_fs_info))
		status = FS_MOUNT_CALL(mount, write_fs_info, info, mask);
	else
		status = B_READ_ONLY_DEVICE;

	put_mount(mount);
	return status;
}


static dev_t
fs_next_device(int32* _cookie)
{
	struct fs_mount* mount = NULL;
	dev_t device = *_cookie;

	mutex_lock(&sMountMutex);

	// Since device IDs are assigned sequentially, this algorithm
	// does work good enough. It makes sure that the device list
	// returned is sorted, and that no device is skipped when an
	// already visited device got unmounted.

	while (device < sNextMountID) {
		mount = find_mount(device++);
		if (mount != NULL && mount->volume->private_volume != NULL)
			break;
	}

	*_cookie = device;

	if (mount != NULL)
		device = mount->id;
	else
		device = B_BAD_VALUE;

	mutex_unlock(&sMountMutex);

	return device;
}


ssize_t
fs_read_attr(int fd, const char *attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	int attrFD = attr_open(fd, NULL, attribute, O_RDONLY, true);
	if (attrFD < 0)
		return attrFD;

	ssize_t bytesRead = _kern_read(attrFD, pos, buffer, readBytes);

	_kern_close(attrFD);

	return bytesRead;
}


static status_t
get_cwd(char* buffer, size_t size, bool kernel)
{
	// Get current working directory from io context
	struct io_context* context = get_current_io_context(kernel);
	status_t status;

	FUNCTION(("vfs_get_cwd: buf %p, size %ld\n", buffer, size));

	mutex_lock(&context->io_mutex);

	struct vnode* vnode = context->cwd;
	if (vnode)
		inc_vnode_ref_count(vnode);

	mutex_unlock(&context->io_mutex);

	if (vnode) {
		status = dir_vnode_to_path(vnode, buffer, size, kernel);
		put_vnode(vnode);
	} else
		status = B_ERROR;

	return status;
}


static status_t
set_cwd(int fd, char* path, bool kernel)
{
	struct io_context* context;
	struct vnode* vnode = NULL;
	struct vnode* oldDirectory;
	status_t status;

	FUNCTION(("set_cwd: path = \'%s\'\n", path));

	// Get vnode for passed path, and bail if it failed
	status = fd_and_path_to_vnode(fd, path, true, &vnode, NULL, kernel);
	if (status < 0)
		return status;

	if (!S_ISDIR(vnode->Type())) {
		// nope, can't cwd to here
		status = B_NOT_A_DIRECTORY;
		goto err;
	}

	// Get current io context and lock
	context = get_current_io_context(kernel);
	mutex_lock(&context->io_mutex);

	// save the old current working directory first
	oldDirectory = context->cwd;
	context->cwd = vnode;

	mutex_unlock(&context->io_mutex);

	if (oldDirectory)
		put_vnode(oldDirectory);

	return B_NO_ERROR;

err:
	put_vnode(vnode);
	return status;
}


//	#pragma mark - kernel mirrored syscalls


dev_t
_kern_mount(const char* path, const char* device, const char* fsName,
	uint32 flags, const char* args, size_t argsLength)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return fs_mount(pathBuffer.LockBuffer(), device, fsName, flags, args, true);
}


status_t
_kern_unmount(const char* path, uint32 flags)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return fs_unmount(pathBuffer.LockBuffer(), -1, flags, true);
}


status_t
_kern_read_fs_info(dev_t device, struct fs_info* info)
{
	if (info == NULL)
		return B_BAD_VALUE;

	return fs_read_info(device, info);
}


status_t
_kern_write_fs_info(dev_t device, const struct fs_info* info, int mask)
{
	if (info == NULL)
		return B_BAD_VALUE;

	return fs_write_info(device, info, mask);
}


status_t
_kern_sync(void)
{
	// Note: _kern_sync() is also called from _user_sync()
	int32 cookie = 0;
	dev_t device;
	while ((device = next_dev(&cookie)) >= 0) {
		status_t status = fs_sync(device);
		if (status != B_OK && status != B_BAD_VALUE) {
			dprintf("sync: device %ld couldn't sync: %s\n", device,
				strerror(status));
		}
	}

	return B_OK;
}


dev_t
_kern_next_device(int32* _cookie)
{
	return fs_next_device(_cookie);
}


status_t
_kern_get_next_fd_info(team_id teamID, uint32* _cookie, fd_info* info,
	size_t infoSize)
{
	if (infoSize != sizeof(fd_info))
		return B_BAD_VALUE;

	// get the team
	Team* team = Team::Get(teamID);
	if (team == NULL)
		return B_BAD_TEAM_ID;
	BReference<Team> teamReference(team, true);

	// now that we have a team reference, its I/O context won't go away
	io_context* context = team->io_context;
	MutexLocker contextLocker(context->io_mutex);

	uint32 slot = *_cookie;

	struct file_descriptor* descriptor;
	while (slot < context->table_size
		&& (descriptor = context->fds[slot]) == NULL) {
		slot++;
	}

	if (slot >= context->table_size)
		return B_ENTRY_NOT_FOUND;

	info->number = slot;
	info->open_mode = descriptor->open_mode;

	struct vnode* vnode = fd_vnode(descriptor);
	if (vnode != NULL) {
		info->device = vnode->device;
		info->node = vnode->id;
	} else if (descriptor->u.mount != NULL) {
		info->device = descriptor->u.mount->id;
		info->node = -1;
	}

	*_cookie = slot + 1;
	return B_OK;
}


int
_kern_open_entry_ref(dev_t device, ino_t inode, const char* name, int openMode,
	int perms)
{
	if ((openMode & O_CREAT) != 0) {
		return file_create_entry_ref(device, inode, name, openMode, perms,
			true);
	}

	return file_open_entry_ref(device, inode, name, openMode, true);
}


/*!	\brief Opens a node specified by a FD + path pair.

	At least one of \a fd and \a path must be specified.
	If only \a fd is given, the function opens the node identified by this
	FD. If only a path is given, this path is opened. If both are given and
	the path is absolute, \a fd is ignored; a relative path is reckoned off
	of the directory (!) identified by \a fd.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. May be \c NULL.
	\param openMode The open mode.
	\return A FD referring to the newly opened node, or an error code,
			if an error occurs.
*/
int
_kern_open(int fd, const char* path, int openMode, int perms)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	if (openMode & O_CREAT)
		return file_create(fd, pathBuffer.LockBuffer(), openMode, perms, true);

	return file_open(fd, pathBuffer.LockBuffer(), openMode, true);
}


/*!	\brief Opens a directory specified by entry_ref or node_ref.

	The supplied name may be \c NULL, in which case directory identified
	by \a device and \a inode will be opened. Otherwise \a device and
	\a inode identify the parent directory of the directory to be opened
	and \a name its entry name.

	\param device If \a name is specified the ID of the device the parent
		   directory of the directory to be opened resides on, otherwise
		   the device of the directory itself.
	\param inode If \a name is specified the node ID of the parent
		   directory of the directory to be opened, otherwise node ID of the
		   directory itself.
	\param name The entry name of the directory to be opened. If \c NULL,
		   the \a device + \a inode pair identify the node to be opened.
	\return The FD of the newly opened directory or an error code, if
			something went wrong.
*/
int
_kern_open_dir_entry_ref(dev_t device, ino_t inode, const char* name)
{
	return dir_open_entry_ref(device, inode, name, true);
}


/*!	\brief Opens a directory specified by a FD + path pair.

	At least one of \a fd and \a path must be specified.
	If only \a fd is given, the function opens the directory identified by this
	FD. If only a path is given, this path is opened. If both are given and
	the path is absolute, \a fd is ignored; a relative path is reckoned off
	of the directory (!) identified by \a fd.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. May be \c NULL.
	\return A FD referring to the newly opened directory, or an error code,
			if an error occurs.
*/
int
_kern_open_dir(int fd, const char* path)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return dir_open(fd, pathBuffer.LockBuffer(), true);
}


status_t
_kern_fcntl(int fd, int op, uint32 argument)
{
	return common_fcntl(fd, op, argument, true);
}


status_t
_kern_fsync(int fd)
{
	return common_sync(fd, true);
}


status_t
_kern_lock_node(int fd)
{
	return common_lock_node(fd, true);
}


status_t
_kern_unlock_node(int fd)
{
	return common_unlock_node(fd, true);
}


status_t
_kern_create_dir_entry_ref(dev_t device, ino_t inode, const char* name,
	int perms)
{
	return dir_create_entry_ref(device, inode, name, perms, true);
}


/*!	\brief Creates a directory specified by a FD + path pair.

	\a path must always be specified (it contains the name of the new directory
	at least). If only a path is given, this path identifies the location at
	which the directory shall be created. If both \a fd and \a path are given
	and the path is absolute, \a fd is ignored; a relative path is reckoned off
	of the directory (!) identified by \a fd.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. Must not be \c NULL.
	\param perms The access permissions the new directory shall have.
	\return \c B_OK, if the directory has been created successfully, another
			error code otherwise.
*/
status_t
_kern_create_dir(int fd, const char* path, int perms)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return dir_create(fd, pathBuffer.LockBuffer(), perms, true);
}


status_t
_kern_remove_dir(int fd, const char* path)
{
	if (path) {
		KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		return dir_remove(fd, pathBuffer.LockBuffer(), true);
	}

	return dir_remove(fd, NULL, true);
}


/*!	\brief Reads the contents of a symlink referred to by a FD + path pair.

	At least one of \a fd and \a path must be specified.
	If only \a fd is given, the function the symlink to be read is the node
	identified by this FD. If only a path is given, this path identifies the
	symlink to be read. If both are given and the path is absolute, \a fd is
	ignored; a relative path is reckoned off of the directory (!) identified
	by \a fd.
	If this function fails with B_BUFFER_OVERFLOW, the \a _bufferSize pointer
	will still be updated to reflect the required buffer size.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. May be \c NULL.
	\param buffer The buffer into which the contents of the symlink shall be
		   written.
	\param _bufferSize A pointer to the size of the supplied buffer.
	\return The length of the link on success or an appropriate error code
*/
status_t
_kern_read_link(int fd, const char* path, char* buffer, size_t* _bufferSize)
{
	if (path) {
		KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		return common_read_link(fd, pathBuffer.LockBuffer(),
			buffer, _bufferSize, true);
	}

	return common_read_link(fd, NULL, buffer, _bufferSize, true);
}


/*!	\brief Creates a symlink specified by a FD + path pair.

	\a path must always be specified (it contains the name of the new symlink
	at least). If only a path is given, this path identifies the location at
	which the symlink shall be created. If both \a fd and \a path are given and
	the path is absolute, \a fd is ignored; a relative path is reckoned off
	of the directory (!) identified by \a fd.

	\param fd The FD. May be < 0.
	\param toPath The absolute or relative path. Must not be \c NULL.
	\param mode The access permissions the new symlink shall have.
	\return \c B_OK, if the symlink has been created successfully, another
			error code otherwise.
*/
status_t
_kern_create_symlink(int fd, const char* path, const char* toPath, int mode)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return common_create_symlink(fd, pathBuffer.LockBuffer(),
		toPath, mode, true);
}


status_t
_kern_create_link(int pathFD, const char* path, int toFD, const char* toPath,
	bool traverseLeafLink)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	KPath toPathBuffer(toPath, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK || toPathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return common_create_link(pathFD, pathBuffer.LockBuffer(), toFD,
		toPathBuffer.LockBuffer(), traverseLeafLink, true);
}


/*!	\brief Removes an entry specified by a FD + path pair from its directory.

	\a path must always be specified (it contains at least the name of the entry
	to be deleted). If only a path is given, this path identifies the entry
	directly. If both \a fd and \a path are given and the path is absolute,
	\a fd is ignored; a relative path is reckoned off of the directory (!)
	identified by \a fd.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. Must not be \c NULL.
	\return \c B_OK, if the entry has been removed successfully, another
			error code otherwise.
*/
status_t
_kern_unlink(int fd, const char* path)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return common_unlink(fd, pathBuffer.LockBuffer(), true);
}


/*!	\brief Moves an entry specified by a FD + path pair to a an entry specified
		   by another FD + path pair.

	\a oldPath and \a newPath must always be specified (they contain at least
	the name of the entry). If only a path is given, this path identifies the
	entry directly. If both a FD and a path are given and the path is absolute,
	the FD is ignored; a relative path is reckoned off of the directory (!)
	identified by the respective FD.

	\param oldFD The FD of the old location. May be < 0.
	\param oldPath The absolute or relative path of the old location. Must not
		   be \c NULL.
	\param newFD The FD of the new location. May be < 0.
	\param newPath The absolute or relative path of the new location. Must not
		   be \c NULL.
	\return \c B_OK, if the entry has been moved successfully, another
			error code otherwise.
*/
status_t
_kern_rename(int oldFD, const char* oldPath, int newFD, const char* newPath)
{
	KPath oldPathBuffer(oldPath, false, B_PATH_NAME_LENGTH + 1);
	KPath newPathBuffer(newPath, false, B_PATH_NAME_LENGTH + 1);
	if (oldPathBuffer.InitCheck() != B_OK || newPathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return common_rename(oldFD, oldPathBuffer.LockBuffer(),
		newFD, newPathBuffer.LockBuffer(), true);
}


status_t
_kern_access(int fd, const char* path, int mode, bool effectiveUserGroup)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	return common_access(fd, pathBuffer.LockBuffer(), mode, effectiveUserGroup,
		true);
}


/*!	\brief Reads stat data of an entity specified by a FD + path pair.

	If only \a fd is given, the stat operation associated with the type
	of the FD (node, attr, attr dir etc.) is performed. If only \a path is
	given, this path identifies the entry for whose node to retrieve the
	stat data. If both \a fd and \a path are given and the path is absolute,
	\a fd is ignored; a relative path is reckoned off of the directory (!)
	identified by \a fd and specifies the entry whose stat data shall be
	retrieved.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. Must not be \c NULL.
	\param traverseLeafLink If \a path is given, \c true specifies that the
		   function shall not stick to symlinks, but traverse them.
	\param stat The buffer the stat data shall be written into.
	\param statSize The size of the supplied stat buffer.
	\return \c B_OK, if the the stat data have been read successfully, another
			error code otherwise.
*/
status_t
_kern_read_stat(int fd, const char* path, bool traverseLeafLink,
	struct stat* stat, size_t statSize)
{
	struct stat completeStat;
	struct stat* originalStat = NULL;
	status_t status;

	if (statSize > sizeof(struct stat))
		return B_BAD_VALUE;

	// this supports different stat extensions
	if (statSize < sizeof(struct stat)) {
		originalStat = stat;
		stat = &completeStat;
	}

	status = vfs_read_stat(fd, path, traverseLeafLink, stat, true);

	if (status == B_OK && originalStat != NULL)
		memcpy(originalStat, stat, statSize);

	return status;
}


/*!	\brief Writes stat data of an entity specified by a FD + path pair.

	If only \a fd is given, the stat operation associated with the type
	of the FD (node, attr, attr dir etc.) is performed. If only \a path is
	given, this path identifies the entry for whose node to write the
	stat data. If both \a fd and \a path are given and the path is absolute,
	\a fd is ignored; a relative path is reckoned off of the directory (!)
	identified by \a fd and specifies the entry whose stat data shall be
	written.

	\param fd The FD. May be < 0.
	\param path The absolute or relative path. Must not be \c NULL.
	\param traverseLeafLink If \a path is given, \c true specifies that the
		   function shall not stick to symlinks, but traverse them.
	\param stat The buffer containing the stat data to be written.
	\param statSize The size of the supplied stat buffer.
	\param statMask A mask specifying which parts of the stat data shall be
		   written.
	\return \c B_OK, if the the stat data have been written successfully,
			another error code otherwise.
*/
status_t
_kern_write_stat(int fd, const char* path, bool traverseLeafLink,
	const struct stat* stat, size_t statSize, int statMask)
{
	struct stat completeStat;

	if (statSize > sizeof(struct stat))
		return B_BAD_VALUE;

	// this supports different stat extensions
	if (statSize < sizeof(struct stat)) {
		memset((uint8*)&completeStat + statSize, 0,
			sizeof(struct stat) - statSize);
		memcpy(&completeStat, stat, statSize);
		stat = &completeStat;
	}

	status_t status;

	if (path) {
		// path given: write the stat of the node referred to by (fd, path)
		KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		status = common_path_write_stat(fd, pathBuffer.LockBuffer(),
			traverseLeafLink, stat, statMask, true);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor* descriptor
			= get_fd(get_current_io_context(true), fd);
		if (descriptor == NULL)
			return B_FILE_ERROR;

		if (descriptor->ops->fd_write_stat)
			status = descriptor->ops->fd_write_stat(descriptor, stat, statMask);
		else
			status = B_UNSUPPORTED;

		put_fd(descriptor);
	}

	return status;
}


int
_kern_open_attr_dir(int fd, const char* path, bool traverseLeafLink)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	if (path != NULL)
		pathBuffer.SetTo(path);

	return attr_dir_open(fd, path ? pathBuffer.LockBuffer() : NULL,
		traverseLeafLink, true);
}


int
_kern_open_attr(int fd, const char* path, const char* name, uint32 type,
	int openMode)
{
	KPath pathBuffer(path, false, B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	if ((openMode & O_CREAT) != 0) {
		return attr_create(fd, pathBuffer.LockBuffer(), name, type, openMode,
			true);
	}

	return attr_open(fd, pathBuffer.LockBuffer(), name, openMode, true);
}


status_t
_kern_remove_attr(int fd, const char* name)
{
	return attr_remove(fd, name, true);
}


status_t
_kern_rename_attr(int fromFile, const char* fromName, int toFile,
	const char* toName)
{
	return attr_rename(fromFile, fromName, toFile, toName, true);
}


int
_kern_open_index_dir(dev_t device)
{
	return index_dir_open(device, true);
}


status_t
_kern_create_index(dev_t device, const char* name, uint32 type, uint32 flags)
{
	return index_create(device, name, type, flags, true);
}


status_t
_kern_read_index_stat(dev_t device, const char* name, struct stat* stat)
{
	return index_name_read_stat(device, name, stat, true);
}


status_t
_kern_remove_index(dev_t device, const char* name)
{
	return index_remove(device, name, true);
}


status_t
_kern_getcwd(char* buffer, size_t size)
{
	TRACE(("_kern_getcwd: buf %p, %ld\n", buffer, size));

	// Call vfs to get current working directory
	return get_cwd(buffer, size, true);
}


status_t
_kern_setcwd(int fd, const char* path)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	if (path != NULL)
		pathBuffer.SetTo(path);

	return set_cwd(fd, path != NULL ? pathBuffer.LockBuffer() : NULL, true);
}


//	#pragma mark - userland syscalls


dev_t
_user_mount(const char* userPath, const char* userDevice,
	const char* userFileSystem, uint32 flags, const char* userArgs,
	size_t argsLength)
{
	char fileSystem[B_FILE_NAME_LENGTH];
	KPath path, device;
	char* args = NULL;
	status_t status;

	if (!IS_USER_ADDRESS(userPath)
		|| !IS_USER_ADDRESS(userFileSystem)
		|| !IS_USER_ADDRESS(userDevice))
		return B_BAD_ADDRESS;

	if (path.InitCheck() != B_OK || device.InitCheck() != B_OK)
		return B_NO_MEMORY;

	if (user_strlcpy(path.LockBuffer(), userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	if (userFileSystem != NULL
		&& user_strlcpy(fileSystem, userFileSystem, sizeof(fileSystem)) < B_OK)
		return B_BAD_ADDRESS;

	if (userDevice != NULL
		&& user_strlcpy(device.LockBuffer(), userDevice, B_PATH_NAME_LENGTH)
			< B_OK)
		return B_BAD_ADDRESS;

	if (userArgs != NULL && argsLength > 0) {
		// this is a safety restriction
		if (argsLength >= 65536)
			return B_NAME_TOO_LONG;

		args = (char*)malloc(argsLength + 1);
		if (args == NULL)
			return B_NO_MEMORY;

		if (user_strlcpy(args, userArgs, argsLength + 1) < B_OK) {
			free(args);
			return B_BAD_ADDRESS;
		}
	}
	path.UnlockBuffer();
	device.UnlockBuffer();

	status = fs_mount(path.LockBuffer(),
		userDevice != NULL ? device.Path() : NULL,
		userFileSystem ? fileSystem : NULL, flags, args, false);

	free(args);
	return status;
}


status_t
_user_unmount(const char* userPath, uint32 flags)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return fs_unmount(path, -1, flags & ~B_UNMOUNT_BUSY_PARTITION, false);
}


status_t
_user_read_fs_info(dev_t device, struct fs_info* userInfo)
{
	struct fs_info info;
	status_t status;

	if (userInfo == NULL)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo))
		return B_BAD_ADDRESS;

	status = fs_read_info(device, &info);
	if (status != B_OK)
		return status;

	if (user_memcpy(userInfo, &info, sizeof(struct fs_info)) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_write_fs_info(dev_t device, const struct fs_info* userInfo, int mask)
{
	struct fs_info info;

	if (userInfo == NULL)
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&info, userInfo, sizeof(struct fs_info)) != B_OK)
		return B_BAD_ADDRESS;

	return fs_write_info(device, &info, mask);
}


dev_t
_user_next_device(int32* _userCookie)
{
	int32 cookie;
	dev_t device;

	if (!IS_USER_ADDRESS(_userCookie)
		|| user_memcpy(&cookie, _userCookie, sizeof(int32)) != B_OK)
		return B_BAD_ADDRESS;

	device = fs_next_device(&cookie);

	if (device >= B_OK) {
		// update user cookie
		if (user_memcpy(_userCookie, &cookie, sizeof(int32)) != B_OK)
			return B_BAD_ADDRESS;
	}

	return device;
}


status_t
_user_sync(void)
{
	return _kern_sync();
}


status_t
_user_get_next_fd_info(team_id team, uint32* userCookie, fd_info* userInfo,
	size_t infoSize)
{
	struct fd_info info;
	uint32 cookie;

	// only root can do this (or should root's group be enough?)
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	if (infoSize != sizeof(fd_info))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userCookie) || !IS_USER_ADDRESS(userInfo)
		|| user_memcpy(&cookie, userCookie, sizeof(uint32)) != B_OK)
		return B_BAD_ADDRESS;

	status_t status = _kern_get_next_fd_info(team, &cookie, &info, infoSize);
	if (status != B_OK)
		return status;

	if (user_memcpy(userCookie, &cookie, sizeof(uint32)) != B_OK
		|| user_memcpy(userInfo, &info, infoSize) != B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_entry_ref_to_path(dev_t device, ino_t inode, const char* leaf,
	char* userPath, size_t pathLength)
{
	if (!IS_USER_ADDRESS(userPath))
		return B_BAD_ADDRESS;

	KPath path(B_PATH_NAME_LENGTH + 1);
	if (path.InitCheck() != B_OK)
		return B_NO_MEMORY;

	// copy the leaf name onto the stack
	char stackLeaf[B_FILE_NAME_LENGTH];
	if (leaf) {
		if (!IS_USER_ADDRESS(leaf))
			return B_BAD_ADDRESS;

		int length = user_strlcpy(stackLeaf, leaf, B_FILE_NAME_LENGTH);
		if (length < 0)
			return length;
		if (length >= B_FILE_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		leaf = stackLeaf;
	}

	status_t status = vfs_entry_ref_to_path(device, inode, leaf,
		path.LockBuffer(), path.BufferSize());
	if (status != B_OK)
		return status;

	path.UnlockBuffer();

	int length = user_strlcpy(userPath, path.Path(), pathLength);
	if (length < 0)
		return length;
	if (length >= (int)pathLength)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


status_t
_user_normalize_path(const char* userPath, bool traverseLink, char* buffer)
{
	if (userPath == NULL || buffer == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userPath) || !IS_USER_ADDRESS(buffer))
		return B_BAD_ADDRESS;

	// copy path from userland
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;
	char* path = pathBuffer.LockBuffer();

	if (user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	status_t error = normalize_path(path, pathBuffer.BufferSize(), traverseLink,
		false);
	if (error != B_OK)
		return error;

	// copy back to userland
	int len = user_strlcpy(buffer, path, B_PATH_NAME_LENGTH);
	if (len < 0)
		return len;
	if (len >= B_PATH_NAME_LENGTH)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


int
_user_open_entry_ref(dev_t device, ino_t inode, const char* userName,
	int openMode, int perms)
{
	char name[B_FILE_NAME_LENGTH];

	if (userName == NULL || device < 0 || inode < 0)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, sizeof(name)) < B_OK)
		return B_BAD_ADDRESS;

	if ((openMode & O_CREAT) != 0) {
		return file_create_entry_ref(device, inode, name, openMode, perms,
		 false);
	}

	return file_open_entry_ref(device, inode, name, openMode, false);
}


int
_user_open(int fd, const char* userPath, int openMode, int perms)
{
	KPath path(B_PATH_NAME_LENGTH + 1);
	if (path.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* buffer = path.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(buffer, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	if ((openMode & O_CREAT) != 0)
		return file_create(fd, buffer, openMode, perms, false);

	return file_open(fd, buffer, openMode, false);
}


int
_user_open_dir_entry_ref(dev_t device, ino_t inode, const char* userName)
{
	if (userName != NULL) {
		char name[B_FILE_NAME_LENGTH];

		if (!IS_USER_ADDRESS(userName)
			|| user_strlcpy(name, userName, sizeof(name)) < B_OK)
			return B_BAD_ADDRESS;

		return dir_open_entry_ref(device, inode, name, false);
	}
	return dir_open_entry_ref(device, inode, NULL, false);
}


int
_user_open_dir(int fd, const char* userPath)
{
	if (userPath == NULL)
		return dir_open(fd, NULL, false);

	KPath path(B_PATH_NAME_LENGTH + 1);
	if (path.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* buffer = path.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(buffer, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return dir_open(fd, buffer, false);
}


/*!	\brief Opens a directory's parent directory and returns the entry name
		   of the former.

	Aside from that it returns the directory's entry name, this method is
	equivalent to \code _user_open_dir(fd, "..") \endcode. It really is
	equivalent, if \a userName is \c NULL.

	If a name buffer is supplied and the name does not fit the buffer, the
	function fails. A buffer of size \c B_FILE_NAME_LENGTH should be safe.

	\param fd A FD referring to a directory.
	\param userName Buffer the directory's entry name shall be written into.
		   May be \c NULL.
	\param nameLength Size of the name buffer.
	\return The file descriptor of the opened parent directory, if everything
			went fine, an error code otherwise.
*/
int
_user_open_parent_dir(int fd, char* userName, size_t nameLength)
{
	bool kernel = false;

	if (userName && !IS_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	// open the parent dir
	int parentFD = dir_open(fd, (char*)"..", kernel);
	if (parentFD < 0)
		return parentFD;
	FDCloser fdCloser(parentFD, kernel);

	if (userName) {
		// get the vnodes
		struct vnode* parentVNode = get_vnode_from_fd(parentFD, kernel);
		struct vnode* dirVNode = get_vnode_from_fd(fd, kernel);
		VNodePutter parentVNodePutter(parentVNode);
		VNodePutter dirVNodePutter(dirVNode);
		if (!parentVNode || !dirVNode)
			return B_FILE_ERROR;

		// get the vnode name
		char _buffer[sizeof(struct dirent) + B_FILE_NAME_LENGTH];
		struct dirent* buffer = (struct dirent*)_buffer;
		status_t status = get_vnode_name(dirVNode, parentVNode, buffer,
			sizeof(_buffer), get_current_io_context(false));
		if (status != B_OK)
			return status;

		// copy the name to the userland buffer
		int len = user_strlcpy(userName, buffer->d_name, nameLength);
		if (len < 0)
			return len;
		if (len >= (int)nameLength)
			return B_BUFFER_OVERFLOW;
	}

	return fdCloser.Detach();
}


status_t
_user_fcntl(int fd, int op, uint32 argument)
{
	status_t status = common_fcntl(fd, op, argument, false);
	if (op == F_SETLKW)
		syscall_restart_handle_post(status);

	return status;
}


status_t
_user_fsync(int fd)
{
	return common_sync(fd, false);
}


status_t
_user_flock(int fd, int operation)
{
	FUNCTION(("_user_fcntl(fd = %d, op = %d)\n", fd, operation));

	// Check if the operation is valid
	switch (operation & ~LOCK_NB) {
		case LOCK_UN:
		case LOCK_SH:
		case LOCK_EX:
			break;

		default:
			return B_BAD_VALUE;
	}

	struct file_descriptor* descriptor;
	struct vnode* vnode;
	descriptor = get_fd_and_vnode(fd, &vnode, false);
	if (descriptor == NULL)
		return B_FILE_ERROR;

	if (descriptor->type != FDTYPE_FILE) {
		put_fd(descriptor);
		return B_BAD_VALUE;
	}

	struct flock flock;
	flock.l_start = 0;
	flock.l_len = OFF_MAX;
	flock.l_whence = 0;
	flock.l_type = (operation & LOCK_SH) != 0 ? F_RDLCK : F_WRLCK;

	status_t status;
	if ((operation & LOCK_UN) != 0)
		status = release_advisory_lock(vnode, &flock);
	else {
		status = acquire_advisory_lock(vnode,
			thread_get_current_thread()->team->session_id, &flock,
			(operation & LOCK_NB) == 0);
	}

	syscall_restart_handle_post(status);

	put_fd(descriptor);
	return status;
}


status_t
_user_lock_node(int fd)
{
	return common_lock_node(fd, false);
}


status_t
_user_unlock_node(int fd)
{
	return common_unlock_node(fd, false);
}


status_t
_user_create_dir_entry_ref(dev_t device, ino_t inode, const char* userName,
	int perms)
{
	char name[B_FILE_NAME_LENGTH];
	status_t status;

	if (!IS_USER_ADDRESS(userName))
		return B_BAD_ADDRESS;

	status = user_strlcpy(name, userName, sizeof(name));
	if (status < 0)
		return status;

	return dir_create_entry_ref(device, inode, name, perms, false);
}


status_t
_user_create_dir(int fd, const char* userPath, int perms)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return dir_create(fd, path, perms, false);
}


status_t
_user_remove_dir(int fd, const char* userPath)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (userPath != NULL) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;
	}

	return dir_remove(fd, userPath ? path : NULL, false);
}


status_t
_user_read_link(int fd, const char* userPath, char* userBuffer,
	size_t* userBufferSize)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1), linkBuffer;
	if (pathBuffer.InitCheck() != B_OK || linkBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	size_t bufferSize;

	if (!IS_USER_ADDRESS(userBuffer) || !IS_USER_ADDRESS(userBufferSize)
		|| user_memcpy(&bufferSize, userBufferSize, sizeof(size_t)) != B_OK)
		return B_BAD_ADDRESS;

	char* path = pathBuffer.LockBuffer();
	char* buffer = linkBuffer.LockBuffer();

	if (userPath) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;

		if (bufferSize > B_PATH_NAME_LENGTH)
			bufferSize = B_PATH_NAME_LENGTH;
	}

	status_t status = common_read_link(fd, userPath ? path : NULL, buffer,
		&bufferSize, false);

	// we also update the bufferSize in case of errors
	// (the real length will be returned in case of B_BUFFER_OVERFLOW)
	if (user_memcpy(userBufferSize, &bufferSize, sizeof(size_t)) != B_OK)
		return B_BAD_ADDRESS;

	if (status != B_OK)
		return status;

	if (user_memcpy(userBuffer, buffer, bufferSize) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_create_symlink(int fd, const char* userPath, const char* userToPath,
	int mode)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	KPath toPathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK || toPathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();
	char* toPath = toPathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| !IS_USER_ADDRESS(userToPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK
		|| user_strlcpy(toPath, userToPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return common_create_symlink(fd, path, toPath, mode, false);
}


status_t
_user_create_link(int pathFD, const char* userPath, int toFD,
	const char* userToPath, bool traverseLeafLink)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	KPath toPathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK || toPathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();
	char* toPath = toPathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| !IS_USER_ADDRESS(userToPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK
		|| user_strlcpy(toPath, userToPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	status_t status = check_path(toPath);
	if (status != B_OK)
		return status;

	return common_create_link(pathFD, path, toFD, toPath, traverseLeafLink,
		false);
}


status_t
_user_unlink(int fd, const char* userPath)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return common_unlink(fd, path, false);
}


status_t
_user_rename(int oldFD, const char* userOldPath, int newFD,
	const char* userNewPath)
{
	KPath oldPathBuffer(B_PATH_NAME_LENGTH + 1);
	KPath newPathBuffer(B_PATH_NAME_LENGTH + 1);
	if (oldPathBuffer.InitCheck() != B_OK || newPathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* oldPath = oldPathBuffer.LockBuffer();
	char* newPath = newPathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userOldPath) || !IS_USER_ADDRESS(userNewPath)
		|| user_strlcpy(oldPath, userOldPath, B_PATH_NAME_LENGTH) < B_OK
		|| user_strlcpy(newPath, userNewPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return common_rename(oldFD, oldPath, newFD, newPath, false);
}


status_t
_user_create_fifo(int fd, const char* userPath, mode_t perms)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK) {
		return B_BAD_ADDRESS;
	}

	// split into directory vnode and filename path
	char filename[B_FILE_NAME_LENGTH];
	struct vnode* dir;
	status_t status = fd_and_path_to_dir_vnode(fd, path, &dir, filename, false);
	if (status != B_OK)
		return status;

	VNodePutter _(dir);

	// the underlying FS needs to support creating FIFOs
	if (!HAS_FS_CALL(dir, create_special_node))
		return B_UNSUPPORTED;

	// create the entry	-- the FIFO sub node is set up automatically
	fs_vnode superVnode;
	ino_t nodeID;
	status = FS_CALL(dir, create_special_node, filename, NULL,
		S_IFIFO | (perms & S_IUMSK), 0, &superVnode, &nodeID);

	// create_special_node() acquired a reference for us that we don't need.
	if (status == B_OK)
		put_vnode(dir->mount->volume, nodeID);

	return status;
}


status_t
_user_create_pipe(int* userFDs)
{
	// rootfs should support creating FIFOs, but let's be sure
	if (!HAS_FS_CALL(sRoot, create_special_node))
		return B_UNSUPPORTED;

	// create the node	-- the FIFO sub node is set up automatically
	fs_vnode superVnode;
	ino_t nodeID;
	status_t status = FS_CALL(sRoot, create_special_node, NULL, NULL,
		S_IFIFO | S_IRUSR | S_IWUSR, 0, &superVnode, &nodeID);
	if (status != B_OK)
		return status;

	// We've got one reference to the node and need another one.
	struct vnode* vnode;
	status = get_vnode(sRoot->mount->id, nodeID, &vnode, true, false);
	if (status != B_OK) {
		// that should not happen
		dprintf("_user_create_pipe(): Failed to lookup vnode (%ld, %lld)\n",
			sRoot->mount->id, sRoot->id);
		return status;
	}

	// Everything looks good so far. Open two FDs for reading respectively
	// writing.
	int fds[2];
	fds[0] = open_vnode(vnode, O_RDONLY, false);
	fds[1] = open_vnode(vnode, O_WRONLY, false);

	FDCloser closer0(fds[0], false);
	FDCloser closer1(fds[1], false);

	status = (fds[0] >= 0 ? (fds[1] >= 0 ? B_OK : fds[1]) : fds[0]);

	// copy FDs to userland
	if (status == B_OK) {
		if (!IS_USER_ADDRESS(userFDs)
			|| user_memcpy(userFDs, fds, sizeof(fds)) != B_OK) {
			status = B_BAD_ADDRESS;
		}
	}

	// keep FDs, if everything went fine
	if (status == B_OK) {
		closer0.Detach();
		closer1.Detach();
	}

	return status;
}


status_t
_user_access(int fd, const char* userPath, int mode, bool effectiveUserGroup)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (!IS_USER_ADDRESS(userPath)
		|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return common_access(fd, path, mode, effectiveUserGroup, false);
}


status_t
_user_read_stat(int fd, const char* userPath, bool traverseLink,
	struct stat* userStat, size_t statSize)
{
	struct stat stat;
	status_t status;

	if (statSize > sizeof(struct stat))
		return B_BAD_VALUE;

	if (!IS_USER_ADDRESS(userStat))
		return B_BAD_ADDRESS;

	if (userPath) {
		// path given: get the stat of the node referred to by (fd, path)
		if (!IS_USER_ADDRESS(userPath))
			return B_BAD_ADDRESS;

		KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		char* path = pathBuffer.LockBuffer();

		ssize_t length = user_strlcpy(path, userPath, B_PATH_NAME_LENGTH);
		if (length < B_OK)
			return length;
		if (length >= B_PATH_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		status = common_path_read_stat(fd, path, traverseLink, &stat, false);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor* descriptor
			= get_fd(get_current_io_context(false), fd);
		if (descriptor == NULL)
			return B_FILE_ERROR;

		if (descriptor->ops->fd_read_stat)
			status = descriptor->ops->fd_read_stat(descriptor, &stat);
		else
			status = B_UNSUPPORTED;

		put_fd(descriptor);
	}

	if (status != B_OK)
		return status;

	return user_memcpy(userStat, &stat, statSize);
}


status_t
_user_write_stat(int fd, const char* userPath, bool traverseLeafLink,
	const struct stat* userStat, size_t statSize, int statMask)
{
	if (statSize > sizeof(struct stat))
		return B_BAD_VALUE;

	struct stat stat;

	if (!IS_USER_ADDRESS(userStat)
		|| user_memcpy(&stat, userStat, statSize) < B_OK)
		return B_BAD_ADDRESS;

	// clear additional stat fields
	if (statSize < sizeof(struct stat))
		memset((uint8*)&stat + statSize, 0, sizeof(struct stat) - statSize);

	status_t status;

	if (userPath) {
		// path given: write the stat of the node referred to by (fd, path)
		if (!IS_USER_ADDRESS(userPath))
			return B_BAD_ADDRESS;

		KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != B_OK)
			return B_NO_MEMORY;

		char* path = pathBuffer.LockBuffer();

		ssize_t length = user_strlcpy(path, userPath, B_PATH_NAME_LENGTH);
		if (length < B_OK)
			return length;
		if (length >= B_PATH_NAME_LENGTH)
			return B_NAME_TOO_LONG;

		status = common_path_write_stat(fd, path, traverseLeafLink, &stat,
			statMask, false);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor* descriptor
			= get_fd(get_current_io_context(false), fd);
		if (descriptor == NULL)
			return B_FILE_ERROR;

		if (descriptor->ops->fd_write_stat) {
			status = descriptor->ops->fd_write_stat(descriptor, &stat,
				statMask);
		} else
			status = B_UNSUPPORTED;

		put_fd(descriptor);
	}

	return status;
}


int
_user_open_attr_dir(int fd, const char* userPath, bool traverseLeafLink)
{
	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (userPath != NULL) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;
	}

	return attr_dir_open(fd, userPath ? path : NULL, traverseLeafLink, false);
}


ssize_t
_user_read_attr(int fd, const char* attribute, off_t pos, void* userBuffer,
	size_t readBytes)
{
	int attr = attr_open(fd, NULL, attribute, O_RDONLY, false);
	if (attr < 0)
		return attr;

	ssize_t bytes = _user_read(attr, pos, userBuffer, readBytes);
	_user_close(attr);

	return bytes;
}


ssize_t
_user_write_attr(int fd, const char* attribute, uint32 type, off_t pos,
	const void* buffer, size_t writeBytes)
{
	// Try to support the BeOS typical truncation as well as the position
	// argument
	int attr = attr_create(fd, NULL, attribute, type,
		O_CREAT | O_WRONLY | (pos != 0 ? 0 : O_TRUNC), false);
	if (attr < 0)
		return attr;

	ssize_t bytes = _user_write(attr, pos, buffer, writeBytes);
	_user_close(attr);

	return bytes;
}


status_t
_user_stat_attr(int fd, const char* attribute, struct attr_info* userAttrInfo)
{
	int attr = attr_open(fd, NULL, attribute, O_RDONLY, false);
	if (attr < 0)
		return attr;

	struct file_descriptor* descriptor
		= get_fd(get_current_io_context(false), attr);
	if (descriptor == NULL) {
		_user_close(attr);
		return B_FILE_ERROR;
	}

	struct stat stat;
	status_t status;
	if (descriptor->ops->fd_read_stat)
		status = descriptor->ops->fd_read_stat(descriptor, &stat);
	else
		status = B_UNSUPPORTED;

	put_fd(descriptor);
	_user_close(attr);

	if (status == B_OK) {
		attr_info info;
		info.type = stat.st_type;
		info.size = stat.st_size;

		if (user_memcpy(userAttrInfo, &info, sizeof(struct attr_info)) != B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


int
_user_open_attr(int fd, const char* userPath, const char* userName,
	uint32 type, int openMode)
{
	char name[B_FILE_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	KPath pathBuffer(B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (userPath != NULL) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;
	}

	if ((openMode & O_CREAT) != 0) {
		return attr_create(fd, userPath ? path : NULL, name, type, openMode,
			false);
	}

	return attr_open(fd, userPath ? path : NULL, name, openMode, false);
}


status_t
_user_remove_attr(int fd, const char* userName)
{
	char name[B_FILE_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_remove(fd, name, false);
}


status_t
_user_rename_attr(int fromFile, const char* userFromName, int toFile,
	const char* userToName)
{
	if (!IS_USER_ADDRESS(userFromName)
		|| !IS_USER_ADDRESS(userToName))
		return B_BAD_ADDRESS;

	KPath fromNameBuffer(B_FILE_NAME_LENGTH);
	KPath toNameBuffer(B_FILE_NAME_LENGTH);
	if (fromNameBuffer.InitCheck() != B_OK || toNameBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* fromName = fromNameBuffer.LockBuffer();
	char* toName = toNameBuffer.LockBuffer();

	if (user_strlcpy(fromName, userFromName, B_FILE_NAME_LENGTH) < B_OK
		|| user_strlcpy(toName, userToName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return attr_rename(fromFile, fromName, toFile, toName, false);
}


int
_user_open_index_dir(dev_t device)
{
	return index_dir_open(device, false);
}


status_t
_user_create_index(dev_t device, const char* userName, uint32 type,
	uint32 flags)
{
	char name[B_FILE_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return index_create(device, name, type, flags, false);
}


status_t
_user_read_index_stat(dev_t device, const char* userName, struct stat* userStat)
{
	char name[B_FILE_NAME_LENGTH];
	struct stat stat;
	status_t status;

	if (!IS_USER_ADDRESS(userName)
		|| !IS_USER_ADDRESS(userStat)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	status = index_name_read_stat(device, name, &stat, false);
	if (status == B_OK) {
		if (user_memcpy(userStat, &stat, sizeof(stat)) != B_OK)
			return B_BAD_ADDRESS;
	}

	return status;
}


status_t
_user_remove_index(dev_t device, const char* userName)
{
	char name[B_FILE_NAME_LENGTH];

	if (!IS_USER_ADDRESS(userName)
		|| user_strlcpy(name, userName, B_FILE_NAME_LENGTH) < B_OK)
		return B_BAD_ADDRESS;

	return index_remove(device, name, false);
}


status_t
_user_getcwd(char* userBuffer, size_t size)
{
	if (size == 0)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userBuffer))
		return B_BAD_ADDRESS;

	if (size > kMaxPathLength)
		size = kMaxPathLength;

	KPath pathBuffer(size);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	TRACE(("user_getcwd: buf %p, %ld\n", userBuffer, size));

	char* path = pathBuffer.LockBuffer();

	status_t status = get_cwd(path, size, false);
	if (status != B_OK)
		return status;

	// Copy back the result
	if (user_strlcpy(userBuffer, path, size) < B_OK)
		return B_BAD_ADDRESS;

	return status;
}


status_t
_user_setcwd(int fd, const char* userPath)
{
	TRACE(("user_setcwd: path = %p\n", userPath));

	KPath pathBuffer(B_PATH_NAME_LENGTH);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	char* path = pathBuffer.LockBuffer();

	if (userPath != NULL) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;
	}

	return set_cwd(fd, userPath != NULL ? path : NULL, false);
}


status_t
_user_change_root(const char* userPath)
{
	// only root is allowed to chroot()
	if (geteuid() != 0)
		return B_NOT_ALLOWED;

	// alloc path buffer
	KPath pathBuffer(B_PATH_NAME_LENGTH);
	if (pathBuffer.InitCheck() != B_OK)
		return B_NO_MEMORY;

	// copy userland path to kernel
	char* path = pathBuffer.LockBuffer();
	if (userPath != NULL) {
		if (!IS_USER_ADDRESS(userPath)
			|| user_strlcpy(path, userPath, B_PATH_NAME_LENGTH) < B_OK)
			return B_BAD_ADDRESS;
	}

	// get the vnode
	struct vnode* vnode;
	status_t status = path_to_vnode(path, true, &vnode, NULL, false);
	if (status != B_OK)
		return status;

	// set the new root
	struct io_context* context = get_current_io_context(false);
	mutex_lock(&sIOContextRootLock);
	struct vnode* oldRoot = context->root;
	context->root = vnode;
	mutex_unlock(&sIOContextRootLock);

	put_vnode(oldRoot);

	return B_OK;
}


int
_user_open_query(dev_t device, const char* userQuery, size_t queryLength,
	uint32 flags, port_id port, int32 token)
{
	char* query;

	if (device < 0 || userQuery == NULL || queryLength == 0)
		return B_BAD_VALUE;

	// this is a safety restriction
	if (queryLength >= 65536)
		return B_NAME_TOO_LONG;

	query = (char*)malloc(queryLength + 1);
	if (query == NULL)
		return B_NO_MEMORY;
	if (user_strlcpy(query, userQuery, queryLength + 1) < B_OK) {
		free(query);
		return B_BAD_ADDRESS;
	}

	int fd = query_open(device, query, flags, port, token, false);

	free(query);
	return fd;
}


#include "vfs_request_io.cpp"
