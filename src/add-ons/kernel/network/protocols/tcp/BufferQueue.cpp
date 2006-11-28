/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "BufferQueue.h"

#include <KernelExport.h>


#define TRACE_BUFFER_QUEUE
#ifdef TRACE_BUFFER_QUEUE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


BufferQueue::BufferQueue(size_t maxBytes)
	:
	fMaxBytes(maxBytes),
	fNumBytes(0),
	fContiguousBytes(0),
	fFirstSequence(0),
	fLastSequence(0)
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
		this, buffer, buffer->size, (uint32)sequence));

	buffer->sequence = sequence;

	if (fList.IsEmpty() || sequence >= fLastSequence) {
		// we usually just add the buffer to the end of the queue
		fList.Add(buffer);

		if (sequence == fLastSequence && fLastSequence - fFirstSequence == fNumBytes) {
			// there is no hole in the buffer, we can make the whole buffer available
			fContiguousBytes += buffer->size;
		}

		fLastSequence = sequence + buffer->size;
		fNumBytes += buffer->size;
		return;
	}

	if (fLastSequence < sequence + buffer->size)
		fLastSequence = sequence + buffer->size;

	if (fFirstSequence > sequence) {
		// this buffer contains data that is already long gone - trim it
		gBufferModule->remove_header(buffer, fFirstSequence - sequence);
		sequence = fFirstSequence;
	}

	// find for the place where to insert the buffer into the queue

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
			// we already have at least part of this data - ignore new data whenever
			// it makes sense (because some TCP implementations send bogus data when
			// probing the window)
			if (previous->size >= buffer->size) {
				gBufferModule->free(buffer);
				buffer = NULL;
			} else {
				fList.Remove(previous);
				gBufferModule->free(previous);
			}
		} else if (tcp_sequence(previous->sequence + previous->size) > sequence)
			gBufferModule->remove_header(buffer, previous->sequence + previous->size - sequence);
	}

	if (buffer != NULL && next != NULL
		&& tcp_sequence(sequence + buffer->size) > next->sequence) {
		// we already have at least part of this data
		if (tcp_sequence(next->sequence + next->size) < sequence + buffer->size) {
			gBufferModule->free(next);
			next = (net_buffer *)next->link.next;
		} else
			gBufferModule->remove_trailer(buffer, next->sequence - (sequence + buffer->size));
	}

	if (buffer == NULL)
		return;

	fList.Insert(next, buffer);

	// we might need to update the number of bytes available

	if (fLastSequence - fFirstSequence == fNumBytes)
		fContiguousBytes = fNumBytes;
	else if (fFirstSequence + fContiguousBytes == sequence) {
		// the complicated case: the new segment may have connected almost all
		// buffers in the queue (but not all, or the above would be true)

		do {
			fContiguousBytes += buffer->size;

			buffer = (struct net_buffer *)buffer->link.next;
		} while (buffer != NULL && fFirstSequence + fContiguousBytes == buffer->sequence);
	}
}


/*!
	Removes all data in the queue up to the \a sequence number as specified.

	NOTE:
	  If there are missing segments in the buffers to be removed,
	  fContiguousBytes is not maintained correctly!
*/
status_t
BufferQueue::RemoveUntil(tcp_sequence sequence)
{
	TRACE(("BufferQueue@%p::RemoveUntil(sequence %lu)\n", this, (uint32)sequence));

	fFirstSequence = sequence;

	SegmentList::Iterator iterator = fList.GetIterator();
	net_buffer *buffer = NULL;
	while ((buffer = iterator.Next()) != NULL) {
		if (sequence <= buffer->sequence) {
			fFirstSequence = buffer->sequence;
				// just in case there is a hole, how unlikely this may ever be
			break;
		}

		if (sequence >= buffer->sequence + buffer->size) {
			// remove this buffer completely
			iterator.Remove();
			fNumBytes -= buffer->size;

			fContiguousBytes -= buffer->size;
			gBufferModule->free(buffer);
		} else {
			// remove the header as far as needed
			size_t size = sequence - buffer->sequence;
			gBufferModule->remove_header(buffer, size);

			fNumBytes -= size;
			fContiguousBytes -= size;
		}
	}

	return B_OK;
}


/*!
	Clones the requested data in the buffer queue into the provided \a buffer.
*/
status_t
BufferQueue::Get(net_buffer *buffer, tcp_sequence sequence, size_t bytes)
{
	TRACE(("BufferQueue@%p::Get(sequence %lu, bytes %lu)\n", this, (uint32)sequence, bytes));

	if (bytes == 0)
		return B_OK;

	if (sequence >= fLastSequence || sequence < fFirstSequence) {
		// we don't have the requested data
		return B_BAD_VALUE;
	}
	if (tcp_sequence(sequence + bytes) > fLastSequence)
		bytes = fLastSequence - sequence;

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
	if (source->sequence > sequence)
		panic("source %p, sequence = %lu (%lu)\n", source, source->sequence, (uint32)sequence);

	// clone the data

	uint32 offset = sequence - source->sequence;

	while (source != NULL && bytesLeft > 0) {
		size_t size = min_c(source->size - offset, bytesLeft);
		status_t status = gBufferModule->append_cloned(buffer, source, offset, size);
		if (status < B_OK)
			return status;

		bytesLeft -= size;
		offset = 0;
		source = iterator.Next();
	}

	return B_OK;
}


/*!
	Creates a new buffer containing \a bytes bytes from the start of the
	buffer queue. If \a remove is \c true, the data is removed from the
	queue, if not, the data is cloned from the queue.
*/
status_t
BufferQueue::Get(size_t bytes, bool remove, net_buffer **_buffer)
{
	if (Available() < bytes || bytes == 0)
		return B_BAD_VALUE;

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
		fList.Remove(buffer);

		if (fList.First() != NULL)
			fFirstSequence = fList.First()->sequence;
	}

	// clone/copy the remaining data

	SegmentList::Iterator iterator = fList.GetIterator();
	net_buffer *source = NULL;
	status_t status = B_OK;
	while (bytesLeft > 0 && (source = iterator.Next()) != NULL) {
		size_t size = min_c(source->size, bytesLeft);
		status_t status = gBufferModule->append_cloned(buffer, source, 0, size);
		if (status < B_OK)
			break;

		bytesLeft -= size;

		if (!remove)
			continue;

		// remove either the whole buffer or only the part we cloned

		if (size == source->size) {
			iterator.Remove();
			gBufferModule->free(source);
		} else
			gBufferModule->remove_header(source, size);
	}

	if (status == B_OK) {
		*_buffer = buffer;
		if (remove) {
			fNumBytes -= bytes;
			fContiguousBytes -= bytes;
		}
	} else
		gBufferModule->free(buffer);

	return status;
}


size_t
BufferQueue::Available(tcp_sequence sequence) const
{
	if (sequence > (uint32)fFirstSequence + fContiguousBytes)
		return 0;

	return fContiguousBytes + fFirstSequence - sequence;
}
