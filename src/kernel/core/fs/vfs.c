/* Virtual File System */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <kernel.h>
#include <stage2.h>
#include <vfs.h>
#include <vm.h>
#include <vm_cache.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <thread.h>
#include <memheap.h>
#include <arch/cpu.h>
#include <elf.h>
#include <rootfs.h>
#include <bootfs.h>
#include <devfs.h>
#include <Errors.h>
#include <atomic.h>
#include <OS.h>
#include <fd.h>

#include <rootfs.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <resource.h>
#include <sys/stat.h>

#ifndef MAKE_NOIZE
#	define MAKE_NOIZE 0
#endif
#if MAKE_NOIZE
#	define PRINT(x) dprintf x
#	define FUNCTION(x) dprintf x
#else
#	define PRINT(x) ;
#	define FUNCTION(x) ;
#endif


struct vnode {
	struct vnode    *next;
	struct vnode    *mount_prev;
	struct vnode    *mount_next;
	struct vm_cache *cache;
	fs_id            fsid;
	vnode_id         vnid;
	fs_vnode         priv_vnode;
	struct fs_mount *mount;
	struct vnode    *covered_by;
	int              ref_count;
	bool             delete_me;
	bool             busy;
};

struct vnode_hash_key {
	fs_id      fsid;
	vnode_id   vnid;
};

struct fs_container {
	struct fs_container *next;
	struct fs_calls *calls;
	const char *name;
};
#define FS_CALL(vnode,call) (vnode->mount->fs->calls->call)
static struct fs_container *fs_list;

struct fs_mount {
	struct fs_mount     *next;
	struct fs_container *fs;
	fs_id                id;
	void                *cookie;
	char                *mount_point;
	recursive_lock       rlock;
	struct vnode        *root_vnode;
	struct vnode        *covers_vnode;
	struct vnode        *vnodes_head;
	struct vnode        *vnodes_tail;
	bool                 unmounting;
};

static mutex vfs_mutex;
static mutex vfs_mount_mutex;
static mutex vfs_mount_op_mutex;
static mutex vfs_vnode_mutex;

/* function declarations */
static int vfs_mount(char *path, const char *device, const char *fs_name, void *args, bool kernel);
static int vfs_unmount(char *path, bool kernel);
static int vfs_open(char *path, stream_type st, int omode, bool kernel);
static int vfs_seek(int fd, off_t pos, int seek_type, bool kernel);

static ssize_t vfs_read(struct file_descriptor *, void *, off_t, size_t *);
static ssize_t vfs_write(struct file_descriptor *, const void *, off_t, size_t *);
static int vfs_ioctl(struct file_descriptor *, ulong, void *buf, size_t len);
static status_t vfs_read_dir(struct file_descriptor *,struct dirent *buffer,size_t bufferSize,uint32 *_count);
static status_t vfs_rewind_dir(struct file_descriptor *);
static int vfs_rstat(struct file_descriptor *, struct stat *);
static int vfs_close(struct file_descriptor *, int, struct io_context *);
static void vfs_free_fd(struct file_descriptor *);

static int vfs_create(char *path, stream_type stream_type, void *args, bool kernel);

/* the vfs fd_ops structure */
struct fd_ops vfsops = {
	"vfs",
	vfs_read,
	vfs_write,
	vfs_ioctl,
	vfs_read_dir,
	vfs_rewind_dir,
	vfs_rstat,
	vfs_close,
	vfs_free_fd
};

#define VNODE_HASH_TABLE_SIZE 1024
static void *vnode_table;
static struct vnode *root_vnode;
static vnode_id next_vnode_id = 0;

#define MOUNTS_HASH_TABLE_SIZE 16
static void *mounts_table;
static fs_id next_fsid = 0;


static int
mount_compare(void *_m, const void *_key)
{
	struct fs_mount *mount = _m;
	const fs_id *id = _key;

	if(mount->id == *id)
		return 0;
	else
		return -1;
}


static unsigned int
mount_hash(void *_m, const void *_key, unsigned int range)
{
	struct fs_mount *mount = _m;
	const fs_id *id = _key;

	if(mount)
		return mount->id % range;
	else
		return *id % range;
}


static struct fs_mount *
fsid_to_mount(fs_id id)
{
	struct fs_mount *mount;

	mutex_lock(&vfs_mount_mutex);

	mount = hash_lookup(mounts_table, &id);

	mutex_unlock(&vfs_mount_mutex);

	return mount;
}


static int
vnode_compare(void *_v, const void *_key)
{
	struct vnode *v = _v;
	const struct vnode_hash_key *key = _key;

	if(v->fsid == key->fsid && v->vnid == key->vnid)
		return 0;
	else
		return -1;
}


static unsigned int
vnode_hash(void *_v, const void *_key, unsigned int range)
{
	struct vnode *v = _v;
	const struct vnode_hash_key *key = _key;

#define VHASH(fsid, vnid) (((uint32)((vnid)>>32) + (uint32)(vnid)) ^ (uint32)(fsid))

	if (v != NULL)
		return (VHASH(v->fsid, v->vnid) % range);
	else
		return (VHASH(key->fsid, key->vnid) % range);

#undef VHASH
}

/*
static int init_vnode(struct vnode *v)
{
	v->next = NULL;
	v->mount_prev = NULL;
	v->mount_next = NULL;
	v->priv_vnode = NULL;
	v->cache = NULL;
	v->ref_count = 0;
	v->vnid = 0;
	v->fsid = 0;
	v->delete_me = false;
	v->busy = false;
	v->covered_by = NULL;
	v->mount = NULL;

	return 0;
}
*/


static void
add_vnode_to_mount_list(struct vnode *v, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	v->mount_next = mount->vnodes_head;
	v->mount_prev = NULL;
	mount->vnodes_head = v;
	if(!mount->vnodes_tail)
		mount->vnodes_tail = v;

	recursive_lock_unlock(&mount->rlock);
}


static void
remove_vnode_from_mount_list(struct vnode *v, struct fs_mount *mount)
{
	recursive_lock_lock(&mount->rlock);

	if(v->mount_next)
		v->mount_next->mount_prev = v->mount_prev;
	else
		mount->vnodes_tail = v->mount_prev;
	if(v->mount_prev)
		v->mount_prev->mount_next = v->mount_next;
	else
		mount->vnodes_head = v->mount_next;

	v->mount_prev = v->mount_next = NULL;

	recursive_lock_unlock(&mount->rlock);
}


static struct vnode *
create_new_vnode(void)
{
	struct vnode *v;

	v = (struct vnode *)kmalloc(sizeof(struct vnode));
	if (v == NULL)
		return NULL;

	memset(v, 0, sizeof(struct vnode));//max_commit - old_store_commitment + commitment(v);
	return v;
}


static int
dec_vnode_ref_count(struct vnode *v, bool free_mem, bool r)
{
	int err;
	int old_ref;

	mutex_lock(&vfs_vnode_mutex);

	if (v->busy == true)
		panic("dec_vnode_ref_count called on vnode that was busy! vnode %p\n", v);

	old_ref = atomic_add(&v->ref_count, -1);

	PRINT(("dec_vnode_ref_count: vnode 0x%x, ref now %d\n", v, v->ref_count));

	if (old_ref == 1) {
		v->busy = true;

		mutex_unlock(&vfs_vnode_mutex);

		/* if we have a vm_cache attached, remove it */
		if (v->cache)
			vm_cache_release_ref((vm_cache_ref *)v->cache);
		v->cache = NULL;

		if (v->delete_me)
			v->mount->fs->calls->fs_removevnode(v->mount->cookie, v->priv_vnode, r);
		else
			v->mount->fs->calls->fs_putvnode(v->mount->cookie, v->priv_vnode, r);

		remove_vnode_from_mount_list(v, v->mount);

		mutex_lock(&vfs_vnode_mutex);
		hash_remove(vnode_table, v);
		mutex_unlock(&vfs_vnode_mutex);

		if (free_mem)
			kfree(v);
		err = 1;
	} else {
		mutex_unlock(&vfs_vnode_mutex);
		err = 0;
	}
	return err;
}


static int
inc_vnode_ref_count(struct vnode *v)
{
	atomic_add(&v->ref_count, 1);
	PRINT(("inc_vnode_ref_count: vnode 0x%x, ref now %d\n", v, v->ref_count));
	return 0;
}


static struct vnode *
lookup_vnode(fs_id fsid, vnode_id vnid)
{
	struct vnode_hash_key key;

	key.fsid = fsid;
	key.vnid = vnid;

	return hash_lookup(vnode_table, &key);
}


static int
get_vnode(fs_id fsid, vnode_id vnid, struct vnode **outv, int r)
{
	struct vnode *v;
	int err;

	FUNCTION(("get_vnode: fsid %d vnid 0x%x 0x%x\n", fsid, vnid));

	mutex_lock(&vfs_vnode_mutex);

	do {
		v = lookup_vnode(fsid, vnid);
		if (v) {
			if (v->busy) {
				mutex_unlock(&vfs_vnode_mutex);
				thread_snooze(10000); // 10 ms
				mutex_lock(&vfs_vnode_mutex);
				continue;
			}
		}
	} while(0);

	PRINT(("get_vnode: tried to lookup vnode, got 0x%x\n", v));

	if (v) {
		inc_vnode_ref_count(v);
	} else {
		// we need to create a new vnode and read it in
		v = create_new_vnode();
		if(!v) {
			err = ENOMEM;
			goto err;
		}
		v->fsid = fsid;
		v->vnid = vnid;
		v->mount = fsid_to_mount(fsid);
		if(!v->mount) {
			err = ERR_INVALID_HANDLE;
			goto err;
		}
		v->busy = true;
		hash_insert(vnode_table, v);
		mutex_unlock(&vfs_vnode_mutex);

		add_vnode_to_mount_list(v, v->mount);

		err = v->mount->fs->calls->fs_getvnode(v->mount->cookie, vnid, &v->priv_vnode, r);

		if(err < 0)
			remove_vnode_from_mount_list(v, v->mount);

		mutex_lock(&vfs_vnode_mutex);
		if(err < 0)
			goto err1;

		v->busy = false;
		v->ref_count = 1;
	}

	mutex_unlock(&vfs_vnode_mutex);

	PRINT(("get_vnode: returning 0x%x\n", v));

	*outv = v;
	return B_NO_ERROR;

err1:
	hash_remove(vnode_table, v);
err:
	mutex_unlock(&vfs_vnode_mutex);
	if(v)
		kfree(v);

	return err;
}


static void
put_vnode(struct vnode *v)
{
	dec_vnode_ref_count(v, true, false);
}


int
vfs_get_vnode(fs_id fsid, vnode_id vnid, fs_vnode *fsv)
{
	int err;
	struct vnode *v;

	err = get_vnode(fsid, vnid, &v, true);
	if (err < 0)
		return err;

	*fsv = v->priv_vnode;
	return B_NO_ERROR;
}


int
vfs_put_vnode(fs_id fsid, vnode_id vnid)
{
	struct vnode *v;

	mutex_lock(&vfs_vnode_mutex);

	v = lookup_vnode(fsid, vnid);

	mutex_unlock(&vfs_vnode_mutex);
	if (v)
		dec_vnode_ref_count(v, true, true);

	return B_NO_ERROR;
}


void
vfs_vnode_acquire_ref(void *v)
{
	FUNCTION(("vfs_vnode_acquire_ref: vnode 0x%x\n", v));
	inc_vnode_ref_count((struct vnode *)v);
}


void
vfs_vnode_release_ref(void *v)
{
	FUNCTION(("vfs_vnode_release_ref: vnode 0x%x\n", v));
	dec_vnode_ref_count((struct vnode *)v, true, false);
}


int
vfs_remove_vnode(fs_id fsid, vnode_id vnid)
{
	struct vnode *v;

	mutex_lock(&vfs_vnode_mutex);

	v = lookup_vnode(fsid, vnid);
	if(v)
		v->delete_me = true;

	mutex_unlock(&vfs_vnode_mutex);
	return 0;
}


void *
vfs_get_cache_ptr(void *vnode)
{
	return ((struct vnode *)vnode)->cache;
}


int
vfs_set_cache_ptr(void *vnode, void *cache)
{
	if(test_and_set((int *)&(((struct vnode *)vnode)->cache), (int)cache, 0) == 0)
		return 0;
	else
		return -1;
}


static void
vfs_free_fd(struct file_descriptor *f)
{
	if (f->vnode) {
		f->vnode->mount->fs->calls->fs_close(f->vnode->mount->cookie, f->vnode->priv_vnode, f->cookie);
		f->vnode->mount->fs->calls->fs_freecookie(f->vnode->mount->cookie, f->vnode->priv_vnode, f->cookie);
		dec_vnode_ref_count(f->vnode, true, false);
	}
}


static struct vnode *
get_vnode_from_fd(struct io_context *ioctx, int fd)
{
	struct file_descriptor *f;
	struct vnode *v;

	f = get_fd(ioctx, fd);
	if (!f)
		return NULL;

	v = f->vnode;
	if (!v)
		goto out;
	inc_vnode_ref_count(v);

out:
	put_fd(f);

	return v;
}


static struct fs_container *
find_fs(const char *fs_name)
{
	struct fs_container *fs = fs_list;
	while(fs != NULL) {
		if(strcmp(fs_name, fs->name) == 0) {
			return fs;
		}
		fs = fs->next;
	}
	return NULL;
}


static int
path_to_vnode(char *path, struct vnode **v, bool kernel)
{
	char *p = path;
	char *next_p;
	struct vnode *curr_v;
	struct vnode *next_v;
	vnode_id vnid;
	int err = 0;

	if (!p)
		return EINVAL;

	// figure out if we need to start at root or at cwd
	if (*p == '/') {
		while (*++p == '/')
			;
		curr_v = root_vnode;
		inc_vnode_ref_count(curr_v);
	} else {
		struct io_context *ioctx = get_current_io_context(kernel);
		mutex_lock(&ioctx->io_mutex);
		curr_v = ioctx->cwd;
		inc_vnode_ref_count(curr_v);
		mutex_unlock(&ioctx->io_mutex);
	}

	for (;;) {
//		dprintf("path_to_vnode: top of loop. p 0x%x, *p = %c, p = '%s'\n", p, *p, p);

		// done?
		if(*p == '\0') {
			err = 0;
			break;
		}

		// walk to find the next component
		for (next_p = p+1; *next_p != '\0' && *next_p != '/'; next_p++);

		if (*next_p == '/') {
			*next_p = '\0';
			do
				next_p++;
			while (*next_p == '/');
		}

		// see if the .. is at the root of a mount
		if (strcmp("..", p) == 0 && curr_v->mount->root_vnode == curr_v) {
			// move to the covered vnode so we pass the '..' parse to the underlying filesystem
			if (curr_v->mount->covers_vnode) {
				next_v = curr_v->mount->covers_vnode;
				inc_vnode_ref_count(next_v);
				dec_vnode_ref_count(curr_v, true, false);
				curr_v = next_v;
			}
		}

		// tell the filesystem to parse this path
		err = curr_v->mount->fs->calls->fs_lookup(curr_v->mount->cookie, curr_v->priv_vnode, p, &vnid);
		if (err < 0) {
			dec_vnode_ref_count(curr_v, true, false);
			goto out;
		}

		// lookup the vnode, the call to fs_lookup should have caused a get_vnode to be called
		// from inside the filesystem, thus the vnode would have to be in the list and it's
		// ref count incremented at this point
		mutex_lock(&vfs_vnode_mutex);
		next_v = lookup_vnode(curr_v->fsid, vnid);
		mutex_unlock(&vfs_vnode_mutex);

		if (!next_v) {
			// pretty screwed up here
			panic("path_to_vnode: could not lookup vnode (fsid 0x%x vnid 0x%Lx)\n", curr_v->fsid, vnid);
			err = ERR_VFS_PATH_NOT_FOUND;
			dec_vnode_ref_count(curr_v, true, false);
			goto out;
		}

		// decrease the ref count on the old dir we just looked up into
		dec_vnode_ref_count(curr_v, true, false);

		p = next_p;
		curr_v = next_v;

		// see if we hit a mount point
		if (curr_v->covered_by) {
			next_v = curr_v->covered_by;
			inc_vnode_ref_count(next_v);
			dec_vnode_ref_count(curr_v, true, false);
			curr_v = next_v;
		}
	}

	*v = curr_v;

out:
	return err;
}


/** Returns the vnode in the next to last segment of the path, and returns
 *	the last portion in filename.
 */

static int
path_to_dir_vnode(char *path, struct vnode **v, char *filename, bool kernel)
{
	char *p;

	p = strrchr(path, '/');
	if (!p) {
		// this path is single segment with no '/' in it
		// ex. "foo"
		strcpy(filename, path);
		strcpy(path, ".");
	} else {
		// replace the filename portion of the path with a '.'
		strcpy(filename, p+1);

		if(p[1] != '\0'){
			p[1] = '.';
			p[2] = '\0';
		}

	}
	return path_to_vnode(path, v, kernel);
}


//////////////////////////////////////////////////////////////////////////////////
//
// XXX -- Warning: horrible hack, just ahead!
//
// the following code implements a version of vnode_to_path()
// which takes an arbitrary vnode and generates the full file path string.
// this is used by vfs_get_cwd() so that the user shell can implement 'pwd'
//
// however...
//
// the method used is to peek into the private vnodes of the filesystem modules.
// this is *BAD MOJO* and only works because all of the current filesystems
// were implemented with a common structure, mimicked by the struct below.
//
// the hack was required because the VFS layer, as it was forked from NewOS,
// did not have a means of acquiring the path info thru a standard method.
// Once we have our own proper implementation of the VFS layer, then the
// gross hack below can be removed...
// 

int vnode_to_path(struct vnode *, char *, int);


typedef struct generic_vnode {
	struct generic_vnode *all_next;
	vnode_id              id;
	char                 *name;
	void                 *redir_vnode;
	struct generic_vnode *parent;
	struct generic_vnode *dir_next;
}
generic_vnode;


int vnode_to_path(struct vnode *v, char *buf, int buflen)
{
	int rc;  // return code

	if ((v == NULL) || (buf == NULL) || (buflen <= 0))
		return EINVAL;
	
	//
	// ask the filesystem containing the given vnode to fetch
	// its local vnode structure for us (returned in v->priv_vnode).
	// this fs vnode contains the leaf name and parent link required
	//
	rc = v->mount->
	     fs->calls->
         fs_getvnode(v->mount->cookie, v->vnid, &v->priv_vnode, true);
	if (rc < 0)
		return rc;
	
	else {
		//
		// construct the full path, which is:
		// {mount_point}/{relative_path}
		//
		// the mount point has already been stored,
		// but the relative path has to be generated on-the-fly
		//
		generic_vnode *g = (generic_vnode *) v->priv_vnode;

		char *mpoint     = v->mount->mount_point;
		int   mpointlen  = strlen (mpoint);
		
		char rpath[SYS_MAX_NAME_LEN];  // buffer to hold relative path
		int  i = sizeof rpath;         // current insertion point
		int  rpathlen = 0;
		int  len;

		//
		// generate relative path:
		// start with given leaf vnode and back up to mount point,
		// copying leaf names into buffer (right to left) as we go
		//
		rpath[--i] = '\0';
		
		for (; g; g = g->parent) {
			// 
			if (g->name[0]) {
				len = strlen (g->name);
				i -= len;
				memcpy (rpath+i, g->name, len);
				rpathlen += len;
			}
			if (g->parent) {
				if (g->parent->id == g->id)
					// hit mount point
					break;
				
				rpath[--i] = '/';
				++rpathlen;
			}
		}
		
		if ((mpointlen + rpathlen) > buflen)
			return ENOBUFS;
		
		//
		// copy results to output buffer
		//
		strcpy (buf, mpoint);
		if (rpathlen > 0)
			strcat (buf, rpath+i);
		
		return B_OK;
	}
}


//
//
// XXX - end of the vnode_to_path() hack
//
//////////////////////////////////////////////////////////////////////////////////


/** Sets up a new io_control structure, and inherits the properties
 *	of the parent io_control if it is given.
 */

void *
vfs_new_io_context(void *_parent_ioctx)
{
	size_t table_size;
	struct io_context *ioctx;
	struct io_context *parent_ioctx;

	parent_ioctx = (struct io_context *)_parent_ioctx;
	if (parent_ioctx)
		table_size = parent_ioctx->table_size;
	else
		table_size = DEFAULT_FD_TABLE_SIZE;

	ioctx = kmalloc(sizeof(struct io_context));
	if (ioctx == NULL)
		return NULL;

	memset(ioctx, 0, sizeof(struct io_context));

	ioctx->fds = kmalloc(sizeof(struct file_descriptor *) * table_size);
	if (ioctx->fds == NULL) {
		kfree(ioctx);
		return NULL;
	}

	memset(ioctx->fds, 0, sizeof(struct file_descriptor *) * table_size);

	if (mutex_init(&ioctx->io_mutex, "ioctx_mutex") < 0) {
		kfree(ioctx->fds);
		kfree(ioctx);
		return NULL;
	}

	/*
	 * copy parent files
	 */
	if (parent_ioctx) {
		size_t i;

		mutex_lock(&parent_ioctx->io_mutex);

		ioctx->cwd= parent_ioctx->cwd;
		if (ioctx->cwd) {
			inc_vnode_ref_count(ioctx->cwd);
		}


		for (i = 0; i< table_size; i++) {
			if (parent_ioctx->fds[i]) {
				ioctx->fds[i]= parent_ioctx->fds[i];
				atomic_add(&ioctx->fds[i]->ref_count, 1);
			}
		}

		mutex_unlock(&parent_ioctx->io_mutex);
	} else {
		ioctx->cwd = root_vnode;

		if (ioctx->cwd) {
			inc_vnode_ref_count(ioctx->cwd);
		}
	}

	ioctx->table_size = table_size;

	return ioctx;
}


int
vfs_free_io_context(void *_ioctx)
{
	struct io_context *ioctx = (struct io_context *)_ioctx;
	int i;

	if (ioctx->cwd)
		dec_vnode_ref_count(ioctx->cwd, true, false);

	mutex_lock(&ioctx->io_mutex);

	for (i = 0; i < ioctx->table_size; i++) {
		if (ioctx->fds[i])
			put_fd(ioctx->fds[i]);
	}

	mutex_unlock(&ioctx->io_mutex);

	mutex_destroy(&ioctx->io_mutex);

	kfree(ioctx->fds);
	kfree(ioctx);
	return 0;
}


static int
vfs_mount(char *path, const char *device, const char *fs_name, void *args, bool kernel)
{
	struct fs_mount *mount;
	int err = 0;
	struct vnode *covered_vnode = NULL;
	vnode_id root_id;

#if MAKE_NOIZE
	dprintf("vfs_mount: entry. path = '%s', fs_name = '%s'\n", path, fs_name);
#endif

	mutex_lock(&vfs_mount_op_mutex);

	mount = (struct fs_mount *)kmalloc(sizeof(struct fs_mount));
	if(mount == NULL)  {
		err = ENOMEM;
		goto err;
	}

	mount->mount_point = kstrdup(path);
	if(mount->mount_point == NULL) {
		err = ENOMEM;
		goto err1;
	}

	mount->fs = find_fs(fs_name);
	if(mount->fs == NULL) {
		err = ERR_VFS_INVALID_FS;
		goto err2;
	}

	recursive_lock_create(&mount->rlock);
	mount->id = next_fsid++;
	mount->unmounting = false;

	if(!root_vnode) {
		// we haven't mounted anything yet
		if(strcmp(path, "/") != 0) {
			err = ERR_VFS_GENERAL;
			goto err3;
		}

		err = mount->fs->calls->fs_mount(&mount->cookie, mount->id, device, NULL, &root_id);
		if(err < 0) {
			err = ERR_VFS_GENERAL;
			goto err3;
		}

		mount->covers_vnode = NULL; // this is the root mount
	} else {
		err = path_to_vnode(path,&covered_vnode,kernel);
		if(err < 0) {
			goto err2;
		}

		if(!covered_vnode) {
			err = ERR_VFS_GENERAL;
			goto err2;
		}

		// XXX insert check to make sure covered_vnode is a DIR, or maybe it's okay for it not to be

		if((covered_vnode != root_vnode) && (covered_vnode->mount->root_vnode == covered_vnode)){
			err = ERR_VFS_ALREADY_MOUNTPOINT;
			goto err2;
		}

		mount->covers_vnode = covered_vnode;

		// mount it
		err = mount->fs->calls->fs_mount(&mount->cookie, mount->id, device, NULL, &root_id);
		if(err < 0)
			goto err4;
	}

	mutex_lock(&vfs_mount_mutex);

	// insert mount struct into list
	hash_insert(mounts_table, mount);

	mutex_unlock(&vfs_mount_mutex);

	err = get_vnode(mount->id, root_id, &mount->root_vnode, 0);
	if(err < 0)
		goto err5;

	// XXX may be a race here
	if(mount->covers_vnode)
		mount->covers_vnode->covered_by = mount->root_vnode;

	if(!root_vnode)
		root_vnode = mount->root_vnode;

	mutex_unlock(&vfs_mount_op_mutex);

	return 0;

err5:
	mount->fs->calls->fs_unmount(mount->cookie);
err4:
	if(mount->covers_vnode)
		dec_vnode_ref_count(mount->covers_vnode, true, false);
err3:
	recursive_lock_destroy(&mount->rlock);
err2:
	kfree(mount->mount_point);
err1:
	kfree(mount);
err:
	mutex_unlock(&vfs_mount_op_mutex);

	return err;
}


static int
vfs_unmount(char *path, bool kernel)
{
	struct vnode *v;
	struct fs_mount *mount;
	int err;

#if MAKE_NOIZE
	dprintf("vfs_unmount: entry. path = '%s', kernel %d\n", path, kernel);
#endif

	err = path_to_vnode(path, &v, kernel);
	if(err < 0) {
		err = ERR_VFS_PATH_NOT_FOUND;
		goto err;
	}

	mutex_lock(&vfs_mount_op_mutex);

	mount = fsid_to_mount(v->fsid);
	if(!mount) {
		panic("vfs_unmount: fsid_to_mount failed on root vnode @%p of mount\n", v);
	}

	if(mount->root_vnode != v) {
		// not mountpoint
		dec_vnode_ref_count(v, true, false);
		err = ERR_VFS_NOT_MOUNTPOINT;
		goto err1;
	}

	/* grab the vnode master mutex to keep someone from creating a vnode
	while we're figuring out if we can continue */
	mutex_lock(&vfs_vnode_mutex);

	/* simulate the root vnode having it's refcount decremented */
	mount->root_vnode->ref_count -= 2;

	/* cycle through the list of vnodes associated with this mount and
	make sure all of them are not busy or have refs on them */
	err = 0;
	for(v = mount->vnodes_head; v; v = v->mount_next) {
		if(v->busy || v->ref_count != 0) {
			mount->root_vnode->ref_count += 2;
			mutex_unlock(&vfs_vnode_mutex);
			dec_vnode_ref_count(mount->root_vnode, true, false);
			err = EBUSY;
			goto err1;
		}
	}

	/* we can safely continue, mark all of the vnodes busy and this mount
	structure in unmounting state */
	for(v = mount->vnodes_head; v; v = v->mount_next)
		if(v != mount->root_vnode)
			v->busy = true;
	mount->unmounting = true;

	mutex_unlock(&vfs_vnode_mutex);

	mount->covers_vnode->covered_by = NULL;
	dec_vnode_ref_count(mount->covers_vnode, true, false);

	/* release the ref on the root vnode twice */
	dec_vnode_ref_count(mount->root_vnode, true, false);
	dec_vnode_ref_count(mount->root_vnode, true, false);

	// XXX when full vnode cache in place, will need to force
	// a putvnode/removevnode here

	/* remove the mount structure from the hash table */
	mutex_lock(&vfs_mount_mutex);
	hash_remove(mounts_table, mount);
	mutex_unlock(&vfs_mount_mutex);

	mutex_unlock(&vfs_mount_op_mutex);

	mount->fs->calls->fs_unmount(mount->cookie);

	kfree(mount->mount_point);
	kfree(mount);

	return 0;

err1:
	mutex_unlock(&vfs_mount_op_mutex);
err:
	return err;
}


static int
vfs_sync(void)
{
	struct hash_iterator iter;
	struct fs_mount *mount;

#if MAKE_NOIZE
	dprintf("vfs_sync: entry.\n");
#endif

	/* cycle through and call sync on each mounted fs */
	mutex_lock(&vfs_mount_op_mutex);
	mutex_lock(&vfs_mount_mutex);

	hash_open(mounts_table, &iter);
	while((mount = hash_next(mounts_table, &iter))) {
		mount->fs->calls->fs_sync(mount->cookie);
	}
	hash_close(mounts_table, &iter, false);

	mutex_unlock(&vfs_mount_mutex);
	mutex_unlock(&vfs_mount_op_mutex);

	return 0;
}


static int
vfs_open(char *path, stream_type st, int omode, bool kernel)
{
	int fd;
	struct vnode *v;
	file_cookie cookie;
	struct file_descriptor *f;
	int err;

//dprintf("vfs_open: %s: omode = %d\n", path, omode);
#if MAKE_NOIZE
	dprintf("vfs_open: entry. path = '%s', omode %d, kernel %d\n", path, omode, kernel);
#endif

	err = path_to_vnode(path, &v, kernel);
	if (err < 0)
		goto err;

//dprintf("calling fs_open, v= %p\n", v);

	err = v->mount->fs->calls->fs_open(v->mount->cookie, v->priv_vnode, &cookie, st, omode);
	if (err < 0)
		goto err1;

	// file is opened, create a fd
	f = alloc_fd();
	if (!f) {
		// xxx leaks
		err = ENOMEM;
		goto err1;
	}
	f->vnode = v;
	f->cookie = cookie;
	f->ops = &vfsops;
	f->type = FDTYPE_VNODE;

	fd = new_fd(get_current_io_context(kernel), f);
	if (fd < 0) {
		err = ERR_VFS_FD_TABLE_FULL;
		goto err1;
	}

	return fd;

	/* ??? - will this ever be called??? */
	vfs_free_fd(f);
err1:
	dec_vnode_ref_count(v, true, false);
err:
	return err;
}


static int
vfs_close(struct file_descriptor *f, int fd, struct io_context *io)
{
	remove_fd(io, fd);

	return 0;
}


static int
vfs_fsync(int fd, bool kernel)
{
	struct file_descriptor *f;
	struct vnode *v;
	int err;

#if MAKE_NOIZE
	dprintf("vfs_fsync: entry. fd %d kernel %d\n", fd, kernel);
#endif

	f = get_fd(get_current_io_context(kernel), fd);
	if (!f)
		return ERR_INVALID_HANDLE;

	v = f->vnode;
	err = v->mount->fs->calls->fs_fsync(v->mount->cookie, v->priv_vnode);

	put_fd(f);

	return err;
}


static ssize_t
vfs_read(struct file_descriptor *f, void *buffer, off_t pos, size_t *length)
{
	struct vnode *v = f->vnode;

	FUNCTION(("vfs_read: fd = %d, buf 0x%x, pos 0x%x 0x%x, len 0x%x, kernel %d\n", fd, buffer, pos, length, kernel));

	return v->mount->fs->calls->fs_read(v->mount->cookie, v->priv_vnode, f->cookie, buffer, pos, length);
}


static ssize_t
vfs_write(struct file_descriptor *f, const void *buffer, off_t pos, size_t *length)
{
	struct vnode *v = f->vnode;

	FUNCTION(("vfs_write: fd = %d, buf 0x%x, pos 0x%x 0x%x, len 0x%x\n", fd, buffer, pos, length));

	return v->mount->fs->calls->fs_write(v->mount->cookie, v->priv_vnode, f->cookie, buffer, pos, length);
}


static int
vfs_seek(int fd, off_t pos, int seek_type, bool kernel)
{
	struct vnode *v;
	struct file_descriptor *f;
	int err;

#if MAKE_NOIZE
	dprintf("vfs_seek: fd = %d, pos 0x%x 0x%x, seek_type %d, kernel %d\n", fd, pos, seek_type, kernel);
#endif

	f = get_fd(get_current_io_context(kernel), fd);
	if(!f) {
		err = EBADF;
		goto err;
	}

	v = f->vnode;
	err = v->mount->fs->calls->fs_seek(v->mount->cookie, v->priv_vnode, f->cookie, pos, seek_type);

	put_fd(f);

err:
	return err;
}


static status_t 
vfs_read_dir(struct file_descriptor *descriptor, struct dirent *buffer, size_t bufferSize, uint32 *_count)
{
	struct vnode *vnode = descriptor->vnode;

	if (FS_CALL(vnode,fs_read_dir))
		return FS_CALL(vnode,fs_read_dir)(vnode->mount->cookie,vnode->priv_vnode,descriptor->cookie,buffer,bufferSize,_count);
	
	return EOPNOTSUPP;
}


static status_t 
vfs_rewind_dir(struct file_descriptor *descriptor)
{
	struct vnode *vnode = descriptor->vnode;

	if (FS_CALL(vnode,fs_rewind_dir))
		return FS_CALL(vnode,fs_rewind_dir)(vnode->mount->cookie,vnode->priv_vnode,descriptor->cookie);

	return EOPNOTSUPP;
}


static int
vfs_ioctl(struct file_descriptor *f, ulong op, void *buf, size_t len)
{
	struct vnode *v = f->vnode;

	FUNCTION(("vfs_ioctl: f = %p, op 0x%x, buf 0x%x, len 0x%x\n", f, op, buffer, length));

	return v->mount->fs->calls->fs_ioctl(v->mount->cookie, v->priv_vnode, f->cookie, op, buf, len);
}


static int
vfs_create(char *path, stream_type stream_type, void *args, bool kernel)
{
	int err;
	struct vnode *v;
	char filename[SYS_MAX_NAME_LEN];
	vnode_id vnid;

	FUNCTION(("vfs_create: path '%s', stream_type %d, args 0x%x, kernel %d\n", path, stream_type, args, kernel));

	err = path_to_dir_vnode(path, &v, filename, kernel);
	if(err < 0)
		goto err;

	err = v->mount->fs->calls->fs_create(v->mount->cookie, v->priv_vnode, filename, stream_type, args, &vnid);

	dec_vnode_ref_count(v, true, false);
err:
	return err;
}


static int
vfs_unlink(char *path, bool kernel)
{
	int err;
	struct vnode *v;
	char filename[SYS_MAX_NAME_LEN];

	FUNCTION(("vfs_unlink: path '%s', kernel %d\n", path, kernel));

	err = path_to_dir_vnode(path, &v, filename, kernel);
	if(err < 0)
		goto err;

	err = v->mount->fs->calls->fs_unlink(v->mount->cookie, v->priv_vnode, filename);
	dec_vnode_ref_count(v, true, false);
err:
	return err;
}


static int
vfs_rename(char *path, char *newpath, bool kernel)
{
	struct vnode *v1, *v2;
	char filename1[SYS_MAX_NAME_LEN];
	char filename2[SYS_MAX_NAME_LEN];
	int err;

	err = path_to_dir_vnode(path, &v1, filename1, kernel);
	if (err < 0)
		goto err;

	err = path_to_dir_vnode(newpath, &v2, filename2, kernel);
	if (err < 0)
		goto err1;

	if (v1->fsid != v2->fsid) {
		err = ERR_VFS_CROSS_FS_RENAME;
		goto err2;
	}

	if (v1->mount->fs->calls->fs_rename != NULL)
		err = v1->mount->fs->calls->fs_rename(v1->mount->cookie, v1->priv_vnode, filename1, v2->priv_vnode, filename2);
	else
		err = EINVAL;

err2:
	dec_vnode_ref_count(v2, true, false);
err1:
	dec_vnode_ref_count(v1, true, false);
err:
	return err;
}


static int
vfs_rstat(struct file_descriptor *f, struct stat *stat)
{
	int err;
	struct vnode *v = f->vnode;

	FUNCTION(("vfs_rstat: path '%s', stat 0x%x\n", path, stat));

	dprintf("vfs_rstat (%p, %p)\n", f, stat);
	err = v->mount->fs->calls->fs_rstat(v->mount->cookie, v->priv_vnode, stat);
	dprintf("vfs_rstat: fs_stat gave %d\n", err);
	return err;
}


static int
vfs_wstat(char *path, struct stat *stat, int stat_mask, bool kernel)
{
	int err;
	struct vnode *v;

	FUNCTION(("vfs_wstat: path '%s', stat 0x%x, stat_mask %d, kernel %d\n", path, stat, stat_mask, kernel));

	err = path_to_vnode(path, &v, kernel);
	if (err < 0)
		goto err;

	err = v->mount->fs->calls->fs_wstat(v->mount->cookie, v->priv_vnode, stat, stat_mask);

	dec_vnode_ref_count(v, true, false);
err:
	return err;
}


int
vfs_get_vnode_from_fd(int fd, bool kernel, void **vnode)
{
	struct io_context *ioctx;

	ioctx = get_current_io_context(kernel);
	*vnode = get_vnode_from_fd(ioctx, fd);

	if (*vnode == NULL)
		return ERR_INVALID_HANDLE;

	return B_NO_ERROR;
}


int
vfs_get_vnode_from_path(const char *path, bool kernel, void **vnode)
{
	struct vnode *v;
	int err;
	char buf[SYS_MAX_PATH_LEN+1];

#if MAKE_NOIZE
	dprintf("vfs_get_vnode_from_path: entry. path = '%s', kernel %d\n", path, kernel);
#endif

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	err = path_to_vnode(buf, &v, kernel);
	if(err < 0)
		goto err;

	*vnode = v;

err:
	return err;
}


int
vfs_put_vnode_ptr(void *vnode)
{
	struct vnode *v = vnode;

	put_vnode(v);

	return 0;
}


ssize_t
vfs_canpage(void *_v)
{
	struct vnode *v = _v;

#if MAKE_NOIZE
	dprintf("vfs_canpage: vnode 0x%x\n", v);
#endif

	return v->mount->fs->calls->fs_canpage(v->mount->cookie, v->priv_vnode);
}


ssize_t
vfs_readpage(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *v = _v;

#if MAKE_NOIZE
	dprintf("vfs_readpage: vnode 0x%x, vecs 0x%x, pos 0x%x 0x%x\n", v, vecs, pos);
#endif

	return v->mount->fs->calls->fs_readpage(v->mount->cookie, v->priv_vnode, vecs, pos);
}


ssize_t
vfs_writepage(void *_v, iovecs *vecs, off_t pos)
{
	struct vnode *v = _v;

#if MAKE_NOIZE
	dprintf("vfs_writepage: vnode 0x%x, vecs 0x%x, pos 0x%x 0x%x\n", v, vecs, pos);
#endif

	return v->mount->fs->calls->fs_writepage(v->mount->cookie, v->priv_vnode, vecs, pos);
}


//
// XXX -- Fixme: uses vnode_to_path() hack
//

static int
vfs_get_cwd(char* buf, size_t size, bool kernel)
{
	// Get current working directory from io context

#if MAKE_NOIZE
	dprintf("vfs_get_cwd: buf 0x%x, 0x%x\n", buf, size);
#endif

	{
	struct vnode* cwd = get_current_io_context(kernel)->cwd;
	if (cwd)
		//
		// vnode_to_path() takes the cwd vnode and computes
		// a full path string from it, which is then copied
		// into the given buffer.
		//
		// WARNING: the current version of this function
		// utilizes a gross hack and will be removed and/or
		// replaced in the future...
		//
		return vnode_to_path(cwd, buf, size);
	else
		return B_ERROR; 
	}
}


static int
vfs_set_cwd(char* path, bool kernel)
{
	struct io_context* curr_ioctx;
	struct vnode* v = NULL;
	struct vnode* old_cwd;
	struct stat stat;
	int rc;

#if MAKE_NOIZE
	dprintf("vfs_set_cwd: path=\'%s\'\n", path);
#endif

	// Get vnode for passed path, and bail if it failed
	rc = path_to_vnode(path, &v, kernel);
	if (rc < 0) {
		goto err;
	}

	rc = v->mount->fs->calls->fs_rstat(v->mount->cookie, v->priv_vnode, &stat);
	if(rc < 0) {
		goto err1;
	}

	if(!S_ISDIR(stat.st_mode)) {
		// nope, can't cwd to here
		rc = ERR_VFS_WRONG_STREAM_TYPE;
		goto err1;
	}

	// Get current io context and lock
	curr_ioctx = get_current_io_context(kernel);
	mutex_lock(&curr_ioctx->io_mutex);

	// save the old cwd
	old_cwd = curr_ioctx->cwd;

	// Set the new vnode
	curr_ioctx->cwd = v;

	// Unlock the ioctx
	mutex_unlock(&curr_ioctx->io_mutex);

	// Decrease ref count of previous working dir (as the ref is being replaced)
	if (old_cwd)
		dec_vnode_ref_count(old_cwd, true, false);

	return B_NO_ERROR;

err1:
	dec_vnode_ref_count(v, true, false);
err:
	return rc;
}


static int
vfs_dup(int fd, bool kernel)
{
	struct io_context *io = get_current_io_context(kernel);
	struct file_descriptor *f;
	int rc;

#if MAKE_NOIZE
	dprintf("vfs_dup: fd=%d\n", fd);
#endif

	// Try to get the fd structure
	f = get_fd(io, fd);
	if (!f) {
		rc = ERR_INVALID_HANDLE;
		goto err;
	}

	// now put the fd in place
	rc = new_fd(io, f);

	if (rc < 0)
		put_fd(f);

err:
	return rc;
}


static int
vfs_dup2(int ofd, int nfd, bool kernel)
{
	struct io_context *curr_ioctx;
	struct file_descriptor *evicted;
	int rc;

#if MAKE_NOIZE
	dprintf("vfs_dup2: ofd=%d nfd=%d\n", ofd, nfd);
#endif

	// quick check
	if ((ofd < 0) || (nfd < 0)) {
		rc = ERR_INVALID_HANDLE;
		goto err;
	}

	// Get current io context and lock
	curr_ioctx = get_current_io_context(kernel);
	mutex_lock(&curr_ioctx->io_mutex);

	// Check for upper boundary, we do in the locked part
	// because in the future the fd table might be resizeable
	if ((ofd >= curr_ioctx->table_size) || !curr_ioctx->fds[ofd]) {
		rc = ERR_INVALID_HANDLE;
		goto err_1;
	}
	if ((nfd >= curr_ioctx->table_size)) {
		rc = ERR_INVALID_HANDLE;
		goto err_1;
	}

	// Check for identity, note that it cannot be made above
	// because we always want to return an error on invalid
	// handles
	if (ofd == nfd)
		// ToDo: this is buggy and doesn't unlock the io mutex!
		goto success;

	// Now do the work
	evicted = curr_ioctx->fds[nfd];
	curr_ioctx->fds[nfd] = curr_ioctx->fds[ofd];
	atomic_add(&curr_ioctx->fds[ofd]->ref_count, 1);

	// Unlock the ioctx
	mutex_unlock(&curr_ioctx->io_mutex);

	// Say bye bye to the evicted fd
	if (evicted)
		put_fd(evicted);

success:
	rc = nfd;
	return nfd;

err_1:
	mutex_unlock(&curr_ioctx->io_mutex);
err:
	return rc;
}


//	#pragma mark -
//	Calls from within the kernel


int
sys_mount(const char *path, const char *device, const char *fs_name, void *args)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_mount(buf, device, fs_name, args, true);
}


int
sys_unmount(const char *path)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_unmount(buf, true);
}


int
sys_sync(void)
{
	return vfs_sync();
}


int
sys_open(const char *path, stream_type st, int omode)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_open(buf, st, omode, true);
}


int
sys_fsync(int fd)
{
	return vfs_fsync(fd, true);
}


int
sys_seek(int fd, off_t pos, int seek_type)
{
	return vfs_seek(fd, pos, seek_type, true);
}


int
sys_create(const char *path, stream_type stream_type)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_create(buf, stream_type, NULL, true);
}


int
sys_unlink(const char *path)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_unlink(buf, true);
}


int
sys_rename(const char *oldpath, const char *newpath)
{
	char buf1[SYS_MAX_PATH_LEN+1];
	char buf2[SYS_MAX_PATH_LEN+1];

	strncpy(buf1, oldpath, SYS_MAX_PATH_LEN);
	buf1[SYS_MAX_PATH_LEN] = 0;

	strncpy(buf2, newpath, SYS_MAX_PATH_LEN);
	buf2[SYS_MAX_PATH_LEN] = 0;

	return vfs_rename(buf1, buf2, true);
}


int
sys_rstat(const char *path, struct stat *stat)
{
	char buf[SYS_MAX_PATH_LEN+1];
	int err;
	struct vnode *v;

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

#if MAKE_NOIZE
	dprintf("sys_rstat: path '%s', stat 0x%x,\n", path, stat);
#endif
	err = path_to_vnode(buf, &v, true);
	if(err < 0)
		goto err;

	err = v->mount->fs->calls->fs_rstat(v->mount->cookie, v->priv_vnode, stat);

	dec_vnode_ref_count(v, true, false);
err:
	return err;
}


int
sys_wstat(const char *path, struct stat *stat, int stat_mask)
{
	char buf[SYS_MAX_PATH_LEN+1];

	strncpy(buf, path, SYS_MAX_PATH_LEN);
	buf[SYS_MAX_PATH_LEN] = 0;

	return vfs_wstat(buf, stat, stat_mask, true);
}


char *
sys_getcwd(char *buf, size_t size)
{
	char path[SYS_MAX_PATH_LEN];
	int rc;

#if MAKE_NOIZE
	dprintf("sys_getcwd: buf 0x%x, 0x%x\n", buf, size);
#endif

	// Call vfs to get current working directory
	rc = vfs_get_cwd(path,SYS_MAX_PATH_LEN-1,true);
	path[SYS_MAX_PATH_LEN-1] = 0;

	// Copy back the result
	strncpy(buf,path,size);

	// Return either NULL or the buffer address to indicate failure or success
	return  (rc < 0) ? NULL : buf;
}


int
sys_setcwd(const char* _path)
{
	char path[SYS_MAX_PATH_LEN];

#if MAKE_NOIZE
	dprintf("sys_setcwd: path=0x%x\n", _path);
#endif

	// Copy new path to kernel space
	strncpy(path, _path, SYS_MAX_PATH_LEN-1);
	path[SYS_MAX_PATH_LEN-1] = 0;

	// Call vfs to set new working directory
	return vfs_set_cwd(path,true);
}


int
sys_dup(int fd)
{
	return vfs_dup(fd, true);
}


int
sys_dup2(int ofd, int nfd)
{
	return vfs_dup2(ofd, nfd, true);
}


//	#pragma mark -
//	Calls from userland (with extra address checks)


int
user_mount(const char *upath, const char *udevice, const char *ufs_name, void *args)
{
	char path[SYS_MAX_PATH_LEN+1];
	char fs_name[SYS_MAX_OS_NAME_LEN+1];
	char device[SYS_MAX_PATH_LEN+1];
	int rc;

	if ((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if ((addr)ufs_name >= KERNEL_BASE && (addr)ufs_name <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if (udevice) {
		if((addr)udevice >= KERNEL_BASE && (addr)udevice <= KERNEL_TOP)
			return ERR_VM_BAD_USER_MEMORY;
	}

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN);
	if (rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN] = 0;

	rc = user_strncpy(fs_name, ufs_name, SYS_MAX_OS_NAME_LEN);
	if (rc < 0)
		return rc;
	fs_name[SYS_MAX_OS_NAME_LEN] = 0;

	if (udevice) {
		rc = user_strncpy(device, udevice, SYS_MAX_PATH_LEN);
		if(rc < 0)
			return rc;
		device[SYS_MAX_PATH_LEN] = 0;
	} else {
		device[0] = 0;
	}

	return vfs_mount(path, device, fs_name, args, false);
}


int
user_unmount(const char *upath)
{
	char path[SYS_MAX_PATH_LEN+1];
	int rc;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN);
	if (rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN] = 0;

	return vfs_unmount(path, false);
}


int
user_sync(void)
{
	return vfs_sync();
}


int
user_open(const char *upath, stream_type st, int omode)
{
	char path[SYS_MAX_PATH_LEN];
	int rc;

dprintf("user_open\n");

	if ((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN-1] = 0;
dprintf("calling vfs_open(%s)\n", path);

	return vfs_open(path, st, omode, false);
}


int
user_fsync(int fd)
{
	return vfs_fsync(fd, false);
}


int
user_seek(int fd, off_t pos, int seek_type)
{
	return vfs_seek(fd, pos, seek_type, false);
}


int
user_create(const char *upath, stream_type stream_type)
{
	char path[SYS_MAX_PATH_LEN];
	int rc;

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN-1] = 0;

	return vfs_create(path, stream_type, NULL, false);
}


int
user_unlink(const char *upath)
{
	char path[SYS_MAX_PATH_LEN+1];
	int rc;

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN] = 0;

	return vfs_unlink(path, false);
}


int
user_rename(const char *uoldpath, const char *unewpath)
{
	char oldpath[SYS_MAX_PATH_LEN+1];
	char newpath[SYS_MAX_PATH_LEN+1];
	int rc;

	if((addr)uoldpath >= KERNEL_BASE && (addr)uoldpath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)unewpath >= KERNEL_BASE && (addr)unewpath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(oldpath, uoldpath, SYS_MAX_PATH_LEN);
	if(rc < 0)
		return rc;
	oldpath[SYS_MAX_PATH_LEN] = 0;

	rc = user_strncpy(newpath, unewpath, SYS_MAX_PATH_LEN);
	if(rc < 0)
		return rc;
	newpath[SYS_MAX_PATH_LEN] = 0;

	return vfs_rename(oldpath, newpath, false);
}


int
user_rstat(const char *upath, struct stat *ustat)
{
	char path[SYS_MAX_PATH_LEN];
	struct stat stat;
	int rc, rc2;
	struct vnode *v = NULL;

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)ustat >= KERNEL_BASE && (addr)ustat <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN-1] = 0;

	rc = path_to_vnode(path, &v, false);
	if (rc < 0)
		return rc;
		
	rc = v->mount->fs->calls->fs_rstat(v->mount->cookie, v->priv_vnode, &stat);

	dec_vnode_ref_count(v, true, false);

	rc2 = user_memcpy(ustat, &stat, sizeof(struct stat));
	if(rc2 < 0)
		return rc2;
		
	return rc;
}


int
user_wstat(const char *upath, struct stat *ustat, int stat_mask)
{
	char path[SYS_MAX_PATH_LEN+1];
	struct stat stat;
	int rc;

	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	if((addr)ustat >= KERNEL_BASE && (addr)ustat <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN);
	if(rc < 0)
		return rc;
	path[SYS_MAX_PATH_LEN] = 0;

	rc = user_memcpy(&stat, ustat, sizeof(struct stat));
	if(rc < 0)
		return rc;

	return vfs_wstat(path, &stat, stat_mask, false);
}


int
user_getcwd(char *buf, size_t size)
{
	char path[SYS_MAX_PATH_LEN];
	int rc, rc2;

#if MAKE_NOIZE
	dprintf("user_getcwd: buf 0x%x, 0x%x\n", buf, size);
#endif

	// Check if userspace address is inside "shared" kernel space
	if((addr)buf >= KERNEL_BASE && (addr)buf <= KERNEL_TOP)
		return NULL; //ERR_VM_BAD_USER_MEMORY;

	// Call vfs to get current working directory
	rc = vfs_get_cwd(path, SYS_MAX_PATH_LEN-1, false);
	if(rc < 0)
		return rc;

	// Copy back the result
	rc2 = user_strncpy(buf, path, size);
	if(rc2 < 0)
		return rc2;

	return rc;
}


int
user_setcwd(const char* upath)
{
	char path[SYS_MAX_PATH_LEN];
	int rc;

#if MAKE_NOIZE
	dprintf("user_setcwd: path=0x%x\n", upath);
#endif

	// Check if userspace address is inside "shared" kernel space
	if((addr)upath >= KERNEL_BASE && (addr)upath <= KERNEL_TOP)
		return ERR_VM_BAD_USER_MEMORY;

	// Copy new path to kernel space
	rc = user_strncpy(path, upath, SYS_MAX_PATH_LEN-1);
	if (rc < 0) {
		return rc;
	}
	path[SYS_MAX_PATH_LEN-1] = 0;

	// Call vfs to set new working directory
	return vfs_set_cwd(path,false);
}


int
user_dup(int fd)
{
	return vfs_dup(fd, false);
}


int
user_dup2(int ofd, int nfd)
{
	return vfs_dup2(ofd, nfd, false);
}


//	#pragma mark -


static int
vfs_resize_fd_table(struct io_context *ioctx, const int new_size)
{
	void	* new_fds;
	int		ret;

	if (new_size < 0 || new_size > MAX_FD_TABLE_SIZE)
		return -1;

	mutex_lock(&ioctx->io_mutex);

	if (new_size < ioctx->table_size) {
		int i;

		/* Make sure none of the fds being dropped are in use */
		for(i = new_size; i < ioctx->table_size; i++) {
			if (ioctx->fds[i]) {
				ret = -1;
				goto error;
			}
		}

		new_fds = kmalloc(sizeof(struct file_descriptor *) * new_size);
		if (!new_fds) {
			ret = -1;
			goto error;
		}

		memcpy(new_fds, ioctx->fds, sizeof(struct file_descriptor *) * new_size);
	} else {
		new_fds = kmalloc(sizeof(struct file_descriptor *) * new_size);
		if (!new_fds) {
			ret = -1;
			goto error;
		}

		memcpy(new_fds, ioctx->fds, sizeof(struct file_descriptor *) * ioctx->table_size);
		memset(((char *)new_fds) + (sizeof(struct file_descriptor *) * ioctx->table_size), 0,
				(sizeof(struct file_descriptor *) * new_size) - (sizeof(struct file_descriptor *) * ioctx->table_size));
	}

	kfree(ioctx->fds);
	ioctx->fds = new_fds;
	ioctx->table_size = new_size;

	mutex_unlock(&ioctx->io_mutex);

	return 0;

error:
	mutex_unlock(&ioctx->io_mutex);

	return ret;
}


int
vfs_getrlimit(int resource, struct rlimit * rlp)
{
	if (!rlp) {
		return -1;
	}

	switch (resource) {
		case RLIMIT_NOFILE:
		{
			struct io_context *ioctx = get_current_io_context(false);

			mutex_lock(&ioctx->io_mutex);

			rlp->rlim_cur = ioctx->table_size;
			rlp->rlim_max = MAX_FD_TABLE_SIZE;

			mutex_unlock(&ioctx->io_mutex);

			return 0;
		}

		default:
			return -1;
	}
}


int
vfs_setrlimit(int resource, const struct rlimit * rlp)
{
	if (!rlp)
		return -1;

	switch (resource) {
		case RLIMIT_NOFILE:
			return vfs_resize_fd_table(get_current_io_context(false), rlp->rlim_cur);

		default:
			return -1;
	}
}


#ifdef DEBUG

int
vfs_test(void)
{
	int fd;
	int err;

	dprintf("vfs_test() entry\n");

	fd = sys_open("/", STREAM_TYPE_DIR, 0);
	dprintf("fd = %d\n", fd);
	sys_close(fd);

	fd = sys_open("/", STREAM_TYPE_DIR, 0);
	dprintf("fd = %d\n", fd);

	sys_create("/foo", STREAM_TYPE_DIR);
	sys_create("/foo/bar", STREAM_TYPE_DIR);
	sys_create("/foo/bar/gar", STREAM_TYPE_DIR);
	sys_create("/foo/bar/tar", STREAM_TYPE_DIR);

#if 1
	fd = sys_open("/foo/bar", STREAM_TYPE_DIR, 0);
	if(fd < 0)
		panic("unable to open /foo/bar\n");

	{
		char buf[64];
		ssize_t len;

		sys_seek(fd, 0, SEEK_SET);
		for(;;) {
			len = sys_read(fd, buf, -1, sizeof(buf));
//			if(len <= 0)
//				panic("readdir returned %Ld\n", (long long)len);
			if(len > 0)
				dprintf("readdir returned name = '%s'\n", buf);
			else {
				dprintf("readdir returned %s\n", strerror(len));
				break;
			}
		}
	}

	// do some unlink tests
	err = sys_unlink("/foo/bar");
	if(err == B_NO_ERROR)
		panic("unlink of full directory should not have passed\n");
	sys_unlink("/foo/bar/gar");
	sys_unlink("/foo/bar/tar");
	err = sys_unlink("/foo/bar");
	if(err != B_NO_ERROR)
		panic("unlink of empty directory should have worked\n");

	sys_create("/test", STREAM_TYPE_DIR);
	sys_create("/test", STREAM_TYPE_DIR);
	err = sys_mount("/test", NULL, "rootfs", NULL);
	if(err < 0)
		panic("failed mount test\n");

#endif
#if 1

	fd = sys_open("/boot", STREAM_TYPE_DIR, 0);
	sys_close(fd);

	fd = sys_open("/boot", STREAM_TYPE_DIR, 0);
	if(fd < 0)
		panic("unable to open dir /boot\n");
	{
		char buf[64];
		ssize_t len;

		sys_seek(fd, 0, SEEK_SET);
		for(;;) {
			len = sys_read(fd, buf, -1, sizeof(buf));
//			if(len < 0)
//				panic("readdir returned %Ld\n", (long long)len);
			if(len > 0)
				dprintf("sys_read returned name = '%s'\n", buf);
			else {
				dprintf("sys_read returned %s\n", strerror(len));
				break;
			}
		}
	}
	sys_close(fd);

	fd = sys_open("/boot/kernel", STREAM_TYPE_FILE, 0);
	if(fd < 0)
		panic("unable to open kernel file '/boot/kernel'\n");
	{
		char buf[64];
		ssize_t len;

		len = sys_read(fd, buf, 0, sizeof(buf));
		if(len < 0)
			panic("failed on read\n");
		dprintf("read returned %Ld\n", (long long)len);
	}
	sys_close(fd);
	{
		struct stat stat;

		err = sys_rstat("/boot/kernel", &stat);
		if(err < 0)
			panic("err stating '/boot/kernel'\n");
		dprintf("stat results:\n");
		dprintf("\tvnid 0x%Lx\n\ttype %d\n\tsize 0x%Lx\n", stat.st_ino, stat.st_mode, stat.st_size);
	}

#endif
	dprintf("vfs_test() done\n");
//	panic("foo\n");
	return 0;
}

#endif


image_id
vfs_load_fs_module(const char *name)
{
	image_id id;
	void (*bootstrap)();
	char path[SYS_MAX_PATH_LEN];

//	sprintf(path, "/boot/addons/fs/%s", name);

	id = elf_load_kspace(path, "");
	if(id < 0)
		return id;

	bootstrap = (void *)elf_lookup_symbol(id, "fs_bootstrap");
	if(!bootstrap)
		return ERR_VFS_INVALID_FS;

	bootstrap();

	return id;
}


int
vfs_bootstrap_all_filesystems(void)
{
	int err;
	int fd;

	// bootstrap the root filesystem
	bootstrap_rootfs();

	err = sys_mount("/", NULL, "rootfs", NULL);
	if(err < 0)
		panic("error mounting rootfs!\n");

	sys_setcwd("/");

	// bootstrap the bootfs
	bootstrap_bootfs();

	sys_create("/boot", STREAM_TYPE_DIR);
	err = sys_mount("/boot", NULL, "bootfs", NULL);
	if(err < 0)
		panic("error mounting bootfs\n");

	// bootstrap the devfs
	bootstrap_devfs();

	sys_create("/dev", STREAM_TYPE_DIR);
	err = sys_mount("/dev", NULL, "devfs", NULL);
	if(err < 0)
		panic("error mounting devfs\n");

	fd = sys_open("/boot/addons/fs", STREAM_TYPE_DIR, 0);
	if (fd >= 0) {
		ssize_t len;
		char buf[SYS_MAX_NAME_LEN];

		while ((len = sys_read(fd, buf, 0, sizeof(buf))) > 0) {
			vfs_load_fs_module(buf);
		}
		sys_close(fd);
	}

	return B_NO_ERROR;
}


int
vfs_register_filesystem(const char *name, struct fs_calls *calls)
{
	struct fs_container *container;

	container = (struct fs_container *)kmalloc(sizeof(struct fs_container));
	if(container == NULL)
		return ENOMEM;

	container->name = name;
	container->calls = calls;

	mutex_lock(&vfs_mutex);

	container->next = fs_list;
	fs_list = container;

	mutex_unlock(&vfs_mutex);
	return 0;
}


int
vfs_init(kernel_args *ka)
{
	{
		struct vnode *v;
		vnode_table = hash_init(VNODE_HASH_TABLE_SIZE, (addr)&v->next - (addr)v,
			&vnode_compare, &vnode_hash);
		if(vnode_table == NULL)
			panic("vfs_init: error creating vnode hash table\n");
	}
	{
		struct fs_mount *mount;
		mounts_table = hash_init(MOUNTS_HASH_TABLE_SIZE, (addr)&mount->next - (addr)mount,
			&mount_compare, &mount_hash);
		if(mounts_table == NULL)
			panic("vfs_init: error creating mounts hash table\n");
	}
	fs_list = NULL;
	root_vnode = NULL;

	if(mutex_init(&vfs_mutex, "vfs_lock") < 0)
		panic("vfs_init: error allocating vfs lock\n");

	if(mutex_init(&vfs_mount_op_mutex, "vfs_mount_op_lock") < 0)
		panic("vfs_init: error allocating vfs_mount_op lock\n");

	if(mutex_init(&vfs_mount_mutex, "vfs_mount_lock") < 0)
		panic("vfs_init: error allocating vfs_mount lock\n");

	if(mutex_init(&vfs_vnode_mutex, "vfs_vnode_lock") < 0)
		panic("vfs_init: error allocating vfs_vnode lock\n");

	return 0;
}
