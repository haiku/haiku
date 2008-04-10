/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_FIFO_H
#define UNIX_FIFO_H

#include <Referenceable.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <net_buffer.h>


#define UNIX_FIFO_SHUTDOWN_READ		1
#define UNIX_FIFO_SHUTDOWN_WRITE	2

#define UNIX_FIFO_SHUTDOWN			1
	// error code returned by Read()/Write()

#define UNIX_FIFO_MINIMAL_CAPACITY	1024
#define UNIX_FIFO_MAXIMAL_CAPACITY	(128 * 1024)


class UnixBufferQueue {
public:
	UnixBufferQueue(size_t capacity);
	~UnixBufferQueue();

	size_t	Readable() const	{ return fSize; }
	size_t	Writable() const
		{ return fCapacity >= fSize ? fCapacity - fSize : 0; }

	status_t Read(size_t size, net_buffer** _buffer);
	status_t Write(net_buffer* buffer);

	size_t Capacity() const		{ return fCapacity; }
	void SetCapacity(size_t capacity);

private:
	typedef DoublyLinkedList<net_buffer, DoublyLinkedListCLink<net_buffer> >
		BufferList;

	BufferList	fBuffers;
	size_t		fSize;
	size_t		fCapacity;
};



class UnixFifo : public Referenceable {
public:
	UnixFifo(size_t capacity);
	~UnixFifo();

	status_t Init();

	bool Lock()
	{
		return benaphore_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		benaphore_unlock(&fLock);
	}

	void Shutdown(uint32 shutdown);

	bool IsReadShutdown() const
	{
		return (fShutdown & UNIX_FIFO_SHUTDOWN_READ);
	}

	bool IsWriteShutdown() const
	{
		return (fShutdown & UNIX_FIFO_SHUTDOWN_WRITE);
	}

	status_t Read(size_t numBytes, bigtime_t timeout, net_buffer** _buffer);
	status_t Write(net_buffer* buffer, bigtime_t timeout);

	size_t Readable() const;
	size_t Writable() const;

	void SetBufferCapacity(size_t capacity);

private:
	struct Request : DoublyLinkedListLinkImpl<Request> {
		Request(size_t size)
			:
			size(size)
		{
		}

		size_t		size;
	};

	typedef DoublyLinkedList<Request> RequestList;

private:
	status_t _Read(Request& request, size_t numBytes, bigtime_t timeout,
		net_buffer** _buffer);
	status_t _Write(Request& request, net_buffer* buffer, bigtime_t timeout);

private:
	benaphore			fLock;
	UnixBufferQueue		fBuffer;
	RequestList			fReaders;
	RequestList			fWriters;
	size_t				fReadRequested;
	size_t				fWriteRequested;
	sem_id				fReaderSem;
	sem_id				fWriterSem;
	uint32				fShutdown;
};


typedef AutoLocker<UnixFifo> UnixFifoLocker;


#endif	// UNIX_FIFO_H
