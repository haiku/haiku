/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>
#include <NodeMonitor.h>

#include <util/kernel_cpp.h>
#include <cbuf.h>
#include <vfs.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <vm.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "builtin_fs.h"


namespace pipefs {

// ToDo: handles file names suboptimally - it has all file names
//	in a single linked list, no hash lookups or whatever.

#define PIPEFS_TRACE 1

#if PIPEFS_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

struct read_request {
	read_request	*next;
	void			*buffer;
	size_t			buffer_size;
	size_t			bytes_read;
};

class ReadRequests {
	public:
		ReadRequests();
		~ReadRequests();

		status_t Lock();
		status_t Unlock();

		status_t Add(read_request &request);
		status_t Remove(read_request &request);

	private:
		benaphore		fLock;
		read_request	*fFirst, *fCurrent, *fLast;
};

class Inode;
struct dir_cookie;

class Volume {
	public:
		Volume(mount_id id);
		~Volume();

		status_t		InitCheck();
		mount_id		ID() const { return fID; }
		Inode			&RootNode() const { return *fRootNode; }

		void			Lock();
		void			Unlock();

		Inode			*Lookup(vnode_id id);
		Inode			*FindNode(const char *name);
		Inode			*CreateNode(const char *name, int32 type);
		status_t		DeleteNode(Inode *inode, bool forceDelete = false);
		status_t		RemoveNode(Inode *directory, const char *name);

		vnode_id		GetNextNodeID() { return fNextNodeID++; }
		ReadRequests	&GetReadRequests() { return fReadRequests; }

		Inode			*FirstEntry() const { return fFirstEntry; }

		dir_cookie		*FirstDirCookie() const { return fFirstDirCookie; }
		void			InsertCookie(dir_cookie *cookie);

	private:
		void			RemoveCookie(dir_cookie *cookie);
		void			UpdateDirCookies(Inode *inode);

		status_t		RemoveNode(Inode *inode);
		status_t		InsertNode(Inode *inode);

		ReadRequests	fReadRequests;
		mutex			fLock;
		mount_id		fID;
		Inode 			*fRootNode;
		vnode_id		fNextNodeID;
		hash_table		*fNodeHash;

		// root directory contents - we don't support other directories
		Inode			*fFirstEntry;
		dir_cookie		*fFirstDirCookie;
};

class Inode {
	public:
		Inode(Volume *volume, const char *name, int32 type);
		~Inode();
		
		status_t	InitCheck();
		vnode_id	ID() const { return fID; }
		const char	*Name() const { return fName; }
		int32		Type() const { return fType; }

		Inode		*Next() const { return fNext; }
		void		SetNext(Inode *inode) { fNext = inode; }

		benaphore	*ReadLock() { return &fReadLock; }
		benaphore	*WriteLock() { return &fWriteLock; }

		static int32 HashNextOffset();
		static uint32 hash_func(void *_node, const void *_key, uint32 range);
		static int compare_func(void *_node, const void *_key);


	private:
		Inode		*fNext;
		Inode		*fHashNext;
		vnode_id	fID;
		int32		fType;
		const char	*fName;

		benaphore	fReadLock;
		benaphore	fWriteLock;
};


struct dir_cookie {
	dir_cookie		*next;
	dir_cookie		*prev;
	Inode			*current;
};

struct file_cookie {
	int				open_mode;
};


#define PIPEFS_HASH_SIZE 16


Volume::Volume(mount_id id)
	:
	fID(id),
	fRootNode(NULL),
	fNextNodeID(0),
	fNodeHash(NULL),
	fFirstEntry(NULL),
	fFirstDirCookie(NULL)
{
	if (mutex_init(&fLock, "pipefs_mutex") < B_OK)
		return;

	fNodeHash = hash_init(PIPEFS_HASH_SIZE, Inode::HashNextOffset(),
		&Inode::compare_func, &Inode::hash_func);
	if (fNodeHash == NULL)
		return;

	// create the root vnode
	fRootNode = CreateNode("", S_IFDIR);
	if (fRootNode == NULL)
		return;
}


Volume::~Volume()
{
	// put_vnode on the root to release the ref to it
	if (fRootNode)
		vfs_put_vnode(ID(), fRootNode->ID());

	if (fNodeHash != NULL) {
		// delete all of the inodes
		struct hash_iterator i;
		hash_open(fNodeHash, &i);

		Inode *inode;
		while ((inode = (Inode *)hash_next(fNodeHash, &i)) != NULL) {
			DeleteNode(inode, true);
		}

		hash_close(fNodeHash, &i, false);

		hash_uninit(fNodeHash);
	}
	mutex_destroy(&fLock);
}


status_t 
Volume::InitCheck()
{
	if (fLock.sem < B_OK
		|| fRootNode == NULL)
		return B_ERROR;

	return B_OK;
}


void
Volume::Lock()
{
	mutex_lock(&fLock);
}


void
Volume::Unlock()
{
	mutex_unlock(&fLock);
}


Inode *
Volume::CreateNode(const char *name, int32 type)
{
	Inode *inode = new Inode(this, name, type);
	if (inode == NULL)
		return NULL;

	if (inode->InitCheck() != B_OK) {
		delete inode;
		return NULL;
	}

	if (type == S_IFIFO)
		InsertNode(inode);
	hash_insert(fNodeHash, inode);

	return inode;
}


status_t
Volume::DeleteNode(Inode *inode, bool forceDelete)
{
	// remove it from the global hash table
	hash_remove(fNodeHash, inode);

	delete inode;
	return 0;
}


void
Volume::InsertCookie(dir_cookie *cookie)
{
	cookie->next = fFirstDirCookie;
	fFirstDirCookie = cookie;
	cookie->prev = NULL;
}


void
Volume::RemoveCookie(dir_cookie *cookie)
{
	if (cookie->next)
		cookie->next->prev = cookie->prev;

	if (cookie->prev)
		cookie->prev->next = cookie->next;

	if (fFirstDirCookie == cookie)
		fFirstDirCookie = cookie->next;

	cookie->prev = cookie->next = NULL;
}


/* makes sure none of the dircookies point to the vnode passed in */

void
Volume::UpdateDirCookies(Inode *inode)
{
	dir_cookie *cookie;

	for (cookie = fFirstDirCookie; cookie; cookie = cookie->next) {
		if (cookie->current == inode)
			cookie->current = inode->Next();
	}
}


Inode *
Volume::Lookup(vnode_id id)
{
	return (Inode *)hash_lookup(fNodeHash, &id);
}


Inode *
Volume::FindNode(const char *name)
{
	if (!strcmp(name, ".")
		|| !strcmp(name, ".."))
		return fRootNode;

	for (Inode *inode = fFirstEntry; inode; inode = inode->Next()) {
		if (!strcmp(inode->Name(), name))
			return inode;
	}
	return NULL;
}


status_t
Volume::InsertNode(Inode *inode)
{
	inode->SetNext(fFirstEntry);
	fFirstEntry = inode;

	return B_OK;
}


status_t
Volume::RemoveNode(Inode *findNode)
{
	Inode *inode, *last;

	for (inode = fFirstEntry, last = NULL; inode; last = inode, inode = inode->Next()) {
		if (inode == findNode) {
			// make sure no dir cookies point to this node
			UpdateDirCookies(inode);

			if (last)
				last->SetNext(inode->Next());
			else
				fFirstEntry = inode->Next();

			inode->SetNext(NULL);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


status_t
Volume::RemoveNode(Inode *directory, const char *name)
{
	Lock();

	status_t status = B_OK;

	Inode *inode = FindNode(name);
	if (inode == NULL)
		status = B_ENTRY_NOT_FOUND;
	else if (inode->Type() == S_IFDIR)
		status = B_NOT_ALLOWED;

	if (status < B_OK)
		goto err;

	RemoveNode(inode);
	notify_listener(B_ENTRY_REMOVED, ID(), directory->ID(), 0, inode->ID(), name);

	// schedule this vnode to be removed when it's ref goes to zero
	vfs_remove_vnode(ID(), inode->ID());

err:
	Unlock();

	return status;
}


//	#pragma mark -


Inode::Inode(Volume *volume, const char *name, int32 type)
	:
	fNext(NULL)
{
	fName = strdup(name);
	if (fName == NULL)
		return;

	fID = volume->GetNextNodeID();
	fType = type;

	if (benaphore_init(&fReadLock, "pipe read") == B_OK)
		benaphore_init(&fWriteLock, "pipe write");
}


Inode::~Inode()
{
	free(const_cast<char *>(fName));
}


status_t 
Inode::InitCheck()
{
	if (fName == NULL
		|| fReadLock.sem < B_OK
		|| fWriteLock.sem < B_OK)
		return B_ERROR;

	return B_OK;
}


int32 
Inode::HashNextOffset()
{
	Inode *inode;
	return (addr)&inode->fHashNext - (addr)inode;
}


uint32
Inode::hash_func(void *_node, const void *_key, uint32 range)
{
	Inode *inode = (Inode *)_node;
	const vnode_id *key = (const vnode_id *)_key;

	if (inode != NULL)
		return inode->ID() % range;

	return (*key) % range;
}


int
Inode::compare_func(void *_node, const void *_key)
{
	Inode *inode = (Inode *)_node;
	const vnode_id *key = (const vnode_id *)_key;

	if (inode->ID() == *key)
		return 0;

	return -1;
}


//	#pragma mark -


ReadRequests::ReadRequests()
	:
	fFirst(NULL),
	fCurrent(NULL),
	fLast(NULL)
{
	benaphore_init(&fLock, "pipefs read requests");
}


ReadRequests::~ReadRequests()
{
	benaphore_destroy(&fLock);
}


status_t 
ReadRequests::Lock()
{
	return benaphore_lock(&fLock);
}


status_t 
ReadRequests::Unlock()
{
	return benaphore_unlock(&fLock);
}


status_t 
ReadRequests::Add(read_request &request)
{
	if (fLast != NULL)
		fLast->next = &request;

	if (fFirst == NULL)
		fFirst = &request;

	fLast = &request;		
	request.next = NULL;
}


status_t 
ReadRequests::Remove(read_request &request)
{
}


//	#pragma mark -


static status_t
pipefs_mount(mount_id id, const char *device, void *args, fs_volume *_volume, vnode_id *_rootVnodeID)
{
	TRACE(("pipefs_mount: entry\n"));

	Volume *volume = new Volume(id);
	if (volume == NULL)
		return B_NO_MEMORY;

	status_t status = volume->InitCheck();
	if (status < B_OK) {
		delete volume;
		return status;
	}

	*_rootVnodeID = volume->RootNode().ID();
	*_volume = volume;

	return B_OK;
}


static status_t
pipefs_unmount(fs_volume _volume)
{
	struct Volume *volume = (struct Volume *)_volume;

	TRACE(("pipefs_unmount: entry fs = %p\n", _volume));
	delete volume;

	return 0;
}


static status_t
pipefs_sync(fs_volume fs)
{
	TRACE(("pipefs_sync: entry\n"));

	return 0;
}


static status_t
pipefs_lookup(fs_volume _volume, fs_vnode _dir, const char *name, vnode_id *_id, int *_type)
{
	Volume *volume = (Volume *)_volume;
	status_t status;

	TRACE(("pipefs_lookup: entry dir %p, name '%s'\n", _dir, name));

	Inode *directory = (Inode *)_dir;
	if (directory->Type() != S_IFDIR)
		return B_NOT_A_DIRECTORY;

	volume->Lock();

	// look it up
	Inode *inode = volume->FindNode(name);
	if (inode == NULL) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	Inode *dummy;
	status = vfs_get_vnode(volume->ID(), inode->ID(), (fs_vnode *)&dummy);
	if (status < B_OK)
		goto err;

	*_id = inode->ID();
	*_type = inode->Type();

err:
	volume->Unlock();

	return status;
}


static status_t
pipefs_get_vnode_name(fs_volume _volume, fs_vnode _node, char *buffer, size_t bufferSize)
{
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_get_vnode_name(): inode = %p\n", inode));

	strlcpy(buffer, inode->Name(), bufferSize);
	return B_OK;
}


static status_t
pipefs_get_vnode(fs_volume _volume, vnode_id id, fs_vnode *_inode, bool reenter)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("pipefs_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	if (!reenter)
		volume->Lock();

	*_inode = volume->Lookup(id);

	if (!reenter)
		volume->Unlock();

	TRACE(("pipefs_getnvnode: looked it up at %p\n", *_inode));

	if (*_inode)
		return B_OK;

	return B_ENTRY_NOT_FOUND;
}


static status_t
pipefs_put_vnode(fs_volume _volume, fs_vnode _node, bool reenter)
{
#if PIPEFS_TRACE
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_putvnode: entry on vnode 0x%Lx, r %d\n", inode->ID(), reenter));
#endif
	return B_OK;
}


static status_t
pipefs_remove_vnode(fs_volume _volume, fs_vnode _node, bool reenter)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_removevnode: remove %p (0x%Lx), r %d\n", inode, inode->ID(), reenter));

	if (!reenter)
		volume->Lock();

	if (inode->Next()) {
		// can't remove node if it's linked to the dir
		panic("pipefs_remove_vnode(): vnode %p asked to be removed is present in dir\n", inode);
	}

	volume->DeleteNode(inode);

	if (!reenter)
		volume->Unlock();

	return 0;
}


static status_t
pipefs_create(fs_volume _volume, fs_vnode _dir, const char *name, int omode, int perms,
	fs_cookie *_cookie, vnode_id *_newVnodeID)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("pipefs_create_dir: dir = %p, name = '%s', perms = %d, &id = %p\n",
		_dir, name, perms, _newVnodeID));

	volume->Lock();

	Inode *directory = (Inode *)_dir;
	status_t status = B_OK;

	Inode *inode = volume->FindNode(name);
	if (inode != NULL) {
		status = B_FILE_EXISTS;
		goto err;
	}

	inode = volume->CreateNode(name, S_IFIFO);
	if (inode == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	volume->Unlock();

	notify_listener(B_ENTRY_CREATED, volume->ID(), directory->ID(), 0, inode->ID(), name);
	return B_OK;

err:
	volume->Unlock();
	return status;
}


static status_t
pipefs_open(fs_volume _volume, fs_vnode _v, int openMode, fs_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it

	*_cookie = NULL;
		// initialize the cookie, because pipefs_free_cookie() relies on it

	return B_OK;
}


static status_t
pipefs_close(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _vnode, _cookie));

	return 0;
}


static status_t
pipefs_free_cookie(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
//	pipefs_cookie *cookie = _cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _vnode, _cookie));

//	if (cookie)
//		free(cookie);

	return 0;
}


static status_t
pipefs_fsync(fs_volume _volume, fs_vnode _v)
{
	return B_OK;
}


static ssize_t
pipefs_read(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t pos,
	void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Volume *fs = (Volume *)_volume;

	(void)pos;
	TRACE(("pipefs_read: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", _node, cookie, pos, *_length));

	read_request request;
	request.buffer = buffer;
	request.buffer_size = *_length;
	request.bytes_read = 0;

	ReadRequests &requests = fs->GetReadRequests();

	if (requests.Lock() != B_OK)
		return B_ERROR;

	requests.Add(request);
	requests.Unlock();

	Inode *inode = (Inode *)_node;
	status_t status = benaphore_lock_etc(inode->ReadLock(),
						cookie->open_mode & O_NONBLOCK ? B_TIMEOUT : 0, 0);
	
	if (requests.Lock() != B_OK)
		panic("pipefs: could not get lock for read requests");

//	if (status == B_OK) {
//		requests.FillRequest(request);

	requests.Remove(request);
	requests.Unlock();

	if (status == B_TIMED_OUT && request.bytes_read > 0)
		status = B_OK;

	if (status == B_OK) {
		*_length = request.bytes_read;
	}
	return status;
}


static ssize_t
pipefs_write(fs_volume fs, fs_vnode _node, fs_cookie cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	TRACE(("pipefs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", _node, cookie, pos, *_length));

	return EINVAL;
}


static off_t
pipefs_seek(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie, off_t pos, int seekType)
{
	return ESPIPE;
}


static status_t
pipefs_create_dir(fs_volume _volume, fs_vnode _dir, const char *name, int perms, vnode_id *_newID)
{
	return ENOSYS;
}


static status_t
pipefs_remove_dir(fs_volume _volume, fs_vnode _dir, const char *name)
{
	return EPERM;
}


static status_t
pipefs_open_dir(fs_volume _volume, fs_vnode _node, fs_cookie *_cookie)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("pipefs_open_dir(): vnode = %p\n", _node));

	Inode *inode = (Inode *)_node;
	if (inode->Type() != S_IFDIR)
		return B_BAD_VALUE;

	if (inode != &volume->RootNode())
		panic("pipefs: found directory that's not the root!");

	dir_cookie *cookie = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	volume->Lock();

	cookie->current = volume->FirstEntry();
	volume->InsertCookie(cookie);

	*_cookie = cookie;

	volume->Unlock();

	return B_OK;
}


static status_t
pipefs_read_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie,
	struct dirent *dirent, size_t bufferSize, uint32 *_num)
{
	Volume *volume = (Volume *)_volume;
	status_t status = 0;

	TRACE(("pipefs_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _node, _cookie, dirent, bufferSize,_num));

	if (_node != &volume->RootNode())
		return B_BAD_VALUE;

	volume->Lock();

	dir_cookie *cookie = (dir_cookie *)_cookie;
	if (cookie->current == NULL) {
		// we're at the end of the directory
		*_num = 0;
		status = B_OK;
		goto err;
	}

	dirent->d_dev = volume->ID();
	dirent->d_ino = cookie->current->ID();
	dirent->d_reclen = strlen(cookie->current->Name()) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, cookie->current->Name(), bufferSize);
	if (status < 0)
		goto err;

	cookie->current = cookie->current->Next();

err:
	volume->Unlock();

	return status;
}


static status_t
pipefs_rewind_dir(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	Volume *volume = (Volume *)_volume;
	volume->Lock();

	dir_cookie *cookie = (dir_cookie *)_cookie;
	cookie->current = volume->FirstEntry();

	volume->Unlock();
	return B_OK;
}


static status_t
pipefs_close_dir(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _node, _cookie));

	return 0;
}


static status_t
pipefs_free_dir_cookie(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	dir_cookie *cookie = (dir_cookie *)_cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _vnode, cookie));

	free(cookie);
	return 0;
}


static status_t
pipefs_ioctl(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie, ulong op,
	void *buffer, size_t length)
{
	TRACE(("pipefs_ioctl: vnode %p, cookie %p, op %ld, buf %p, len %ld\n",
		_vnode, _cookie, op, buffer, length));

	return EINVAL;
}


static status_t
pipefs_can_page(fs_volume _volume, fs_vnode _v)
{
	return -1;
}


static ssize_t
pipefs_read_pages(fs_volume _volume, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static ssize_t
pipefs_write_pages(fs_volume _volume, fs_vnode _v, iovecs *vecs, off_t pos)
{
	return EPERM;
}


static status_t
pipefs_unlink(fs_volume _volume, fs_vnode _dir, const char *name)
{
	Volume *volume = (Volume *)_volume;
	Inode *directory = (Inode *)_dir;

	TRACE(("pipefs_unlink: dir %p (0x%Lx), name '%s'\n", _dir, directory->ID(), name));

	return volume->RemoveNode(directory, name);
}


static status_t
pipefs_read_stat(fs_volume _volume, fs_vnode _node, struct stat *stat)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_rstat: vnode %p (0x%Lx), stat %p\n", inode, inode->ID(), stat));

	stat->st_dev = volume->ID();
	stat->st_ino = inode->ID();
	stat->st_size = 0;	// ToDo: oughta get me!
	stat->st_mode = inode->Type() | 0777;

	return 0;
}


//	#pragma mark -


static struct fs_ops pipefs_ops = {
	&pipefs_mount,
	&pipefs_unmount,
	NULL,
	NULL,
	&pipefs_sync,

	&pipefs_lookup,
	&pipefs_get_vnode_name,

	&pipefs_get_vnode,
	&pipefs_put_vnode,
	&pipefs_remove_vnode,

	&pipefs_can_page,
	&pipefs_read_pages,
	&pipefs_write_pages,

	/* common */
	&pipefs_ioctl,
	&pipefs_fsync,

	NULL,	// fs_read_link()
	NULL,	// fs_write_link()
	NULL,	// fs_symlink()
	NULL,	// fs_link()
	&pipefs_unlink,
	NULL,	// fs_rename()

	NULL,	// fs_access()
	&pipefs_read_stat,
	NULL,	// fs_write_stat()

	/* file */
	&pipefs_create,
	&pipefs_open,
	&pipefs_close,
	&pipefs_free_cookie,
	&pipefs_read,
	&pipefs_write,

	/* directory */
	&pipefs_create_dir,
	&pipefs_remove_dir,
	&pipefs_open_dir,
	&pipefs_close_dir,
	&pipefs_free_dir_cookie,
	&pipefs_read_dir,
	&pipefs_rewind_dir,

	// the other operations are not supported (attributes, indices, queries)
	NULL,
};

}	// namespace pipefs

extern "C" status_t
bootstrap_pipefs(void)
{
	TRACE(("bootstrap_pipefs: entry\n"));

	return vfs_register_filesystem("pipefs", &pipefs::pipefs_ops);
}
