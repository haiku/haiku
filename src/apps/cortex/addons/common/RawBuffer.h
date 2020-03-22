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


// RawBuffer.h
// eamoon@meadgroup.com
//
// * PURPOSE
// A basic representation of a media buffer.  RawBuffer
// instances may either allocate and maintain their own buffer or
// represent external data.
//
// * HISTORY
// e.moon 	16jun99
//		realtime allocation support
// e.moon		31mar99
//		begun
//

#ifndef __RawBuffer_H__
#define __RawBuffer_H__

#include <SupportDefs.h>

struct rtm_pool;

class RawBuffer {
public:						// ctor/dtor/accessors
	virtual ~RawBuffer();

	// allocate buffer (if frames > 0)
	// [16jun99] if pFromPool is nonzero, uses realtime allocator
	//           w/ the provided pool; otherwise uses standard
	//           new[] allocator.
	RawBuffer(
		uint32 frameSize=1,
		uint32 frames=0,
		bool circular=true,
		rtm_pool* pFromPool=0);

	// point to given data (does NOT take responsibility for
	// deleting it; use adopt() for that.)
	RawBuffer(
		void* pData,
		uint32 frameSize,
		uint32 frames,
		bool bCircular=true,
		rtm_pool* pFromPool=0);

	// generate reference to the given target buffer
	RawBuffer(const RawBuffer& clone);
	RawBuffer& operator=(const RawBuffer& clone);

	// returns pointer to start of buffer
	char* data() const;
	// returns pointer to given frame
	char* frame(uint32 frame) const;

	uint32 frameSize() const;
	uint32 frames() const;
	uint32 size() const;

	bool isCircular() const;
	bool ownsBuffer() const;

	rtm_pool* pool() const;

	// resize buffer, re-allocating if necessary to contain
	// designated number of frames
	// Does not preserve buffer contents.

	void resize(uint32 frames);

	// take ownership of buffer from target
	// (deletes current buffer data, if any owned)

	void adopt(
		void* pData,
		uint32 frameSize,
		uint32 frames,
		bool bCircular=true,
		rtm_pool* pPool=0);

	// returns false if the target doesn't own the data, but references it
	// one way or the other
	bool adopt(RawBuffer& target);

	// adopt currently ref'd data (if any; returns false if no buffer data or
	// already owned)
	bool adopt();

public:						// operations

	// fill the buffer with zeroes

	void zero();

	// raw copy to destination buffer, returning the number of
	// frames written, and adjusting both offsets accordingly.
	//
	// no frames will be written if the buffers' frame sizes
	// differ.
	//
	// rawCopyTo() will repeat the source data as many times as
	// necessary to fill the desired number of frames, but
	// will write no more than the target buffer's size.

	uint32 rawCopyTo(
		RawBuffer& target,
		uint32* pioFromFrame,
		uint32* pioTargetFrame,
		uint32 frames) const;

	// more convenient version of above if you don't care
	// how the offsets change.

	uint32 rawCopyTo(
		RawBuffer& target,
		uint32 fromFrame,
		uint32 targetFrame,
		uint32 frames) const;

protected:					// internal operations
	// free owned data, if any
	// [16jun99] uses proper rtm_free() call if needed
	void free();

protected:					// members
	void*				m_pData;
	rtm_pool*		m_pPool;
	uint32			m_frameSize;
	uint32			m_frames;
	uint32			m_allocatedSize;

	bool					m_bCircular;
	bool					m_bOwnData;
};

#endif /* __RawBuffer_H__ */
