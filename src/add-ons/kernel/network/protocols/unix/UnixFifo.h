/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_FIFO_H
#define UNIX_FIFO_H

#include <Referenceable.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <net_buffer.h>


#define UNIX_FIFO_SHUTDOWN_READ		1
#define UNIX_FIFO_SHUTDOWN_WRITE	2

#define UNIX_FIFO_SHUTDOWN			(B_ERRORS_END + 1)
	// error code returned by Read()/Write()

#define UNIX_FIFO_MINIMAL_CAPACITY	1024
#define UNIX_FIFO_MAXIMAL_CAPACITY	(128 * 1024)


enum class UnixFifoType {
	Stream,
	Datagram
};


struct ring_buffer;

class UnixRequest : public DoublyLinkedListLinkImpl<UnixRequest> {
public:
	UnixRequest(const iovec* vecs, size_t count,
			ancillary_data_container* ancillaryData,
			struct sockaddr_storage* address);

	off_t TotalSize() const			{ return fTotalSize; }
	off_t BytesTransferred() const	{ return fBytesTransferred; }
	off_t BytesRemaining() const	{ return fTotalSize - fBytesTransferred; }

	void AddBytesTransferred(size_t size);
	bool GetCurrentChunk(void*& data, size_t& size);

	ancillary_data_container* AncillaryData() const	 { return fAncillaryData; }
	void AddAncillaryData(ancillary_data_container* data);
	void UnsetAncillaryData();

	struct sockaddr_storage* Address() const	{ return fAddress; }

private:
	const iovec*					fVecs;
	size_t							fVecCount;
	ancillary_data_container*		fAncillaryData;
	off_t							fTotalSize;
	off_t							fBytesTransferred;
	size_t							fVecIndex;
	size_t							fVecOffset;
	struct sockaddr_storage*		fAddress;
};


class UnixBufferQueue {
public:
	UnixBufferQueue(size_t capacity, UnixFifoType type);
	~UnixBufferQueue();

	status_t Init();

	size_t	Readable() const;
	size_t	Writable() const;

	status_t Read(UnixRequest& request);
	status_t Write(UnixRequest& request);

	size_t Capacity() const				{ return fCapacity; }
	status_t SetCapacity(size_t capacity);

private:
	struct AncillaryDataEntry : DoublyLinkedListLinkImpl<AncillaryDataEntry> {
		ancillary_data_container*	data;
		size_t						offset;
	};

	typedef DoublyLinkedList<AncillaryDataEntry> AncillaryDataList;

	struct DatagramEntry : DoublyLinkedListLinkImpl<DatagramEntry> {
		struct sockaddr_storage	address;
		size_t	size;
	};

	typedef DoublyLinkedList<DatagramEntry> DatagramList;

	ring_buffer*		fBuffer;
	size_t				fCapacity;
	AncillaryDataList	fAncillaryData;
	DatagramList		fDatagrams;
	UnixFifoType		fType;
};


class UnixFifo final : public BReferenceable {
public:
	UnixFifo(size_t capacity, UnixFifoType type);
	~UnixFifo();

	status_t Init();

	bool Lock()
	{
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
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

	ssize_t Read(const iovec* vecs, size_t vecCount,
		ancillary_data_container** _ancillaryData,
		struct sockaddr_storage* address, bigtime_t timeout);
	ssize_t Write(const iovec* vecs, size_t vecCount,
		ancillary_data_container* ancillaryData,
		const struct sockaddr_storage* address, bigtime_t timeout);

	size_t Readable() const;
	size_t Writable() const;

	status_t SetBufferCapacity(size_t capacity);

private:
	typedef DoublyLinkedList<UnixRequest> RequestList;

private:
	status_t _Read(UnixRequest& request, bigtime_t timeout);
	status_t _Write(UnixRequest& request, bigtime_t timeout);
	status_t _WriteNonBlocking(UnixRequest& request);
	size_t _MinimumWritableSize(const UnixRequest& request) const;

private:
	mutex				fLock;
	UnixBufferQueue		fBuffer;
	RequestList			fReaders;
	RequestList			fWriters;
	off_t				fReadRequested;
	off_t				fWriteRequested;
	ConditionVariable	fReadCondition;
	ConditionVariable	fWriteCondition;
	uint32				fShutdown;
	UnixFifoType		fType;
};


typedef AutoLocker<UnixFifo> UnixFifoLocker;


#endif	// UNIX_FIFO_H
