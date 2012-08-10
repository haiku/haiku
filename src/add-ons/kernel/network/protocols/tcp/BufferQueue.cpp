/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "BufferQueue.h"

#include <KernelExport.h>


//#define TRACE_BUFFER_QUEUE
#ifdef TRACE_BUFFER_QUEUE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#if DEBUG_BUFFER_QUEUE
#	define VERIFY() Verify();
#else
#	define VERIFY() ;
#endif


BufferQueue::BufferQueue(size_t maxBytes)
	:
	fMaxBytes(maxBytes),
	fNumBytes(0),
	fContiguousBytes(0),
	fFirstSequence(0),
	fLastSequence(0),
	fPushPointer(0)
{
}


BufferQueue::~BufferQueue()
{
	// free up any buffers left in the queue

	net_buffer *buffer;
	while ((buffer = fList.RemoveHead()) != NULL) {
		gBufferModule->free(buffer);
	}
}


void
BufferQueue::SetMaxBytes(size_t maxBytes)
{
	fMaxBytes = maxBytes;
}


void
BufferQueue::SetInitialSequence(tcp_sequence sequence)
{
	TRACE(("BufferQueue@%p::SetInitialSequence(%lu)\n", this,
		sequence.Number()));

	fFirstSequence = fLastSequence = sequence;
}



void
BufferQueue::Add(net_buffer *buffer)
{
	Add(buffer, fLastSequence);
}


void
BufferQueue::Add(net_buffer *buffer, tcp_sequence sequence)
{
	TRACE(("BufferQueue@%p::Add(buffer %p, size %lu, sequence %lu)\n",
		this, buffer, buffer->size, sequence.Number()));
	TRACE(("  in: first: %lu, last: %lu, num: %lu, cont: %lu\n",
		fFirstSequence.Number(), fLastSequence.Number(), fNumBytes,
		fContiguousBytes));
	VERIFY();

	if (tcp_sequence(sequence + buffer->size) <= fFirstSequence
		|| buffer->size == 0) {
		// This buffer does not contain any data of interest
		gBufferModule->free(buffer);
		return;
	}
	if (sequence < fFirstSequence) {
		// this buffer contains data that is already long gone - trim it
		gBufferModule->remove_header(buffer,
			(fFirstSequence - sequence).Number());
		sequence = fFirstSequence;
	}

	if (fList.IsEmpty() || sequence >= fLastSequence) {
		// we usually just add the buffer to the end of the queue
		fList.Add(buffer);
		buffer->sequence = sequence.Number();

		if (sequence == fLastSequence
			&& fLastSequence - fFirstSequence == fNumBytes) {
			// there is no hole in the buffer, we can make the whole buffer
			// available
			fContiguousBytes += buffer->size;
		}

		fLastSequence = sequence + buffer->size;
		fNumBytes += buffer->size;

		TRACE(("  out0: first: %lu, last: %lu, num: %lu, cont: %lu\n",
			fFirstSequence.Number(), fLastSequence.Number(), fNumBytes,
			fContiguousBytes));
		VERIFY();
		return;
	}

	if (fLastSequence < sequence + buffer->size)
		fLastSequence = sequence + buffer->size;

	// find the place where to insert the buffer into the queue

	SegmentList::ReverseIterator iterator = fList.GetReverseIterator();
	net_buffer *previous = NULL;
	net_buffer *next = NULL;
	while ((previous = iterator.Next()) != NULL) {
		if (sequence >= previous->sequence) {
			// The new fragment can be inserted after this one
			break;
		}

		next = previous;
	}

	// check if we have duplicate data, and remove it if that is the case
	if (previous != NULL) {
		if (sequence == previous->sequence) {
			// we already have at least part of this data - ignore new data
			// whenever it makes sense (because some TCP implementations send
			// bogus data when probing the window)
			if (previous->size >= buffer->size) {
				gBufferModule->free(buffer);
				buffer = NULL;
			} else {
				fList.Remove(previous);
				fNumBytes -= previous->size;
				gBufferModule->free(previous);
			}
		} else if (tcp_sequence(previous->sequence + previous->size)
				>= sequence + buffer->size) {
			// We already know this data
			gBufferModule->free(buffer);
			buffer = NULL;
		} else if (tcp_sequence(previous->sequence + previous->size)
				> sequence) {
			// We already have the first part of this buffer
			gBufferModule->remove_header(buffer,
				(previous->sequence + previous->size - sequence).Number());
			sequence = previous->sequence + previous->size;
		}
	}

	// "next" always starts at or after the buffer sequence
	ASSERT(next == NULL || buffer == NULL || next->sequence >= sequence);

	while (buffer != NULL && next != NULL
		&& tcp_sequence(sequence + buffer->size) > next->sequence) {
		// we already have at least part of this data
		if (tcp_sequence(next->sequence + next->size)
				<= sequence + buffer->size) {
			net_buffer *remove = next;
			next = (net_buffer *)next->link.next;

			fList.Remove(remove);
			fNumBytes -= remove->size;
			gBufferModule->free(remove);
		} else if (tcp_sequence(next->sequence) > sequence) {
			// We have the end of this buffer already
			gBufferModule->remove_trailer(buffer,
				(sequence + buffer->size - next->sequence).Number());
		} else {
			// We already have this data
			gBufferModule->free(buffer);
			buffer = NULL;
		}
	}

	if (buffer == NULL) {
		TRACE(("  out1: first: %lu, last: %lu, num: %lu, cont: %lu\n",
			fFirstSequence.Number(), fLastSequence.Number(), fNumBytes,
			fContiguousBytes));
		VERIFY();
		return;
	}

	fList.Insert(next, buffer);
	buffer->sequence = sequence.Number();
	fNumBytes += buffer->size;

	// we might need to update the number of bytes available

	if (fLastSequence - fFirstSequence == fNumBytes)
		fContiguousBytes = fNumBytes;
	else if (fFirstSequence + fContiguousBytes == sequence) {
		// the complicated case: the new segment may have connected almost all
		// buffers in the queue (but not all, or the above would be true)

		do {
			fContiguousBytes += buffer->size;

			buffer = (struct net_buffer *)buffer->link.next;
		} while (buffer != NULL
			&& fFirstSequence + fContiguousBytes == buffer->sequence);
	}

	TRACE(("  out2: first: %lu, last: %lu, num: %lu, cont: %lu\n",
		fFirstSequence.Number(), fLastSequence.Number(), fNumBytes,
		fContiguousBytes));
	VERIFY();
}


/*!	Removes all data in the queue up to the \a sequence number as specified.

	NOTE: If there are missing segments in the buffers to be removed,
	fContiguousBytes is not maintained correctly!
*/
status_t
BufferQueue::RemoveUntil(tcp_sequence sequence)
{
	TRACE(("BufferQueue@%p::RemoveUntil(sequence %lu)\n", this,
		sequence.Number()));
	VERIFY();

	if (sequence < fFirstSequence)
		return B_OK;

	SegmentList::Iterator iterator = fList.GetIterator();
	tcp_sequence lastRemoved = fFirstSequence;
	net_buffer *buffer = NULL;
	while ((buffer = iterator.Next()) != NULL && buffer->sequence < sequence) {
		ASSERT(lastRemoved == buffer->sequence);
			// This assures that the queue has no holes, and fContiguousBytes
			// is maintained correctly.

		if (sequence >= buffer->sequence + buffer->size) {
			// remove this buffer completely
			iterator.Remove();
			fNumBytes -= buffer->size;

			fContiguousBytes -= buffer->size;
			lastRemoved = buffer->sequence + buffer->size;
			gBufferModule->free(buffer);
		} else {
			// remove the header as far as needed
			size_t size = (sequence - buffer->sequence).Number();
			gBufferModule->remove_header(buffer, size);

			buffer->sequence += size;
			fNumBytes -= size;
			fContiguousBytes -= size;
			break;
		}
	}

	if (fList.IsEmpty())
		fFirstSequence = fLastSequence;
	else
		fFirstSequence = fList.Head()->sequence;

	VERIFY();
	return B_OK;
}


/*!	Clones the requested data in the buffer queue into the provided \a buffer.
*/
status_t
BufferQueue::Get(net_buffer *buffer, tcp_sequence sequence, size_t bytes)
{
	TRACE(("BufferQueue@%p::Get(sequence %lu, bytes %lu)\n", this,
		sequence.Number(), bytes));
	VERIFY();

	if (bytes == 0)
		return B_OK;

	if (sequence >= fLastSequence || sequence < fFirstSequence) {
		// we don't have the requested data
		return B_BAD_VALUE;
	}
	if (tcp_sequence(sequence + bytes) > fLastSequence)
		bytes = (fLastSequence - sequence).Number();

	size_t bytesLeft = bytes;

	// find first buffer matching the sequence

	SegmentList::Iterator iterator = fList.GetIterator();
	net_buffer *source = NULL;
	while ((source = iterator.Next()) != NULL) {
		if (sequence < source->sequence + source->size)
			break;
	}

	if (source == NULL)
		panic("we should have had that data...");
	if (tcp_sequence(source->sequence) > sequence) {
		panic("source %p, sequence = %" B_PRIu32 " (%" B_PRIu32 ")\n", source,
			source->sequence, sequence.Number());
	}

	// clone the data

	uint32 offset = (sequence - source->sequence).Number();

	while (source != NULL && bytesLeft > 0) {
		size_t size = min_c(source->size - offset, bytesLeft);
		status_t status = gBufferModule->append_cloned(buffer, source, offset,
			size);
		if (status < B_OK)
			return status;

		bytesLeft -= size;
		offset = 0;
		source = iterator.Next();
	}

	VERIFY();
	return B_OK;
}


/*!	Creates a new buffer containing \a bytes bytes from the start of the
	buffer queue. If \a remove is \c true, the data is removed from the
	queue, if not, the data is cloned from the queue.
*/
status_t
BufferQueue::Get(size_t bytes, bool remove, net_buffer **_buffer)
{
	if (bytes > Available())
		bytes = Available();

	if (bytes == 0) {
		// we don't need to create a buffer when there is no data
		*_buffer = NULL;
		return B_OK;
	}

	net_buffer *buffer = fList.First();
	size_t bytesLeft = bytes;
	ASSERT(buffer != NULL);

	if (!remove || buffer->size > bytes) {
		// we need a new buffer
		buffer = gBufferModule->create(256);
		if (buffer == NULL)
			return B_NO_MEMORY;
	} else {
		// we can reuse this buffer
		bytesLeft -= buffer->size;
		fFirstSequence += buffer->size;

		fList.Remove(buffer);
	}

	// clone/copy the remaining data

	SegmentList::Iterator iterator = fList.GetIterator();
	net_buffer *source = NULL;
	status_t status = B_OK;
	while (bytesLeft > 0 && (source = iterator.Next()) != NULL) {
		size_t size = min_c(source->size, bytesLeft);
		status = gBufferModule->append_cloned(buffer, source, 0, size);
		if (status < B_OK)
			break;

		bytesLeft -= size;

		if (!remove)
			continue;

		// remove either the whole buffer or only the part we cloned

		fFirstSequence += size;

		if (size == source->size) {
			iterator.Remove();
			gBufferModule->free(source);
		} else {
			gBufferModule->remove_header(source, size);
			source->sequence += size;
		}
	}

	if (remove && buffer->size) {
		fNumBytes -= buffer->size;
		fContiguousBytes -= buffer->size;
	}

	// We always return what we got, or else we would lose data
	if (status < B_OK && buffer->size == 0) {
		// We could not remove any bytes from the buffer, so
		// let this call fail.
		gBufferModule->free(buffer);
		VERIFY();
		return status;
	}

	*_buffer = buffer;
	VERIFY();
	return B_OK;
}


size_t
BufferQueue::Available(tcp_sequence sequence) const
{
	if (sequence > (fFirstSequence + fContiguousBytes).Number())
		return 0;

	return (fContiguousBytes + fFirstSequence - sequence).Number();
}


void
BufferQueue::SetPushPointer()
{
	if (fList.IsEmpty())
		fPushPointer = 0;
	else
		fPushPointer = fList.Tail()->sequence + fList.Tail()->size;
}

#if DEBUG_BUFFER_QUEUE

/*!	Perform a sanity check of the whole queue.
*/
void
BufferQueue::Verify() const
{
	ASSERT(Available() == 0 || fList.First() != NULL);

	if (fList.First() == NULL) {
		ASSERT(fNumBytes == 0);
		return;
	}

	SegmentList::ConstIterator iterator = fList.GetIterator();
	size_t numBytes = 0;
	size_t contiguousBytes = 0;
	bool contiguous = true;
	tcp_sequence last = fFirstSequence;

	while (net_buffer* buffer = iterator.Next()) {
		if (contiguous && buffer->sequence == last)
			contiguousBytes += buffer->size;
		else
			contiguous = false;

		ASSERT(last <= buffer->sequence);
		ASSERT(buffer->size > 0);

		numBytes += buffer->size;
		last = buffer->sequence + buffer->size;
	}

	ASSERT(last == fLastSequence);
	ASSERT(contiguousBytes == fContiguousBytes);
	ASSERT(numBytes == fNumBytes);
}


void
BufferQueue::Dump() const
{
	SegmentList::ConstIterator iterator = fList.GetIterator();
	int32 number = 0;
	while (net_buffer* buffer = iterator.Next()) {
		kprintf("      %" B_PRId32 ". buffer %p, sequence %" B_PRIu32 ", size %"
			B_PRIu32 "\n", ++number, buffer, buffer->sequence, buffer->size);
	}
}

#endif	// DEBUG_BUFFER_QUEUE
