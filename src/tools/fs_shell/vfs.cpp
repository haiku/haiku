/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/* Virtual File System and File System Interface Layer */

#include "vfs.h"

#include <stdlib.h>

#include "fd.h"
#include "fssh_atomic.h"
#include "fssh_defs.h"
#include "fssh_dirent.h"
#include "fssh_errno.h"
#include "fssh_fcntl.h"
#include "fssh_fs_info.h"
#include "fssh_fs_volume.h"
#include "fssh_kernel_export.h"
#include "fssh_module.h"
#include "fssh_stat.h"
#include "fssh_stdio.h"
#include "fssh_string.h"
#include "fssh_unistd.h"
#include "hash.h"
#include "KPath.h"
#include "posix_compatibility.h"
#include "syscalls.h"


//#define TRACE_VFS
#ifdef TRACE_VFS
#	define TRACE(x) fssh_dprintf x
#	define FUNCTION(x) fssh_dprintf x
#else
#	define TRACE(x) ;
#	define FUNCTION(x) ;
#endif

#define ADD_DEBUGGER_COMMANDS

#define ASSERT_LOCKED_MUTEX(x)
#define ASSERT(x)

namespace FSShell {


const static uint32_t kMaxUnusedVnodes = 16;
	// This is the maximum number of unused vnodes that the system
	// will keep around (weak limit, if there is enough memory left,
	// they won't get flushed even when hitting that limit).
	// It may be chosen with respect to the available memory or enhanced
	// by some timestamp/frequency heurism.

struct vnode {
	struct vnode	*next;
	vm_cache_ref	*cache;
	fssh_mount_id	device;
	list_link		mount_link;
	list_link		unused_link;
	fssh_vnode_id	id;
	fssh_fs_vnode		private_node;
	struct fs_mount	*mount;
	struct vnode	*covered_by;
	int32_t			ref_count;
	uint8_t			remove : 1;
	uint8_t			busy : 1;
	uint8_t			unpublished : 1;
	struct file_descriptor *mandatory_locked_by;
};

struct vnode_hash_key {
	fssh_mount_id	device;
	fssh_vnode_id	vnode;
};

#define FS_CALL(vnode, op) (vnode->mount->fs->op)
#define FS_MOUNT_CALL(mount, op) (mount->fs->op)

/**	\brief Structure to manage a mounted file system

	Note: The root_vnode and covers_vnode fields (what others?) are
	initialized in fs_mount() and not changed afterwards. That is as soon
	as the mount is mounted and it is made sure it won't be unmounted
	(e.g. by holding a reference to a vnode of that mount) (read) access
	to those fields is always safe, even without additional locking. Morever
	while mounted the mount holds a reference to the covers_vnode, and thus
	making the access path vnode->mount->covers_vnode->mount->... safe if a
	reference to vnode is held (note that for the root mount covers_vnode
	is NULL, though).
 */
struct fs_mount {
	struct fs_mount	*next;
	fssh_file_system_module_info *fs;
	fssh_mount_id		id;
	void			*cookie;
	char			*device_name;
	char			*fs_name;
	recursive_lock	rlock;	// guards the vnodes list
	struct vnode	*root_vnode;
	struct vnode	*covers_vnode;
	struct list		vnodes;
	bool			unmounting;
	bool			owns_file_device;
};

static mutex sFileSystemsMutex;

/**	\brief Guards sMountsTable.
 *
 *	The holder is allowed to read/write access the sMountsTable.
 *	Manipulation of the fs_mount structures themselves
 *	(and their destruction) requires different locks though.
 */
static mutex sMountMutex;

/**	\brief Guards mount/unmount operations.
 *
 *	The fs_mount() and fs_unmount() hold the lock during their whole operation.
 *	That is locking the lock ensures that no FS is mounted/unmounted. In
 *	particular this means that
 *	- sMountsTable will not be modified,
 *	- the fields immutable after initialization of the fs_mount structures in
 *	  sMountsTable will not be modified,
 *	- vnode::covered_by of any vnode in sVnodeTable will not be modified.
 *	
 *	The thread trying to lock the lock must not hold sVnodeMutex or
 *	sMountMutex.
 */
static recursive_lock sMountOpLock;

/**	\brief Guards the vnode::covered_by field of any vnode
 *
 *	The holder is allowed to read access the vnode::covered_by field of any
 *	vnode. Additionally holding sMountOpLock allows for write access.
 *
 *	The thread trying to lock the must not hold sVnodeMutex.
 */
static mutex sVnodeCoveredByMutex;

/**	\brief Guards sVnodeTable.
 *
 *	The holder is allowed to read/write access sVnodeTable and to
 *	to any unbusy vnode in that table, save
 *	to the immutable fields (device, id, private_node, mount) to which
 *	only read-only access is allowed, and to the field covered_by, which is
 *	guarded by sMountOpLock and sVnodeCoveredByMutex.
 *
 *	The thread trying to lock the mutex must not hold sMountMutex.
 *	You must not have this mutex held when calling create_sem(), as this
 *	might call vfs_free_unused_vnodes().
 */
static mutex sVnodeMutex;

#define VNODE_HASH_TABLE_SIZE 1024
static hash_table *sVnodeTable;
static list sUnusedVnodeList;
static uint32_t sUnusedVnodes = 0;
static struct vnode *sRoot;

#define MOUNTS_HASH_TABLE_SIZE 16
static hash_table *sMountsTable;
static fssh_mount_id sNextMountID = 1;

fssh_mode_t __fssh_gUmask = 022;

/* function declarations */

// file descriptor operation prototypes
static fssh_status_t file_read(struct file_descriptor *, fssh_off_t pos,
			void *buffer, fssh_size_t *);
static fssh_status_t file_write(struct file_descriptor *, fssh_off_t pos,
			const void *buffer, fssh_size_t *);
static fssh_off_t file_seek(struct file_descriptor *, fssh_off_t pos,
			int seek_type);
static void file_free_fd(struct file_descriptor *);
static fssh_status_t file_close(struct file_descriptor *);
static fssh_status_t dir_read(struct file_descriptor *,
			struct fssh_dirent *buffer, fssh_size_t bufferSize,
			uint32_t *_count);
static fssh_status_t dir_read(struct vnode *vnode, fssh_fs_cookie cookie,
			struct fssh_dirent *buffer, fssh_size_t bufferSize,
			uint32_t *_count);
static fssh_status_t dir_rewind(struct file_descriptor *);
static void dir_free_fd(struct file_descriptor *);
static fssh_status_t dir_close(struct file_descriptor *);
static fssh_status_t attr_dir_read(struct file_descriptor *,
			struct fssh_dirent *buffer, fssh_size_t bufferSize,
			uint32_t *_count);
static fssh_status_t attr_dir_rewind(struct file_descriptor *);
static void attr_dir_free_fd(struct file_descriptor *);
static fssh_status_t attr_dir_close(struct file_descriptor *);
static fssh_status_t attr_read(struct file_descriptor *, fssh_off_t pos,
			void *buffer, fssh_size_t *);
static fssh_status_t attr_write(struct file_descriptor *, fssh_off_t pos,
			const void *buffer, fssh_size_t *);
static fssh_off_t attr_seek(struct file_descriptor *, fssh_off_t pos,
			int seek_type);
static void attr_free_fd(struct file_descriptor *);
static fssh_status_t attr_close(struct file_descriptor *);
static fssh_status_t attr_read_stat(struct file_descriptor *,
			struct fssh_stat *);
static fssh_status_t attr_write_stat(struct file_descriptor *,
			const struct fssh_stat *, int statMask);
static fssh_status_t index_dir_read(struct file_descriptor *,
			struct fssh_dirent *buffer, fssh_size_t bufferSize,
			uint32_t *_count);
static fssh_status_t index_dir_rewind(struct file_descriptor *);
static void index_dir_free_fd(struct file_descriptor *);
static fssh_status_t index_dir_close(struct file_descriptor *);
static fssh_status_t query_read(struct file_descriptor *,
			struct fssh_dirent *buffer, fssh_size_t bufferSize,
			uint32_t *_count);
static fssh_status_t query_rewind(struct file_descriptor *);
static void query_free_fd(struct file_descriptor *);
static fssh_status_t query_close(struct file_descriptor *);

static fssh_status_t common_ioctl(struct file_descriptor *, uint32_t, void *buf,
			fssh_size_t len);
static fssh_status_t common_read_stat(struct file_descriptor *,
			struct fssh_stat *);
static fssh_status_t common_write_stat(struct file_descriptor *,
			const struct fssh_stat *, int statMask);

static fssh_status_t vnode_path_to_vnode(struct vnode *vnode, char *path,
			bool traverseLeafLink, int count, struct vnode **_vnode,
			fssh_vnode_id *_parentID, int *_type);
static fssh_status_t dir_vnode_to_path(struct vnode *vnode, char *buffer,
			fssh_size_t bufferSize);
static fssh_status_t fd_and_path_to_vnode(int fd, char *path,
			bool traverseLeafLink, struct vnode **_vnode,
			fssh_vnode_id *_parentID, bool kernel);
static void inc_vnode_ref_count(struct vnode *vnode);
static fssh_status_t dec_vnode_ref_count(struct vnode *vnode, bool reenter);
static inline void put_vnode(struct vnode *vnode);

static struct fd_ops sFileOps = {
	file_read,
	file_write,
	file_seek,
	common_ioctl,
	NULL,
	NULL,
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
	VNodePutter(struct vnode *vnode = NULL) : fVNode(vnode) {}

	~VNodePutter()
	{
		Put();
	}

	void SetTo(struct vnode *vnode)
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

	struct vnode *Detach()
	{
		struct vnode *vnode = fVNode;
		fVNode = NULL;
		return vnode;
	}

private:
	struct vnode *fVNode;
};


static int
mount_compare(void *_m, const void *_key)
{
	struct fs_mount *mount = (fs_mount *)_m;
	const fssh_mount_id *id = (fssh_mount_id *)_key;

	if (mount->id == *id)
		return 0;

	return -1;
}


static uint32_t
mount_hash(void *_m, const void *_key, uint32_t range)
{
	struct fs_mount *mount = (fs_mount *)_m;
	const fssh_mount_id *id = (fssh_mount_id *)_key;

	if (mount)
		return mount->id % range;

	return (uint32_t)*id % range;
}


/** Finds the mounted device (the fs_mount structure) with the given ID.
 *	Note, you must hold the gMountMutex lock when you call this function.
 */

static struct fs_mount *
find_mount(fssh_mount_id id)
{
	ASSERT_LOCKED_MUTEX(&sMountMutex);

	return (fs_mount *)hash_lookup(sMountsTable, (void *)&id);
}


static fssh_status_t
get_mount(fssh_mount_id id, struct fs_mount **_mount)
{
	struct fs_mount *mount;
	fssh_status_t status;

	mutex_lock(&sMountMutex);

	mount = find_mount(id);
	if (mount) {
		// ToDo: the volume is locked (against removal) by locking
		//	its root node - investigate if that's a good idea
		if (mount->root_vnode)
			inc_vnode_ref_count(mount->root_vnode);
		else {
			// might have been called during a mount operation in which
			// case the root node may still be NULL
			mount = NULL;
		}
	} else
		status = FSSH_B_BAD_VALUE;

	mutex_unlock(&sMountMutex);

	if (mount == NULL)
		return FSSH_B_BUSY;

	*_mount = mount;
	return FSSH_B_OK;
}


static void
put_mount(struct fs_mount *mount)
{
	if (mount)
		put_vnode(mount->root_vnode);
}


static fssh_status_t
put_file_system(fssh_file_system_module_info *fs)
{
	return fssh_put_module(fs->info.name);
}


/**	Tries to open the specified file system module.
 *	Accepts a file system name of the form "bfs" or "file_systems/bfs/v1".
 *	Returns a pointer to file system module interface, or NULL if it
 *	could not open the module.
 */

static fssh_file_system_module_info *
get_file_system(const char *fsName)
{
	char name[FSSH_B_FILE_NAME_LENGTH];
	if (fssh_strncmp(fsName, "file_systems/", fssh_strlen("file_systems/"))) {
		// construct module name if we didn't get one
		// (we currently support only one API)
		fssh_snprintf(name, sizeof(name), "file_systems/%s/v1", fsName);
		fsName = NULL;
	}

	fssh_file_system_module_info *info;
	if (fssh_get_module(fsName ? fsName : name, (fssh_module_info **)&info) != FSSH_B_OK)
		return NULL;

	return info;
}


/**	Accepts a file system name of the form "bfs" or "file_systems/bfs/v1"
 *	and returns a compatible fs_info.fsh_name name ("bfs" in both cases).
 *	The name is allocated for you, and you have to free() it when you're
 *	done with it.
 *	Returns NULL if the required memory is no available.
 */

static char *
get_file_system_name(const char *fsName)
{
	const fssh_size_t length = fssh_strlen("file_systems/");

	if (fssh_strncmp(fsName, "file_systems/", length)) {
		// the name already seems to be the module's file name
		return fssh_strdup(fsName);
	}

	fsName += length;
	const char *end = fssh_strchr(fsName, '/');
	if (end == NULL) {
		// this doesn't seem to be a valid name, but well...
		return fssh_strdup(fsName);
	}

	// cut off the trailing /v1

	char *name = (char *)malloc(end + 1 - fsName);
	if (name == NULL)
		return NULL;

	fssh_strlcpy(name, fsName, end + 1 - fsName);
	return name;
}


static int
vnode_compare(void *_vnode, const void *_key)
{
	struct vnode *vnode = (struct vnode *)_vnode;
	const struct vnode_hash_key *key = (vnode_hash_key *)_key;

	if (vnode->device == key->device && vnode->id == key->vnode)
		return 0;

	return -1;
}


static uint32_t
vnode_hash(void *_vnode, const void *_key, uint32_t range)
{
	struct vnode *vnode = (struct vnode *)_vnode;
	const struct vnode_hash_key *key = (vnode_hash_key *)_key;

#define VHASH(mountid, vnodeid) (((uint32_t)((vnodeid) >> 32) + (uint32_t)(vnodeid)) ^ (uint32_t)(mountid))

	if (vnode != NULL)
		return VHASH(vnode->device, vnode->id) % range;

	return VHASH(key->device, key->vnode) % range;

#undef VHASH
}


static void
add_vnode_to_mount_list(struct vnode *vnode, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	list_add_link_to_head(&mount->vnodes, &vnode->mount_link);

	recursive_lock_unlock(&mount->rlock);
}


static void
remove_vnode_from_mount_list(struct vnode *vnode, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	list_remove_link(&vnode->mount_link);
	vnode->mount_link.next = vnode->mount_link.prev = NULL;

	recursive_lock_unlock(&mount->rlock);
}


static fssh_status_t
create_new_vnode(struct vnode **_vnode, fssh_mount_id mountID, fssh_vnode_id vnodeID)
{
	FUNCTION(("create_new_vnode()\n"));

	struct vnode *vnode = (struct vnode *)malloc(sizeof(struct vnode));
	if (vnode == NULL)
		return FSSH_B_NO_MEMORY;

	// initialize basic values
	fssh_memset(vnode, 0, sizeof(struct vnode));
	vnode->device = mountID;
	vnode->id = vnodeID;

	// add the vnode to the mount structure
	mutex_lock(&sMountMutex);	
	vnode->mount = find_mount(mountID);
	if (!vnode->mount || vnode->mount->unmounting) {
		mutex_unlock(&sMountMutex);
		free(vnode);
		return FSSH_B_ENTRY_NOT_FOUND;
	}

	hash_insert(sVnodeTable, vnode);
	add_vnode_to_mount_list(vnode, vnode->mount);

	mutex_unlock(&sMountMutex);

	vnode->ref_count = 1;
	*_vnode = vnode;

	return FSSH_B_OK;
}


/**	Frees the vnode and all resources it has acquired, and removes
 *	it from the vnode hash as well as from its mount structure.
 *	Will also make sure that any cache modifications are written back.
 */

static void
free_vnode(struct vnode *vnode, bool reenter)
{
	ASSERT(vnode->ref_count == 0 && vnode->busy);

	// write back any changes in this vnode's cache -- but only
	// if the vnode won't be deleted, in which case the changes
	// will be discarded

	if (!vnode->remove && FS_CALL(vnode, fsync) != NULL)
		FS_CALL(vnode, fsync)(vnode->mount->cookie, vnode->private_node);

	if (!vnode->unpublished) {
		if (vnode->remove)
			FS_CALL(vnode, remove_vnode)(vnode->mount->cookie, vnode->private_node, reenter);
		else
			FS_CALL(vnode, put_vnode)(vnode->mount->cookie, vnode->private_node, reenter);
	}

	// The file system has removed the resources of the vnode now, so we can
	// make it available again (and remove the busy vnode from the hash)
	mutex_lock(&sVnodeMutex);
	hash_remove(sVnodeTable, vnode);
	mutex_unlock(&sVnodeMutex);

	remove_vnode_from_mount_list(vnode, vnode->mount);

	free(vnode);
}


/**	\brief Decrements the reference counter of the given vnode and deletes it,
 *	if the counter dropped to 0.
 *
 *	The caller must, of course, own a reference to the vnode to call this
 *	function.
 *	The caller must not hold the sVnodeMutex or the sMountMutex.
 *
 *	\param vnode the vnode.
 *	\param reenter \c true, if this function is called (indirectly) from within
 *		   a file system.
 *	\return \c FSSH_B_OK, if everything went fine, an error code otherwise.
 */

static fssh_status_t
dec_vnode_ref_count(struct vnode *vnode, bool reenter)
{
	mutex_lock(&sVnodeMutex);

	int32_t oldRefCount = fssh_atomic_add(&vnode->ref_count, -1);

	TRACE(("dec_vnode_ref_count: vnode %p, ref now %ld\n", vnode, vnode->ref_count));

	if (oldRefCount == 1) {
		if (vnode->busy)
			fssh_panic("dec_vnode_ref_count: called on busy vnode %p\n", vnode);

		bool freeNode = false;

		// Just insert the vnode into an unused list if we don't need
		// to delete it
		if (vnode->remove) {
			vnode->busy = true;
			freeNode = true;
		} else {
			list_add_item(&sUnusedVnodeList, vnode);
			if (++sUnusedVnodes > kMaxUnusedVnodes) {
				// there are too many unused vnodes so we free the oldest one
				// ToDo: evaluate this mechanism
				vnode = (struct vnode *)list_remove_head_item(&sUnusedVnodeList);
				vnode->busy = true;
				freeNode = true;
				sUnusedVnodes--;
			}
		}

		mutex_unlock(&sVnodeMutex);

		if (freeNode)
			free_vnode(vnode, reenter);
	} else
		mutex_unlock(&sVnodeMutex);

	return FSSH_B_OK;
}


/**	\brief Increments the reference counter of the given vnode.
 *
 *	The caller must either already have a reference to the vnode or hold
 *	the sVnodeMutex.
 *
 *	\param vnode the vnode.
 */

static void
inc_vnode_ref_count(struct vnode *vnode)
{
	fssh_atomic_add(&vnode->ref_count, 1);
	TRACE(("inc_vnode_ref_count: vnode %p, ref now %ld\n", vnode, vnode->ref_count));
}


/**	\brief Looks up a vnode by mount and node ID in the sVnodeTable.
 *
 *	The caller must hold the sVnodeMutex.
 *
 *	\param mountID the mount ID.
 *	\param vnodeID the node ID.
 *
 *	\return The vnode structure, if it was found in the hash table, \c NULL
 *			otherwise.
 */

static struct vnode *
lookup_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID)
{
	struct vnode_hash_key key;

	key.device = mountID;
	key.vnode = vnodeID;

	return (vnode *)hash_lookup(sVnodeTable, &key);
}


/**	\brief Retrieves a vnode for a given mount ID, node ID pair.
 *
 *	If the node is not yet in memory, it will be loaded.
 *
 *	The caller must not hold the sVnodeMutex or the sMountMutex.
 *
 *	\param mountID the mount ID.
 *	\param vnodeID the node ID.
 *	\param _vnode Pointer to a vnode* variable into which the pointer to the
 *		   retrieved vnode structure shall be written.
 *	\param reenter \c true, if this function is called (indirectly) from within
 *		   a file system.
 *	\return \c FSSH_B_OK, if everything when fine, an error code otherwise.
 */

static fssh_status_t
get_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID, struct vnode **_vnode, int reenter)
{
	FUNCTION(("get_vnode: mountid %ld vnid 0x%Lx %p\n", mountID, vnodeID, _vnode));

	mutex_lock(&sVnodeMutex);

	int32_t tries = 300;
		// try for 3 secs
restart:
	struct vnode *vnode = lookup_vnode(mountID, vnodeID);
	if (vnode && vnode->busy) {
		mutex_unlock(&sVnodeMutex);
		if (--tries < 0) {
			// vnode doesn't seem to become unbusy
			fssh_panic("vnode %d:%lld is not becoming unbusy!\n", (int)mountID, vnodeID);
			return FSSH_B_BUSY;
		}
		fssh_snooze(10000); // 10 ms
		mutex_lock(&sVnodeMutex);
		goto restart;
	}

	TRACE(("get_vnode: tried to lookup vnode, got %p\n", vnode));

	fssh_status_t status;

	if (vnode) {
		if (vnode->ref_count == 0) {
			// this vnode has been unused before
			list_remove_item(&sUnusedVnodeList, vnode);
			sUnusedVnodes--;
		}
		inc_vnode_ref_count(vnode);
	} else {
		// we need to create a new vnode and read it in
		status = create_new_vnode(&vnode, mountID, vnodeID);
		if (status < FSSH_B_OK)
			goto err;

		vnode->busy = true;
		mutex_unlock(&sVnodeMutex);

		status = FS_CALL(vnode, get_vnode)(vnode->mount->cookie, vnodeID, &vnode->private_node, reenter);
		if (status == FSSH_B_OK && vnode->private_node == NULL)
			status = FSSH_B_BAD_VALUE;

		mutex_lock(&sVnodeMutex);

		if (status < FSSH_B_OK)
			goto err1;

		vnode->busy = false;
	}

	mutex_unlock(&sVnodeMutex);

	TRACE(("get_vnode: returning %p\n", vnode));

	*_vnode = vnode;
	return FSSH_B_OK;

err1:
	hash_remove(sVnodeTable, vnode);
	remove_vnode_from_mount_list(vnode, vnode->mount);
err:
	mutex_unlock(&sVnodeMutex);
	if (vnode)
		free(vnode);

	return status;
}


/**	\brief Decrements the reference counter of the given vnode and deletes it,
 *	if the counter dropped to 0.
 *
 *	The caller must, of course, own a reference to the vnode to call this
 *	function.
 *	The caller must not hold the sVnodeMutex or the sMountMutex.
 * 
 *	\param vnode the vnode.
 */

static inline void
put_vnode(struct vnode *vnode)
{
	dec_vnode_ref_count(vnode, false);
}


/**	Disconnects all file descriptors that are associated with the 
 *	\a vnodeToDisconnect, or if this is NULL, all vnodes of the specified
 *	\a mount object.
 *
 *	Note, after you've called this function, there might still be ongoing
 *	accesses - they won't be interrupted if they already happened before.
 *	However, any subsequent access will fail.
 *
 *	This is not a cheap function and should be used with care and rarely.
 *	TODO: there is currently no means to stop a blocking read/write!
 */

void
disconnect_mount_or_vnode_fds(struct fs_mount *mount,
	struct vnode *vnodeToDisconnect)
{
}


/**	\brief Resolves a mount point vnode to the volume root vnode it is covered
 *		   by.
 *
 *	Given an arbitrary vnode, the function checks, whether the node is covered
 *	by the root of a volume. If it is the function obtains a reference to the
 *	volume root node and returns it.
 *
 *	\param vnode The vnode in question.
 *	\return The volume root vnode the vnode cover is covered by, if it is
 *			indeed a mount point, or \c NULL otherwise.
 */

static struct vnode *
resolve_mount_point_to_volume_root(struct vnode *vnode)
{
	if (!vnode)
		return NULL;

	struct vnode *volumeRoot = NULL;

	mutex_lock(&sVnodeCoveredByMutex);
	if (vnode->covered_by) {
		volumeRoot = vnode->covered_by;
		inc_vnode_ref_count(volumeRoot);
	}
	mutex_unlock(&sVnodeCoveredByMutex);

	return volumeRoot;
}


/**	\brief Resolves a mount point vnode to the volume root vnode it is covered
 *		   by.
 *
 *	Given an arbitrary vnode (identified by mount and node ID), the function
 *	checks, whether the node is covered by the root of a volume. If it is the
 *	function returns the mount and node ID of the volume root node. Otherwise
 *	it simply returns the supplied mount and node ID.
 *
 *	In case of error (e.g. the supplied node could not be found) the variables
 *	for storing the resolved mount and node ID remain untouched and an error
 *	code is returned.
 *
 *	\param mountID The mount ID of the vnode in question.
 *	\param nodeID The node ID of the vnode in question.
 *	\param resolvedMountID Pointer to storage for the resolved mount ID.
 *	\param resolvedNodeID Pointer to storage for the resolved node ID.
 *	\return
 *	- \c FSSH_B_OK, if everything went fine,
 *	- another error code, if something went wrong.
 */

fssh_status_t
resolve_mount_point_to_volume_root(fssh_mount_id mountID, fssh_vnode_id nodeID,
	fssh_mount_id *resolvedMountID, fssh_vnode_id *resolvedNodeID)
{
	// get the node
	struct vnode *node;
	fssh_status_t error = get_vnode(mountID, nodeID, &node, false);
	if (error != FSSH_B_OK)
		return error;

	// resolve the node
	struct vnode *resolvedNode = resolve_mount_point_to_volume_root(node);
	if (resolvedNode) {
		put_vnode(node);
		node = resolvedNode;
	}

	// set the return values
	*resolvedMountID = node->device;
	*resolvedNodeID = node->id;

	put_vnode(node);

	return FSSH_B_OK;
}


/**	\brief Resolves a volume root vnode to the underlying mount point vnode.
 *
 *	Given an arbitrary vnode, the function checks, whether the node is the
 *	root of a volume. If it is (and if it is not "/"), the function obtains
 *	a reference to the underlying mount point node and returns it.
 *
 *	\param vnode The vnode in question (caller must have a reference).
 *	\return The mount point vnode the vnode covers, if it is indeed a volume
 *			root and not "/", or \c NULL otherwise.
 */

static struct vnode *
resolve_volume_root_to_mount_point(struct vnode *vnode)
{
	if (!vnode)
		return NULL;

	struct vnode *mountPoint = NULL;

	struct fs_mount *mount = vnode->mount;
	if (vnode == mount->root_vnode && mount->covers_vnode) {
		mountPoint = mount->covers_vnode;
		inc_vnode_ref_count(mountPoint);
	}

	return mountPoint;
}


/**	\brief Gets the directory path and leaf name for a given path.
 *
 *	The supplied \a path is transformed to refer to the directory part of
 *	the entry identified by the original path, and into the buffer \a filename
 *	the leaf name of the original entry is written.
 *	Neither the returned path nor the leaf name can be expected to be
 *	canonical.
 *
 *	\param path The path to be analyzed. Must be able to store at least one
 *		   additional character.
 *	\param filename The buffer into which the leaf name will be written.
 *		   Must be of size FSSH_B_FILE_NAME_LENGTH at least.
 *	\return \c FSSH_B_OK, if everything went fine, \c FSSH_B_NAME_TOO_LONG, if the leaf
 *		   name is longer than \c FSSH_B_FILE_NAME_LENGTH.
 */

static fssh_status_t
get_dir_path_and_leaf(char *path, char *filename)
{
	char *p = fssh_strrchr(path, '/');
		// '/' are not allowed in file names!

	FUNCTION(("get_dir_path_and_leaf(path = %s)\n", path));

	if (!p) {
		// this path is single segment with no '/' in it
		// ex. "foo"
		if (fssh_strlcpy(filename, path, FSSH_B_FILE_NAME_LENGTH) >= FSSH_B_FILE_NAME_LENGTH)
			return FSSH_B_NAME_TOO_LONG;
		fssh_strcpy(path, ".");
	} else {
		p++;
		if (*p == '\0') {
			// special case: the path ends in '/'
			fssh_strcpy(filename, ".");
		} else {
			// normal leaf: replace the leaf portion of the path with a '.'
			if (fssh_strlcpy(filename, p, FSSH_B_FILE_NAME_LENGTH)
				>= FSSH_B_FILE_NAME_LENGTH) {
				return FSSH_B_NAME_TOO_LONG;
			}
		}
		p[0] = '.';
		p[1] = '\0';
	}
	return FSSH_B_OK;
}


static fssh_status_t
entry_ref_to_vnode(fssh_mount_id mountID, fssh_vnode_id directoryID, const char *name, struct vnode **_vnode)
{
	char clonedName[FSSH_B_FILE_NAME_LENGTH + 1];
	if (fssh_strlcpy(clonedName, name, FSSH_B_FILE_NAME_LENGTH) >= FSSH_B_FILE_NAME_LENGTH)
		return FSSH_B_NAME_TOO_LONG;

	// get the directory vnode and let vnode_path_to_vnode() do the rest
	struct vnode *directory;

	fssh_status_t status = get_vnode(mountID, directoryID, &directory, false);
	if (status < 0)
		return status;

	return vnode_path_to_vnode(directory, clonedName, false, 0, _vnode, NULL, NULL);
}


/**	Returns the vnode for the relative path starting at the specified \a vnode.
 *	\a path must not be NULL.
 *	If it returns successfully, \a path contains the name of the last path
 *	component.
 *	Note, this reduces the ref_count of the starting \a vnode, no matter if
 *	it is successful or not!
 */

static fssh_status_t
vnode_path_to_vnode(struct vnode *vnode, char *path, bool traverseLeafLink,
	int count, struct vnode **_vnode, fssh_vnode_id *_parentID, int *_type)
{
	fssh_status_t status = 0;
	fssh_vnode_id lastParentID = vnode->id;
	int type = 0;

	FUNCTION(("vnode_path_to_vnode(vnode = %p, path = %s)\n", vnode, path));

	if (path == NULL) {
		put_vnode(vnode);
		return FSSH_B_BAD_VALUE;
	}

	while (true) {
		struct vnode *nextVnode;
		fssh_vnode_id vnodeID;
		char *nextPath;

		TRACE(("vnode_path_to_vnode: top of loop. p = %p, p = '%s'\n", path, path));

		// done?
		if (path[0] == '\0')
			break;

		// walk to find the next path component ("path" will point to a single
		// path component), and filter out multiple slashes
		for (nextPath = path + 1; *nextPath != '\0' && *nextPath != '/'; nextPath++);

		if (*nextPath == '/') {
			*nextPath = '\0';
			do
				nextPath++;
			while (*nextPath == '/');
		}

		// See if the '..' is at the root of a mount and move to the covered
		// vnode so we pass the '..' path to the underlying filesystem
		if (!fssh_strcmp("..", path)
			&& vnode->mount->root_vnode == vnode
			&& vnode->mount->covers_vnode) {
			nextVnode = vnode->mount->covers_vnode;
			inc_vnode_ref_count(nextVnode);
			put_vnode(vnode);
			vnode = nextVnode;
		}

		// Check if we have the right to search the current directory vnode.
		// If a file system doesn't have the access() function, we assume that
		// searching a directory is always allowed
		if (FS_CALL(vnode, access))
			status = FS_CALL(vnode, access)(vnode->mount->cookie, vnode->private_node, FSSH_X_OK);

		// Tell the filesystem to get the vnode of this path component (if we got the
		// permission from the call above)
		if (status >= FSSH_B_OK)
			status = FS_CALL(vnode, lookup)(vnode->mount->cookie, vnode->private_node, path, &vnodeID, &type);

		if (status < FSSH_B_OK) {
			put_vnode(vnode);
			return status;
		}

		// Lookup the vnode, the call to fs_lookup should have caused a get_vnode to be called
		// from inside the filesystem, thus the vnode would have to be in the list and it's
		// ref count incremented at this point
		mutex_lock(&sVnodeMutex);
		nextVnode = lookup_vnode(vnode->device, vnodeID);
		mutex_unlock(&sVnodeMutex);

		if (!nextVnode) {
			// pretty screwed up here - the file system found the vnode, but the hash
			// lookup failed, so our internal structures are messed up
			fssh_panic("vnode_path_to_vnode: could not lookup vnode (mountid 0x%x vnid 0x%llx)\n",
				(int)vnode->device, vnodeID);
			put_vnode(vnode);
			return FSSH_B_ENTRY_NOT_FOUND;
		}

		// If the new node is a symbolic link, resolve it (if we've been told to do it)
		if (FSSH_S_ISLNK(type) && !(!traverseLeafLink && nextPath[0] == '\0')) {
			fssh_size_t bufferSize;
			char *buffer;

			TRACE(("traverse link\n"));

			// it's not exactly nice style using goto in this way, but hey, it works :-/
			if (count + 1 > FSSH_B_MAX_SYMLINKS) {
				status = FSSH_B_LINK_LIMIT;
				goto resolve_link_error;
			}

			buffer = (char *)malloc(bufferSize = FSSH_B_PATH_NAME_LENGTH);
			if (buffer == NULL) {
				status = FSSH_B_NO_MEMORY;
				goto resolve_link_error;
			}

			if (FS_CALL(nextVnode, read_symlink) != NULL) {
				status = FS_CALL(nextVnode, read_symlink)(
					nextVnode->mount->cookie, nextVnode->private_node, buffer,
					&bufferSize);
			} else
				status = FSSH_B_BAD_VALUE;

			if (status < FSSH_B_OK) {
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
			if (path[0] == '/') {
				// we don't need the old directory anymore
				put_vnode(vnode);

				while (*++path == '/')
					;
				vnode = sRoot;
				inc_vnode_ref_count(vnode);
			}
			inc_vnode_ref_count(vnode);
				// balance the next recursion - we will decrement the ref_count
				// of the vnode, no matter if we succeeded or not

			status = vnode_path_to_vnode(vnode, path, traverseLeafLink, count + 1,
				&nextVnode, &lastParentID, _type);

			free(buffer);

			if (status < FSSH_B_OK) {
				put_vnode(vnode);
				return status;
			}
		} else
			lastParentID = vnode->id;

		// decrease the ref count on the old dir we just looked up into
		put_vnode(vnode);

		path = nextPath;
		vnode = nextVnode;

		// see if we hit a mount point
		struct vnode *mountPoint = resolve_mount_point_to_volume_root(vnode);
		if (mountPoint) {
			put_vnode(vnode);
			vnode = mountPoint;
		}
	}

	*_vnode = vnode;
	if (_type)
		*_type = type;
	if (_parentID)
		*_parentID = lastParentID;

	return FSSH_B_OK;
}


static fssh_status_t
path_to_vnode(char *path, bool traverseLink, struct vnode **_vnode,
	fssh_vnode_id *_parentID, bool kernel)
{
	struct vnode *start = NULL;

	FUNCTION(("path_to_vnode(path = \"%s\")\n", path));

	if (!path)
		return FSSH_B_BAD_VALUE;

	// figure out if we need to start at root or at cwd
	if (*path == '/') {
		if (sRoot == NULL) {
			// we're a bit early, aren't we?
			return FSSH_B_ERROR;
		}

		while (*++path == '/')
			;
		start = sRoot;
		inc_vnode_ref_count(start);
	} else {
		struct io_context *context = get_current_io_context(kernel);

		mutex_lock(&context->io_mutex);
		start = context->cwd;
		if (start != NULL)
			inc_vnode_ref_count(start);
		mutex_unlock(&context->io_mutex);

		if (start == NULL)
			return FSSH_B_ERROR;
	}

	return vnode_path_to_vnode(start, path, traverseLink, 0, _vnode, _parentID, NULL);
}


/** Returns the vnode in the next to last segment of the path, and returns
 *	the last portion in filename.
 *	The path buffer must be able to store at least one additional character.
 */

static fssh_status_t
path_to_dir_vnode(char *path, struct vnode **_vnode, char *filename, bool kernel)
{
	fssh_status_t status = get_dir_path_and_leaf(path, filename);
	if (status != FSSH_B_OK)
		return status;

	return path_to_vnode(path, true, _vnode, NULL, kernel);
}


/**	\brief Retrieves the directory vnode and the leaf name of an entry referred
 *		   to by a FD + path pair.
 *
 *	\a path must be given in either case. \a fd might be omitted, in which
 *	case \a path is either an absolute path or one relative to the current
 *	directory. If both a supplied and \a path is relative it is reckoned off
 *	of the directory referred to by \a fd. If \a path is absolute \a fd is
 *	ignored.
 *
 *	The caller has the responsibility to call put_vnode() on the returned
 *	directory vnode.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. Must not be \c NULL. The buffer
 *	       is modified by this function. It must have at least room for a
 *	       string one character longer than the path it contains.
 *	\param _vnode A pointer to a variable the directory vnode shall be written
 *		   into.
 *	\param filename A buffer of size FSSH_B_FILE_NAME_LENGTH or larger into which
 *		   the leaf name of the specified entry will be written.
 *	\param kernel \c true, if invoked from inside the kernel, \c false if
 *		   invoked from userland.
 *	\return \c FSSH_B_OK, if everything went fine, another error code otherwise.
 */

static fssh_status_t
fd_and_path_to_dir_vnode(int fd, char *path, struct vnode **_vnode,
	char *filename, bool kernel)
{
	if (!path)
		return FSSH_B_BAD_VALUE;
	if (fd < 0)
		return path_to_dir_vnode(path, _vnode, filename, kernel);

	fssh_status_t status = get_dir_path_and_leaf(path, filename);
	if (status != FSSH_B_OK)
		return status;

	return fd_and_path_to_vnode(fd, path, true, _vnode, NULL, kernel);
}


/** Returns a vnode's name in the d_name field of a supplied dirent buffer.
 */

static fssh_status_t
get_vnode_name(struct vnode *vnode, struct vnode *parent,
	struct fssh_dirent *buffer, fssh_size_t bufferSize)
{
	if (bufferSize < sizeof(struct fssh_dirent))
		return FSSH_B_BAD_VALUE;

	// See if vnode is the root of a mount and move to the covered
	// vnode so we get the underlying file system
	VNodePutter vnodePutter;
	if (vnode->mount->root_vnode == vnode && vnode->mount->covers_vnode != NULL) {
		vnode = vnode->mount->covers_vnode;
		inc_vnode_ref_count(vnode);
		vnodePutter.SetTo(vnode);
	}

	if (FS_CALL(vnode, get_vnode_name)) {
		// The FS supports getting the name of a vnode.
		return FS_CALL(vnode, get_vnode_name)(vnode->mount->cookie,
			vnode->private_node, buffer->d_name,
			(char*)buffer + bufferSize - buffer->d_name);
	}

	// The FS doesn't support getting the name of a vnode. So we search the
	// parent directory for the vnode, if the caller let us.

	if (parent == NULL)
		return FSSH_EOPNOTSUPP;

	fssh_fs_cookie cookie;

	fssh_status_t status = FS_CALL(parent, open_dir)(parent->mount->cookie,
		parent->private_node, &cookie);
	if (status >= FSSH_B_OK) {
		while (true) {
			uint32_t num = 1;
			status = dir_read(parent, cookie, buffer, bufferSize, &num);
			if (status < FSSH_B_OK)
				break;
			if (num == 0) {
				status = FSSH_B_ENTRY_NOT_FOUND;
				break;
			}

			if (vnode->id == buffer->d_ino) {
				// found correct entry!
				break;
			}
		}

		FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node,
			cookie);
		FS_CALL(vnode, free_dir_cookie)(vnode->mount->cookie,
			vnode->private_node, cookie);
	}
	return status;
}


static fssh_status_t
get_vnode_name(struct vnode *vnode, struct vnode *parent, char *name,
	fssh_size_t nameSize)
{
	char buffer[sizeof(struct fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
	struct fssh_dirent *dirent = (struct fssh_dirent *)buffer;

	fssh_status_t status = get_vnode_name(vnode, parent, buffer, sizeof(buffer));
	if (status != FSSH_B_OK)
		return status;

	if (fssh_strlcpy(name, dirent->d_name, nameSize) >= nameSize)
		return FSSH_B_BUFFER_OVERFLOW;

	return FSSH_B_OK;
}


/**	Gets the full path to a given directory vnode.
 *	It uses the fs_get_vnode_name() call to get the name of a vnode; if a
 *	file system doesn't support this call, it will fall back to iterating
 *	through the parent directory to get the name of the child.
 *
 *	To protect against circular loops, it supports a maximum tree depth
 *	of 256 levels.
 *
 *	Note that the path may not be correct the time this function returns!
 *	It doesn't use any locking to prevent returning the correct path, as
 *	paths aren't safe anyway: the path to a file can change at any time.
 *
 *	It might be a good idea, though, to check if the returned path exists
 *	in the calling function (it's not done here because of efficiency)
 */

static fssh_status_t
dir_vnode_to_path(struct vnode *vnode, char *buffer, fssh_size_t bufferSize)
{
	FUNCTION(("dir_vnode_to_path(%p, %p, %lu)\n", vnode, buffer, bufferSize));

	if (vnode == NULL || buffer == NULL)
		return FSSH_B_BAD_VALUE;

	/* this implementation is currently bound to FSSH_B_PATH_NAME_LENGTH */
	KPath pathBuffer;
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	char *path = pathBuffer.LockBuffer();
	int32_t insert = pathBuffer.BufferSize();
	int32_t maxLevel = 256;
	int32_t length;
	fssh_status_t status;

	// we don't use get_vnode() here because this call is more
	// efficient and does all we need from get_vnode()
	inc_vnode_ref_count(vnode);

	// resolve a volume root to its mount point
	struct vnode *mountPoint = resolve_volume_root_to_mount_point(vnode);
	if (mountPoint) {
		put_vnode(vnode);
		vnode = mountPoint;
	}

	path[--insert] = '\0';

	while (true) {
		// the name buffer is also used for fs_read_dir()
		char nameBuffer[sizeof(struct fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
		char *name = &((struct fssh_dirent *)nameBuffer)->d_name[0];
		struct vnode *parentVnode;
		fssh_vnode_id parentID;
		int type;

		// lookup the parent vnode
		status = FS_CALL(vnode, lookup)(vnode->mount->cookie, vnode->private_node, "..",
			&parentID, &type);
		if (status < FSSH_B_OK)
			goto out;

		mutex_lock(&sVnodeMutex);
		parentVnode = lookup_vnode(vnode->device, parentID);
		mutex_unlock(&sVnodeMutex);

		if (parentVnode == NULL) {
			fssh_panic("dir_vnode_to_path: could not lookup vnode (mountid 0x%x vnid 0x%llx)\n",
				(int)vnode->device, parentID);
			status = FSSH_B_ENTRY_NOT_FOUND;
			goto out;
		}

		// get the node's name
		status = get_vnode_name(vnode, parentVnode,
			(struct fssh_dirent*)nameBuffer, sizeof(nameBuffer));

		// resolve a volume root to its mount point
		mountPoint = resolve_volume_root_to_mount_point(parentVnode);
		if (mountPoint) {
			put_vnode(parentVnode);
			parentVnode = mountPoint;
			parentID = parentVnode->id;
		}

		bool hitRoot = (parentVnode == vnode);

		// release the current vnode, we only need its parent from now on
		put_vnode(vnode);
		vnode = parentVnode;

		if (status < FSSH_B_OK)
			goto out;

		if (hitRoot) {
			// we have reached "/", which means we have constructed the full
			// path
			break;
		}

		// ToDo: add an explicit check for loops in about 10 levels to do
		// real loop detection

		// don't go deeper as 'maxLevel' to prevent circular loops
		if (maxLevel-- < 0) {
			status = FSSH_ELOOP;
			goto out;
		}

		// add the name in front of the current path
		name[FSSH_B_FILE_NAME_LENGTH - 1] = '\0';
		length = fssh_strlen(name);
		insert -= length;
		if (insert <= 0) {
			status = FSSH_ENOBUFS;
			goto out;
		}
		fssh_memcpy(path + insert, name, length);
		path[--insert] = '/';
	}

	// the root dir will result in an empty path: fix it
	if (path[insert] == '\0')
		path[--insert] = '/';

	TRACE(("  path is: %s\n", path + insert));

	// copy the path to the output buffer
	length = pathBuffer.BufferSize() - insert;
	if (length <= (int)bufferSize)
		fssh_memcpy(buffer, path + insert, length);
	else
		status = FSSH_ENOBUFS;

out:
	put_vnode(vnode);
	return status;
}


/**	Checks the length of every path component, and adds a '.'
 *	if the path ends in a slash.
 *	The given path buffer must be able to store at least one
 *	additional character.
 */

static fssh_status_t
check_path(char *to)
{
	int32_t length = 0;

	// check length of every path component

	while (*to) {
		char *begin;
		if (*to == '/')
			to++, length++;

		begin = to;
		while (*to != '/' && *to)
			to++, length++;

		if (to - begin > FSSH_B_FILE_NAME_LENGTH)
			return FSSH_B_NAME_TOO_LONG;
	}

	if (length == 0)
		return FSSH_B_ENTRY_NOT_FOUND;

	// complete path if there is a slash at the end

	if (*(to - 1) == '/') {
		if (length > FSSH_B_PATH_NAME_LENGTH - 2)
			return FSSH_B_NAME_TOO_LONG;

		to[0] = '.';
		to[1] = '\0';
	}

	return FSSH_B_OK;
}


static struct file_descriptor *
get_fd_and_vnode(int fd, struct vnode **_vnode, bool kernel)
{
	struct file_descriptor *descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	if (fd_vnode(descriptor) == NULL) {
		put_fd(descriptor);
		return NULL;
	}

	// ToDo: when we can close a file descriptor at any point, investigate
	//	if this is still valid to do (accessing the vnode without ref_count
	//	or locking)
	*_vnode = descriptor->u.vnode;
	return descriptor;
}


static struct vnode *
get_vnode_from_fd(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return NULL;

	vnode = fd_vnode(descriptor);
	if (vnode != NULL)
		inc_vnode_ref_count(vnode);

	put_fd(descriptor);
	return vnode;
}


/**	Gets the vnode from an FD + path combination. If \a fd is lower than zero,
 *	only the path will be considered. In this case, the \a path must not be
 *	NULL.
 *	If \a fd is a valid file descriptor, \a path may be NULL for directories,
 *	and should be NULL for files.
 */

static fssh_status_t
fd_and_path_to_vnode(int fd, char *path, bool traverseLeafLink,
	struct vnode **_vnode, fssh_vnode_id *_parentID, bool kernel)
{
	if (fd < 0 && !path)
		return FSSH_B_BAD_VALUE;

	if (fd < 0 || (path != NULL && path[0] == '/')) {
		// no FD or absolute path
		return path_to_vnode(path, traverseLeafLink, _vnode, _parentID, kernel);
	}

	// FD only, or FD + relative path
	struct vnode *vnode = get_vnode_from_fd(fd, kernel);
	if (!vnode)
		return FSSH_B_FILE_ERROR;

	if (path != NULL) {
		return vnode_path_to_vnode(vnode, path, traverseLeafLink, 0,
			_vnode, _parentID, NULL);
	}

	// there is no relative path to take into account

	*_vnode = vnode;
	if (_parentID)
		*_parentID = -1;

	return FSSH_B_OK;
}


static int
get_new_fd(int type, struct fs_mount *mount, struct vnode *vnode,
	fssh_fs_cookie cookie, int openMode, bool kernel)
{
	struct file_descriptor *descriptor;
	int fd;

	// if the vnode is locked, we don't allow creating a new file descriptor for it
	if (vnode && vnode->mandatory_locked_by != NULL)
		return FSSH_B_BUSY;

	descriptor = alloc_fd();
	if (!descriptor)
		return FSSH_B_NO_MEMORY;

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
			fssh_panic("get_new_fd() called with unknown type %d\n", type);
			break;
	}
	descriptor->type = type;
	descriptor->open_mode = openMode;

	fd = new_fd(get_current_io_context(kernel), descriptor);
	if (fd < 0) {
		free(descriptor);
		return FSSH_B_NO_MORE_FDS;
	}

	return fd;
}


//	#pragma mark - public VFS API


extern "C" fssh_status_t
fssh_new_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_fs_vnode privateNode)
{
	FUNCTION(("new_vnode(mountID = %ld, vnodeID = %Ld, node = %p)\n",
		mountID, vnodeID, privateNode));

	if (privateNode == NULL)
		return FSSH_B_BAD_VALUE;

	mutex_lock(&sVnodeMutex);

	// file system integrity check:
	// test if the vnode already exists and bail out if this is the case!

	// ToDo: the R5 implementation obviously checks for a different cookie
	//	and doesn't panic if they are equal

	struct vnode *vnode = lookup_vnode(mountID, vnodeID);
	if (vnode != NULL)
		fssh_panic("vnode %d:%lld already exists (node = %p, vnode->node = %p)!", (int)mountID, vnodeID, privateNode, vnode->private_node);

	fssh_status_t status = create_new_vnode(&vnode, mountID, vnodeID);
	if (status == FSSH_B_OK) {
		vnode->private_node = privateNode;
		vnode->busy = true;
		vnode->unpublished = true;
	}

	TRACE(("returns: %s\n", strerror(status)));

	mutex_unlock(&sVnodeMutex);
	return status;
}


extern "C" fssh_status_t
fssh_publish_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_fs_vnode privateNode)
{
	FUNCTION(("publish_vnode()\n"));

	mutex_lock(&sVnodeMutex);

	struct vnode *vnode = lookup_vnode(mountID, vnodeID);
	fssh_status_t status = FSSH_B_OK;

	if (vnode != NULL && vnode->busy && vnode->unpublished
		&& vnode->private_node == privateNode) {
		vnode->busy = false;
		vnode->unpublished = false;
	} else if (vnode == NULL && privateNode != NULL) {
		status = create_new_vnode(&vnode, mountID, vnodeID);
		if (status == FSSH_B_OK)
			vnode->private_node = privateNode;
	} else
		status = FSSH_B_BAD_VALUE;

	TRACE(("returns: %s\n", strerror(status)));

	mutex_unlock(&sVnodeMutex);
	return status;
}


extern "C" fssh_status_t
fssh_get_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	fssh_fs_vnode *_fsNode)
{
	struct vnode *vnode;

	fssh_status_t status = get_vnode(mountID, vnodeID, &vnode, true);
	if (status < FSSH_B_OK)
		return status;

	*_fsNode = vnode->private_node;
	return FSSH_B_OK;
}


extern "C" fssh_status_t
fssh_put_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID)
{
	struct vnode *vnode;

	mutex_lock(&sVnodeMutex);
	vnode = lookup_vnode(mountID, vnodeID);
	mutex_unlock(&sVnodeMutex);

	if (vnode)
		dec_vnode_ref_count(vnode, true);

	return FSSH_B_OK;
}


extern "C" fssh_status_t
fssh_remove_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID)
{
	struct vnode *vnode;
	bool remove = false;

	mutex_lock(&sVnodeMutex);

	vnode = lookup_vnode(mountID, vnodeID);
	if (vnode != NULL) {
		if (vnode->covered_by != NULL) {
			// this vnode is in use
			mutex_unlock(&sVnodeMutex);
			return FSSH_B_BUSY;
		}

		vnode->remove = true;
		if (vnode->unpublished) {
			// prepare the vnode for deletion
			vnode->busy = true;
			remove = true;
		}
	}

	mutex_unlock(&sVnodeMutex);
	
	if (remove) {
		// if the vnode hasn't been published yet, we delete it here
		fssh_atomic_add(&vnode->ref_count, -1);
		free_vnode(vnode, true);
	}

	return FSSH_B_OK;
}


extern "C" fssh_status_t 
fssh_unremove_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID)
{
	struct vnode *vnode;

	mutex_lock(&sVnodeMutex);

	vnode = lookup_vnode(mountID, vnodeID);
	if (vnode)
		vnode->remove = false;

	mutex_unlock(&sVnodeMutex);
	return FSSH_B_OK;
}


extern "C" fssh_status_t 
fssh_get_vnode_removed(fssh_mount_id mountID, fssh_vnode_id vnodeID,
	bool* removed)
{
	mutex_lock(&sVnodeMutex);

	fssh_status_t result;

	if (struct vnode* vnode = lookup_vnode(mountID, vnodeID)) {
		if (removed)
			*removed = vnode->remove;
		result = FSSH_B_OK;
	} else
		result = FSSH_B_BAD_VALUE;

	mutex_unlock(&sVnodeMutex);
	return result;
}


//	#pragma mark - private VFS API
//	Functions the VFS exports for other parts of the kernel


/** Acquires another reference to the vnode that has to be released
 *	by calling vfs_put_vnode().
 */

void
vfs_acquire_vnode(void *_vnode)
{
	inc_vnode_ref_count((struct vnode *)_vnode);
}


/** This is currently called from file_cache_create() only.
 *	It's probably a temporary solution as long as devfs requires that
 *	fs_read_pages()/fs_write_pages() are called with the standard
 *	open cookie and not with a device cookie.
 *	If that's done differently, remove this call; it has no other
 *	purpose.
 */

fssh_status_t
vfs_get_cookie_from_fd(int fd, void **_cookie)
{
	struct file_descriptor *descriptor;

	descriptor = get_fd(get_current_io_context(true), fd);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	*_cookie = descriptor->cookie;
	return FSSH_B_OK;
}


int
vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode)
{
	*vnode = get_vnode_from_fd(fd, kernel);

	if (*vnode == NULL)
		return FSSH_B_FILE_ERROR;

	return FSSH_B_NO_ERROR;
}


fssh_status_t
vfs_get_vnode_from_path(const char *path, bool kernel, void **_vnode)
{
	TRACE(("vfs_get_vnode_from_path: entry. path = '%s', kernel %d\n", path, kernel));

	KPath pathBuffer(FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	char *buffer = pathBuffer.LockBuffer();
	fssh_strlcpy(buffer, path, pathBuffer.BufferSize());

	struct vnode *vnode;
	fssh_status_t status = path_to_vnode(buffer, true, &vnode, NULL, kernel);
	if (status < FSSH_B_OK)
		return status;

	*_vnode = vnode;
	return FSSH_B_OK;
}


fssh_status_t
vfs_get_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID, void **_vnode)
{
	struct vnode *vnode;

	fssh_status_t status = get_vnode(mountID, vnodeID, &vnode, false);
	if (status < FSSH_B_OK)
		return status;

	*_vnode = vnode;
	return FSSH_B_OK;
}


fssh_status_t
vfs_entry_ref_to_vnode(fssh_mount_id mountID, fssh_vnode_id directoryID,
	const char *name, void **_vnode)
{
	return entry_ref_to_vnode(mountID, directoryID, name, (struct vnode **)_vnode);
}


void
vfssh_fs_vnode_to_node_ref(void *_vnode, fssh_mount_id *_mountID, fssh_vnode_id *_vnodeID)
{
	struct vnode *vnode = (struct vnode *)_vnode;

	*_mountID = vnode->device;
	*_vnodeID = vnode->id;
}


/**	Looks up a vnode with the given mount and vnode ID.
 *	Must only be used with "in-use" vnodes as it doesn't grab a reference
 *	to the node.
 *	It's currently only be used by file_cache_create().
 */

fssh_status_t
vfs_lookup_vnode(fssh_mount_id mountID, fssh_vnode_id vnodeID, void **_vnode)
{
	mutex_lock(&sVnodeMutex);
	struct vnode *vnode = lookup_vnode(mountID, vnodeID);
	mutex_unlock(&sVnodeMutex);

	if (vnode == NULL)
		return FSSH_B_ERROR;

	*_vnode = vnode;
	return FSSH_B_OK;
}


fssh_status_t
vfs_get_fs_node_from_path(fssh_mount_id mountID, const char *path, bool kernel, void **_node)
{
	TRACE(("vfs_get_fs_node_from_path(mountID = %ld, path = \"%s\", kernel %d)\n",
		mountID, path, kernel));

	KPath pathBuffer(FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	fs_mount *mount;
	fssh_status_t status = get_mount(mountID, &mount);
	if (status < FSSH_B_OK)
		return status;

	char *buffer = pathBuffer.LockBuffer();
	fssh_strlcpy(buffer, path, pathBuffer.BufferSize());

	struct vnode *vnode = mount->root_vnode;

	if (buffer[0] == '/')
		status = path_to_vnode(buffer, true, &vnode, NULL, true);
	else {
		inc_vnode_ref_count(vnode);
			// vnode_path_to_vnode() releases a reference to the starting vnode
		status = vnode_path_to_vnode(vnode, buffer, true, 0, &vnode, NULL, NULL);
	}

	put_mount(mount);

	if (status < FSSH_B_OK)
		return status;

	if (vnode->device != mountID) {
		// wrong mount ID - must not gain access on foreign file system nodes
		put_vnode(vnode);
		return FSSH_B_BAD_VALUE;
	}

	*_node = vnode->private_node;
	return FSSH_B_OK;
}


/**	Finds the full path to the file that contains the module \a moduleName,
 *	puts it into \a pathBuffer, and returns FSSH_B_OK for success.
 *	If \a pathBuffer was too small, it returns \c FSSH_B_BUFFER_OVERFLOW,
 *	\c FSSH_B_ENTRY_NOT_FOUNT if no file could be found.
 *	\a pathBuffer is clobbered in any case and must not be relied on if this
 *	functions returns unsuccessfully.
 */

fssh_status_t
vfs_get_module_path(const char *basePath, const char *moduleName, char *pathBuffer,
	fssh_size_t bufferSize)
{
	struct vnode *dir, *file;
	fssh_status_t status;
	fssh_size_t length;
	char *path;

	if (bufferSize == 0 || fssh_strlcpy(pathBuffer, basePath, bufferSize) >= bufferSize)
		return FSSH_B_BUFFER_OVERFLOW;

	status = path_to_vnode(pathBuffer, true, &dir, NULL, true);
	if (status < FSSH_B_OK)
		return status;

	// the path buffer had been clobbered by the above call
	length = fssh_strlcpy(pathBuffer, basePath, bufferSize);
	if (pathBuffer[length - 1] != '/')
		pathBuffer[length++] = '/';

	path = pathBuffer + length;
	bufferSize -= length;

	while (moduleName) {
		int type;

		char *nextPath = fssh_strchr(moduleName, '/');
		if (nextPath == NULL)
			length = fssh_strlen(moduleName);
		else {
			length = nextPath - moduleName;
			nextPath++;
		}

		if (length + 1 >= bufferSize) {
			status = FSSH_B_BUFFER_OVERFLOW;
			goto err;
		}

		fssh_memcpy(path, moduleName, length);
		path[length] = '\0';
		moduleName = nextPath;

		status = vnode_path_to_vnode(dir, path, true, 0, &file, NULL, &type);
		if (status < FSSH_B_OK) {
			// vnode_path_to_vnode() has already released the reference to dir
			return status;
		}

		if (FSSH_S_ISDIR(type)) {
			// goto the next directory
			path[length] = '/';
			path[length + 1] = '\0';
			path += length + 1;
			bufferSize -= length + 1;

			dir = file;
		} else if (FSSH_S_ISREG(type)) {
			// it's a file so it should be what we've searched for
			put_vnode(file);

			return FSSH_B_OK;
		} else {
			TRACE(("vfs_get_module_path(): something is strange here: %d...\n", type));
			status = FSSH_B_ERROR;
			dir = file;
			goto err;
		}
	}

	// if we got here, the moduleName just pointed to a directory, not to
	// a real module - what should we do in this case?
	status = FSSH_B_ENTRY_NOT_FOUND;

err:
	put_vnode(dir);
	return status;
}


/**	\brief Normalizes a given path.
 *
 *	The path must refer to an existing or non-existing entry in an existing
 *	directory, that is chopping off the leaf component the remaining path must
 *	refer to an existing directory.
 *
 *	The returned will be canonical in that it will be absolute, will not
 *	contain any "." or ".." components or duplicate occurrences of '/'s,
 *	and none of the directory components will by symbolic links.
 *
 *	Any two paths referring to the same entry, will result in the same
 *	normalized path (well, that is pretty much the definition of `normalized',
 *	isn't it :-).
 *
 *	\param path The path to be normalized.
 *	\param buffer The buffer into which the normalized path will be written.
 *	\param bufferSize The size of \a buffer.
 *	\param kernel \c true, if the IO context of the kernel shall be used,
 *		   otherwise that of the team this thread belongs to. Only relevant,
 *		   if the path is relative (to get the CWD).
 *	\return \c FSSH_B_OK if everything went fine, another error code otherwise.
 */

fssh_status_t
vfs_normalize_path(const char *path, char *buffer, fssh_size_t bufferSize,
	bool kernel)
{
	if (!path || !buffer || bufferSize < 1)
		return FSSH_B_BAD_VALUE;

	TRACE(("vfs_normalize_path(`%s')\n", path));

	// copy the supplied path to the stack, so it can be modified
	KPath mutablePathBuffer(FSSH_B_PATH_NAME_LENGTH + 1);
	if (mutablePathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	char *mutablePath = mutablePathBuffer.LockBuffer();
	if (fssh_strlcpy(mutablePath, path, FSSH_B_PATH_NAME_LENGTH) >= FSSH_B_PATH_NAME_LENGTH)
		return FSSH_B_NAME_TOO_LONG;

	// get the dir vnode and the leaf name
	struct vnode *dirNode;
	char leaf[FSSH_B_FILE_NAME_LENGTH];
	fssh_status_t error = path_to_dir_vnode(mutablePath, &dirNode, leaf, kernel);
	if (error != FSSH_B_OK) {
		TRACE(("vfs_normalize_path(): failed to get dir vnode: %s\n", strerror(error)));
		return error;
	}

	// if the leaf is "." or "..", we directly get the correct directory
	// vnode and ignore the leaf later
	bool isDir = (fssh_strcmp(leaf, ".") == 0 || fssh_strcmp(leaf, "..") == 0);
	if (isDir)
		error = vnode_path_to_vnode(dirNode, leaf, false, 0, &dirNode, NULL, NULL);
	if (error != FSSH_B_OK) {
		TRACE(("vfs_normalize_path(): failed to get dir vnode for \".\" or \"..\": %s\n",
			strerror(error)));
		return error;
	}

	// get the directory path
	error = dir_vnode_to_path(dirNode, buffer, bufferSize);
	put_vnode(dirNode);
	if (error < FSSH_B_OK) {
		TRACE(("vfs_normalize_path(): failed to get dir path: %s\n", strerror(error)));
		return error;
	}

	// append the leaf name
	if (!isDir) {
		// insert a directory separator only if this is not the file system root
		if ((fssh_strcmp(buffer, "/") != 0
			 && fssh_strlcat(buffer, "/", bufferSize) >= bufferSize)
			|| fssh_strlcat(buffer, leaf, bufferSize) >= bufferSize) {
			return FSSH_B_NAME_TOO_LONG;
		}
	}

	TRACE(("vfs_normalize_path() -> `%s'\n", buffer));
	return FSSH_B_OK;
}


void
vfs_put_vnode(void *_vnode)
{
	put_vnode((struct vnode *)_vnode);
}


fssh_status_t
vfs_get_cwd(fssh_mount_id *_mountID, fssh_vnode_id *_vnodeID)
{
	// Get current working directory from io context
	struct io_context *context = get_current_io_context(false);
	fssh_status_t status = FSSH_B_OK;

	mutex_lock(&context->io_mutex);

	if (context->cwd != NULL) {
		*_mountID = context->cwd->device;
		*_vnodeID = context->cwd->id;
	} else
		status = FSSH_B_ERROR;

	mutex_unlock(&context->io_mutex);
	return status;
}


fssh_status_t
vfs_get_file_map(void *_vnode, fssh_off_t offset, fssh_size_t size,
	fssh_file_io_vec *vecs, fssh_size_t *_count)
{
	struct vnode *vnode = (struct vnode *)_vnode;

	FUNCTION(("vfs_get_file_map: vnode %p, vecs %p, offset %lld, size = %u\n", vnode, vecs, offset, (unsigned)size));

	return FS_CALL(vnode, get_file_map)(vnode->mount->cookie, vnode->private_node, offset, size, vecs, _count);
}


fssh_status_t
vfs_stat_vnode(void *_vnode, struct fssh_stat *stat)
{
	struct vnode *vnode = (struct vnode *)_vnode;

	fssh_status_t status = FS_CALL(vnode, read_stat)(vnode->mount->cookie,
		vnode->private_node, stat);

	// fill in the st_dev and st_ino fields
	if (status == FSSH_B_OK) {
		stat->fssh_st_dev = vnode->device;
		stat->fssh_st_ino = vnode->id;
	}

	return status;
}


fssh_status_t
vfs_get_vnode_name(void *_vnode, char *name, fssh_size_t nameSize)
{
	return get_vnode_name((struct vnode *)_vnode, NULL, name, nameSize);
}


/**	If the given descriptor locked its vnode, that lock will be released.
 */

void
vfs_unlock_vnode_if_locked(struct file_descriptor *descriptor)
{
	struct vnode *vnode = fd_vnode(descriptor);

	if (vnode != NULL && vnode->mandatory_locked_by == descriptor)
		vnode->mandatory_locked_by = NULL;
}


/**	Closes all file descriptors of the specified I/O context that
 *	don't have the FSSH_O_CLOEXEC flag set.
 */

void
vfs_exec_io_context(void *_context)
{
	struct io_context *context = (struct io_context *)_context;
	uint32_t i;

	for (i = 0; i < context->table_size; i++) {
		mutex_lock(&context->io_mutex);

		struct file_descriptor *descriptor = context->fds[i];
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


/** Sets up a new io_control structure, and inherits the properties
 *	of the parent io_control if it is given.
 */

void *
vfs_new_io_context(void *_parentContext)
{
	fssh_size_t tableSize;
	struct io_context *context;
	struct io_context *parentContext;

	context = (io_context *)malloc(sizeof(struct io_context));
	if (context == NULL)
		return NULL;

	fssh_memset(context, 0, sizeof(struct io_context));

	parentContext = (struct io_context *)_parentContext;
	if (parentContext)
		tableSize = parentContext->table_size;
	else
		tableSize = DEFAULT_FD_TABLE_SIZE;

	// allocate space for FDs and their close-on-exec flag
	context->fds = (file_descriptor **)malloc(sizeof(struct file_descriptor *) * tableSize
		+ (tableSize + 7) / 8);
	if (context->fds == NULL) {
		free(context);
		return NULL;
	}

	fssh_memset(context->fds, 0, sizeof(struct file_descriptor *) * tableSize
		+ (tableSize + 7) / 8);
	context->fds_close_on_exec = (uint8_t *)(context->fds + tableSize);

	if (mutex_init(&context->io_mutex, "I/O context") < 0) {
		free(context->fds);
		free(context);
		return NULL;
	}

	// Copy all parent files which don't have the FSSH_O_CLOEXEC flag set

	if (parentContext) {
		fssh_size_t i;

		mutex_lock(&parentContext->io_mutex);

		context->cwd = parentContext->cwd;
		if (context->cwd)
			inc_vnode_ref_count(context->cwd);

		for (i = 0; i < tableSize; i++) {
			struct file_descriptor *descriptor = parentContext->fds[i];

			if (descriptor != NULL && !fd_close_on_exec(parentContext, i)) {
				context->fds[i] = descriptor;
				context->num_used_fds++;
				fssh_atomic_add(&descriptor->ref_count, 1);
				fssh_atomic_add(&descriptor->open_count, 1);
			}
		}

		mutex_unlock(&parentContext->io_mutex);
	} else {
		context->cwd = sRoot;

		if (context->cwd)
			inc_vnode_ref_count(context->cwd);
	}

	context->table_size = tableSize;

	return context;
}


fssh_status_t
vfs_free_io_context(void *_ioContext)
{
	struct io_context *context = (struct io_context *)_ioContext;
	uint32_t i;

	if (context->cwd)
		dec_vnode_ref_count(context->cwd, false);

	mutex_lock(&context->io_mutex);

	for (i = 0; i < context->table_size; i++) {
		if (struct file_descriptor *descriptor = context->fds[i]) {
			close_fd(descriptor);
			put_fd(descriptor);
		}
	}

	mutex_destroy(&context->io_mutex);

	free(context->fds);
	free(context);

	return FSSH_B_OK;
}


fssh_status_t
vfs_init(kernel_args *args)
{
	sVnodeTable = hash_init(VNODE_HASH_TABLE_SIZE, fssh_offsetof(struct vnode, next),
		&vnode_compare, &vnode_hash);
	if (sVnodeTable == NULL)
		fssh_panic("vfs_init: error creating vnode hash table\n");

	list_init_etc(&sUnusedVnodeList, fssh_offsetof(struct vnode, unused_link));

	sMountsTable = hash_init(MOUNTS_HASH_TABLE_SIZE, fssh_offsetof(struct fs_mount, next),
		&mount_compare, &mount_hash);
	if (sMountsTable == NULL)
		fssh_panic("vfs_init: error creating mounts hash table\n");

	sRoot = NULL;

	if (mutex_init(&sFileSystemsMutex, "vfs_lock") < 0)
		fssh_panic("vfs_init: error allocating file systems lock\n");

	if (recursive_lock_init(&sMountOpLock, "vfs_mount_op_lock") < 0)
		fssh_panic("vfs_init: error allocating mount op lock\n");

	if (mutex_init(&sMountMutex, "vfs_mount_lock") < 0)
		fssh_panic("vfs_init: error allocating mount lock\n");

	if (mutex_init(&sVnodeCoveredByMutex, "vfs_vnode_covered_by_lock") < 0)
		fssh_panic("vfs_init: error allocating vnode::covered_by lock\n");

	if (mutex_init(&sVnodeMutex, "vfs_vnode_lock") < 0)
		fssh_panic("vfs_init: error allocating vnode lock\n");

	if (block_cache_init() != FSSH_B_OK)
		return FSSH_B_ERROR;

	return file_cache_init();
}


//	#pragma mark -
//	The filetype-dependent implementations (fd_ops + open/create/rename/remove, ...)


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
create_vnode(struct vnode *directory, const char *name, int openMode, int perms, bool kernel)
{
	struct vnode *vnode;
	fssh_fs_cookie cookie;
	fssh_vnode_id newID;
	int status;

	if (FS_CALL(directory, create) == NULL)
		return FSSH_EROFS;

	status = FS_CALL(directory, create)(directory->mount->cookie, directory->private_node, name, openMode, perms, &cookie, &newID);
	if (status < FSSH_B_OK)
		return status;

	mutex_lock(&sVnodeMutex);
	vnode = lookup_vnode(directory->device, newID);
	mutex_unlock(&sVnodeMutex);

	if (vnode == NULL) {
		fssh_dprintf("vfs: fs_create() returned success but there is no vnode!");
		return FSSH_EINVAL;
	}

	if ((status = get_new_fd(FDTYPE_FILE, NULL, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	// something went wrong, clean up

	FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	put_vnode(vnode);

	FS_CALL(directory, unlink)(directory->mount->cookie, directory->private_node, name);
	
	return status;
}


/** Calls fs_open() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_vnode(struct vnode *vnode, int openMode, bool kernel)
{
	fssh_fs_cookie cookie;
	int status;

	status = FS_CALL(vnode, open)(vnode->mount->cookie, vnode->private_node, openMode, &cookie);
	if (status < 0)
		return status;

	status = get_new_fd(FDTYPE_FILE, NULL, vnode, cookie, openMode, kernel);
	if (status < 0) {
		FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, cookie);
		FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, cookie);
	}
	return status;
}


/** Calls fs open_dir() on the given vnode and returns a new
 *	file descriptor for it
 */

static int
open_dir_vnode(struct vnode *vnode, bool kernel)
{
	fssh_fs_cookie cookie;
	int status;

	status = FS_CALL(vnode, open_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
	if (status < FSSH_B_OK)
		return status;

	// file is opened, create a fd
	status = get_new_fd(FDTYPE_DIR, NULL, vnode, cookie, 0, kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_dir_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	return status;
}


/** Calls fs open_attr_dir() on the given vnode and returns a new
 *	file descriptor for it.
 *	Used by attr_dir_open(), and attr_dir_open_fd().
 */

static int
open_attr_dir_vnode(struct vnode *vnode, bool kernel)
{
	fssh_fs_cookie cookie;
	int status;

	if (FS_CALL(vnode, open_attr_dir) == NULL)
		return FSSH_EOPNOTSUPP;

	status = FS_CALL(vnode, open_attr_dir)(vnode->mount->cookie, vnode->private_node, &cookie);
	if (status < 0)
		return status;

	// file is opened, create a fd
	status = get_new_fd(FDTYPE_ATTR_DIR, NULL, vnode, cookie, 0, kernel);
	if (status >= 0)
		return status;

	FS_CALL(vnode, close_attr_dir)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_dir_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	return status;
}


static int
file_create_entry_ref(fssh_mount_id mountID, fssh_vnode_id directoryID, const char *name, int openMode, int perms, bool kernel)
{
	struct vnode *directory;
	int status;

	FUNCTION(("file_create_entry_ref: name = '%s', omode %x, perms %d, kernel %d\n", name, openMode, perms, kernel));

	// get directory to put the new file in	
	status = get_vnode(mountID, directoryID, &directory, false);
	if (status < FSSH_B_OK)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);
	put_vnode(directory);

	return status;
}


static int
file_create(int fd, char *path, int openMode, int perms, bool kernel)
{
	char name[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *directory;
	int status;

	FUNCTION(("file_create: path '%s', omode %x, perms %d, kernel %d\n", path, openMode, perms, kernel));

	// get directory to put the new file in	
	status = fd_and_path_to_dir_vnode(fd, path, &directory, name, kernel);
	if (status < 0)
		return status;

	status = create_vnode(directory, name, openMode, perms, kernel);

	put_vnode(directory);
	return status;
}


static int
file_open_entry_ref(fssh_mount_id mountID, fssh_vnode_id directoryID, const char *name, int openMode, bool kernel)
{
	struct vnode *vnode;
	int status;

	if (name == NULL || *name == '\0')
		return FSSH_B_BAD_VALUE;

	FUNCTION(("file_open_entry_ref(ref = (%ld, %Ld, %s), openMode = %d)\n",
		mountID, directoryID, name, openMode));

	// get the vnode matching the entry_ref
	status = entry_ref_to_vnode(mountID, directoryID, name, &vnode);
	if (status < FSSH_B_OK)
		return status;

	status = open_vnode(vnode, openMode, kernel);
	if (status < FSSH_B_OK)
		put_vnode(vnode);

	return status;
}


static int
file_open(int fd, char *path, int openMode, bool kernel)
{
	int status = FSSH_B_OK;
	bool traverse = ((openMode & FSSH_O_NOTRAVERSE) == 0);

	FUNCTION(("file_open: fd: %d, entry path = '%s', omode %d, kernel %d\n",
		fd, path, openMode, kernel));

	// get the vnode matching the vnode + path combination
	struct vnode *vnode = NULL;
	fssh_vnode_id parentID;
	status = fd_and_path_to_vnode(fd, path, traverse, &vnode, &parentID, kernel);
	if (status != FSSH_B_OK)
		return status;

	// open the vnode
	status = open_vnode(vnode, openMode, kernel);
	// put only on error -- otherwise our reference was transferred to the FD
	if (status < FSSH_B_OK)
		put_vnode(vnode);

	return status;
}


static fssh_status_t
file_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;
	fssh_status_t status = FSSH_B_OK;

	FUNCTION(("file_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close))
		status = FS_CALL(vnode, close)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return status;
}


static void
file_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static fssh_status_t
file_read(struct file_descriptor *descriptor, fssh_off_t pos, void *buffer, fssh_size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("file_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length, *length));
	return FS_CALL(vnode, read)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static fssh_status_t
file_write(struct file_descriptor *descriptor, fssh_off_t pos, const void *buffer, fssh_size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("file_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	return FS_CALL(vnode, write)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static fssh_off_t
file_seek(struct file_descriptor *descriptor, fssh_off_t pos, int seekType)
{
	fssh_off_t offset;

	FUNCTION(("file_seek(pos = %Ld, seekType = %d)\n", pos, seekType));
	// ToDo: seek should fail for pipes and FIFOs...

	switch (seekType) {
		case FSSH_SEEK_SET:
			offset = 0;
			break;
		case FSSH_SEEK_CUR:
			offset = descriptor->pos;
			break;
		case FSSH_SEEK_END:
		{
			struct vnode *vnode = descriptor->u.vnode;
			struct fssh_stat stat;
			fssh_status_t status;

			if (FS_CALL(vnode, read_stat) == NULL)
				return FSSH_EOPNOTSUPP;

			status = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, &stat);
			if (status < FSSH_B_OK)
				return status;

			offset = stat.fssh_st_size;
			break;
		}
		default:
			return FSSH_B_BAD_VALUE;
	}

	// assumes fssh_off_t is 64 bits wide
	if (offset > 0 && LLONG_MAX - offset < pos)
		return FSSH_EOVERFLOW;

	pos += offset;
	if (pos < 0)
		return FSSH_B_BAD_VALUE;

	return descriptor->pos = pos;
}


static fssh_status_t
dir_create_entry_ref(fssh_mount_id mountID, fssh_vnode_id parentID, const char *name, int perms, bool kernel)
{
	struct vnode *vnode;
	fssh_vnode_id newID;
	fssh_status_t status;

	if (name == NULL || *name == '\0')
		return FSSH_B_BAD_VALUE;

	FUNCTION(("dir_create_entry_ref(dev = %ld, ino = %Ld, name = '%s', perms = %d)\n", mountID, parentID, name, perms));
	
	status = get_vnode(mountID, parentID, &vnode, kernel);
	if (status < FSSH_B_OK)
		return status;

	if (FS_CALL(vnode, create_dir))
		status = FS_CALL(vnode, create_dir)(vnode->mount->cookie, vnode->private_node, name, perms, &newID);
	else
		status = FSSH_EROFS;

	put_vnode(vnode);
	return status;
}


static fssh_status_t
dir_create(int fd, char *path, int perms, bool kernel)
{
	char filename[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *vnode;
	fssh_vnode_id newID;
	fssh_status_t status;

	FUNCTION(("dir_create: path '%s', perms %d, kernel %d\n", path, perms, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, create_dir))
		status = FS_CALL(vnode, create_dir)(vnode->mount->cookie, vnode->private_node, filename, perms, &newID);
	else
		status = FSSH_EROFS;

	put_vnode(vnode);
	return status;
}


static int
dir_open_entry_ref(fssh_mount_id mountID, fssh_vnode_id parentID, const char *name, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("dir_open_entry_ref()\n"));

	if (name && *name == '\0')
		return FSSH_B_BAD_VALUE;

	// get the vnode matching the entry_ref/node_ref
	if (name)
		status = entry_ref_to_vnode(mountID, parentID, name, &vnode);
	else
		status = get_vnode(mountID, parentID, &vnode, false);
	if (status < FSSH_B_OK)
		return status;

	status = open_dir_vnode(vnode, kernel);
	if (status < FSSH_B_OK)
		put_vnode(vnode);

	return status;
}


static int
dir_open(int fd, char *path, bool kernel)
{
	int status = FSSH_B_OK;

	FUNCTION(("dir_open: fd: %d, entry path = '%s', kernel %d\n", fd, path, kernel));

	// get the vnode matching the vnode + path combination
	struct vnode *vnode = NULL;
	fssh_vnode_id parentID;
	status = fd_and_path_to_vnode(fd, path, true, &vnode, &parentID, kernel);
	if (status != FSSH_B_OK)
		return status;

	// open the dir
	status = open_dir_vnode(vnode, kernel);
	if (status < FSSH_B_OK)
		put_vnode(vnode);

	return status;
}


static fssh_status_t
dir_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("dir_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_dir))
		return FS_CALL(vnode, close_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return FSSH_B_OK;
}


static void
dir_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_dir_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static fssh_status_t 
dir_read(struct file_descriptor *descriptor, struct fssh_dirent *buffer,
	fssh_size_t bufferSize, uint32_t *_count)
{
	return dir_read(descriptor->u.vnode, descriptor->cookie, buffer, bufferSize, _count);
}


static void
fix_dirent(struct vnode *parent, struct fssh_dirent *entry)
{
	// set d_pdev and d_pino
	entry->d_pdev = parent->device;
	entry->d_pino = parent->id;

	// If this is the ".." entry and the directory is the root of a FS,
	// we need to replace d_dev and d_ino with the actual values.
	if (fssh_strcmp(entry->d_name, "..") == 0
		&& parent->mount->root_vnode == parent
		&& parent->mount->covers_vnode) {
		inc_vnode_ref_count(parent);
			// vnode_path_to_vnode() puts the node

		struct vnode *vnode;
		fssh_status_t status = vnode_path_to_vnode(parent, "..", false, 0, &vnode,
			NULL, NULL);

		if (status == FSSH_B_OK) {
			entry->d_dev = vnode->device;
			entry->d_ino = vnode->id;
		}
	} else {
		// resolve mount points
		struct vnode *vnode = NULL;
		fssh_status_t status = get_vnode(entry->d_dev, entry->d_ino, &vnode, false);
		if (status != FSSH_B_OK)
			return;

		mutex_lock(&sVnodeCoveredByMutex);
		if (vnode->covered_by) {
			entry->d_dev = vnode->covered_by->device;
			entry->d_ino = vnode->covered_by->id;
		}
		mutex_unlock(&sVnodeCoveredByMutex);

		put_vnode(vnode);
	}
}


static fssh_status_t 
dir_read(struct vnode *vnode, fssh_fs_cookie cookie, struct fssh_dirent *buffer,
	fssh_size_t bufferSize, uint32_t *_count)
{
	if (!FS_CALL(vnode, read_dir))
		return FSSH_EOPNOTSUPP;

	fssh_status_t error = FS_CALL(vnode, read_dir)(vnode->mount->cookie,vnode->private_node,cookie,buffer,bufferSize,_count);
	if (error != FSSH_B_OK)
		return error;

	// we need to adjust the read dirents
	if (*_count > 0) {
		// XXX: Currently reading only one dirent is supported. Make this a loop!
		fix_dirent(vnode, buffer);
	}

	return error;
}


static fssh_status_t 
dir_rewind(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, rewind_dir))
		return FS_CALL(vnode, rewind_dir)(vnode->mount->cookie,vnode->private_node,descriptor->cookie);

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t
dir_remove(int fd, char *path, bool kernel)
{
	char name[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *directory;
	fssh_status_t status;

	if (path != NULL) {
		// we need to make sure our path name doesn't stop with "/", ".", or ".."
		char *lastSlash = fssh_strrchr(path, '/');
		if (lastSlash != NULL) {
			char *leaf = lastSlash + 1;
			if (!fssh_strcmp(leaf, ".."))
				return FSSH_B_NOT_ALLOWED;

			// omit multiple slashes
			while (lastSlash > path && lastSlash[-1] == '/') {
				lastSlash--;
			}

			if (!leaf[0]		
				|| !fssh_strcmp(leaf, ".")) {
				// "name/" -> "name", or "name/." -> "name"
				lastSlash[0] = '\0';
			}
		} else if (!fssh_strcmp(path, ".."))
			return FSSH_B_NOT_ALLOWED;
	}

	status = fd_and_path_to_dir_vnode(fd, path, &directory, name, kernel);
	if (status < FSSH_B_OK)
		return status;

	if (FS_CALL(directory, remove_dir)) {
		status = FS_CALL(directory, remove_dir)(directory->mount->cookie,
			directory->private_node, name);
	} else
		status = FSSH_EROFS;

	put_vnode(directory);
	return status;
}


static fssh_status_t
common_ioctl(struct file_descriptor *descriptor, uint32_t op, void *buffer,
	fssh_size_t length)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (FS_CALL(vnode, ioctl)) {
		return FS_CALL(vnode, ioctl)(vnode->mount->cookie, vnode->private_node,
			descriptor->cookie, op, buffer, length);
	}

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t 
common_fcntl(int fd, int op, uint32_t argument, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_fcntl(fd = %d, op = %d, argument = %lx, %s)\n",
		fd, op, argument, kernel ? "kernel" : "user"));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	switch (op) {
		case FSSH_F_SETFD:
		{
			struct io_context *context = get_current_io_context(kernel);
			// Set file descriptor flags

			// FSSH_O_CLOEXEC is the only flag available at this time
			mutex_lock(&context->io_mutex);
			fd_set_close_on_exec(context, fd, argument == FSSH_FD_CLOEXEC);
			mutex_unlock(&context->io_mutex);
			
			status = FSSH_B_OK;
			break;
		}

		case FSSH_F_GETFD:
		{
			struct io_context *context = get_current_io_context(kernel);

			// Get file descriptor flags
			mutex_lock(&context->io_mutex);
			status = fd_close_on_exec(context, fd) ? FSSH_FD_CLOEXEC : 0;
			mutex_unlock(&context->io_mutex);
			break;
		}

		case FSSH_F_SETFL:
			// Set file descriptor open mode
			if (FS_CALL(vnode, set_flags)) {
				// we only accept changes to FSSH_O_APPEND and FSSH_O_NONBLOCK
				argument &= FSSH_O_APPEND | FSSH_O_NONBLOCK;

				status = FS_CALL(vnode, set_flags)(vnode->mount->cookie,
					vnode->private_node, descriptor->cookie, (int)argument);
				if (status == FSSH_B_OK) {
					// update this descriptor's open_mode field
					descriptor->open_mode = (descriptor->open_mode & ~(FSSH_O_APPEND | FSSH_O_NONBLOCK))
						| argument;
				}
			} else
				status = FSSH_EOPNOTSUPP;
			break;

		case FSSH_F_GETFL:
			// Get file descriptor open mode
			status = descriptor->open_mode;
			break;

		case FSSH_F_DUPFD:
		{
			struct io_context *context = get_current_io_context(kernel);

			status = new_fd_etc(context, descriptor, (int)argument);
			if (status >= 0) {
				mutex_lock(&context->io_mutex);
				fd_set_close_on_exec(context, fd, false);
				mutex_unlock(&context->io_mutex);

				fssh_atomic_add(&descriptor->ref_count, 1);
			}
			break;
		}

		case FSSH_F_GETLK:
		case FSSH_F_SETLK:
		case FSSH_F_SETLKW:
			status = FSSH_B_BAD_VALUE;
			break;

		// ToDo: add support for more ops?

		default:
			status = FSSH_B_BAD_VALUE;
	}

	put_fd(descriptor);
	return status;
}


static fssh_status_t
common_sync(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_fsync: entry. fd %d kernel %d\n", fd, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	if (FS_CALL(vnode, fsync) != NULL)
		status = FS_CALL(vnode, fsync)(vnode->mount->cookie, vnode->private_node);
	else
		status = FSSH_EOPNOTSUPP;

	put_fd(descriptor);
	return status;
}


static fssh_status_t
common_lock_node(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	fssh_status_t status = FSSH_B_OK;

	// We need to set the locking atomically - someone
	// else might set one at the same time
#ifdef __x86_64__
	if (fssh_atomic_test_and_set64((vint64_t *)&vnode->mandatory_locked_by,
			(fssh_addr_t)descriptor, 0) != 0)
#else
	if (fssh_atomic_test_and_set((vint32_t *)&vnode->mandatory_locked_by,
			(fssh_addr_t)descriptor, 0) != 0)
#endif
		status = FSSH_B_BUSY;

	put_fd(descriptor);
	return status;
}


static fssh_status_t
common_unlock_node(int fd, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	fssh_status_t status = FSSH_B_OK;

	// We need to set the locking atomically - someone
	// else might set one at the same time
#ifdef __x86_64__
	if (fssh_atomic_test_and_set64((vint64_t *)&vnode->mandatory_locked_by,
			0, (fssh_addr_t)descriptor) != (int64_t)descriptor)
#else
	if (fssh_atomic_test_and_set((vint32_t *)&vnode->mandatory_locked_by,
			0, (fssh_addr_t)descriptor) != (int32_t)descriptor)
#endif
		status = FSSH_B_BAD_VALUE;

	put_fd(descriptor);
	return status;
}


static fssh_status_t
common_read_link(int fd, char *path, char *buffer, fssh_size_t *_bufferSize,
	bool kernel)
{
	struct vnode *vnode;
	fssh_status_t status;

	status = fd_and_path_to_vnode(fd, path, false, &vnode, NULL, kernel);
	if (status < FSSH_B_OK)
		return status;

	if (FS_CALL(vnode, read_symlink) != NULL) {
		status = FS_CALL(vnode, read_symlink)(vnode->mount->cookie,
			vnode->private_node, buffer, _bufferSize);
	} else
		status = FSSH_B_BAD_VALUE;

	put_vnode(vnode);
	return status;
}


static fssh_status_t
common_create_symlink(int fd, char *path, const char *toPath, int mode,
	bool kernel)
{
	// path validity checks have to be in the calling function!
	char name[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_create_symlink(fd = %d, path = %s, toPath = %s, mode = %d, kernel = %d)\n", fd, path, toPath, mode, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, name, kernel);
	if (status < FSSH_B_OK)
		return status;

	if (FS_CALL(vnode, create_symlink) != NULL)
		status = FS_CALL(vnode, create_symlink)(vnode->mount->cookie, vnode->private_node, name, toPath, mode);
	else
		status = FSSH_EROFS;

	put_vnode(vnode);

	return status;
}


static fssh_status_t
common_create_link(char *path, char *toPath, bool kernel)
{
	// path validity checks have to be in the calling function!
	char name[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *directory, *vnode;
	fssh_status_t status;

	FUNCTION(("common_create_link(path = %s, toPath = %s, kernel = %d)\n", path, toPath, kernel));

	status = path_to_dir_vnode(path, &directory, name, kernel);
	if (status < FSSH_B_OK)
		return status;

	status = path_to_vnode(toPath, true, &vnode, NULL, kernel);
	if (status < FSSH_B_OK)
		goto err;

	if (directory->mount != vnode->mount) {
		status = FSSH_B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(vnode, link) != NULL)
		status = FS_CALL(vnode, link)(directory->mount->cookie, directory->private_node, name, vnode->private_node);
	else
		status = FSSH_EROFS;

err1:
	put_vnode(vnode);
err:
	put_vnode(directory);

	return status;
}


static fssh_status_t
common_unlink(int fd, char *path, bool kernel)
{
	char filename[FSSH_B_FILE_NAME_LENGTH];
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_unlink: fd: %d, path '%s', kernel %d\n", fd, path, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &vnode, filename, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, unlink) != NULL)
		status = FS_CALL(vnode, unlink)(vnode->mount->cookie, vnode->private_node, filename);
	else
		status = FSSH_EROFS;

	put_vnode(vnode);

	return status;
}


static fssh_status_t
common_access(char *path, int mode, bool kernel)
{
	struct vnode *vnode;
	fssh_status_t status;

	status = path_to_vnode(path, true, &vnode, NULL, kernel);
	if (status < FSSH_B_OK)
		return status;

	if (FS_CALL(vnode, access) != NULL)
		status = FS_CALL(vnode, access)(vnode->mount->cookie, vnode->private_node, mode);
	else
		status = FSSH_B_OK;

	put_vnode(vnode);

	return status;
}


static fssh_status_t
common_rename(int fd, char *path, int newFD, char *newPath, bool kernel)
{
	struct vnode *fromVnode, *toVnode;
	char fromName[FSSH_B_FILE_NAME_LENGTH];
	char toName[FSSH_B_FILE_NAME_LENGTH];
	fssh_status_t status;

	FUNCTION(("common_rename(fd = %d, path = %s, newFD = %d, newPath = %s, kernel = %d)\n", fd, path, newFD, newPath, kernel));

	status = fd_and_path_to_dir_vnode(fd, path, &fromVnode, fromName, kernel);
	if (status < 0)
		return status;

	status = fd_and_path_to_dir_vnode(newFD, newPath, &toVnode, toName, kernel);
	if (status < 0)
		goto err;

	if (fromVnode->device != toVnode->device) {
		status = FSSH_B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(fromVnode, rename) != NULL)
		status = FS_CALL(fromVnode, rename)(fromVnode->mount->cookie, fromVnode->private_node, fromName, toVnode->private_node, toName);
	else
		status = FSSH_EROFS;

err1:
	put_vnode(toVnode);
err:
	put_vnode(fromVnode);

	return status;
}


static fssh_status_t
common_read_stat(struct file_descriptor *descriptor, struct fssh_stat *stat)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("common_read_stat: stat %p\n", stat));

	fssh_status_t status = FS_CALL(vnode, read_stat)(vnode->mount->cookie,
		vnode->private_node, stat);

	// fill in the st_dev and st_ino fields
	if (status == FSSH_B_OK) {
		stat->fssh_st_dev = vnode->device;
		stat->fssh_st_ino = vnode->id;
	}

	return status;
}


static fssh_status_t
common_write_stat(struct file_descriptor *descriptor,
	const struct fssh_stat *stat, int statMask)
{
	struct vnode *vnode = descriptor->u.vnode;
	
	FUNCTION(("common_write_stat(vnode = %p, stat = %p, statMask = %d)\n", vnode, stat, statMask));
	if (!FS_CALL(vnode, write_stat))
		return FSSH_EROFS;

	return FS_CALL(vnode, write_stat)(vnode->mount->cookie, vnode->private_node, stat, statMask);
}


static fssh_status_t
common_path_read_stat(int fd, char *path, bool traverseLeafLink,
	struct fssh_stat *stat, bool kernel)
{
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_path_read_stat: fd: %d, path '%s', stat %p,\n", fd, path, stat));

	status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode, NULL, kernel);
	if (status < 0)
		return status;

	status = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, stat);

	// fill in the st_dev and st_ino fields
	if (status == FSSH_B_OK) {
		stat->fssh_st_dev = vnode->device;
		stat->fssh_st_ino = vnode->id;
	}

	put_vnode(vnode);
	return status;
}


static fssh_status_t
common_path_write_stat(int fd, char *path, bool traverseLeafLink,
	const struct fssh_stat *stat, int statMask, bool kernel)
{
	struct vnode *vnode;
	fssh_status_t status;

	FUNCTION(("common_write_stat: fd: %d, path '%s', stat %p, stat_mask %d, kernel %d\n", fd, path, stat, statMask, kernel));

	status = fd_and_path_to_vnode(fd, path, traverseLeafLink, &vnode, NULL, kernel);
	if (status < 0)
		return status;

	if (FS_CALL(vnode, write_stat))
		status = FS_CALL(vnode, write_stat)(vnode->mount->cookie, vnode->private_node, stat, statMask);
	else
		status = FSSH_EROFS;

	put_vnode(vnode);

	return status;
}


static int
attr_dir_open(int fd, char *path, bool kernel)
{
	struct vnode *vnode;
	int status;

	FUNCTION(("attr_dir_open(fd = %d, path = '%s', kernel = %d)\n", fd, path, kernel));

	status = fd_and_path_to_vnode(fd, path, true, &vnode, NULL, kernel);
	if (status < FSSH_B_OK)
		return status;

	status = open_attr_dir_vnode(vnode, kernel);
	if (status < 0)
		put_vnode(vnode);

	return status;
}


static fssh_status_t
attr_dir_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_attr_dir))
		return FS_CALL(vnode, close_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return FSSH_B_OK;
}


static void
attr_dir_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_dir_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static fssh_status_t 
attr_dir_read(struct file_descriptor *descriptor, struct fssh_dirent *buffer,
	fssh_size_t bufferSize, uint32_t *_count)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_read(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, read_attr_dir))
		return FS_CALL(vnode, read_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, buffer, bufferSize, _count);

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t 
attr_dir_rewind(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_dir_rewind(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, rewind_attr_dir))
		return FS_CALL(vnode, rewind_attr_dir)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return FSSH_EOPNOTSUPP;
}


static int
attr_create(int fd, const char *name, uint32_t type, int openMode, bool kernel)
{
	struct vnode *vnode;
	fssh_fs_cookie cookie;
	int status;

	if (name == NULL || *name == '\0')
		return FSSH_B_BAD_VALUE;

	vnode = get_vnode_from_fd(fd, kernel);
	if (vnode == NULL)
		return FSSH_B_FILE_ERROR;

	if (FS_CALL(vnode, create_attr) == NULL) {
		status = FSSH_EROFS;
		goto err;
	}

	status = FS_CALL(vnode, create_attr)(vnode->mount->cookie, vnode->private_node, name, type, openMode, &cookie);
	if (status < FSSH_B_OK)
		goto err;

	if ((status = get_new_fd(FDTYPE_ATTR, NULL, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

	FS_CALL(vnode, remove_attr)(vnode->mount->cookie, vnode->private_node, name);

err:
	put_vnode(vnode);

	return status;
}


static int
attr_open(int fd, const char *name, int openMode, bool kernel)
{
	struct vnode *vnode;
	fssh_fs_cookie cookie;
	int status;

	if (name == NULL || *name == '\0')
		return FSSH_B_BAD_VALUE;

	vnode = get_vnode_from_fd(fd, kernel);
	if (vnode == NULL)
		return FSSH_B_FILE_ERROR;

	if (FS_CALL(vnode, open_attr) == NULL) {
		status = FSSH_EOPNOTSUPP;
		goto err;
	}

	status = FS_CALL(vnode, open_attr)(vnode->mount->cookie, vnode->private_node, name, openMode, &cookie);
	if (status < FSSH_B_OK)
		goto err;

	// now we only need a file descriptor for this attribute and we're done
	if ((status = get_new_fd(FDTYPE_ATTR, NULL, vnode, cookie, openMode, kernel)) >= 0)
		return status;

	FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, cookie);
	FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, cookie);

err:
	put_vnode(vnode);

	return status;
}


static fssh_status_t
attr_close(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_close(descriptor = %p)\n", descriptor));

	if (FS_CALL(vnode, close_attr))
		return FS_CALL(vnode, close_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);

	return FSSH_B_OK;
}


static void
attr_free_fd(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->u.vnode;

	if (vnode != NULL) {
		FS_CALL(vnode, free_attr_cookie)(vnode->mount->cookie, vnode->private_node, descriptor->cookie);
		put_vnode(vnode);
	}
}


static fssh_status_t
attr_read(struct file_descriptor *descriptor, fssh_off_t pos, void *buffer, fssh_size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_read: buf %p, pos %Ld, len %p = %ld\n", buffer, pos, length, *length));
	if (!FS_CALL(vnode, read_attr))
		return FSSH_EOPNOTSUPP;

	return FS_CALL(vnode, read_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static fssh_status_t
attr_write(struct file_descriptor *descriptor, fssh_off_t pos, const void *buffer, fssh_size_t *length)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_write: buf %p, pos %Ld, len %p\n", buffer, pos, length));
	if (!FS_CALL(vnode, write_attr))
		return FSSH_EOPNOTSUPP;

	return FS_CALL(vnode, write_attr)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, pos, buffer, length);
}


static fssh_off_t
attr_seek(struct file_descriptor *descriptor, fssh_off_t pos, int seekType)
{
	fssh_off_t offset;

	switch (seekType) {
		case FSSH_SEEK_SET:
			offset = 0;
			break;
		case FSSH_SEEK_CUR:
			offset = descriptor->pos;
			break;
		case FSSH_SEEK_END:
		{
			struct vnode *vnode = descriptor->u.vnode;
			struct fssh_stat stat;
			fssh_status_t status;

			if (FS_CALL(vnode, read_stat) == NULL)
				return FSSH_EOPNOTSUPP;

			status = FS_CALL(vnode, read_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, &stat);
			if (status < FSSH_B_OK)
				return status;

			offset = stat.fssh_st_size;
			break;
		}
		default:
			return FSSH_B_BAD_VALUE;
	}

	// assumes fssh_off_t is 64 bits wide
	if (offset > 0 && LLONG_MAX - offset < pos)
		return FSSH_EOVERFLOW;

	pos += offset;
	if (pos < 0)
		return FSSH_B_BAD_VALUE;

	return descriptor->pos = pos;
}


static fssh_status_t
attr_read_stat(struct file_descriptor *descriptor, struct fssh_stat *stat)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_read_stat: stat 0x%p\n", stat));

	if (!FS_CALL(vnode, read_attr_stat))
		return FSSH_EOPNOTSUPP;

	return FS_CALL(vnode, read_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, stat);
}


static fssh_status_t
attr_write_stat(struct file_descriptor *descriptor,
	const struct fssh_stat *stat, int statMask)
{
	struct vnode *vnode = descriptor->u.vnode;

	FUNCTION(("attr_write_stat: stat = %p, statMask %d\n", stat, statMask));

	if (!FS_CALL(vnode, write_attr_stat))
		return FSSH_EROFS;

	return FS_CALL(vnode, write_attr_stat)(vnode->mount->cookie, vnode->private_node, descriptor->cookie, stat, statMask);
}


static fssh_status_t
attr_remove(int fd, const char *name, bool kernel)
{
	struct file_descriptor *descriptor;
	struct vnode *vnode;
	fssh_status_t status;

	if (name == NULL || *name == '\0')
		return FSSH_B_BAD_VALUE;

	FUNCTION(("attr_remove: fd = %d, name = \"%s\", kernel %d\n", fd, name, kernel));

	descriptor = get_fd_and_vnode(fd, &vnode, kernel);
	if (descriptor == NULL)
		return FSSH_B_FILE_ERROR;

	if (FS_CALL(vnode, remove_attr))
		status = FS_CALL(vnode, remove_attr)(vnode->mount->cookie, vnode->private_node, name);
	else
		status = FSSH_EROFS;

	put_fd(descriptor);

	return status;
}


static fssh_status_t
attr_rename(int fromfd, const char *fromName, int tofd, const char *toName, bool kernel)
{
	struct file_descriptor *fromDescriptor, *toDescriptor;
	struct vnode *fromVnode, *toVnode;
	fssh_status_t status;

	if (fromName == NULL || *fromName == '\0' || toName == NULL || *toName == '\0')
		return FSSH_B_BAD_VALUE;

	FUNCTION(("attr_rename: from fd = %d, from name = \"%s\", to fd = %d, to name = \"%s\", kernel %d\n", fromfd, fromName, tofd, toName, kernel));

	fromDescriptor = get_fd_and_vnode(fromfd, &fromVnode, kernel);
	if (fromDescriptor == NULL)
		return FSSH_B_FILE_ERROR;

	toDescriptor = get_fd_and_vnode(tofd, &toVnode, kernel);
	if (toDescriptor == NULL) {
		status = FSSH_B_FILE_ERROR;
		goto err;
	}

	// are the files on the same volume?
	if (fromVnode->device != toVnode->device) {
		status = FSSH_B_CROSS_DEVICE_LINK;
		goto err1;
	}

	if (FS_CALL(fromVnode, rename_attr))
		status = FS_CALL(fromVnode, rename_attr)(fromVnode->mount->cookie, fromVnode->private_node, fromName, toVnode->private_node, toName);
	else
		status = FSSH_EROFS;

err1:
	put_fd(toDescriptor);
err:
	put_fd(fromDescriptor);

	return status;
}


static fssh_status_t
index_dir_open(fssh_mount_id mountID, bool kernel)
{
	struct fs_mount *mount;
	fssh_fs_cookie cookie;

	FUNCTION(("index_dir_open(mountID = %ld, kernel = %d)\n", mountID, kernel));

	fssh_status_t status = get_mount(mountID, &mount);
	if (status < FSSH_B_OK)
		return status;

	if (FS_MOUNT_CALL(mount, open_index_dir) == NULL) {
		status = FSSH_EOPNOTSUPP;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, open_index_dir)(mount->cookie, &cookie);
	if (status < FSSH_B_OK)
		goto out;

	// get fd for the index directory
	status = get_new_fd(FDTYPE_INDEX_DIR, mount, NULL, cookie, 0, kernel);
	if (status >= 0)
		goto out;

	// something went wrong
	FS_MOUNT_CALL(mount, close_index_dir)(mount->cookie, cookie);
	FS_MOUNT_CALL(mount, free_index_dir_cookie)(mount->cookie, cookie);

out:
	put_mount(mount);
	return status;
}


static fssh_status_t
index_dir_close(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	FUNCTION(("index_dir_close(descriptor = %p)\n", descriptor));

	if (FS_MOUNT_CALL(mount, close_index_dir))
		return FS_MOUNT_CALL(mount, close_index_dir)(mount->cookie, descriptor->cookie);

	return FSSH_B_OK;
}


static void
index_dir_free_fd(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (mount != NULL) {
		FS_MOUNT_CALL(mount, free_index_dir_cookie)(mount->cookie, descriptor->cookie);
		// ToDo: find a replacement ref_count object - perhaps the root dir?
		//put_vnode(vnode);
	}
}


static fssh_status_t 
index_dir_read(struct file_descriptor *descriptor, struct fssh_dirent *buffer,
	fssh_size_t bufferSize, uint32_t *_count)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, read_index_dir))
		return FS_MOUNT_CALL(mount, read_index_dir)(mount->cookie, descriptor->cookie, buffer, bufferSize, _count);

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t 
index_dir_rewind(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, rewind_index_dir))
		return FS_MOUNT_CALL(mount, rewind_index_dir)(mount->cookie, descriptor->cookie);

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t
index_create(fssh_mount_id mountID, const char *name, uint32_t type, uint32_t flags, bool kernel)
{
	FUNCTION(("index_create(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	struct fs_mount *mount;
	fssh_status_t status = get_mount(mountID, &mount);
	if (status < FSSH_B_OK)
		return status;

	if (FS_MOUNT_CALL(mount, create_index) == NULL) {
		status = FSSH_EROFS;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, create_index)(mount->cookie, name, type, flags);

out:
	put_mount(mount);
	return status;
}


static fssh_status_t
index_name_read_stat(fssh_mount_id mountID, const char *name,
	struct fssh_stat *stat, bool kernel)
{
	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	struct fs_mount *mount;
	fssh_status_t status = get_mount(mountID, &mount);
	if (status < FSSH_B_OK)
		return status;

	if (FS_MOUNT_CALL(mount, read_index_stat) == NULL) {
		status = FSSH_EOPNOTSUPP;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, read_index_stat)(mount->cookie, name, stat);

out:
	put_mount(mount);
	return status;
}


static fssh_status_t
index_remove(fssh_mount_id mountID, const char *name, bool kernel)
{
	FUNCTION(("index_remove(mountID = %ld, name = %s, kernel = %d)\n", mountID, name, kernel));

	struct fs_mount *mount;
	fssh_status_t status = get_mount(mountID, &mount);
	if (status < FSSH_B_OK)
		return status;

	if (FS_MOUNT_CALL(mount, remove_index) == NULL) {
		status = FSSH_EROFS;
		goto out;
	}

	status = FS_MOUNT_CALL(mount, remove_index)(mount->cookie, name);

out:
	put_mount(mount);
	return status;
}


static fssh_status_t
query_close(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	FUNCTION(("query_close(descriptor = %p)\n", descriptor));

	if (FS_MOUNT_CALL(mount, close_query))
		return FS_MOUNT_CALL(mount, close_query)(mount->cookie, descriptor->cookie);

	return FSSH_B_OK;
}


static void
query_free_fd(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (mount != NULL) {
		FS_MOUNT_CALL(mount, free_query_cookie)(mount->cookie, descriptor->cookie);
		// ToDo: find a replacement ref_count object - perhaps the root dir?
		//put_vnode(vnode);
	}
}


static fssh_status_t 
query_read(struct file_descriptor *descriptor, struct fssh_dirent *buffer,
	fssh_size_t bufferSize, uint32_t *_count)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, read_query))
		return FS_MOUNT_CALL(mount, read_query)(mount->cookie, descriptor->cookie, buffer, bufferSize, _count);

	return FSSH_EOPNOTSUPP;
}


static fssh_status_t 
query_rewind(struct file_descriptor *descriptor)
{
	struct fs_mount *mount = descriptor->u.mount;

	if (FS_MOUNT_CALL(mount, rewind_query))
		return FS_MOUNT_CALL(mount, rewind_query)(mount->cookie, descriptor->cookie);

	return FSSH_EOPNOTSUPP;
}


//	#pragma mark -
//	General File System functions


static fssh_dev_t
fs_mount(char *path, const char *device, const char *fsName, uint32_t flags,
	const char *args, bool kernel)
{
	struct fs_mount *mount;
	fssh_status_t status = 0;

	FUNCTION(("fs_mount: entry. path = '%s', fs_name = '%s'\n", path, fsName));

	// The path is always safe, we just have to make sure that fsName is
	// almost valid - we can't make any assumptions about args, though.
	// A NULL fsName is OK, if a device was given and the FS is not virtual.
	// We'll get it from the DDM later.
	if (fsName == NULL) {
		if (!device || flags & FSSH_B_MOUNT_VIRTUAL_DEVICE)
			return FSSH_B_BAD_VALUE;
	} else if (fsName[0] == '\0')
		return FSSH_B_BAD_VALUE;

	RecursiveLocker mountOpLocker(sMountOpLock);

	// If the file system is not a "virtual" one, the device argument should
	// point to a real file/device (if given at all).
	// get the partition
	KPath normalizedDevice;

	if (!(flags & FSSH_B_MOUNT_VIRTUAL_DEVICE) && device) {
		// normalize the device path
//		status = normalizedDevice.SetTo(device, true);
// NOTE: normalizing works only in our namespace.
		status = normalizedDevice.SetTo(device, false);
		if (status != FSSH_B_OK)
			return status;

		device = normalizedDevice.Path();
			// correct path to file device
	}

	mount = (struct fs_mount *)malloc(sizeof(struct fs_mount));
	if (mount == NULL)
		return FSSH_B_NO_MEMORY;

	list_init_etc(&mount->vnodes, fssh_offsetof(struct vnode, mount_link));

	mount->fs_name = get_file_system_name(fsName);
	if (mount->fs_name == NULL) {
		status = FSSH_B_NO_MEMORY;
		goto err1;
	}

	mount->device_name = fssh_strdup(device);
		// "device" can be NULL

	mount->fs = get_file_system(fsName);
	if (mount->fs == NULL) {
		status = FSSH_ENODEV;
		goto err3;
	}

	status = recursive_lock_init(&mount->rlock, "mount rlock");
	if (status < FSSH_B_OK)
		goto err4;

	// initialize structure
	mount->id = sNextMountID++;
	mount->root_vnode = NULL;
	mount->covers_vnode = NULL;
	mount->cookie = NULL;
	mount->unmounting = false;
	mount->owns_file_device = false;

	// insert mount struct into list before we call FS's mount() function
	// so that vnodes can be created for this mount
	mutex_lock(&sMountMutex);
	hash_insert(sMountsTable, mount);
	mutex_unlock(&sMountMutex);

	fssh_vnode_id rootID;

	if (!sRoot) {
		// we haven't mounted anything yet
		if (fssh_strcmp(path, "/") != 0) {
			status = FSSH_B_ERROR;
			goto err5;
		}

		status = FS_MOUNT_CALL(mount, mount)(mount->id, device, flags, args, &mount->cookie, &rootID);
		if (status < 0) {
			// ToDo: why should we hide the error code from the file system here?
			//status = ERR_VFS_GENERAL;
			goto err5;
		}
	} else {
		struct vnode *coveredVnode;
		status = path_to_vnode(path, true, &coveredVnode, NULL, kernel);
		if (status < FSSH_B_OK)
			goto err5;

		// make sure covered_vnode is a DIR
		struct fssh_stat coveredNodeStat;
		status = FS_CALL(coveredVnode, read_stat)(coveredVnode->mount->cookie,
			coveredVnode->private_node, &coveredNodeStat);
		if (status < FSSH_B_OK)
			goto err5;

		if (!FSSH_S_ISDIR(coveredNodeStat.fssh_st_mode)) {
			status = FSSH_B_NOT_A_DIRECTORY;
			goto err5;
		}

		if (coveredVnode->mount->root_vnode == coveredVnode) {
			// this is already a mount point
			status = FSSH_B_BUSY;
			goto err5;
		}

		mount->covers_vnode = coveredVnode;

		// mount it
		status = FS_MOUNT_CALL(mount, mount)(mount->id, device, flags, args, &mount->cookie, &rootID);
		if (status < FSSH_B_OK)
			goto err6;
	}

	// the root node is supposed to be owned by the file system - it must
	// exist at this point
	mount->root_vnode = lookup_vnode(mount->id, rootID);
	if (mount->root_vnode == NULL || mount->root_vnode->ref_count != 1) {
		fssh_panic("fs_mount: file system does not own its root node!\n");
		status = FSSH_B_ERROR;
		goto err7;
	}

	// No race here, since fs_mount() is the only function changing
	// covers_vnode (and holds sMountOpLock at that time).
	mutex_lock(&sVnodeCoveredByMutex);
	if (mount->covers_vnode)
		mount->covers_vnode->covered_by = mount->root_vnode;
	mutex_unlock(&sVnodeCoveredByMutex);

	if (!sRoot)
		sRoot = mount->root_vnode;

	return mount->id;

err7:
	FS_MOUNT_CALL(mount, unmount)(mount->cookie);
err6:
	if (mount->covers_vnode)
		put_vnode(mount->covers_vnode);
err5:
	mutex_lock(&sMountMutex);
	hash_remove(sMountsTable, mount);
	mutex_unlock(&sMountMutex);

	recursive_lock_destroy(&mount->rlock);
err4:
	put_file_system(mount->fs);
	free(mount->device_name);
err3:
	free(mount->fs_name);
err1:
	free(mount);

	return status;
}


static fssh_status_t
fs_unmount(char *path, uint32_t flags, bool kernel)
{
	struct fs_mount *mount;
	struct vnode *vnode;
	fssh_status_t err;

	FUNCTION(("vfs_unmount: entry. path = '%s', kernel %d\n", path, kernel));

	err = path_to_vnode(path, true, &vnode, NULL, kernel);
	if (err < 0)
		return FSSH_B_ENTRY_NOT_FOUND;

	RecursiveLocker mountOpLocker(sMountOpLock);

	mount = find_mount(vnode->device);
	if (!mount)
		fssh_panic("vfs_unmount: find_mount() failed on root vnode @%p of mount\n", vnode);

	if (mount->root_vnode != vnode) {
		// not mountpoint
		put_vnode(vnode);
		return FSSH_B_BAD_VALUE;
	}

	// grab the vnode master mutex to keep someone from creating
	// a vnode while we're figuring out if we can continue
	mutex_lock(&sVnodeMutex);

	bool disconnectedDescriptors = false;

	while (true) {
		bool busy = false;

		// cycle through the list of vnodes associated with this mount and
		// make sure all of them are not busy or have refs on them
		vnode = NULL;
		while ((vnode = (struct vnode *)list_get_next_item(&mount->vnodes, vnode)) != NULL) {
			// The root vnode ref_count needs to be 2 here: one for the file
			// system, one from the path_to_vnode() call above
			if (vnode->busy
				|| ((vnode->ref_count != 0 && mount->root_vnode != vnode)
					|| (vnode->ref_count != 2 && mount->root_vnode == vnode))) {
				// there are still vnodes in use on this mount, so we cannot
				// unmount yet
				busy = true;
				break;
			}
		}

		if (!busy)
			break;

		if ((flags & FSSH_B_FORCE_UNMOUNT) == 0) {
			mutex_unlock(&sVnodeMutex);
			put_vnode(mount->root_vnode);

			return FSSH_B_BUSY;
		}

		if (disconnectedDescriptors) {
			// wait a bit until the last access is finished, and then try again
			mutex_unlock(&sVnodeMutex);
			fssh_snooze(100000);
			// TODO: if there is some kind of bug that prevents the ref counts
			//	from getting back to zero, this will fall into an endless loop...
			mutex_lock(&sVnodeMutex);
			continue;
		}

		// the file system is still busy - but we're forced to unmount it,
		// so let's disconnect all open file descriptors

		mount->unmounting = true;
			// prevent new vnodes from being created

		mutex_unlock(&sVnodeMutex);

		disconnect_mount_or_vnode_fds(mount, NULL);
		disconnectedDescriptors = true;

		mutex_lock(&sVnodeMutex);
	}

	// we can safely continue, mark all of the vnodes busy and this mount
	// structure in unmounting state
	mount->unmounting = true;

	while ((vnode = (struct vnode *)list_get_next_item(&mount->vnodes, vnode)) != NULL) {
		vnode->busy = true;

		if (vnode->ref_count == 0) {
			// this vnode has been unused before
			list_remove_item(&sUnusedVnodeList, vnode);
			sUnusedVnodes--;
		}
	}

	// The ref_count of the root node is 2 at this point, see above why this is
	mount->root_vnode->ref_count -= 2;

	mutex_unlock(&sVnodeMutex);

	mutex_lock(&sVnodeCoveredByMutex);
	mount->covers_vnode->covered_by = NULL;
	mutex_unlock(&sVnodeCoveredByMutex);
	put_vnode(mount->covers_vnode);

	// Free all vnodes associated with this mount.
	// They will be removed from the mount list by free_vnode(), so
	// we don't have to do this.
	while ((vnode = (struct vnode *)list_get_first_item(&mount->vnodes)) != NULL) {
		free_vnode(vnode, false);
	}

	// remove the mount structure from the hash table
	mutex_lock(&sMountMutex);
	hash_remove(sMountsTable, mount);
	mutex_unlock(&sMountMutex);

	mountOpLocker.Unlock();

	FS_MOUNT_CALL(mount, unmount)(mount->cookie);

	// release the file system
	put_file_system(mount->fs);

	free(mount->device_name);
	free(mount->fs_name);
	free(mount);

	return FSSH_B_OK;
}


static fssh_status_t
fs_sync(fssh_dev_t device)
{
	struct fs_mount *mount;
	fssh_status_t status = get_mount(device, &mount);
	if (status < FSSH_B_OK)
		return status;

	mutex_lock(&sMountMutex);

	if (FS_MOUNT_CALL(mount, sync))
		status = FS_MOUNT_CALL(mount, sync)(mount->cookie);

	mutex_unlock(&sMountMutex);

	struct vnode *previousVnode = NULL;
	while (true) {
		// synchronize access to vnode list
		recursive_lock_lock(&mount->rlock);

		struct vnode *vnode = (struct vnode *)list_get_next_item(&mount->vnodes,
			previousVnode);

		fssh_vnode_id id = -1;
		if (vnode != NULL)
			id = vnode->id;

		recursive_lock_unlock(&mount->rlock);

		if (vnode == NULL)
			break;

		// acquire a reference to the vnode

		if (get_vnode(mount->id, id, &vnode, true) == FSSH_B_OK) {
			if (previousVnode != NULL)
				put_vnode(previousVnode);

			if (FS_CALL(vnode, fsync) != NULL)
				FS_CALL(vnode, fsync)(vnode->mount->cookie, vnode->private_node);

			// the next vnode might change until we lock the vnode list again,
			// but this vnode won't go away since we keep a reference to it.
			previousVnode = vnode;
		} else {
			fssh_dprintf("syncing of mount %d stopped due to vnode %lld.\n", (int)mount->id, id);
			break;
		}
	}

	if (previousVnode != NULL)
		put_vnode(previousVnode);

	put_mount(mount);
	return status;
}


static fssh_status_t
fs_read_info(fssh_dev_t device, struct fssh_fs_info *info)
{
	struct fs_mount *mount;
	fssh_status_t status = get_mount(device, &mount);
	if (status < FSSH_B_OK)
		return status;

	fssh_memset(info, 0, sizeof(struct fssh_fs_info));

	if (FS_MOUNT_CALL(mount, read_fs_info))
		status = FS_MOUNT_CALL(mount, read_fs_info)(mount->cookie, info);

	// fill in info the file system doesn't (have to) know about
	if (status == FSSH_B_OK) {
		info->dev = mount->id;
		info->root = mount->root_vnode->id;
		fssh_strlcpy(info->fsh_name, mount->fs_name, sizeof(info->fsh_name));
		if (mount->device_name != NULL) {
			fssh_strlcpy(info->device_name, mount->device_name,
				sizeof(info->device_name));
		}
	}

	// if the call is not supported by the file system, there are still
	// the parts that we filled out ourselves

	put_mount(mount);
	return status;
}


static fssh_status_t
fs_write_info(fssh_dev_t device, const struct fssh_fs_info *info, int mask)
{
	struct fs_mount *mount;
	fssh_status_t status = get_mount(device, &mount);
	if (status < FSSH_B_OK)
		return status;

	if (FS_MOUNT_CALL(mount, write_fs_info))
		status = FS_MOUNT_CALL(mount, write_fs_info)(mount->cookie, info, mask);
	else
		status = FSSH_EROFS;

	put_mount(mount);
	return status;
}


static fssh_dev_t
fs_next_device(int32_t *_cookie)
{
	struct fs_mount *mount = NULL;
	fssh_dev_t device = *_cookie;

	mutex_lock(&sMountMutex);	

	// Since device IDs are assigned sequentially, this algorithm
	// does work good enough. It makes sure that the device list
	// returned is sorted, and that no device is skipped when an
	// already visited device got unmounted.

	while (device < sNextMountID) {
		mount = find_mount(device++);
		if (mount != NULL && mount->cookie != NULL)
			break;
	}

	*_cookie = device;

	if (mount != NULL)
		device = mount->id;
	else
		device = FSSH_B_BAD_VALUE;

	mutex_unlock(&sMountMutex);
	
	return device;
}


static fssh_status_t
get_cwd(char *buffer, fssh_size_t size, bool kernel)
{
	// Get current working directory from io context
	struct io_context *context = get_current_io_context(kernel);
	fssh_status_t status;

	FUNCTION(("vfs_get_cwd: buf %p, size %ld\n", buffer, size));

	mutex_lock(&context->io_mutex);

	if (context->cwd)
		status = dir_vnode_to_path(context->cwd, buffer, size);
	else
		status = FSSH_B_ERROR;

	mutex_unlock(&context->io_mutex);
	return status; 
}


static fssh_status_t
set_cwd(int fd, char *path, bool kernel)
{
	struct io_context *context;
	struct vnode *vnode = NULL;
	struct vnode *oldDirectory;
	struct fssh_stat stat;
	fssh_status_t status;

	FUNCTION(("set_cwd: path = \'%s\'\n", path));

	// Get vnode for passed path, and bail if it failed
	status = fd_and_path_to_vnode(fd, path, true, &vnode, NULL, kernel);
	if (status < 0)
		return status;

	status = FS_CALL(vnode, read_stat)(vnode->mount->cookie, vnode->private_node, &stat);
	if (status < 0)
		goto err;

	if (!FSSH_S_ISDIR(stat.fssh_st_mode)) {
		// nope, can't cwd to here
		status = FSSH_B_NOT_A_DIRECTORY;
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

	return FSSH_B_NO_ERROR;

err:
	put_vnode(vnode);
	return status;
}


//	#pragma mark -
//	Calls from within the kernel


fssh_dev_t
_kern_mount(const char *path, const char *device, const char *fsName,
	uint32_t flags, const char *args, fssh_size_t argsLength)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return fs_mount(pathBuffer.LockBuffer(), device, fsName, flags, args, true);
}


fssh_status_t
_kern_unmount(const char *path, uint32_t flags)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return fs_unmount(pathBuffer.LockBuffer(), flags, true);
}


fssh_status_t
_kern_read_fs_info(fssh_dev_t device, struct fssh_fs_info *info)
{
	if (info == NULL)
		return FSSH_B_BAD_VALUE;

	return fs_read_info(device, info);
}


fssh_status_t
_kern_write_fs_info(fssh_dev_t device, const struct fssh_fs_info *info, int mask)
{
	if (info == NULL)
		return FSSH_B_BAD_VALUE;

	return fs_write_info(device, info, mask);
}


fssh_status_t
_kern_sync(void)
{
	// Note: _kern_sync() is also called from _user_sync()
	int32_t cookie = 0;
	fssh_dev_t device;
	while ((device = fs_next_device(&cookie)) >= 0) {
		fssh_status_t status = fs_sync(device);
		if (status != FSSH_B_OK && status != FSSH_B_BAD_VALUE)
			fssh_dprintf("sync: device %d couldn't sync: %s\n", (int)device, fssh_strerror(status));
	}

	return FSSH_B_OK;
}


fssh_dev_t
_kern_next_device(int32_t *_cookie)
{
	return fs_next_device(_cookie);
}


int
_kern_open_entry_ref(fssh_dev_t device, fssh_ino_t inode, const char *name, int openMode, int perms)
{
	if (openMode & FSSH_O_CREAT)
		return file_create_entry_ref(device, inode, name, openMode, perms, true);

	return file_open_entry_ref(device, inode, name, openMode, true);
}


/**	\brief Opens a node specified by a FD + path pair.
 *
 *	At least one of \a fd and \a path must be specified.
 *	If only \a fd is given, the function opens the node identified by this
 *	FD. If only a path is given, this path is opened. If both are given and
 *	the path is absolute, \a fd is ignored; a relative path is reckoned off
 *	of the directory (!) identified by \a fd.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. May be \c NULL.
 *	\param openMode The open mode.
 *	\return A FD referring to the newly opened node, or an error code,
 *			if an error occurs.
 */

int
_kern_open(int fd, const char *path, int openMode, int perms)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	if (openMode & FSSH_O_CREAT)
		return file_create(fd, pathBuffer.LockBuffer(), openMode, perms, true);

	return file_open(fd, pathBuffer.LockBuffer(), openMode, true);
}


/**	\brief Opens a directory specified by entry_ref or node_ref.
 *
 *	The supplied name may be \c NULL, in which case directory identified
 *	by \a device and \a inode will be opened. Otherwise \a device and
 *	\a inode identify the parent directory of the directory to be opened
 *	and \a name its entry name.
 *
 *	\param device If \a name is specified the ID of the device the parent
 *		   directory of the directory to be opened resides on, otherwise
 *		   the device of the directory itself.
 *	\param inode If \a name is specified the node ID of the parent
 *		   directory of the directory to be opened, otherwise node ID of the
 *		   directory itself.
 *	\param name The entry name of the directory to be opened. If \c NULL,
 *		   the \a device + \a inode pair identify the node to be opened.
 *	\return The FD of the newly opened directory or an error code, if
 *			something went wrong.
 */

int
_kern_open_dir_entry_ref(fssh_dev_t device, fssh_ino_t inode, const char *name)
{
	return dir_open_entry_ref(device, inode, name, true);
}


/**	\brief Opens a directory specified by a FD + path pair.
 *
 *	At least one of \a fd and \a path must be specified.
 *	If only \a fd is given, the function opens the directory identified by this
 *	FD. If only a path is given, this path is opened. If both are given and
 *	the path is absolute, \a fd is ignored; a relative path is reckoned off
 *	of the directory (!) identified by \a fd.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. May be \c NULL.
 *	\return A FD referring to the newly opened directory, or an error code,
 *			if an error occurs.
 */

int
_kern_open_dir(int fd, const char *path)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return dir_open(fd, pathBuffer.LockBuffer(), true);
}


fssh_status_t 
_kern_fcntl(int fd, int op, uint32_t argument)
{
	return common_fcntl(fd, op, argument, true);
}


fssh_status_t
_kern_fsync(int fd)
{
	return common_sync(fd, true);
}


fssh_status_t
_kern_lock_node(int fd)
{
	return common_lock_node(fd, true);
}


fssh_status_t
_kern_unlock_node(int fd)
{
	return common_unlock_node(fd, true);
}


fssh_status_t
_kern_create_dir_entry_ref(fssh_dev_t device, fssh_ino_t inode, const char *name, int perms)
{
	return dir_create_entry_ref(device, inode, name, perms, true);
}


/**	\brief Creates a directory specified by a FD + path pair.
 *
 *	\a path must always be specified (it contains the name of the new directory
 *	at least). If only a path is given, this path identifies the location at
 *	which the directory shall be created. If both \a fd and \a path are given and
 *	the path is absolute, \a fd is ignored; a relative path is reckoned off
 *	of the directory (!) identified by \a fd.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. Must not be \c NULL.
 *	\param perms The access permissions the new directory shall have.
 *	\return \c FSSH_B_OK, if the directory has been created successfully, another
 *			error code otherwise.
 */

fssh_status_t
_kern_create_dir(int fd, const char *path, int perms)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return dir_create(fd, pathBuffer.LockBuffer(), perms, true);
}


fssh_status_t
_kern_remove_dir(int fd, const char *path)
{
	if (path) {
		KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != FSSH_B_OK)
			return FSSH_B_NO_MEMORY;

		return dir_remove(fd, pathBuffer.LockBuffer(), true);
	}

	return dir_remove(fd, NULL, true);
}


/**	\brief Reads the contents of a symlink referred to by a FD + path pair.
 *
 *	At least one of \a fd and \a path must be specified.
 *	If only \a fd is given, the function the symlink to be read is the node
 *	identified by this FD. If only a path is given, this path identifies the
 *	symlink to be read. If both are given and the path is absolute, \a fd is
 *	ignored; a relative path is reckoned off of the directory (!) identified
 *	by \a fd.
 *	If this function fails with FSSH_B_BUFFER_OVERFLOW, the \a _bufferSize pointer
 *	will still be updated to reflect the required buffer size.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. May be \c NULL.
 *	\param buffer The buffer into which the contents of the symlink shall be
 *		   written.
 *	\param _bufferSize A pointer to the size of the supplied buffer.
 *	\return The length of the link on success or an appropriate error code
 */

fssh_status_t
_kern_read_link(int fd, const char *path, char *buffer, fssh_size_t *_bufferSize)
{
	if (path) {
		KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != FSSH_B_OK)
			return FSSH_B_NO_MEMORY;

		return common_read_link(fd, pathBuffer.LockBuffer(), 
			buffer, _bufferSize, true);
	}

	return common_read_link(fd, NULL, buffer, _bufferSize, true);
}


/**	\brief Creates a symlink specified by a FD + path pair.
 *
 *	\a path must always be specified (it contains the name of the new symlink
 *	at least). If only a path is given, this path identifies the location at
 *	which the symlink shall be created. If both \a fd and \a path are given and
 *	the path is absolute, \a fd is ignored; a relative path is reckoned off
 *	of the directory (!) identified by \a fd.
 *
 *	\param fd The FD. May be < 0.
 *	\param toPath The absolute or relative path. Must not be \c NULL.
 *	\param mode The access permissions the new symlink shall have.
 *	\return \c FSSH_B_OK, if the symlink has been created successfully, another
 *			error code otherwise.
 */

fssh_status_t
_kern_create_symlink(int fd, const char *path, const char *toPath, int mode)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	KPath toPathBuffer(toPath, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK || toPathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	char *toBuffer = toPathBuffer.LockBuffer();

	fssh_status_t status = check_path(toBuffer);
	if (status < FSSH_B_OK)
		return status;

	return common_create_symlink(fd, pathBuffer.LockBuffer(), 
		toBuffer, mode, true);
}


fssh_status_t
_kern_create_link(const char *path, const char *toPath)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	KPath toPathBuffer(toPath, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK || toPathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return common_create_link(pathBuffer.LockBuffer(), 
		toPathBuffer.LockBuffer(), true);
}


/**	\brief Removes an entry specified by a FD + path pair from its directory.
 *
 *	\a path must always be specified (it contains at least the name of the entry
 *	to be deleted). If only a path is given, this path identifies the entry
 *	directly. If both \a fd and \a path are given and the path is absolute,
 *	\a fd is ignored; a relative path is reckoned off of the directory (!)
 *	identified by \a fd.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. Must not be \c NULL.
 *	\return \c FSSH_B_OK, if the entry has been removed successfully, another
 *			error code otherwise.
 */

fssh_status_t
_kern_unlink(int fd, const char *path)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return common_unlink(fd, pathBuffer.LockBuffer(), true);
}


/**	\brief Moves an entry specified by a FD + path pair to a an entry specified
 *		   by another FD + path pair.
 *
 *	\a oldPath and \a newPath must always be specified (they contain at least
 *	the name of the entry). If only a path is given, this path identifies the
 *	entry directly. If both a FD and a path are given and the path is absolute,
 *	the FD is ignored; a relative path is reckoned off of the directory (!)
 *	identified by the respective FD.
 *
 *	\param oldFD The FD of the old location. May be < 0.
 *	\param oldPath The absolute or relative path of the old location. Must not
 *		   be \c NULL.
 *	\param newFD The FD of the new location. May be < 0.
 *	\param newPath The absolute or relative path of the new location. Must not
 *		   be \c NULL.
 *	\return \c FSSH_B_OK, if the entry has been moved successfully, another
 *			error code otherwise.
 */

fssh_status_t
_kern_rename(int oldFD, const char *oldPath, int newFD, const char *newPath)
{
	KPath oldPathBuffer(oldPath, false, FSSH_B_PATH_NAME_LENGTH + 1);
	KPath newPathBuffer(newPath, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (oldPathBuffer.InitCheck() != FSSH_B_OK || newPathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return common_rename(oldFD, oldPathBuffer.LockBuffer(), 
		newFD, newPathBuffer.LockBuffer(), true);
}


fssh_status_t
_kern_access(const char *path, int mode)
{
	KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	return common_access(pathBuffer.LockBuffer(), mode, true);
}


/**	\brief Reads stat data of an entity specified by a FD + path pair.
 *
 *	If only \a fd is given, the stat operation associated with the type
 *	of the FD (node, attr, attr dir etc.) is performed. If only \a path is
 *	given, this path identifies the entry for whose node to retrieve the
 *	stat data. If both \a fd and \a path are given and the path is absolute,
 *	\a fd is ignored; a relative path is reckoned off of the directory (!)
 *	identified by \a fd and specifies the entry whose stat data shall be
 *	retrieved.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. Must not be \c NULL.
 *	\param traverseLeafLink If \a path is given, \c true specifies that the
 *		   function shall not stick to symlinks, but traverse them.
 *	\param stat The buffer the stat data shall be written into.
 *	\param statSize The size of the supplied stat buffer.
 *	\return \c FSSH_B_OK, if the the stat data have been read successfully, another
 *			error code otherwise.
 */

fssh_status_t
_kern_read_stat(int fd, const char *path, bool traverseLeafLink,
	fssh_struct_stat *stat, fssh_size_t statSize)
{
	fssh_struct_stat completeStat;
	fssh_struct_stat *originalStat = NULL;
	fssh_status_t status;

	if (statSize > sizeof(fssh_struct_stat))
		return FSSH_B_BAD_VALUE;

	// this supports different stat extensions
	if (statSize < sizeof(fssh_struct_stat)) {
		originalStat = stat;
		stat = &completeStat;
	}

	if (path) {
		// path given: get the stat of the node referred to by (fd, path)
		KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != FSSH_B_OK)
			return FSSH_B_NO_MEMORY;

		status = common_path_read_stat(fd, pathBuffer.LockBuffer(), 
			traverseLeafLink, stat, true);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor *descriptor
			= get_fd(get_current_io_context(true), fd);
		if (descriptor == NULL)
			return FSSH_B_FILE_ERROR;

		if (descriptor->ops->fd_read_stat)
			status = descriptor->ops->fd_read_stat(descriptor, stat);
		else
			status = FSSH_EOPNOTSUPP;

		put_fd(descriptor);
	}

	if (status == FSSH_B_OK && originalStat != NULL)
		fssh_memcpy(originalStat, stat, statSize);

	return status;
}


/**	\brief Writes stat data of an entity specified by a FD + path pair.
 *
 *	If only \a fd is given, the stat operation associated with the type
 *	of the FD (node, attr, attr dir etc.) is performed. If only \a path is
 *	given, this path identifies the entry for whose node to write the
 *	stat data. If both \a fd and \a path are given and the path is absolute,
 *	\a fd is ignored; a relative path is reckoned off of the directory (!)
 *	identified by \a fd and specifies the entry whose stat data shall be
 *	written.
 *
 *	\param fd The FD. May be < 0.
 *	\param path The absolute or relative path. Must not be \c NULL.
 *	\param traverseLeafLink If \a path is given, \c true specifies that the
 *		   function shall not stick to symlinks, but traverse them.
 *	\param stat The buffer containing the stat data to be written.
 *	\param statSize The size of the supplied stat buffer.
 *	\param statMask A mask specifying which parts of the stat data shall be
 *		   written.
 *	\return \c FSSH_B_OK, if the the stat data have been written successfully,
 *			another error code otherwise.
 */

fssh_status_t
_kern_write_stat(int fd, const char *path, bool traverseLeafLink,
	const fssh_struct_stat *stat, fssh_size_t statSize, int statMask)
{
	fssh_struct_stat completeStat;

	if (statSize > sizeof(fssh_struct_stat))
		return FSSH_B_BAD_VALUE;

	// this supports different stat extensions
	if (statSize < sizeof(fssh_struct_stat)) {
		fssh_memset((uint8_t *)&completeStat + statSize, 0, sizeof(fssh_struct_stat) - statSize);
		fssh_memcpy(&completeStat, stat, statSize);
		stat = &completeStat;
	}

	fssh_status_t status;

	if (path) {
		// path given: write the stat of the node referred to by (fd, path)
		KPath pathBuffer(path, false, FSSH_B_PATH_NAME_LENGTH + 1);
		if (pathBuffer.InitCheck() != FSSH_B_OK)
			return FSSH_B_NO_MEMORY;

		status = common_path_write_stat(fd, pathBuffer.LockBuffer(), 
			traverseLeafLink, stat, statMask, true);
	} else {
		// no path given: get the FD and use the FD operation
		struct file_descriptor *descriptor
			= get_fd(get_current_io_context(true), fd);
		if (descriptor == NULL)
			return FSSH_B_FILE_ERROR;

		if (descriptor->ops->fd_write_stat)
			status = descriptor->ops->fd_write_stat(descriptor, stat, statMask);
		else
			status = FSSH_EOPNOTSUPP;

		put_fd(descriptor);
	}

	return status;
}


int
_kern_open_attr_dir(int fd, const char *path)
{
	KPath pathBuffer(FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	if (path != NULL)
		pathBuffer.SetTo(path);

	return attr_dir_open(fd, path ? pathBuffer.LockBuffer() : NULL, true);
}


int
_kern_create_attr(int fd, const char *name, uint32_t type, int openMode)
{
	return attr_create(fd, name, type, openMode, true);
}


int
_kern_open_attr(int fd, const char *name, int openMode)
{
	return attr_open(fd, name, openMode, true);
}


fssh_status_t
_kern_remove_attr(int fd, const char *name)
{
	return attr_remove(fd, name, true);
}


fssh_status_t
_kern_rename_attr(int fromFile, const char *fromName, int toFile, const char *toName)
{
	return attr_rename(fromFile, fromName, toFile, toName, true);
}


int
_kern_open_index_dir(fssh_dev_t device)
{
	return index_dir_open(device, true);
}


fssh_status_t
_kern_create_index(fssh_dev_t device, const char *name, uint32_t type, uint32_t flags)
{
	return index_create(device, name, type, flags, true);
}


fssh_status_t
_kern_read_index_stat(fssh_dev_t device, const char *name, fssh_struct_stat *stat)
{
	return index_name_read_stat(device, name, stat, true);
}


fssh_status_t
_kern_remove_index(fssh_dev_t device, const char *name)
{
	return index_remove(device, name, true);
}


fssh_status_t
_kern_getcwd(char *buffer, fssh_size_t size)
{
	TRACE(("_kern_getcwd: buf %p, %ld\n", buffer, size));

	// Call vfs to get current working directory
	return get_cwd(buffer, size, true);
}


fssh_status_t
_kern_setcwd(int fd, const char *path)
{
	KPath pathBuffer(FSSH_B_PATH_NAME_LENGTH + 1);
	if (pathBuffer.InitCheck() != FSSH_B_OK)
		return FSSH_B_NO_MEMORY;

	if (path != NULL)
		pathBuffer.SetTo(path);

	return set_cwd(fd, path != NULL ? pathBuffer.LockBuffer() : NULL, true);
}


fssh_status_t
_kern_initialize_volume(const char* fsName, const char *partition,
	const char *name, const char *parameters)
{
	if (!fsName || ! partition)
		return FSSH_B_BAD_VALUE;

	// The partition argument should point to a real file/device.

	// open partition
	int fd = fssh_open(partition, FSSH_O_RDWR);
	if (fd < 0)
		return fssh_errno;

	// get the file system module
	fssh_file_system_module_info* fsModule = get_file_system(fsName);
	if (fsModule == NULL) {
		fssh_close(fd);
		return FSSH_ENODEV;
	}

	// initialize
	fssh_status_t status;
	if (fsModule->initialize) {
		status = (*fsModule->initialize)(fd, -1, name, parameters, -1);
			// We've got no partition or job IDs -- the FS will hopefully
			// ignore that.
	} else
		status = FSSH_B_NOT_SUPPORTED;

	// put the file system module, close partition
	put_file_system(fsModule);
	fssh_close(fd);

	return status;
}

}	// namespace FSShell
