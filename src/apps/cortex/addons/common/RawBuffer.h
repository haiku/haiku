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

class rtm_pool;

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
