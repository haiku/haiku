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
#include <thread.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "builtin_fs.h"

// ToDo: handles file names suboptimally - it has all file names
//	in a singly linked list, no hash lookups or whatever.

#define PIPEFS_TRACE 0

#if PIPEFS_TRACE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


namespace pipefs {

class Volume;
class Inode;
struct dir_cookie;

struct read_request {
	read_request	*prev;
	read_request	*next;
	void			*buffer;
	size_t			buffer_size;
	size_t			bytes_read;
	team_id			team;

	size_t SpaceLeft() const { return buffer_size - bytes_read; }
	void Fill(Inode *inode);
	status_t PutBuffer(const void **_buffer, size_t *_bufferSize);
};

class ReadRequests {
	public:
		ReadRequests();
		~ReadRequests();

		void Lock();
		void Unlock();

		status_t Add(read_request &request);
		status_t Remove(read_request &request);

		read_request *GetCurrent() const { return fCurrent; }
		void SkipCurrent() { if (fCurrent) fCurrent = fCurrent->next; }

	private:
		mutex			fLock;
		read_request	*fFirst, *fCurrent, *fLast;
};

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

		sem_id		ReadLock() { return fReadLock; }
		mutex		*WriteMutex() { return &fWriteMutex; }

		status_t	WriteBufferToChain(const void *buffer, size_t bufferSize);
		status_t	ReadBufferFromChain(void *buffer, size_t *_bufferSize);
		off_t		BytesInChain() const { return cbuf_get_length(fBufferChain); }

		static int32 HashNextOffset();
		static uint32 hash_func(void *_node, const void *_key, uint32 range);
		static int compare_func(void *_node, const void *_key);

	private:
		Inode		*fNext;
		Inode		*fHashNext;
		vnode_id	fID;
		int32		fType;
		const char	*fName;

		cbuf		*fBufferChain;

		sem_id		fReadLock;
		mutex		fWriteMutex;
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
	vfs_new_vnode(ID(), inode->ID(), inode);

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
	fNext(NULL),
	fHashNext(NULL),
	fBufferChain(NULL)
{
	fName = strdup(name);
	if (fName == NULL)
		return;

	fID = volume->GetNextNodeID();
	fType = type;

	if (type == S_IFIFO) {
		fReadLock = create_sem(0, "pipe read");
		mutex_init(&fWriteMutex, "pipe write");
	}
}


Inode::~Inode()
{
	free(const_cast<char *>(fName));

	if (fType == S_IFIFO) {	
		delete_sem(fReadLock);
		mutex_destroy(&fWriteMutex);
	}
}


status_t 
Inode::InitCheck()
{
	if (fName == NULL
		|| fType == S_IFIFO && (fReadLock < B_OK || fWriteMutex.sem < B_OK))
		return B_ERROR;

	return B_OK;
}


/** Adds the specified buffer to the inode's buffer chain by copying
 *	its contents. The Inode::WriteLock() must be held when calling
 *	this method.
 *	Releases the reader sem if necessary, so that blocking
 *	readers will get started.
 *	Returns B_NO_MEMORY if the chain couldn't be allocated, B_BAD_ADDRESS
 *	if copying from the buffer failed, and B_OK on success.
 */

status_t
Inode::WriteBufferToChain(const void *buffer, size_t bufferSize)
{
	cbuf *chain = cbuf_get_chain(bufferSize);
	if (chain == NULL)
		return B_NO_MEMORY;
	
	if (cbuf_user_memcpy_to_chain(chain, 0, buffer, bufferSize) < B_OK)
		return B_BAD_ADDRESS;

	// join this chain with our already existing chain (if any)

	chain = cbuf_merge_chains(fBufferChain, chain);

	// let waiting readers go on
	if (fBufferChain == NULL)
		release_sem(ReadLock());

	fBufferChain = chain;
	return B_OK;
}


/** Reads the contents of the buffer chain to the specified
 *	buffer, if any.
 *	Unblocks other waiting readers if there would be still
 *	some bytes left in the stream.
 */

status_t 
Inode::ReadBufferFromChain(void *buffer, size_t *_bufferSize)
{
	if (fBufferChain == NULL) {
		*_bufferSize = 0;
		return B_OK;
	}

	size_t length = BytesInChain();

	// we read *_bufferSize bytes at maximum - but never
	// more than there are in the chain
	if (*_bufferSize > length)
		*_bufferSize = length;
	else {
		length = *_bufferSize;

		// we unlock another reader here, so that it can read
		// the rest of the pending bytes
		release_sem(ReadLock());
	}

	if (cbuf_user_memcpy_from_chain(buffer, fBufferChain, 0, length) < B_OK)
		return B_BAD_ADDRESS;

	if (cbuf_truncate_head(fBufferChain, length) < B_OK) {
		// if that call fails, the next read will duplicate the input
		dprintf("pipefs: cbuf_truncate_head() failed for inode %Ld\n", ID());
	}

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


void
read_request::Fill(Inode *inode)
{
	size_t length = SpaceLeft();

	if (inode->ReadBufferFromChain(buffer, &length) < B_OK) {
		// ToDo: should add status to read_request
		dprintf("reading from chain failed!");
	} else
		bytes_read += length;
}


/** Copies the specified buffer into the request. This function currently
 *	only works for the local address space.
 */

status_t
read_request::PutBuffer(const void **_buffer, size_t *_bufferSize)
{
	TRACE(("pipefs: read_request::PutUserBuffer(buffer = %p, size = %lu)\n", *_buffer, *_bufferSize));

	size_t bytes = *_bufferSize;
	if (bytes > SpaceLeft())
		bytes = SpaceLeft();

	uint8 *source = (uint8 *)*_buffer;

	if (user_memcpy((uint8 *)buffer + bytes_read, source, bytes) < B_OK)
		return B_BAD_ADDRESS;

	bytes_read += bytes;
	*_buffer = (void *)(source + bytes);
	*_bufferSize -= bytes;

	return B_OK;
}


//	#pragma mark -


ReadRequests::ReadRequests()
	:
	fFirst(NULL),
	fCurrent(NULL),
	fLast(NULL)
{
	mutex_init(&fLock, "pipefs read requests");
}


ReadRequests::~ReadRequests()
{
	mutex_destroy(&fLock);
}


void
ReadRequests::Lock()
{
	mutex_lock(&fLock);
}


void
ReadRequests::Unlock()
{
	mutex_unlock(&fLock);
}


status_t 
ReadRequests::Add(read_request &request)
{
	if (fLast != NULL)
		fLast->next = &request;

	if (fFirst == NULL)
		fFirst = &request;

	// ToDo: could directly skip full requests
	if (fCurrent == NULL)
		fCurrent = &request;

	request.prev = fLast;
	request.next = NULL;
	fLast = &request;

	return B_OK;
}


status_t 
ReadRequests::Remove(read_request &request)
{
	if (request.next != NULL)
		request.next->prev = request.prev;
	if (request.prev != NULL)
		request.prev->next = request.next;

	// update pointers

	if (fCurrent == &request)
		fCurrent = fCurrent->next;

	if (fLast == &request)
		fLast = request.prev;

	if (fFirst == &request)
		fFirst = request.next;

	return B_OK;
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
#if TRACE_PIPEFS
	Inode *inode = (Inode *)_node;
	TRACE(("pipefs_putvnode: entry on vnode 0x%Lx, r %d\n", inode->ID(), reenter));
#endif

	// ToDo: delete pipe - it isn't needed anymore!

	//delete inode;
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
pipefs_create(fs_volume _volume, fs_vnode _dir, const char *name, int openMode, int perms,
	fs_cookie *_cookie, vnode_id *_newVnodeID)
{
	Volume *volume = (Volume *)_volume;

	TRACE(("pipefs_create(): dir = %p, name = '%s', perms = %d, &id = %p\n",
		_dir, name, perms, _newVnodeID));

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

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

	cookie->open_mode = openMode;

	*_cookie = (void *)cookie;
	*_newVnodeID = inode->ID();
	notify_listener(B_ENTRY_CREATED, volume->ID(), directory->ID(), 0, inode->ID(), name);

	return B_OK;

err:
	volume->Unlock();
	free(cookie);

	return status;
}


static status_t
pipefs_open(fs_volume _volume, fs_vnode _node, int openMode, fs_cookie *_cookie)
{
	// allow to open the file, but it can't be done anything with it

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	cookie->open_mode = openMode;

	*_cookie = (void *)cookie;

	return B_OK;
}


static status_t
pipefs_close(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie)
{
	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _vnode, _cookie));

	return 0;
}


static status_t
pipefs_free_cookie(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	file_cookie *cookie = (file_cookie *)_cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _node, _cookie));

	free(cookie);

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
	Volume *volume = (Volume *)_volume;

	(void)pos;
	TRACE(("pipefs_read: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", _node, cookie, pos, *_length));

	if ((cookie->open_mode & O_RWMASK) != O_RDONLY)
		return B_NOT_ALLOWED;

	// issue read request

	read_request request;
	request.buffer = buffer;
	request.buffer_size = *_length;
	request.bytes_read = 0;
	request.team = team_get_current_team_id();

	ReadRequests &requests = volume->GetReadRequests();

	requests.Lock();
	requests.Add(request);
	requests.Unlock();

	// ToDo: here is the race condition that another reader issues
	//	its read request after this one, but locks earlier; this
	//	will currently lead to a dead-lock or failure - that could
	//	be solved by attaching a thread to the request

	// wait for it to be filled

	Inode *inode = (Inode *)_node;
	status_t status = acquire_sem_etc(inode->ReadLock(), 1,
						(cookie->open_mode & O_NONBLOCK ? B_TIMEOUT : 0 ) | B_CAN_INTERRUPT, 0);

	requests.Lock();

	if (status == B_OK)
		request.Fill(inode);

	requests.Remove(request);
	requests.Unlock();

	if (status == B_TIMED_OUT || B_INTERRUPTED && request.bytes_read > 0)
		status = B_OK;

	if (status == B_OK)
		*_length = request.bytes_read;

	return status;
}


static ssize_t
pipefs_write(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t pos,
	const void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;
	team_id team = team_get_current_team_id();

	TRACE(("pipefs_write: vnode %p, cookie %p, pos 0x%Lx , len 0x%lx\n", _node, cookie, pos, *_length));

	if ((cookie->open_mode & O_RWMASK) != O_WRONLY)
		return B_NOT_ALLOWED;

	mutex_lock(inode->WriteMutex());

	ReadRequests &requests = volume->GetReadRequests();
	requests.Lock();

	size_t bytesLeft = *_length;
	read_request *request = NULL;

	do {
		request = requests.GetCurrent();
		if (request != NULL) {
			// fill this request

			request->Fill(inode);
			if (request->SpaceLeft() > 0
				&& (team == B_SYSTEM_TEAM || request->team == team)) {
				// ToDo: This is something where we can optimize the buffer
				//	hand-shaking considerably: we should have a function
				//	that copies the data to another address space - either
				//	remapping copy on write or a direct copy.

				// place our data into that buffer
				request->PutBuffer(&buffer, &bytesLeft);
				if (bytesLeft == 0) {
					release_sem(inode->ReadLock());
					break;
				}
			}

			requests.SkipCurrent();
		}
	} while (request != NULL);

	status_t status;

	if (request == NULL && bytesLeft > 0) {
		// there is no read matching request pending, so we have to put
		// our data in a temporary buffer

		status = inode->WriteBufferToChain(buffer, bytesLeft);
		if (status != B_OK)
			*_length -= bytesLeft;
	} else {
		// could write everything without the need to copy!
		status = B_OK;
	}

	requests.Unlock();
	mutex_unlock(inode->WriteMutex());

	return status;
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

	volume->Unlock();

	*_cookie = (void *)cookie;

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
	stat->st_size = inode->BytesInChain();
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
