/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Copyright 2007-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "fifo.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <new>

#include <KernelExport.h>
#include <NodeMonitor.h>
#include <Select.h>

#include <condition_variable.h>
#include <debug_hex_dump.h>
#include <lock.h>
#include <select_sync_pool.h>
#include <syscall_restart.h>
#include <team.h>
#include <thread.h>
#include <slab/Slab.h>
#include <util/DoublyLinkedList.h>
#include <util/AutoLock.h>
#include <util/ring_buffer.h>
#include <vfs.h>
#include <vfs_defs.h>
#include <vm/vm.h>


//#define TRACE_FIFO
#ifdef TRACE_FIFO
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...)
#endif


namespace fifo {


struct file_cookie;
class Inode;

static object_cache* sRingBufferCache;


class RingBuffer {
public:
								RingBuffer();
								~RingBuffer();

			status_t			CreateBuffer();
			void				DeleteBuffer();

			ssize_t				Write(const void* buffer, size_t length,
									size_t minimum, bool isUser, bool* wasEmpty);
			ssize_t				Read(void* buffer, size_t length, bool isUser, bool* wasFull);
			ssize_t				DebugPeek(size_t offset, uint8* out) const;

			size_t				Readable() const;
			size_t				Writable() const;

private:
			uint8*				fBuffer;
			uint32				fBufferSize;

			mutex				fWriteLock;
			uint32				fWriteHead;

			uint32				fWriteAvailable;
			uint32				fReadHead;
};


class ReadRequest : public DoublyLinkedListLinkImpl<ReadRequest> {
public:
	ReadRequest(file_cookie* cookie)
		:
		fThread(thread_get_current_thread()),
		fCookie(cookie),
		fNotified(true)
	{
		B_INITIALIZE_SPINLOCK(&fLock);
	}

	bool IsNotified()
	{
		InterruptsSpinLocker _(fLock);
		return fNotified;
	}

	void SetNotified(bool notified)
	{
		InterruptsSpinLocker _(fLock);
		fNotified = notified;
	}

	void Notify(status_t status = B_OK)
	{
		InterruptsSpinLocker spinLocker(fLock);
		TRACE("ReadRequest %p::Notify(), fNotified %d\n", this, fNotified);

		if (!fNotified) {
			fNotified = true;
			thread_unblock(fThread, status);
		}
	}

	Thread* GetThread() const
	{
		return fThread;
	}

	file_cookie* Cookie() const
	{
		return fCookie;
	}

private:
	spinlock		fLock;
	Thread*			fThread;
	file_cookie*	fCookie;
	bool			fNotified;
};


class WriteRequest : public DoublyLinkedListLinkImpl<WriteRequest> {
public:
	WriteRequest(Thread* thread, size_t minimalWriteCount)
		:
		fThread(thread),
		fMinimalWriteCount(minimalWriteCount)
	{
	}

	Thread* GetThread() const
	{
		return fThread;
	}

	size_t MinimalWriteCount() const
	{
		return fMinimalWriteCount;
	}

private:
	Thread*	fThread;
	size_t	fMinimalWriteCount;
};


typedef DoublyLinkedList<ReadRequest> ReadRequestList;
typedef DoublyLinkedList<WriteRequest> WriteRequestList;


class Inode {
public:
								Inode();
								~Inode();

			status_t			InitCheck();

			bool				IsActive() const { return fActive; }
			timespec			CreationTime() const { return fCreationTime; }
			void				SetCreationTime(timespec creationTime)
									{ fCreationTime = creationTime; }
			timespec			ModificationTime() const
									{ return fModificationTime; }
			void				SetModificationTime(timespec modificationTime)
									{ fModificationTime = modificationTime; }

			rw_lock*			ChangeLock() { return &fChangeLock; }

			status_t			Write(const void* data,
									size_t* _length, bool nonBlocking,
									bool isUser);
			status_t			ReadDataFromBuffer(void* data, size_t* _length,
									bool nonBlocking, bool isUser,
									ReadRequest& request);
			size_t				BytesAvailable() const
									{ return fBuffer.Readable(); }
			size_t				BytesWritable() const
									{ return fBuffer.Writable(); }

			void				AddReadRequest(ReadRequest& request);
			void				RemoveReadRequest(ReadRequest& request);
			status_t			WaitForReadRequest(ReadRequest& request);

			void				NotifyBytesRead(bool wasFull, size_t bytes);
			void				NotifyReadDone();
			void				NotifyBytesWritten(bool wasEmpty);
			void				NotifyEndClosed(bool writer);

			status_t			Open(int openMode);
			void				Close(file_cookie* cookie);
			int32				ReaderCount() const { return fReaderCount; }
			int32				WriterCount() const { return fWriterCount; }

			status_t			Select(uint8 event, selectsync* sync,
									int openMode);
			status_t			Deselect(uint8 event, selectsync* sync,
									int openMode);

			void				Dump(bool dumpData) const;
	static	int					Dump(int argc, char** argv);

private:
			timespec			fCreationTime;
			timespec			fModificationTime;

			RingBuffer			fBuffer;

			spinlock			fReadRequestsLock;
			spinlock			fWriteRequestsLock;
			ReadRequestList		fReadRequests;
			WriteRequestList	fWriteRequests;

			rw_lock				fChangeLock;
			ConditionVariable	fActiveCondition;

			int32				fReaderCount;
			int32				fWriterCount;
			bool				fActive;

			select_sync_pool*	fReadSelectSyncPool;
			select_sync_pool*	fWriteSelectSyncPool;
};


class FIFOInode : public Inode {
public:
	FIFOInode(fs_vnode* vnode)
		:
		Inode(),
		fSuperVnode(*vnode)
	{
	}

	fs_vnode*	SuperVnode() { return &fSuperVnode; }

private:
	fs_vnode	fSuperVnode;
};


struct file_cookie {
	int	open_mode;
			// guarded by Inode::fChangeLock

	void SetNonBlocking(bool nonBlocking)
	{
		if (nonBlocking)
			open_mode |= O_NONBLOCK;
		else
			open_mode &= ~(int)O_NONBLOCK;
	}
};


// #pragma mark -


RingBuffer::RingBuffer()
	:
	fBuffer(NULL)
{
	mutex_init(&fWriteLock, "fifo ring write");
}


RingBuffer::~RingBuffer()
{
	DeleteBuffer();
	mutex_destroy(&fWriteLock);
}


status_t
RingBuffer::CreateBuffer()
{
	if (fBuffer != NULL)
		return B_OK;

	fBuffer = (uint8*)object_cache_alloc(sRingBufferCache, 0);
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	fWriteAvailable = fBufferSize = VFS_FIFO_BUFFER_CAPACITY;
	fReadHead = fWriteHead = 0;
	return B_OK;
}


void
RingBuffer::DeleteBuffer()
{
	if (fBuffer != NULL) {
		object_cache_free(sRingBufferCache, fBuffer, 0);
		fBuffer = NULL;
	}
}


inline ssize_t
RingBuffer::Write(const void* data, size_t length, size_t minimum, bool isUser, bool* wasEmpty)
{
	if (fBuffer == NULL)
		return B_NO_MEMORY;
	if (isUser && !IS_USER_ADDRESS(data))
		return B_BAD_ADDRESS;

	MutexLocker _(fWriteLock);
	uint32 writeAvailable = atomic_get((int32*)&fWriteAvailable);
	if (writeAvailable == 0 || writeAvailable < minimum)
		return 0;
	if (length > writeAvailable)
		length = writeAvailable;

	uint32 position = fWriteHead;
	if ((position + length) <= fBufferSize) {
		// simple copy
		if (isUser) {
			if (user_memcpy(fBuffer + position, data, length) != B_OK)
				return B_BAD_ADDRESS;
		} else
			memcpy(fBuffer + position, data, length);
	} else {
		// need to copy both ends
		uint32 upper = fBufferSize - position;
		uint32 lower = length - upper;

		if (isUser) {
			if (user_memcpy(fBuffer + position, data, upper) != B_OK
					|| user_memcpy(fBuffer, (uint8*)data + upper, lower) != B_OK)
				return B_BAD_ADDRESS;
		} else {
			memcpy(fBuffer + position, data, upper);
			memcpy(fBuffer, (uint8*)data + upper, lower);
		}
	}

	atomic_set((int32*)&fWriteHead, (fWriteHead + length) % fBufferSize);
	uint32 previouslyAvailable = atomic_add((int32*)&fWriteAvailable, -length);

	if (wasEmpty != NULL)
		*wasEmpty = (previouslyAvailable == fBufferSize);

	return length;
}


inline ssize_t
RingBuffer::Read(void* data, size_t length, bool isUser, bool* wasFull)
{
	if (fBuffer == NULL)
		return B_NO_MEMORY;
	if (isUser && !IS_USER_ADDRESS(data))
		return B_BAD_ADDRESS;

	uint32 readHead = 0;
	uint32 readable = 0;
	for (int retries = 3; retries != 0; retries--) {
		// atomic ordering shouldn't matter, so long as
		// the add() at the end comes after the test_and_set()
		uint32 readEnd = atomic_get((int32*)&fWriteHead);
		readHead = atomic_get((int32*)&fReadHead);
		if (readEnd < readHead || (readEnd == readHead && fWriteAvailable == 0))
			readEnd += fBufferSize;

		readable = readEnd - readHead;
		if (readable == 0)
			break;
		if (readable > length)
			readable = length;

		// move the read head forwards
		if ((uint32)atomic_test_and_set((int32*)&fReadHead,
				(readHead + length) % fBufferSize, readHead) == readHead)
			break;

		readable = 0;
	}

	if (readable == 0)
		return 0;
	length = readable;

	status_t status = B_OK;
	if ((readHead + length) <= fBufferSize) {
		// simple copy
		if (isUser) {
			if (user_memcpy(data, fBuffer + readHead, length) != B_OK)
				status = B_BAD_ADDRESS;
		} else
			memcpy(data, fBuffer + readHead, length);
	} else {
		// need to copy both ends
		size_t upper = fBufferSize - readHead;
		size_t lower = length - upper;

		if (isUser) {
			if (user_memcpy(data, fBuffer + readHead, upper) != B_OK
				|| user_memcpy((uint8*)data + upper, fBuffer, lower) != B_OK)
				status = B_BAD_ADDRESS;
		} else {
			memcpy(data, fBuffer + readHead, upper);
			memcpy((uint8*)data + upper, fBuffer, lower);
		}
	}

	// release the buffer space
	uint32 previouslyAvailable = atomic_add((int32*)&fWriteAvailable, length);
	if (status != B_OK)
		return status;

	if (wasFull != NULL)
		*wasFull = (previouslyAvailable == 0);
	return length;
}


inline ssize_t
RingBuffer::DebugPeek(size_t offset, uint8* out) const
{
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	uint32 readEnd = fWriteHead;
	if (readEnd < fReadHead || (readEnd == fReadHead && fWriteAvailable == 0))
		readEnd += fBufferSize;

	if ((fReadHead + offset) >= readEnd)
		return 0;

	*out = fBuffer[(fReadHead + offset) % fBufferSize];
	return 1;
}


inline size_t
RingBuffer::Readable() const
{
	// This is slightly inaccurate, as any currently-in-progress reads
	// will not be accounted for here.
	return fBufferSize - atomic_get((int32*)&fWriteAvailable);
}


inline size_t
RingBuffer::Writable() const
{
	return atomic_get((int32*)&fWriteAvailable);
}


//	#pragma mark -


Inode::Inode()
	:
	fReadRequestsLock(B_SPINLOCK_INITIALIZER),
	fWriteRequestsLock(B_SPINLOCK_INITIALIZER),
	fReadRequests(),
	fWriteRequests(),
	fReaderCount(0),
	fWriterCount(0),
	fActive(false),
	fReadSelectSyncPool(NULL),
	fWriteSelectSyncPool(NULL)
{
	rw_lock_init(&fChangeLock, "fifo change");
	fActiveCondition.Init(this, "fifo");

	bigtime_t time = real_time_clock();
	fModificationTime.tv_sec = time / 1000000;
	fModificationTime.tv_nsec = (time % 1000000) * 1000;
	fCreationTime = fModificationTime;
}


Inode::~Inode()
{
	rw_lock_destroy(&fChangeLock);
}


status_t
Inode::InitCheck()
{
	return B_OK;
}


/*!	Writes the specified data bytes to the inode's ring buffer. The
	request lock must be held when calling this method.
	Notifies readers if necessary, so that blocking readers will get started.
	Returns B_OK for success, B_BAD_ADDRESS if copying from the buffer failed,
	and various semaphore errors (like B_WOULD_BLOCK in non-blocking mode). If
	the returned length is > 0, the returned error code can be ignored.
*/
status_t
Inode::Write(const void* _data, size_t* _length, bool nonBlocking,
	bool isUser)
{
	const uint8* data = (const uint8*)_data;
	size_t dataSize = *_length;
	size_t& written = *_length;
	written = 0;

	TRACE("Inode %p::Write(data = %p, bytes = %zu)\n", this, data,
		dataSize);

	ReadLocker changeLocker(ChangeLock());

	// A request up to VFS_FIFO_ATOMIC_WRITE_SIZE bytes shall not be
	// interleaved with other writer's data.
	size_t minToWrite = 1;
	if (dataSize <= VFS_FIFO_ATOMIC_WRITE_SIZE)
		minToWrite = dataSize;

	while (dataSize > 0) {
		// Wait until enough space in the buffer is available.
		while (!fActive || (fBuffer.Writable() < minToWrite && fReaderCount > 0)) {
			if (nonBlocking)
				return B_WOULD_BLOCK;

			ConditionVariableEntry entry;
			fActiveCondition.Add(&entry);

			InterruptsSpinLocker writeRequestsLocker(fWriteRequestsLock);
			WriteRequest request(thread_get_current_thread(), minToWrite);
			fWriteRequests.Add(&request);
			writeRequestsLocker.Unlock();

			TRACE("Inode %p::%s(): wait for writable, request %p\n", this, __FUNCTION__,
				&request);

			status_t status = B_OK;
			// the situation might have changed, recheck before waiting
			if (!fActive || (fBuffer.Writable() < minToWrite && fReaderCount > 0)) {
				changeLocker.Unlock();
				status = entry.Wait(B_CAN_INTERRUPT);
				changeLocker.Lock();
			}

			writeRequestsLocker.Lock();
			fWriteRequests.Remove(&request);
			writeRequestsLocker.Unlock();

			if (status != B_OK)
				return status;
		}

		// write only as long as there are readers left
		if (fActive && fReaderCount == 0) {
			if (written == 0)
				send_signal(find_thread(NULL), SIGPIPE);
			return EPIPE;
		}

		// write as much as we can

		size_t toWrite = (fActive ? fBuffer.Writable() : 0);
		if (toWrite > dataSize)
			toWrite = dataSize;
		if (toWrite == 0)
			continue;

		bool wasEmpty = false;
		ssize_t bytesWritten = fBuffer.Write(data, toWrite, minToWrite, isUser, &wasEmpty);
		if (bytesWritten < 0)
			return bytesWritten;
		if (bytesWritten == 0)
			continue;

		data += bytesWritten;
		dataSize -= bytesWritten;
		written += bytesWritten;

		NotifyBytesWritten(wasEmpty);
	}

	return B_OK;
}


status_t
Inode::ReadDataFromBuffer(void* data, size_t* _length, bool nonBlocking,
	bool isUser, ReadRequest& request)
{
	size_t dataSize = *_length;
	*_length = 0;

	// wait until our request is first in queue
	status_t error;
	if (fReadRequests.Head() != &request) {
		if (nonBlocking)
			return B_WOULD_BLOCK;

		TRACE("Inode %p::%s(): wait for request %p to become the first "
			"request.\n", this, __FUNCTION__, &request);

		error = WaitForReadRequest(request);
		if (error != B_OK)
			return error;
	}

	while (dataSize > 0) {
		// wait until data are available
		while (fBuffer.Readable() == 0) {
			if (nonBlocking)
				return B_WOULD_BLOCK;

			if (fActive && fWriterCount == 0)
				return B_OK;

			TRACE("Inode %p::%s(): wait for data, request %p\n", this, __FUNCTION__,
				&request);

			request.SetNotified(false);
			// the situation might have changed, recheck before waiting
			if (fBuffer.Readable() != 0) {
				request.SetNotified(true);
				break;
			}

			error = WaitForReadRequest(request);
			if (error != B_OK)
				return error;
		}

		// read as much as we can
		size_t toRead = fBuffer.Readable();
		if (toRead > dataSize)
			toRead = dataSize;

		bool wasFull = false;
		ssize_t bytesRead = fBuffer.Read(data, toRead, isUser, &wasFull);
		if (bytesRead < 0)
			return bytesRead;
		if (bytesRead == 0)
			continue;

		NotifyBytesRead(wasFull, bytesRead);

		*_length = bytesRead;
		break;
	}

	return B_OK;
}


void
Inode::AddReadRequest(ReadRequest& request)
{
	InterruptsSpinLocker _(fReadRequestsLock);
	fReadRequests.Add(&request);
}


void
Inode::RemoveReadRequest(ReadRequest& request)
{
	InterruptsSpinLocker _(fReadRequestsLock);
	fReadRequests.Remove(&request);
}


status_t
Inode::WaitForReadRequest(ReadRequest& request)
{
	// add the entry to wait on
	thread_prepare_to_block(thread_get_current_thread(), B_CAN_INTERRUPT,
		THREAD_BLOCK_TYPE_OTHER, "fifo read request");

	if (request.IsNotified())
		return B_OK;

	// wait
	rw_lock_read_unlock(&fChangeLock);
	status_t status = thread_block();

	if (status != B_OK) {
		// Before going to lock again, we need to make sure no one tries to
		// unblock us. Otherwise that would screw with mutex_lock().
		request.SetNotified(true);
	}

	rw_lock_read_lock(&fChangeLock);

	return status;
}


void
Inode::NotifyBytesRead(bool wasFull, size_t bytes)
{
	// notify writer, if something can be written now
	size_t writable = fBuffer.Writable();
	if (bytes > 0) {
		// notify select()ors only, if nothing was writable before
		if (wasFull) {
			if (fWriteSelectSyncPool != NULL)
				notify_select_event_pool(fWriteSelectSyncPool, B_SELECT_WRITE);
		}

		// If any of the waiting writers has a minimal write count that has
		// now become satisfied, we notify all of them (condition variables
		// don't support doing that selectively).
		InterruptsSpinLocker _(fWriteRequestsLock);
		WriteRequest* request;
		WriteRequestList::Iterator iterator = fWriteRequests.GetIterator();
		while ((request = iterator.Next()) != NULL) {
			size_t minWriteCount = request->MinimalWriteCount();
			if (minWriteCount > 0 && minWriteCount <= writable
					&& minWriteCount > writable - bytes) {
				fActiveCondition.NotifyAll();
				break;
			}
		}
	}
}


void
Inode::NotifyReadDone()
{
	// notify next reader, if there's still something to be read
	if (fBuffer.Readable() > 0) {
		InterruptsSpinLocker _(fReadRequestsLock);
		if (ReadRequest* request = fReadRequests.First())
			request->Notify();
	}
}


void
Inode::NotifyBytesWritten(bool wasEmpty)
{
	// notify reader, if something can be read now
	if (wasEmpty && fBuffer.Readable() > 0) {
		if (fReadSelectSyncPool != NULL)
			notify_select_event_pool(fReadSelectSyncPool, B_SELECT_READ);

		InterruptsSpinLocker _(fReadRequestsLock);
		if (ReadRequest* request = fReadRequests.First())
			request->Notify();
	}
}


void
Inode::NotifyEndClosed(bool writer)
{
	TRACE("Inode %p::%s(%s)\n", this, __FUNCTION__,
		writer ? "writer" : "reader");

	if (writer) {
		// Our last writer has been closed; if the pipe
		// contains no data, unlock all waiting readers
		TRACE("  buffer readable: %zu\n", fBuffer.Readable());
		if (fBuffer.Readable() == 0) {
			InterruptsSpinLocker readRequestsLocker(fReadRequestsLock);
			ReadRequestList::Iterator iterator = fReadRequests.GetIterator();
			while (ReadRequest* request = iterator.Next())
				request->Notify();
			readRequestsLocker.Unlock();

			if (fReadSelectSyncPool != NULL)
				notify_select_event_pool(fReadSelectSyncPool, B_SELECT_DISCONNECTED);
		}
	} else {
		// Last reader is gone. Wake up all writers.
		fActiveCondition.NotifyAll();

		if (fWriteSelectSyncPool != NULL)
			notify_select_event_pool(fWriteSelectSyncPool, B_SELECT_ERROR);
	}
}


status_t
Inode::Open(int openMode)
{
	WriteLocker locker(ChangeLock());

	if ((openMode & O_ACCMODE) == O_WRONLY || (openMode & O_ACCMODE) == O_RDWR)
		fWriterCount++;

	if ((openMode & O_ACCMODE) == O_RDONLY || (openMode & O_ACCMODE) == O_RDWR)
		fReaderCount++;

	bool shouldWait = false;
	if ((openMode & O_ACCMODE) == O_WRONLY && fReaderCount == 0) {
		if ((openMode & O_NONBLOCK) != 0)
			return ENXIO;
		shouldWait = true;
	}
	if ((openMode & O_ACCMODE) == O_RDONLY && fWriterCount == 0
		&& (openMode & O_NONBLOCK) == 0) {
		shouldWait = true;
	}
	if (shouldWait) {
		// prepare for waiting for the condition variable.
		ConditionVariableEntry waitEntry;
		fActiveCondition.Add(&waitEntry);
		locker.Unlock();
		status_t status = waitEntry.Wait(B_CAN_INTERRUPT);
		if (status != B_OK)
			return status;
		locker.Lock();
	}

	if (fReaderCount > 0 && fWriterCount > 0) {
		TRACE("Inode %p::Open(): fifo becomes active\n", this);
		fBuffer.CreateBuffer();
		fActive = true;

		// notify all waiting writers that they can start
		if (fWriteSelectSyncPool != NULL)
			notify_select_event_pool(fWriteSelectSyncPool, B_SELECT_WRITE);
		fActiveCondition.NotifyAll();
	}
	return B_OK;
}


void
Inode::Close(file_cookie* cookie)
{
	WriteLocker locker(ChangeLock());

	int openMode = cookie->open_mode;
	TRACE("Inode %p::Close(openMode = %" B_PRId32 ")\n", this, openMode);

	// Notify all currently reading file descriptors
	ReadRequestList::Iterator iterator = fReadRequests.GetIterator();
	while (ReadRequest* request = iterator.Next()) {
		if (request->Cookie() == cookie)
			request->Notify(B_FILE_ERROR);
	}

	if ((openMode & O_ACCMODE) == O_WRONLY || (openMode & O_ACCMODE) == O_RDWR) {
		if (--fWriterCount == 0)
			NotifyEndClosed(true);
	}

	if ((openMode & O_ACCMODE) == O_RDONLY || (openMode & O_ACCMODE) == O_RDWR) {
		if (--fReaderCount == 0)
			NotifyEndClosed(false);
	}

	if (fWriterCount == 0) {
		// Notify any still reading writers to stop
		// TODO: This only works reliable if there is only one writer - we could
		// do the same thing done for the read requests.
		fActiveCondition.NotifyAll(B_FILE_ERROR);
	}

	if (fReaderCount == 0 && fWriterCount == 0) {
		fActive = false;
		fBuffer.DeleteBuffer();
	}
}


status_t
Inode::Select(uint8 event, selectsync* sync, int openMode)
{
	WriteLocker locker(ChangeLock());

	bool writer = true;
	select_sync_pool** pool;
	// B_SELECT_READ can happen on write-only opened fds, so restrain B_SELECT_READ to O_RDWR
	if ((event == B_SELECT_READ && (openMode & O_RWMASK) == O_RDWR)
		|| (openMode & O_RWMASK) == O_RDONLY) {
		pool = &fReadSelectSyncPool;
		writer = false;
	} else if ((openMode & O_RWMASK) == O_RDWR || (openMode & O_RWMASK) == O_WRONLY) {
		pool = &fWriteSelectSyncPool;
	} else
		return B_NOT_ALLOWED;

	if (add_select_sync_pool_entry(pool, sync, event) != B_OK)
		return B_ERROR;

	// signal right away, if the condition holds already
	if (writer) {
		if ((event == B_SELECT_WRITE && fBuffer.Writable() > 0)
			|| (event == B_SELECT_ERROR && fReaderCount == 0)) {
			return notify_select_event(sync, event);
		}
	} else {
		if ((event == B_SELECT_READ && fBuffer.Readable() > 0)
			|| (event == B_SELECT_DISCONNECTED && fWriterCount == 0)) {
			return notify_select_event(sync, event);
		}
	}

	return B_OK;
}


status_t
Inode::Deselect(uint8 event, selectsync* sync, int openMode)
{
	WriteLocker locker(ChangeLock());

	select_sync_pool** pool;
	if ((event == B_SELECT_READ && (openMode & O_RWMASK) == O_RDWR)
		|| (openMode & O_RWMASK) == O_RDONLY) {
		pool = &fReadSelectSyncPool;
	} else if ((openMode & O_RWMASK) == O_RDWR || (openMode & O_RWMASK) == O_WRONLY) {
		pool = &fWriteSelectSyncPool;
	} else
		return B_NOT_ALLOWED;

	remove_select_sync_pool_entry(pool, sync, event);
	return B_OK;
}


void
Inode::Dump(bool dumpData) const
{
	kprintf("FIFO %p\n", this);
	kprintf("  active:        %s\n", fActive ? "true" : "false");
	kprintf("  readers:       %" B_PRId32 "\n", fReaderCount);
	kprintf("  writers:       %" B_PRId32 "\n", fWriterCount);

	if (!fReadRequests.IsEmpty()) {
		kprintf(" pending readers:\n");
		for (ReadRequestList::ConstIterator it = fReadRequests.GetIterator();
			ReadRequest* request = it.Next();) {
			kprintf("    %p: thread %" B_PRId32 ", cookie: %p\n", request,
				request->GetThread()->id, request->Cookie());
		}
	}

	if (!fWriteRequests.IsEmpty()) {
		kprintf(" pending writers:\n");
		for (WriteRequestList::ConstIterator it = fWriteRequests.GetIterator();
			WriteRequest* request = it.Next();) {
			kprintf("    %p:  thread %" B_PRId32 ", min count: %zu\n", request,
				request->GetThread()->id, request->MinimalWriteCount());
		}
	}

	kprintf("  %zu bytes buffered\n", fBuffer.Readable());

	if (dumpData && fBuffer.Readable() > 0) {
		struct DataProvider : BKernel::HexDumpDataProvider {
			DataProvider(const RingBuffer& buffer)
				:
				fBuffer(buffer),
				fOffset(0)
			{
			}

			virtual bool HasMoreData() const
			{
				return fOffset < fBuffer.Readable();
			}

			virtual uint8 NextByte()
			{
				uint8 byte = '\0';
				if (fOffset < fBuffer.Readable()) {
					fBuffer.DebugPeek(fOffset, &byte);
					fOffset++;
				}
				return byte;
			}

			virtual bool GetAddressString(char* buffer, size_t bufferSize) const
			{
				snprintf(buffer, bufferSize, "    %4zx", fOffset);
				return true;
			}

		private:
			const RingBuffer&	fBuffer;
			size_t				fOffset;
		};

		DataProvider dataProvider(fBuffer);
		BKernel::print_hex_dump(dataProvider, fBuffer.Readable());
	}
}


/*static*/ int
Inode::Dump(int argc, char** argv)
{
	bool dumpData = false;
	int argi = 1;
	if (argi < argc && strcmp(argv[argi], "-d") == 0) {
		dumpData = true;
		argi++;
	}

	if (argi >= argc || argi + 2 < argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	Inode* node = (Inode*)parse_expression(argv[argi]);
	if (IS_USER_ADDRESS(node)) {
		kprintf("invalid FIFO address\n");
		return 0;
	}

	node->Dump(dumpData);
	return 0;
}


//	#pragma mark - vnode API


static status_t
fifo_put_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	FIFOInode* fifo = (FIFOInode*)vnode->private_node;
	fs_vnode* superVnode = fifo->SuperVnode();

	status_t error = B_OK;
	if (superVnode->ops->put_vnode != NULL)
		error = superVnode->ops->put_vnode(volume, superVnode, reenter);

	delete fifo;

	return error;
}


static status_t
fifo_remove_vnode(fs_volume* volume, fs_vnode* vnode, bool reenter)
{
	FIFOInode* fifo = (FIFOInode*)vnode->private_node;
	fs_vnode* superVnode = fifo->SuperVnode();

	status_t error = B_OK;
	if (superVnode->ops->remove_vnode != NULL)
		error = superVnode->ops->remove_vnode(volume, superVnode, reenter);

	delete fifo;

	return error;
}


static status_t
fifo_open(fs_volume* _volume, fs_vnode* _node, int openMode,
	void** _cookie)
{
	Inode* inode = (Inode*)_node->private_node;

	TRACE("fifo_open(): node = %p, openMode = %d\n", inode, openMode);

	file_cookie* cookie = (file_cookie*)malloc(sizeof(file_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	TRACE("  open cookie = %p\n", cookie);
	cookie->open_mode = openMode;
	status_t status = inode->Open(openMode);
	if (status != B_OK) {
		free(cookie);
		return status;
	}

	*_cookie = (void*)cookie;

	return B_OK;
}


static status_t
fifo_close(fs_volume* volume, fs_vnode* vnode, void* _cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	FIFOInode* fifo = (FIFOInode*)vnode->private_node;

	fifo->Close(cookie);

	return B_OK;
}


static status_t
fifo_free_cookie(fs_volume* _volume, fs_vnode* _node, void* _cookie)
{
	file_cookie* cookie = (file_cookie*)_cookie;

	TRACE("fifo_freecookie: entry vnode %p, cookie %p\n", _node, _cookie);

	free(cookie);

	return B_OK;
}


static status_t
fifo_fsync(fs_volume* _volume, fs_vnode* _node)
{
	return B_BAD_VALUE;
}


static status_t
fifo_read(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t /*pos*/, void* buffer, size_t* _length)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	TRACE("fifo_read(vnode = %p, cookie = %p, length = %lu, mode = %d)\n",
		inode, cookie, *_length, cookie->open_mode);

	ReadLocker _(inode->ChangeLock());

	if (inode->IsActive() && inode->WriterCount() == 0) {
		// as long there is no writer, and the pipe is empty,
		// we always just return 0 to indicate end of file
		if (inode->BytesAvailable() == 0) {
			*_length = 0;
			return B_OK;
		}
	}

	// issue read request

	ReadRequest request(cookie);
	inode->AddReadRequest(request);

	TRACE("  issue read request %p\n", &request);

	size_t length = *_length;
	status_t status = inode->ReadDataFromBuffer(buffer, &length,
		(cookie->open_mode & O_NONBLOCK) != 0, is_called_via_syscall(),
		request);

	inode->RemoveReadRequest(request);
	inode->NotifyReadDone();

	TRACE("  done reading request %p, length %zu\n", &request, length);

	if (length > 0)
		status = B_OK;

	*_length = length;
	return status;
}


static status_t
fifo_write(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	off_t /*pos*/, const void* buffer, size_t* _length)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	TRACE("fifo_write(vnode = %p, cookie = %p, length = %lu)\n",
		_node, cookie, *_length);

	size_t length = *_length;
	if (length == 0)
		return B_OK;

	// copy data into ring buffer
	status_t status = inode->Write(buffer, &length,
		(cookie->open_mode & O_NONBLOCK) != 0, is_called_via_syscall());

	if (length > 0)
		status = B_OK;

	*_length = length;
	return status;
}


static status_t
fifo_read_stat(fs_volume* volume, fs_vnode* vnode, struct ::stat* st)
{
	FIFOInode* fifo = (FIFOInode*)vnode->private_node;
	fs_vnode* superVnode = fifo->SuperVnode();

	if (superVnode->ops->read_stat == NULL)
		return B_BAD_VALUE;

	status_t error = superVnode->ops->read_stat(volume, superVnode, st);
	if (error != B_OK)
		return error;


	ReadLocker _(fifo->ChangeLock());

	st->st_size = fifo->BytesAvailable();

	st->st_blksize = 4096;

	// TODO: Just pass the changes to our modification time on to the super node.
	st->st_atim.tv_sec = time(NULL);
	st->st_atim.tv_nsec = 0;
	st->st_mtim = st->st_ctim = fifo->ModificationTime();

	return B_OK;
}


static status_t
fifo_write_stat(fs_volume* volume, fs_vnode* vnode, const struct ::stat* st,
	uint32 statMask)
{
	// we cannot change the size of anything
	if ((statMask & B_STAT_SIZE) != 0)
		return B_BAD_VALUE;

	FIFOInode* fifo = (FIFOInode*)vnode->private_node;
	fs_vnode* superVnode = fifo->SuperVnode();

	if (superVnode->ops->write_stat == NULL)
		return B_BAD_VALUE;

	status_t error = superVnode->ops->write_stat(volume, superVnode, st,
		statMask);
	if (error != B_OK)
		return error;

	return B_OK;
}


static status_t
fifo_ioctl(fs_volume* _volume, fs_vnode* _node, void* _cookie, uint32 op,
	void* buffer, size_t length)
{
	file_cookie* cookie = (file_cookie*)_cookie;
	Inode* inode = (Inode*)_node->private_node;

	TRACE("fifo_ioctl: vnode %p, cookie %p, op %" B_PRId32 ", buf %p, len %ld\n",
		_node, _cookie, op, buffer, length);

	switch (op) {
		case FIONREAD:
		{
			if (buffer == NULL)
				return B_BAD_VALUE;

			int available = (int)inode->BytesAvailable();

			if (is_called_via_syscall()) {
				if (!IS_USER_ADDRESS(buffer)
					|| user_memcpy(buffer, &available, sizeof(available))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}
			} else
				*(int*)buffer = available;

			return B_OK;
		}

		case B_SET_BLOCKING_IO:
		case B_SET_NONBLOCKING_IO:
		{
			WriteLocker locker(inode->ChangeLock());
			cookie->SetNonBlocking(op == B_SET_NONBLOCKING_IO);
			return B_OK;
		}
	}

	return EINVAL;
}


static status_t
fifo_set_flags(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	int flags)
{
	Inode* inode = (Inode*)_node->private_node;
	file_cookie* cookie = (file_cookie*)_cookie;

	TRACE("fifo_set_flags(vnode = %p, flags = %x)\n", _node, flags);

	WriteLocker locker(inode->ChangeLock());
	cookie->open_mode = (cookie->open_mode & ~(O_APPEND | O_NONBLOCK)) | flags;
	return B_OK;
}


static status_t
fifo_select(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	uint8 event, selectsync* sync)
{
	file_cookie* cookie = (file_cookie*)_cookie;

	TRACE("fifo_select(vnode = %p)\n", _node);
	Inode* inode = (Inode*)_node->private_node;
	if (!inode)
		return B_ERROR;

	return inode->Select(event, sync, cookie->open_mode);
}


static status_t
fifo_deselect(fs_volume* _volume, fs_vnode* _node, void* _cookie,
	uint8 event, selectsync* sync)
{
	file_cookie* cookie = (file_cookie*)_cookie;

	TRACE("fifo_deselect(vnode = %p)\n", _node);
	Inode* inode = (Inode*)_node->private_node;
	if (inode == NULL)
		return B_ERROR;

	return inode->Deselect(event, sync, cookie->open_mode);
}


static bool
fifo_can_page(fs_volume* _volume, fs_vnode* _node, void* cookie)
{
	return false;
}


static status_t
fifo_read_pages(fs_volume* _volume, fs_vnode* _node, void* cookie, off_t pos,
	const iovec* vecs, size_t count, size_t* _numBytes)
{
	return B_NOT_ALLOWED;
}


static status_t
fifo_write_pages(fs_volume* _volume, fs_vnode* _node, void* cookie,
	off_t pos, const iovec* vecs, size_t count, size_t* _numBytes)
{
	return B_NOT_ALLOWED;
}


static status_t
fifo_get_super_vnode(fs_volume* volume, fs_vnode* vnode, fs_volume* superVolume,
	fs_vnode* _superVnode)
{
	FIFOInode* fifo = (FIFOInode*)vnode->private_node;
	fs_vnode* superVnode = fifo->SuperVnode();

	if (superVnode->ops->get_super_vnode != NULL) {
		return superVnode->ops->get_super_vnode(volume, superVnode, superVolume,
			_superVnode);
	}

	*_superVnode = *superVnode;

	return B_OK;
}


static fs_vnode_ops sFIFOVnodeOps = {
	NULL,	// lookup
	NULL,	// get_vnode_name
					// TODO: This is suboptimal! We'd need to forward the
					// super node's hook, if it has got one.

	&fifo_put_vnode,
	&fifo_remove_vnode,

	&fifo_can_page,
	&fifo_read_pages,
	&fifo_write_pages,

	NULL,	// io()
	NULL,	// cancel_io()

	NULL,	// get_file_map

	/* common */
	&fifo_ioctl,
	&fifo_set_flags,
	&fifo_select,
	&fifo_deselect,
	&fifo_fsync,

	NULL,	// fs_read_link
	NULL,	// fs_symlink
	NULL,	// fs_link
	NULL,	// unlink
	NULL,	// rename

	NULL,	// fs_access()
	&fifo_read_stat,
	&fifo_write_stat,
	NULL,

	/* file */
	NULL,	// create()
	&fifo_open,
	&fifo_close,
	&fifo_free_cookie,
	&fifo_read,
	&fifo_write,

	/* directory */
	NULL,	// create_dir
	NULL,	// remove_dir
	NULL,	// open_dir
	NULL,	// close_dir
	NULL,	// free_dir_cookie
	NULL,	// read_dir
	NULL,	// rewind_dir

	/* attribute directory operations */
	NULL,	// open_attr_dir
	NULL,	// close_attr_dir
	NULL,	// free_attr_dir_cookie
	NULL,	// read_attr_dir
	NULL,	// rewind_attr_dir

	/* attribute operations */
	NULL,	// create_attr
	NULL,	// open_attr
	NULL,	// close_attr
	NULL,	// free_attr_cookie
	NULL,	// read_attr
	NULL,	// write_attr

	NULL,	// read_attr_stat
	NULL,	// write_attr_stat
	NULL,	// rename_attr
	NULL,	// remove_attr

	/* support for node and FS layers */
	NULL,	// create_special_node
	&fifo_get_super_vnode,
};


}	// namespace fifo


using namespace fifo;


// #pragma mark -


status_t
create_fifo_vnode(fs_volume* superVolume, fs_vnode* vnode)
{
	FIFOInode* fifo = new(std::nothrow) FIFOInode(vnode);
	if (fifo == NULL)
		return B_NO_MEMORY;

	status_t status = fifo->InitCheck();
	if (status != B_OK) {
		delete fifo;
		return status;
	}

	vnode->private_node = fifo;
	vnode->ops = &sFIFOVnodeOps;

	return B_OK;
}


void
fifo_init()
{
	sRingBufferCache = create_object_cache("fifo ring buffers",
		VFS_FIFO_BUFFER_CAPACITY, CACHE_NO_DEPOT);

	add_debugger_command_etc("fifo", &Inode::Dump,
		"Print info about the specified FIFO node",
		"[ \"-d\" ] <address>\n"
		"Prints information about the FIFO node specified by address\n"
		"<address>. If \"-d\" is given, the data in the FIFO's ring buffer\n"
		"hexdumped as well.\n",
		0);
}
