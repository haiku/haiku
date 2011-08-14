/*
 * Copyright (c) 1999-2000, Eric Moon.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions, and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// RawBuffer.cpp
// e.moon 31mar99

#include "RawBuffer.h"

#include <RealtimeAlloc.h>
#include <Debug.h>
#include <cstring>

// -------------------------------------------------------- //
// ctor/dtor/accessors
// -------------------------------------------------------- //

// allocate buffer (if frames > 0)
RawBuffer::RawBuffer(
	uint32 frameSize,
	uint32 frames,
	bool bCircular,
	rtm_pool* pFromPool) :
	
	m_pData(0),
	m_pPool(pFromPool),
	m_frameSize(frameSize),
	m_frames(frames),
	m_allocatedSize(0),
	m_bCircular(bCircular),
	m_bOwnData(true)
{
	
	if(m_frames)
		resize(m_frames);
}
		
// point to given data (does NOT take responsibility for
// deleting it; use adopt() for that.)
RawBuffer::RawBuffer(
	void* pData,
	uint32 frameSize,
	uint32 frames,
	bool bCircular,
	rtm_pool* pFromPool) :
		
	m_pData(pData),
	m_pPool(pFromPool),
	m_frameSize(frameSize),
	m_frames(frames),
	m_allocatedSize(0),
	m_bCircular(bCircular),
	m_bOwnData(false)
{}

RawBuffer::RawBuffer(const RawBuffer& clone) {
	operator=(clone);
}

// generate a reference to the buffer
RawBuffer& RawBuffer::operator=(const RawBuffer& clone) {
	m_pData = clone.m_pData;
	m_allocatedSize = clone.m_allocatedSize;
	m_frameSize = clone.m_frameSize;
	m_frames = clone.m_frames;
	m_bCircular = clone.m_bCircular;
	m_pPool = clone.m_pPool;
	m_bOwnData = false;

	return *this;
}

// deallocate if I own the data
RawBuffer::~RawBuffer() {
	free();
}

char* RawBuffer::data() const { return (char*)m_pData; }
// returns pointer to given frame
char* RawBuffer::frame(uint32 frame) const {
	return data() + (frame * frameSize());
}
uint32 RawBuffer::frameSize() const { return m_frameSize; }
uint32 RawBuffer::frames() const { return m_frames; }
uint32 RawBuffer::size() const { return m_frames * m_frameSize; }

bool RawBuffer::isCircular() const { return m_bCircular; }
bool RawBuffer::ownsBuffer() const { return m_bOwnData; }

rtm_pool* RawBuffer::pool() const { return m_pPool; }

// resize buffer, re-allocating if necessary to contain
// designated number of frames.
// Does not preserve buffer contents.
	
void RawBuffer::resize(uint32 frames) {
	uint32 sizeRequired = frames * m_frameSize;
	
	// already have enough storage?
	if(sizeRequired 	< m_allocatedSize &&
		m_bOwnData) {
		m_frames = frames;
		return;
	}

	// free existing storage
	free();		
	
	// allocate
	m_pData = (m_pPool) ?
		rtm_alloc(m_pPool, sizeRequired) :
		new int8[sizeRequired];
		
	m_bOwnData = true;
	m_allocatedSize = sizeRequired;
	m_frames = frames;
	
}
	
// take ownership of buffer from target
// (deletes current buffer data, if any owned)

void RawBuffer::adopt(
	void* pData,
	uint32 frameSize,
	uint32 frames,
	bool bCircular,
	rtm_pool* pPool) {

	// clean up myself first
	free();
	
	// reference
	operator=(RawBuffer(pData, frameSize, frames, bCircular, pPool));

	// mark ownership
	m_bOwnData = true;
}

// returns false if the target doesn't own the data, but references it
// one way or the other

bool RawBuffer::adopt(RawBuffer& target) {

	// reference
	operator=(target);

	// take ownership if possible
	
	if(!target.m_bOwnData) {
		m_bOwnData = false;
		return false;
	}

	target.m_bOwnData = false;
	m_bOwnData = true;
	return true;
}
	
// adopt currently ref'd data (if any; returns false if no buffer data or
// already owned)

bool RawBuffer::adopt() {
	if(!m_pData || m_bOwnData)
		return false;
	
	m_bOwnData = true;
	return true;
}

// -------------------------------------------------------- //
// operations
// -------------------------------------------------------- //

// fill the buffer with zeroes
	
void RawBuffer::zero() {
	if(!m_pData || !m_frames)
		return;
	
	memset(m_pData, 0, m_frames * m_frameSize);
}

// raw copy to destination buffer, returning the number of
// frames written, and adjusting both offsets accordingly.
//
// no frames will be written if the buffers' frame sizes
// differ.
	
uint32 RawBuffer::rawCopyTo(
	RawBuffer& target,
	uint32* pioFromFrame,
	uint32* pioTargetFrame,
	uint32 frames) const {

	if(m_frameSize != target.m_frameSize)
		return 0;
	
	ASSERT(m_pData);
	ASSERT(m_frames);
	ASSERT(target.m_pData);

	// convert frame counts to byte offsets
	uint32 fromOffset = *pioFromFrame * m_frameSize;
	uint32 targetOffset = *pioTargetFrame * m_frameSize;

	// figure buffer sizes in bytes
	uint32 size = m_frames * m_frameSize;	
	uint32 targetSize = target.m_frames * target.m_frameSize;

	// figure amount to write	
	uint32 toCopy = frames * m_frameSize;	
	if(target.m_bCircular) {
		if(toCopy > targetSize)
			toCopy = targetSize;
	} else {
		if(toCopy > (targetSize-targetOffset))
			toCopy = (targetSize-targetOffset);
	}
	uint32 remaining = toCopy;

	// do it
	while(remaining) {
	
		// figure a contiguous area to fill
		uint32 targetChunk = targetSize - targetOffset;
	
		if(targetChunk > remaining)
			targetChunk = remaining;	

		// fill it (from one or more source areas)
		while(targetChunk > 0) {
		
			// figure a contiguous source area
			uint32 sourceChunk = size - fromOffset;
			if(sourceChunk > targetChunk)
				sourceChunk = targetChunk;
	
			// copy it
			memcpy(
				(int8*)target.m_pData + targetOffset,
				(int8*)m_pData + fromOffset,
				sourceChunk);
	
			// advance offsets
			targetOffset += sourceChunk;
			if(targetOffset == targetSize)
				targetOffset = 0;
			
			fromOffset += sourceChunk;
			if(fromOffset == size)
				fromOffset = 0;

			// figure remaining portion of target area to fill				
			targetChunk -= sourceChunk;
			remaining -= sourceChunk;
		}	
	}
	
	// write new offsets
	*pioFromFrame = fromOffset / m_frameSize;
	*pioTargetFrame = targetOffset / m_frameSize;
	
	return toCopy;
}
	
// more convenient version of above if you don't care
// how the offsets change.

uint32 RawBuffer::rawCopyTo(
	RawBuffer& target,
	uint32 fromOffset,
	uint32 targetOffset,
	uint32 frames) const {
	
	return rawCopyTo(target, &fromOffset, &targetOffset, frames);
}

// -------------------------------------------------------- //
// internal operations
// -------------------------------------------------------- //

// free owned data, if any
// [16jun99] uses proper rtm_free() call if needed
void RawBuffer::free() {
	if(!(m_bOwnData && m_pData))
		return;
		
	if(m_pPool)
		rtm_free(m_pData);
	else
		delete [] (int8 *)m_pData;
		
	m_pData = 0;
}


// END -- RawBuffer.cpp --
