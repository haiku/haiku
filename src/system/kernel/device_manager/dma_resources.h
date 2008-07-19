/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DMA_RESOURCES_H
#define DMA_RESOURCES_H

#include <sys/uio.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>


struct IOOperation;
struct IORequest;


struct dma_restrictions {
	addr_t	low_address;
	addr_t	high_address;
	size_t	alignment;
	size_t	boundary;
	size_t	max_transfer_size;
	uint32	max_segment_count;
	size_t	max_segment_size;
	uint32	flags;
};


class DMABuffer : public DoublyLinkedListLinkImpl<DMABuffer> {
public:
	static	DMABuffer*			Create(size_t count, void* bounceBuffer,
									addr_t physicalBounceBuffer,
									size_t bounceBufferSize);

			iovec*				Vecs() { return fVecs; }
			iovec&				VecAt(size_t index) { return fVecs[index]; }
			uint32				VecCount() const { return fVecCount; }
			void				SetVecCount(uint32 count);

			void				AddVec(void* base, size_t size);

			void*				BounceBuffer() const { return fBounceBuffer; }
			addr_t				PhysicalBounceBuffer() const
									{ return fPhysicalBounceBuffer; }
			size_t				BounceBufferSize() const
									{ return fBounceBufferSize; }

			void				SetToBounceBuffer(size_t length);
			bool				UsesBounceBuffer() const
									{ return fVecCount >= 1
										&& (addr_t)fVecs[0].iov_base
											== fPhysicalBounceBuffer; }

private:
			void*				fBounceBuffer;
			addr_t				fPhysicalBounceBuffer;
			size_t				fBounceBufferSize;
			uint32				fVecCount;
			iovec				fVecs[1];
};


typedef DoublyLinkedList<DMABuffer> DMABufferList;


class DMAResource {
public:
								DMAResource();
								~DMAResource();

			status_t			Init(const dma_restrictions& restrictions,
									size_t blockSize, uint32 bufferCount);

			status_t			CreateBuffer(DMABuffer** _buffer)
									{ return CreateBuffer(0, _buffer); }
			status_t			CreateBuffer(size_t size, DMABuffer** _buffer);

			status_t			TranslateNext(IORequest* request,
									IOOperation* operation);
			void				RecycleBuffer(DMABuffer* buffer);

			uint32				BufferCount() const { return fBufferCount; }

private:
			bool				_NeedsBoundsBuffers() const;
			void				_RestrictBoundaryAndSegmentSize(addr_t base,
									addr_t& length);

			mutex				fLock;
			dma_restrictions	fRestrictions;
			size_t				fBlockSize;
			uint32				fBufferCount;
			size_t				fBounceBufferSize;
			DMABufferList		fDMABuffers;
			iovec*				fScratchVecs;
};

#endif	// DMA_RESOURCES_H
