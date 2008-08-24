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


struct device_node;
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


struct DMABounceBuffer : public DoublyLinkedListLinkImpl<DMABounceBuffer> {
	void*	address;
	addr_t	physical_address;
	size_t	size;
};

typedef DoublyLinkedList<DMABounceBuffer> DMABounceBufferList;


class DMABuffer : public DoublyLinkedListLinkImpl<DMABuffer> {
public:
	static	DMABuffer*			Create(size_t count);

			iovec*				Vecs() { return fVecs; }
			iovec&				VecAt(size_t index) { return fVecs[index]; }
			uint32				VecCount() const { return fVecCount; }
			void				SetVecCount(uint32 count);

			void				AddVec(void* base, size_t size);

			void				SetBounceBuffer(DMABounceBuffer* bounceBuffer)
									{ fBounceBuffer = bounceBuffer; }
			DMABounceBuffer*	BounceBuffer() const { return fBounceBuffer; }

			void*				BounceBufferAddress() const
									{ return fBounceBuffer
										? fBounceBuffer->address : NULL; }
			addr_t				PhysicalBounceBufferAddress() const
									{ return fBounceBuffer
										? fBounceBuffer->physical_address
										: 0; }
			size_t				BounceBufferSize() const
									{ return fBounceBuffer
										? fBounceBuffer->size : 0; }

			bool				UsesBounceBufferAt(uint32 index);

			void				Dump() const;

private:
			DMABounceBuffer*	fBounceBuffer;
			uint32				fVecCount;
			iovec				fVecs[1];
};


typedef DoublyLinkedList<DMABuffer> DMABufferList;


class DMAResource {
public:
								DMAResource();
								~DMAResource();

			status_t			Init(const dma_restrictions& restrictions,
									size_t blockSize, uint32 bufferCount,
									uint32 bounceBufferCount);
			status_t			Init(device_node* node, size_t blockSize,
									uint32 bufferCount,
									uint32 bounceBufferCount);

			status_t			CreateBuffer(DMABuffer** _buffer);
			status_t			CreateBounceBuffer(DMABounceBuffer** _buffer);

			status_t			TranslateNext(IORequest* request,
									IOOperation* operation);
			void				RecycleBuffer(DMABuffer* buffer);

			uint32				BufferCount() const { return fBufferCount; }

private:
			bool				_NeedsBoundsBuffers() const;
			void				_RestrictBoundaryAndSegmentSize(addr_t base,
									addr_t& length);
			void				_CutBuffer(DMABuffer& buffer,
									addr_t& physicalBounceBuffer,
									size_t& bounceLeft, size_t toCut);
			size_t				_AddBounceBuffer(DMABuffer& buffer,
									addr_t& physicalBounceBuffer,
									size_t& bounceLeft, size_t length,
									bool fixedLength);

			mutex				fLock;
			dma_restrictions	fRestrictions;
			size_t				fBlockSize;
			uint32				fBufferCount;
			uint32				fBounceBufferCount;
			size_t				fBounceBufferSize;
			DMABufferList		fDMABuffers;
			DMABounceBufferList	fBounceBuffers;
			iovec*				fScratchVecs;
};

#endif	// DMA_RESOURCES_H
