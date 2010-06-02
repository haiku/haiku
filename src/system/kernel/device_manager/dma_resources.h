/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DMA_RESOURCES_H
#define DMA_RESOURCES_H


#include <sys/uio.h>

#include <fs_interface.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>


struct device_node;
struct IOOperation;
struct IORequest;


typedef struct generic_io_vec {
	generic_addr_t	base;
	generic_size_t	length;
} generic_io_vec;


struct dma_restrictions {
	generic_addr_t	low_address;
	generic_addr_t	high_address;
	generic_size_t	alignment;
	generic_size_t	boundary;
	generic_size_t	max_transfer_size;
	uint32			max_segment_count;
	generic_size_t	max_segment_size;
	uint32			flags;
};


struct DMABounceBuffer : public DoublyLinkedListLinkImpl<DMABounceBuffer> {
	void*		address;
	phys_addr_t	physical_address;
	phys_size_t	size;
};

typedef DoublyLinkedList<DMABounceBuffer> DMABounceBufferList;


class DMABuffer : public DoublyLinkedListLinkImpl<DMABuffer> {
public:
	static	DMABuffer*			Create(size_t count);

			generic_io_vec*		Vecs() { return fVecs; }
			generic_io_vec&		VecAt(size_t index) { return fVecs[index]; }
			uint32				VecCount() const { return fVecCount; }
			void				SetVecCount(uint32 count);

			void				AddVec(generic_addr_t base,
									generic_size_t size);

			void				SetBounceBuffer(DMABounceBuffer* bounceBuffer)
									{ fBounceBuffer = bounceBuffer; }
			DMABounceBuffer*	BounceBuffer() const { return fBounceBuffer; }

			void*				BounceBufferAddress() const
									{ return fBounceBuffer
										? fBounceBuffer->address : NULL; }
			phys_addr_t			PhysicalBounceBufferAddress() const
									{ return fBounceBuffer
										? fBounceBuffer->physical_address
										: 0; }
			phys_size_t			BounceBufferSize() const
									{ return fBounceBuffer
										? fBounceBuffer->size : 0; }

			bool				UsesBounceBufferAt(uint32 index);

			void				Dump() const;

private:
			DMABounceBuffer*	fBounceBuffer;
			uint32				fVecCount;
			generic_io_vec		fVecs[1];
};


typedef DoublyLinkedList<DMABuffer> DMABufferList;


class DMAResource {
public:
								DMAResource();
								~DMAResource();

			status_t			Init(const dma_restrictions& restrictions,
									generic_size_t blockSize,
									uint32 bufferCount,
									uint32 bounceBufferCount);
			status_t			Init(device_node* node,
									generic_size_t blockSize,
									uint32 bufferCount,
									uint32 bounceBufferCount);

			status_t			CreateBuffer(DMABuffer** _buffer);
			status_t			CreateBounceBuffer(DMABounceBuffer** _buffer);

			status_t			TranslateNext(IORequest* request,
									IOOperation* operation,
									generic_size_t maxOperationLength);
			void				RecycleBuffer(DMABuffer* buffer);

			generic_size_t		BlockSize() const	{ return fBlockSize; }
			uint32				BufferCount() const { return fBufferCount; }

private:
			bool				_NeedsBoundsBuffers() const;
			void				_RestrictBoundaryAndSegmentSize(
									generic_addr_t base,
									generic_addr_t& length);
			void				_CutBuffer(DMABuffer& buffer,
									phys_addr_t& physicalBounceBuffer,
									phys_size_t& bounceLeft,
									generic_size_t toCut);
			phys_size_t			_AddBounceBuffer(DMABuffer& buffer,
									phys_addr_t& physicalBounceBuffer,
									phys_size_t& bounceLeft,
									generic_size_t length, bool fixedLength);

			mutex				fLock;
			dma_restrictions	fRestrictions;
			generic_size_t		fBlockSize;
			uint32				fBufferCount;
			uint32				fBounceBufferCount;
			phys_size_t			fBounceBufferSize;
			DMABufferList		fDMABuffers;
			DMABounceBufferList	fBounceBuffers;
			generic_io_vec*		fScratchVecs;
};

#endif	// DMA_RESOURCES_H
