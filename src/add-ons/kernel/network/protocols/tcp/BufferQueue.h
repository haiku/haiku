/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef BUFFER_QUEUE_H
#define BUFFER_QUEUE_H


#include "tcp.h"

#include <util/DoublyLinkedList.h>


typedef DoublyLinkedList<struct net_buffer,
	DoublyLinkedListCLink<struct net_buffer> > SegmentList;

class BufferQueue {
public:
								BufferQueue(size_t maxBytes);
								~BufferQueue();

			void				SetMaxBytes(size_t maxBytes);
			void				SetInitialSequence(tcp_sequence sequence);

			void				Add(net_buffer* buffer);
			void				Add(net_buffer* buffer, tcp_sequence sequence);
			status_t			RemoveUntil(tcp_sequence sequence);
			status_t			Get(net_buffer* buffer, tcp_sequence sequence,
									size_t bytes);
			status_t			Get(size_t bytes, bool remove,
									net_buffer** _buffer);

			size_t				Available() const { return fContiguousBytes; }
			size_t				Available(tcp_sequence sequence) const;

	inline	size_t				PushedData() const;
			void				SetPushPointer();

			size_t				Used() const { return fNumBytes; }
	inline	size_t				Free() const;
			size_t				Size() const { return fMaxBytes; }

			bool				IsContiguous() const
									{ return fNumBytes == fContiguousBytes; }

			tcp_sequence		FirstSequence() const { return fFirstSequence; }
			tcp_sequence		LastSequence() const { return fLastSequence; }
			tcp_sequence		NextSequence() const
									{ return fFirstSequence + fContiguousBytes; }

#if DEBUG_TCP_BUFFER_QUEUE
			void				Verify() const;
			void				Dump() const;
#endif

private:
			SegmentList			fList;
			size_t				fMaxBytes;
			size_t				fNumBytes;
			size_t				fContiguousBytes;
			tcp_sequence		fFirstSequence;
			tcp_sequence		fLastSequence;
			tcp_sequence		fPushPointer;
};


size_t
BufferQueue::PushedData() const
{
	// We must check if fPushPointer is not 0 here due to
	// tcp_sequence's special handling of >
	return fPushPointer != 0 && fPushPointer > fFirstSequence
		? (fPushPointer - fFirstSequence).Number() : 0;
}


size_t
BufferQueue::Free() const
{
	// Max bytes is a soft limit, so it can be lower than the actual amount of
	// data in the queue. TCP should never advertize a window outside the max
	// buffer size, though.
	if (fMaxBytes > fNumBytes)
		return fMaxBytes - fNumBytes;

	return 0;
}

#endif	// BUFFER_QUEUE_H
