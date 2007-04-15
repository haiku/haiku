/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <OS.h>

#include "Packet.h"
#include "PacketQueue.h"

#define TRACE_PACKET_QUEUE
#ifdef TRACE_PACKET_QUEUE
  #define TRACE printf
#else
  #define TRACE(a...)
#endif


PacketQueue::PacketQueue(int max_packets)
 :	fQueue(new Packet* [max_packets])
 ,	fSem(create_sem(0, "packet queue sem"))
 ,	fTerminate(false)
 ,	fWriteIndex(0)
 ,	fReadIndex(0)
 ,	fMaxPackets(max_packets)
 ,	fPacketCount(0)
{
}


PacketQueue::~PacketQueue()
{
	Empty();
	delete_sem(fSem);
	delete [] fQueue;
}


void
PacketQueue::Empty()
{
	while (fPacketCount--) {
		delete fQueue[fReadIndex];
		fReadIndex = (fReadIndex + 1) % fMaxPackets;
	}
}
			

status_t
PacketQueue::Insert(Packet *packet)
{
	if (fTerminate) {
		return B_NOT_ALLOWED;
	}
	if (atomic_add(&fPacketCount, 1) == fMaxPackets) {
		atomic_add(&fPacketCount, -1);
		return B_WOULD_BLOCK;
	}
	fQueue[fWriteIndex] = packet;
	fWriteIndex = (fWriteIndex + 1) % fMaxPackets;
	release_sem(fSem);
	return B_OK;
}


status_t
PacketQueue::Remove(Packet **packet)
{
	if (fTerminate) {
		return B_NOT_ALLOWED;
	}
	if (acquire_sem(fSem) != B_OK)
		return B_ERROR;
	if (fTerminate) {
		return B_NOT_ALLOWED;
	}
	*packet = fQueue[fReadIndex];
	atomic_add(&fPacketCount, -1);
	fReadIndex = (fReadIndex + 1) % fMaxPackets;
	return B_OK;
}


void
PacketQueue::Flush(bigtime_t timeout)
{
	if (fTerminate) {
		return;
	}

	timeout += system_time();
	
	while (acquire_sem_etc(fSem, 1, B_ABSOLUTE_TIMEOUT, timeout) == B_OK) {
		if (fTerminate) {
			return;
		}
		Packet *packet = fQueue[fReadIndex];
		atomic_add(&fPacketCount, -1);
		fReadIndex = (fReadIndex + 1) % fMaxPackets;
		delete packet;
	}
}


// peeks into queue and delivers a copy
status_t
PacketQueue::Peek(Packet **packet)
{
	if (fTerminate) {
		return B_NOT_ALLOWED;
	}
	if (acquire_sem(fSem) != B_OK)
		return B_ERROR;
	if (fTerminate) {
		return B_NOT_ALLOWED;
	}
	*packet = new Packet(*fQueue[fReadIndex]);
	release_sem(fSem);
	return B_OK;
}


void
PacketQueue::Terminate()
{
	fTerminate = true;
	release_sem(fSem);
}


void
PacketQueue::Restart()
{
	Empty();

	delete_sem(fSem);
	fSem = create_sem(0, "packet queue sem");
	fTerminate = false;
	fWriteIndex = 0;
	fReadIndex = 0;
	fPacketCount = 0;
}
