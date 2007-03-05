/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <NodeMonitor.h>

#include <util/kernel_cpp.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <util/ring_buffer.h>
#include <vfs.h>
#include "vfs_select.h"
#include <select_sync_pool.h>
#include <debug.h>
#include <khash.h>
#include <lock.h>
#include <vm.h>
#include <team.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>


// ToDo: handles file names suboptimally - it has all file names
//	in a singly linked list, no hash lookups or whatever.

//#define TRACE_PIPEFS
#ifdef TRACE_PIPEFS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


#define PIPEFS_HASH_SIZE		16
#define PIPEFS_MAX_BUFFER_SIZE	32768


namespace pipefs {

class Volume;
class Inode;
struct dir_cookie;

class RingBuffer {
	public:
		RingBuffer();
		~RingBuffer();

		size_t Write(const void *buffer, size_t length);
		size_t Read(void *buffer, size_t length);
		ssize_t UserWrite(const void *buffer, ssize_t length);
		ssize_t UserRead(void *buffer, ssize_t length);

		size_t Readable() const;
		size_t Writable() const;

	private:
		struct ring_buffer	*fBuffer;
};

class ReadRequest : public DoublyLinkedListLinkImpl<ReadRequest> {
	public:
		ReadRequest(void *buffer, size_t bufferSize);
		~ReadRequest();

		status_t	Wait(bool nonBlocking);
		void		Notify();
		void		Abort();

		status_t	PutBuffer(RingBuffer &buffer, size_t *_bytesRead = NULL,
						bool releasePartiallyFilled = true);
		status_t	PutData(const void **_data, size_t *_dataSize);

		size_t		SpaceLeft() const { return fBufferSize - fBytesRead; }
		size_t		BytesRead() const { return fBytesRead; }
		team_id		Team() const { return fTeam; }

		bool		IsQueued() const { return fIsQueued; }
		void		SetQueued(bool queued);

	private:
		sem_id		fLock;
		team_id		fTeam;
		void		*fBuffer;
		size_t		fBufferSize;
		size_t		fBytesRead;
		bool		fIsQueued;
};

typedef DoublyLinkedList<ReadRequest> RequestList;

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
		Inode			*CreateNode(Inode *parent, const char *name, int32 type);
		status_t		DeleteNode(Inode *inode, bool forceDelete = false);
		status_t		RemoveNode(Inode *directory, const char *name);

		vnode_id		GetNextNodeID() { return fNextNodeID++; }

		Inode			*FirstEntry() const { return fFirstEntry; }

		dir_cookie		*FirstDirCookie() const { return fFirstDirCookie; }
		void			InsertCookie(dir_cookie *cookie);

	private:
		void			RemoveCookie(dir_cookie *cookie);
		void			UpdateDirCookies(Inode *inode);

		status_t		RemoveNode(Inode *inode);
		status_t		InsertNode(Inode *inode);

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
		Inode(Volume *volume, Inode *parent, const char *name, int32 type);
		~Inode();

		status_t	InitCheck();
		vnode_id	ID() const { return fID; }
		const char	*Name() const { return fName; }

		int32		Type() const { return fType; }
		void		SetMode(mode_t mode) { fType = (fType & ~S_IUMSK) | (mode & S_IUMSK); }
		gid_t		GroupID() const { return fGroupID; }
		void		SetGroupID(gid_t gid) { fGroupID = gid; }
		uid_t		UserID() const { return fUserID; }
		void		SetUserID(uid_t uid) { fUserID = uid; }
		time_t		CreationTime() const { return fCreationTime; }
		void		SetCreationTime(time_t creationTime) { fCreationTime = creationTime; }
		time_t		ModificationTime() const { return fModificationTime; }
		void		SetModificationTime(time_t modificationTime) { fModificationTime = modificationTime; }

		Inode		*Next() const { return fNext; }
		void		SetNext(Inode *inode) { fNext = inode; }

		benaphore	*RequestLock() { return &fRequestLock; }
		RequestList	&Requests() { return fRequests; }

		status_t	WriteDataToBuffer(const void **_data, size_t *_bytesLeft, bool nonBlocking);
		size_t		BytesAvailable() const { return fBuffer.Readable(); }

		void		MayReleaseWriter();
		void		FillPendingRequests(const void **_buffer, size_t *_bytesLeft);
		void 		FillPendingRequests();
		status_t	AddRequest(ReadRequest &request);
		status_t	RemoveRequest(ReadRequest &request);

		void		Open(int openMode);
		void		Close(int openMode);
		int32		ReaderCount() const { return fReaderCount; }
		int32		WriterCount() const { return fWriterCount; }

		status_t	Select(uint8 event, uint32 ref, selectsync *sync);
		status_t	Deselect(uint8 event, selectsync *sync);

		static int32 HashNextOffset();
		static uint32 hash_func(void *_node, const void *_key, uint32 range);
		static int compare_func(void *_node, const void *_key);

	private:
		Inode		*fNext;
		Inode		*fHashNext;
		vnode_id	fID;
		int32		fType;
		const char	*fName;
		gid_t		fGroupID;
		uid_t		fUserID;
		time_t		fCreationTime;
		time_t		fModificationTime;

		RingBuffer	fBuffer;

		RequestList	fRequests;

		benaphore	fRequestLock;
		sem_id		fWriteLock;

		int32		fReaderCount;
		int32		fWriterCount;

		select_sync_pool	*fSelectSyncPool;
};


struct dir_cookie {
	dir_cookie		*next;
	dir_cookie		*prev;
	Inode			*current;
	int				state;	// iteration state
};

// directory iteration states
enum {
	ITERATION_STATE_DOT		= 0,
	ITERATION_STATE_DOT_DOT	= 1,
	ITERATION_STATE_OTHERS	= 2,
	ITERATION_STATE_BEGIN	= ITERATION_STATE_DOT,
};

struct file_cookie {
	int				open_mode;
};


//---------------------


RingBuffer::RingBuffer()
{
	fBuffer = create_ring_buffer(PIPEFS_MAX_BUFFER_SIZE);
}


RingBuffer::~RingBuffer()
{
	delete_ring_buffer(fBuffer);
}


inline size_t
RingBuffer::Write(const void *buffer, size_t length)
{
	return ring_buffer_write(fBuffer, (const uint8 *)buffer, length);
}


inline size_t
RingBuffer::Read(void *buffer, size_t length)
{
	return ring_buffer_read(fBuffer, (uint8 *)buffer, length);
}


inline ssize_t
RingBuffer::UserWrite(const void *buffer, ssize_t length)
{
	return ring_buffer_user_write(fBuffer, (const uint8 *)buffer, length);
}


inline ssize_t
RingBuffer::UserRead(void *buffer, ssize_t length)
{
	return ring_buffer_user_read(fBuffer, (uint8 *)buffer, length);
}


inline size_t
RingBuffer::Readable() const
{
	return ring_buffer_readable(fBuffer);
}


inline size_t
RingBuffer::Writable() const
{
	return ring_buffer_writable(fBuffer);
}


//	#pragma mark -


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
	fRootNode = CreateNode(NULL, "", S_IFDIR | 0777);
	if (fRootNode == NULL)
		return;
}


Volume::~Volume()
{
	// put_vnode on the root to release the ref to it
	if (fRootNode)
		put_vnode(ID(), fRootNode->ID());

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
Volume::CreateNode(Inode *parent, const char *name, int32 type)
{
	Inode *inode = new Inode(this, parent, name, type);
	if (inode == NULL)
		return NULL;

	if (inode->InitCheck() != B_OK) {
		delete inode;
		return NULL;
	}

	if (S_ISFIFO(type))
		InsertNode(inode);

	hash_insert(fNodeHash, inode);
	publish_vnode(ID(), inode->ID(), inode);

	if (fRootNode != NULL)
		fRootNode->SetModificationTime(time(NULL));

	return inode;
}


status_t
Volume::DeleteNode(Inode *inode, bool forceDelete)
{
	// remove it from the global hash table
	hash_remove(fNodeHash, inode);

	if (fRootNode != NULL)
		fRootNode->SetModificationTime(time(NULL));

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
	else if (S_ISDIR(inode->Type()))
		status = B_NOT_ALLOWED;

	if (status < B_OK)
		goto err;

	// schedule this vnode to be removed when it's ref goes to zero
	status = remove_vnode(ID(), inode->ID());
	if (status < B_OK)
		goto err;

	RemoveNode(inode);
	notify_entry_removed(ID(), directory->ID(), name, inode->ID());

err:
	Unlock();

	return status;
}


//	#pragma mark -


Inode::Inode(Volume *volume, Inode *parent, const char *name, int32 type)
	:
	fNext(NULL),
	fHashNext(NULL),
	fReaderCount(0),
	fWriterCount(0),
	fSelectSyncPool(NULL)
{
	fName = strdup(name);
	if (fName == NULL)
		return;

	fID = volume->GetNextNodeID();
	fType = type;

	if (S_ISFIFO(type)) {
		fWriteLock = create_sem(1, "pipe write");
		benaphore_init(&fRequestLock, "pipe request");
	}

	fUserID = geteuid();
	fGroupID = parent ? parent->GroupID() : getegid();

	fCreationTime = fModificationTime = time(NULL);
}


Inode::~Inode()
{
	free(const_cast<char *>(fName));

	if (S_ISFIFO(fType)) {
		delete_sem(fWriteLock);
		benaphore_destroy(&fRequestLock);
	}
}


status_t 
Inode::InitCheck()
{
	if (fName == NULL
		|| S_ISFIFO(fType) && (fRequestLock.sem < B_OK || fWriteLock < B_OK))
		return B_ERROR;

	return B_OK;
}


/** Writes the specified data bytes to the inode's ring buffer. The
 *	Inode::WriteLock() must be held when calling this method.
 *	Releases the reader sem if necessary, so that blocking
 *	readers will get started.
 *	Returns B_OK for success, B_BAD_ADDRESS if copying from the buffer failed,
 *	and various semaphore errors (like B_WOULD_BLOCK in non-blocking mode).
 */

status_t
Inode::WriteDataToBuffer(const void **_data, size_t *_bytesLeft, bool nonBlocking)
{
	const void *data = *_data;
	size_t dataSize = *_bytesLeft;

	TRACE(("Inode::WriteDataToBuffer(data = %p, bytes = %lu)\n", data, dataSize));

	// if this is a blocking call, we need to make sure that data can
	// still come in and we don't block the whole inode data transfer
	// to prevent deadlocks from happening

	if (!nonBlocking)
		benaphore_unlock(&fRequestLock);

	status_t status = acquire_sem_etc(fWriteLock, 1, (nonBlocking ? B_TIMEOUT : 0)
						| B_CAN_INTERRUPT, 0);

	if (!nonBlocking)
		benaphore_lock(&fRequestLock);

	if (status != B_OK)
		return status;

	// ensure that we don't write more than PIPEFS_MAX_BUFFER_SIZE
	// into a pipe without blocking

	size_t spaceLeft = fBuffer.Writable();
	if (dataSize > spaceLeft)
		dataSize = spaceLeft;

	if (fBuffer.UserWrite(data, dataSize) < B_OK) {
		release_sem(fWriteLock);
		return B_BAD_ADDRESS;
	}

	*_data = (const void *)((addr_t)data + dataSize);
	*_bytesLeft -= dataSize;

	MayReleaseWriter();

	return B_OK;
}


void 
Inode::MayReleaseWriter()
{
	if (fBuffer.Writable() > 0) {
		if (fSelectSyncPool)
			notify_select_event_pool(fSelectSyncPool, B_SELECT_WRITE);

		release_sem(fWriteLock);
	}
}


/**	This functions fills pending requests out of the ring buffer.
 *	It either stops when there are no more requests to satisfy, or
 *	the ring buffer is empty, whatever comes first.
 *	You must hold the request lock when calling this function.
 */

void
Inode::FillPendingRequests()
{
	size_t bytesLeft = fBuffer.Readable();

	TRACE(("Inode::FillPendingRequests(): bytesLeft = %lu\n", bytesLeft));

	ReadRequest *request;
	RequestList::Iterator iterator = fRequests.GetIterator();
	while (bytesLeft != 0 && (request = iterator.Next()) != NULL) {
		// notify the request, so that it can be filled
		size_t space = request->SpaceLeft();
		if (space == 0)
			continue;

		if (space > bytesLeft)
			bytesLeft = 0;
		else
			bytesLeft -= space;

		request->Notify();
		MayReleaseWriter();
	}
	
	if (fSelectSyncPool)
		notify_select_event_pool(fSelectSyncPool, B_SELECT_READ);
}


/** This function feeds the pending read requests using the provided
 *	buffer directly. It will also make sure that bytes in the ring
 *	buffer are served first.
 *	It only does something as long as the first request in the queue
 *	shares the same team context as the caller (write context).
 *	You must hold the request lock when calling this function.
 */

void
Inode::FillPendingRequests(const void **_data, size_t *_bytesLeft)
{
	team_id team = team_get_current_team_id();

	TRACE(("Inode::FillPendingRequests(data = %p, bytes = %lu)\n", *_data, *_bytesLeft));

	ReadRequest *request;
	RequestList::Iterator iterator = fRequests.GetIterator();
	while (*_bytesLeft != 0 && (request = iterator.Next()) != NULL) {
		// Note that we leave this loop not to mess up the request queue
		if (request->Team() != team)
			break;

		// try to fill this request from the ring buffer first
		size_t bytesRead;
		if (request->PutBuffer(fBuffer, &bytesRead, false) != B_OK)
			continue;

		MayReleaseWriter();

		if (request->SpaceLeft() > 0) {
			// ToDo: This is something where we can optimize the buffer
			//	hand-shaking considerably: we should have a function
			//	that copies the data to another address space - either
			//	remapping copy on write or a direct copy.

			// place our data into that buffer
			request->PutData(_data, _bytesLeft);
		}
	}
}


/** This function adds a request into the queue.
 *	If there is already data waiting in the pipe, the request will
 *	be fulfilled.
 *	This function is called from within the readers thread only.
 */

status_t
Inode::AddRequest(ReadRequest &request)
{
	BenaphoreLocker locker(RequestLock());
	if (!locker.IsLocked())
		return B_ERROR;

	if (BytesAvailable() > 0 && request.PutBuffer(fBuffer) == B_OK) {
		MayReleaseWriter();
	} else {
		fRequests.Add(&request);
		request.SetQueued(true);
	}

	return B_OK;
}


/** This function removes a request from the queue.
 *	This function is called from within the readers thread only.
 */

status_t
Inode::RemoveRequest(ReadRequest &request)
{
	TRACE(("Inode::RemoveRequest(request = %p)\n", &request));

	BenaphoreLocker locker(RequestLock());
	if (!locker.IsLocked())
		return B_ERROR;

	// we might have some data waiting now, if the direct team
	// handshake couldn't be done

	if (BytesAvailable() > 0 && request.PutBuffer(fBuffer) == B_OK)
		release_sem(fWriteLock);

	if (request.IsQueued())
		fRequests.Remove(&request);

	return B_OK;
}


void
Inode::Open(int openMode)
{
	if ((openMode & O_ACCMODE) == O_WRONLY)
		atomic_add(&fWriterCount, 1);

	if ((openMode & O_ACCMODE) == O_RDONLY || (openMode & O_ACCMODE) == O_RDWR)
		atomic_add(&fReaderCount, 1);
}


void
Inode::Close(int openMode)
{
	TRACE(("Inode::Close(openMode = %d)\n", openMode));

	if ((openMode & O_ACCMODE) == O_WRONLY
		&& atomic_add(&fWriterCount, -1) == 1) {
		// Our last writer has been closed; if the pipe
		// contains no data, unlock all waiting readers

		BenaphoreLocker locker(RequestLock());

		if (BytesAvailable() == 0) {
			ReadRequest *request;
			RequestList::Iterator iterator = fRequests.GetIterator();
			while ((request = iterator.Next()) != NULL)
				request->Abort();
		}
	}

	if ((openMode & O_ACCMODE) == O_RDONLY || (openMode & O_ACCMODE) == O_RDWR)
		atomic_add(&fReaderCount, -1);
}


int32
Inode::HashNextOffset()
{
	Inode *inode;
	return (addr_t)&inode->fHashNext - (addr_t)inode;
}


uint32
Inode::hash_func(void *_node, const void *_key, uint32 range)
{
	Inode *inode = (Inode *)_node;
	const vnode_id *key = (const vnode_id *)_key;

	if (inode != NULL)
		return inode->ID() % range;

	return (uint64)*key % range;
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


status_t
Inode::Select(uint8 event, uint32 ref, selectsync *sync)
{
	if (add_select_sync_pool_entry(&fSelectSyncPool, sync, ref, event) != B_OK)
		return B_ERROR;

	if ((event == B_SELECT_READ && BytesAvailable() > 0)
		|| (event == B_SELECT_WRITE && fBuffer.Writable() > 0))
		return notify_select_event(sync, ref, event);

	return B_OK;
}


status_t
Inode::Deselect(uint8 event, selectsync *sync)
{
	remove_select_sync_pool_entry(&fSelectSyncPool, sync, event);
	return B_OK;
}


//	#pragma mark -


ReadRequest::ReadRequest(void *buffer, size_t bufferSize)
	:
	fBuffer(buffer),
	fBufferSize(bufferSize),
	fBytesRead(0),
	fIsQueued(false)
{
	fLock = create_sem(0, "request lock");
	fTeam = team_get_current_team_id();
}


ReadRequest::~ReadRequest()
{
	delete_sem(fLock);
}


status_t 
ReadRequest::Wait(bool nonBlocking)
{
	TRACE(("pipefs: request@%p waits for data (%sblocking), thread 0x%lx\n",
		this, nonBlocking ? "non" : "", find_thread(NULL)));
	return acquire_sem_etc(fLock, 1, (nonBlocking ? B_TIMEOUT : 0) | B_CAN_INTERRUPT, 0);
}


void
ReadRequest::Notify()
{
	release_sem(fLock);
}


void
ReadRequest::Abort()
{
	fBuffer = NULL;
	release_sem(fLock);
}


void
ReadRequest::SetQueued(bool queued)
{
	fIsQueued = queued;
}


/** Reads the contents of the ring buffer into the requests data buffer
 *	if any (canceled requests don't have a buffer anymore).
 *	This function must only be called in the team context that initiated
 *	the read request.
 */

status_t
ReadRequest::PutBuffer(RingBuffer &buffer, size_t *_bytesRead, bool releasePartiallyFilled)
{
	TRACE(("pipefs: ReadRequest::PutBuffer()\n"));

	if (_bytesRead)
		*_bytesRead = 0;

	if (fBuffer == NULL)
		return B_CANCELED;

	size_t length = buffer.Readable();
	size_t spaceLeft = SpaceLeft();

	// we read spaceLeft bytes at maximum - but never
	// more than there are in the buffer
	if (spaceLeft < length)
		length = spaceLeft;

	if (length == 0)
		return B_OK;

	if (buffer.UserRead(fBuffer, length) < B_OK) {
		// if the buffer is just invalid, we release the reader as well
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	fBytesRead += length;
	if (_bytesRead)
		*_bytesRead = length;

	if (releasePartiallyFilled || SpaceLeft() == 0)
		release_sem(fLock);

	return B_OK;
}


/** Copies the specified buffer into the request. This function currently
 *	only works for the local address space (both, sender and receiver must
 *	be in the same address space).
 */

status_t
ReadRequest::PutData(const void **_buffer, size_t *_bytesLeft)
{
	TRACE(("pipefs: ReadRequest::PutData(buffer = %p, size = %lu)\n", *_buffer, *_bytesLeft));

	size_t bytes = *_bytesLeft;
	if (bytes > SpaceLeft())
		bytes = SpaceLeft();

	uint8 *source = (uint8 *)*_buffer;

	if (user_memcpy((uint8 *)fBuffer + fBytesRead, source, bytes) < B_OK) {
		release_sem(fLock);
		return B_BAD_ADDRESS;
	}

	fBytesRead += bytes;
	*_buffer = (void *)(source + bytes);
	*_bytesLeft -= bytes;

	release_sem(fLock);
	return B_OK;
}


//	#pragma mark -


static status_t
pipefs_mount(mount_id id, const char *device, uint32 flags, const char *args,
	fs_volume *_volume, vnode_id *_rootVnodeID)
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
	if (!S_ISDIR(directory->Type()))
		return B_NOT_A_DIRECTORY;

	volume->Lock();

	// look it up
	Inode *inode = volume->FindNode(name);
	if (inode == NULL) {
		status = B_ENTRY_NOT_FOUND;
		goto err;
	}

	Inode *dummy;
	status = get_vnode(volume->ID(), inode->ID(), (fs_vnode *)&dummy);
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
	Inode *inode;

	TRACE(("pipefs_getvnode: asking for vnode 0x%Lx, r %d\n", id, reenter));

	if (!reenter)
		volume->Lock();

	inode = volume->Lookup(id);

	if (!reenter)
		volume->Unlock();

	TRACE(("pipefs_getnvnode: looked it up at %p\n", *_inode));

	if (inode == NULL)
		return B_ENTRY_NOT_FOUND;

	*_inode = inode;
	return B_OK;
}


static status_t
pipefs_put_vnode(fs_volume _volume, fs_vnode _node, bool reenter)
{
#ifdef TRACE_PIPEFS
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
pipefs_create(fs_volume _volume, fs_vnode _dir, const char *name, int openMode, int mode,
	fs_cookie *_cookie, vnode_id *_newVnodeID)
{
	Volume *volume = (Volume *)_volume;
	bool wasCreated = true;

	TRACE(("pipefs_create(): dir = %p, name = '%s', perms = %d, &id = %p\n",
		_dir, name, mode, _newVnodeID));

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	volume->Lock();

	Inode *directory = (Inode *)_dir;
	status_t status = B_OK;

	Inode *inode = volume->FindNode(name);
	if (inode != NULL && (openMode & O_EXCL) != 0) {
		status = B_FILE_EXISTS;
		goto err;
	}

	if (inode == NULL) {
		// we need to create a new pipe
		inode = volume->CreateNode(directory, name, S_IFIFO | mode);
		if (inode == NULL) {
			status = B_NO_MEMORY;
			goto err;
		}
	} else {
		// we can just open the pipe again
		void *dummy;
		get_vnode(volume->ID(), inode->ID(), &dummy);
		wasCreated = false;
	}

	volume->Unlock();

	cookie->open_mode = openMode;
	inode->Open(openMode);

	TRACE(("  create cookie = %p, node = %p\n", cookie, inode));
	*_cookie = (void *)cookie;
	*_newVnodeID = inode->ID();

	if (wasCreated)
		notify_entry_created(volume->ID(), directory->ID(), name, inode->ID());

	return B_OK;

err:
	volume->Unlock();
	free(cookie);

	return status;
}


static status_t
pipefs_open(fs_volume _volume, fs_vnode _node, int openMode, fs_cookie *_cookie)
{
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_open(): node = %p, openMode = %d\n", inode, openMode));

	file_cookie *cookie = (file_cookie *)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	TRACE(("  open cookie = %p\n", cookie));
	cookie->open_mode = openMode;
	inode->Open(openMode);

	*_cookie = (void *)cookie;

	return B_OK;
}


static status_t
pipefs_close(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_close: entry vnode %p, cookie %p\n", _node, _cookie));

	inode->Close(cookie->open_mode);
	return B_OK;
}


static status_t
pipefs_free_cookie(fs_volume _volume, fs_vnode _node, fs_cookie _cookie)
{
	file_cookie *cookie = (file_cookie *)_cookie;

	TRACE(("pipefs_freecookie: entry vnode %p, cookie %p\n", _node, _cookie));

	free(cookie);

	return B_OK;
}


static status_t
pipefs_fsync(fs_volume _volume, fs_vnode _v)
{
	return B_OK;
}


static status_t
pipefs_read(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t /*pos*/,
	void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_read(vnode = %p, cookie = %p, length = %lu, mode = %d)\n", _node, cookie, *_length,
		cookie->open_mode));

	if ((cookie->open_mode & O_RWMASK) != O_RDONLY)
		return B_NOT_ALLOWED;

	if (inode->WriterCount() == 0) {
		BenaphoreLocker locker(inode->RequestLock());

		// as long there is no writer, and the pipe is empty,
		// we always just return 0 to indicate end of file
		if (inode->BytesAvailable() == 0) {
			*_length = 0;
			return B_OK;
		}
	}

	// issue read request

	ReadRequest request(buffer, *_length);
	if (inode->AddRequest(request) != B_OK)
		return B_ERROR;

	// wait for it to be filled

	status_t status = request.Wait((cookie->open_mode & O_NONBLOCK) != 0);
	inode->RemoveRequest(request);

	if ((status == B_TIMED_OUT || status == B_INTERRUPTED || status == B_WOULD_BLOCK)
		&& request.BytesRead() > 0)
		status = B_OK;

	if (status == B_OK)
		*_length = request.BytesRead();

	return status;
}


static status_t
pipefs_write(fs_volume _volume, fs_vnode _node, fs_cookie _cookie, off_t /*pos*/,
	const void *buffer, size_t *_length)
{
	file_cookie *cookie = (file_cookie *)_cookie;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_write(vnode = %p, cookie = %p, length = %lu)\n", _node, cookie, *_length));

	if ((cookie->open_mode & O_RWMASK) != O_WRONLY)
		return B_NOT_ALLOWED;

	if (inode->ReaderCount() == 0) {
		// if there is no reader yet, we signal the thread and return
		send_signal(find_thread(NULL), SIGPIPE);
		return EPIPE;
	}

	BenaphoreLocker locker(inode->RequestLock());

	size_t bytesLeft = *_length;
	inode->FillPendingRequests(&buffer, &bytesLeft);	

	status_t status;

	if (bytesLeft != 0) {
		// we could not place all our data in pending requests, so
		// have to put them into a temporary buffer

		status = inode->WriteDataToBuffer(&buffer, &bytesLeft, (cookie->open_mode & O_NONBLOCK) != 0);
		if (status == B_OK) {
			inode->FillPendingRequests();
			*_length -= bytesLeft;
		}
	} else {
		// could write everything without the need to copy!
		status = B_OK;
	}

	return status;
}


static status_t
pipefs_create_dir(fs_volume _volume, fs_vnode _dir, const char *name, int perms, vnode_id *_newID)
{
	TRACE(("pipefs: create directory \"%s\" requested...\n", name));
	return B_NOT_SUPPORTED;
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
	if (!S_ISDIR(inode->Type()))
		return B_BAD_VALUE;

	if (inode != &volume->RootNode())
		panic("pipefs: found directory that's not the root!");

	dir_cookie *cookie = (dir_cookie *)malloc(sizeof(dir_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	volume->Lock();

	cookie->current = volume->FirstEntry();
	cookie->state = ITERATION_STATE_BEGIN;
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
	Inode *inode = (Inode *)_node;
	status_t status = 0;

	TRACE(("pipefs_read_dir: vnode %p, cookie %p, buffer = %p, bufferSize = %ld, num = %p\n", _node, _cookie, dirent, bufferSize,_num));

	if (_node != &volume->RootNode())
		return B_BAD_VALUE;

	volume->Lock();

	dir_cookie *cookie = (dir_cookie *)_cookie;
	Inode *childNode = NULL;
	const char *name = NULL;
	Inode *nextChildNode = NULL;
	int nextState = cookie->state;

	switch (cookie->state) {
		case ITERATION_STATE_DOT:
			childNode = inode;
			name = ".";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		case ITERATION_STATE_DOT_DOT:
			childNode = inode; // parent of the root node is the root node
			name = "..";
			nextChildNode = volume->FirstEntry();
			nextState = cookie->state + 1;
			break;
		default:
			childNode = cookie->current;
			if (childNode) {
				name = childNode->Name();
				nextChildNode = childNode->Next();
			}
			break;
	}

	if (!childNode) {
		// we're at the end of the directory
		*_num = 0;
		status = B_OK;
		goto err;
	}

	dirent->d_dev = volume->ID();
	dirent->d_ino = inode->ID();
	dirent->d_reclen = strlen(name) + sizeof(struct dirent);

	if (dirent->d_reclen > bufferSize) {
		status = ENOBUFS;
		goto err;
	}

	status = user_strlcpy(dirent->d_name, name, bufferSize);
	if (status < 0)
		goto err;

	cookie->current = nextChildNode;
	cookie->state = nextState;
	status = B_OK;

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
	cookie->state = ITERATION_STATE_BEGIN;

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
pipefs_set_flags(fs_volume _volume, fs_vnode _vnode, fs_cookie _cookie, int flags)
{
	file_cookie *cookie = (file_cookie *)_cookie;

	TRACE(("pipefs_set_flags(vnode = %p, flags = %x)\n", _vnode, flags));
	cookie->open_mode = (cookie->open_mode & ~(O_APPEND | O_NONBLOCK)) | flags;
	return B_OK;
}


static status_t
pipefs_select(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, uint8 event,
	uint32 ref, selectsync *sync)
{
	TRACE(("pipefs_select(vnode = %p)\n", _node));
	Inode *inode = (Inode *)_node;
	if (!inode)
		return B_ERROR;

	BenaphoreLocker locker(inode->RequestLock());
	return inode->Select(event, ref, sync);
}


static status_t
pipefs_deselect(fs_volume _fs, fs_vnode _node, fs_cookie _cookie, uint8 event,
	selectsync *sync)
{
	TRACE(("pipefs_deselect(vnode = %p)\n", _node));
	Inode *inode = (Inode *)_node;
	if (!inode)
		return B_ERROR;
	
	BenaphoreLocker locker(inode->RequestLock());
	return inode->Deselect(event, sync);
}


static bool
pipefs_can_page(fs_volume _volume, fs_vnode _v, fs_cookie cookie)
{
	return false;
}


static status_t
pipefs_read_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
{
	return EPERM;
}


static status_t
pipefs_write_pages(fs_volume _volume, fs_vnode _v, fs_cookie cookie, off_t pos,
	const iovec *vecs, size_t count, size_t *_numBytes, bool reenter)
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

	TRACE(("pipefs_read_stat: vnode %p (0x%Lx), stat %p\n", inode, inode->ID(), stat));

	stat->st_dev = volume->ID();
	stat->st_ino = inode->ID();

	BenaphoreLocker locker(inode->RequestLock());

	stat->st_size = inode->BytesAvailable();
	stat->st_mode = inode->Type();

	stat->st_nlink = 1;
	stat->st_blksize = 4096;

	stat->st_uid = inode->UserID();
	stat->st_gid = inode->GroupID();

	stat->st_atime = time(NULL);
	stat->st_mtime = stat->st_ctime = inode->ModificationTime();
	stat->st_crtime = inode->CreationTime();

	return B_OK;
}


static status_t
pipefs_write_stat(fs_volume _volume, fs_vnode _node, const struct stat *stat, uint32 statMask)
{
	Volume *volume = (Volume *)_volume;
	Inode *inode = (Inode *)_node;

	TRACE(("pipefs_write_stat: vnode %p (0x%Lx), stat %p\n", inode, inode->ID(), stat));

	// we cannot change the size of anything
	if (statMask & B_STAT_SIZE)
		return B_BAD_VALUE;

	volume->Lock();
		// the inode's locks don't exist for the root node
		// (but would be more appropriate to use here)

	if (statMask & B_STAT_MODE)
		inode->SetMode(stat->st_mode);

	if (statMask & B_STAT_UID)
		inode->SetUserID(stat->st_uid);
	if (statMask & B_STAT_GID)
		inode->SetGroupID(stat->st_gid);

	if (statMask & B_STAT_MODIFICATION_TIME)
		inode->SetModificationTime(stat->st_mtime);
	if (statMask & B_STAT_CREATION_TIME) 
		inode->SetCreationTime(stat->st_crtime);

	volume->Unlock();

	notify_stat_changed(volume->ID(), inode->ID(), statMask);
	return B_OK;
}


static status_t
pipefs_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}

}	// namespace pipefs

using namespace pipefs;

file_system_module_info gPipeFileSystem = {
	{
		"file_systems/pipefs" B_CURRENT_FS_API_VERSION,
		0,
		pipefs_std_ops,
	},

	"Pipe File System",

	NULL,	// identify_partition()
	NULL,	// scan_partition()
	NULL,	// free_identify_partition_cookie()
	NULL,	// free_partition_content_cookie()

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

	NULL,	// get_file_map()

	/* common */
	&pipefs_ioctl,
	&pipefs_set_flags,
	&pipefs_select,
	&pipefs_deselect,
	&pipefs_fsync,

	NULL,	// fs_read_link()
	NULL,	// fs_symlink()
	NULL,	// fs_link()
	&pipefs_unlink,
	NULL,	// fs_rename()

	NULL,	// fs_access()
	&pipefs_read_stat,
	&pipefs_write_stat,

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

