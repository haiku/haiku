/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_REQUESTS_H
#define IO_REQUESTS_H

#include <sys/uio.h>

#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include "dma_resources.h"


#define IO_BUFFER_PHYSICAL	0x01	/* buffer points to physical memory */
#define	IO_BUFFER_USER		0x02	/* buffer points to user memory */


struct DMABuffer;
struct IOOperation;


class IOBuffer : public DoublyLinkedListLinkImpl<IOBuffer> {
public:
	static	IOBuffer*			Create(size_t count);

			bool				IsVirtual() const { return !fPhysical; }
			bool				IsPhysical() const { return fPhysical; }
			bool				IsUser() const { return !fUser; }

			void				SetPhysical(bool physical)
									{ fPhysical = physical; }
			void				SetUser(bool user) { fUser = user; }
			void				SetLength(size_t length) { fLength = length; }

			size_t				Length() const { return fLength; }

			iovec*				Vecs() { return fVecs; }
			iovec&				VecAt(size_t index) { return fVecs[index]; }
			size_t				VecCount() const { return fCount; }
			size_t				Capacity() const { return fCapacity; }

			status_t			LockMemory(bool isWrite);
			void				UnlockMemory(bool isWrite);

private:
								~IOBuffer();
									// not implemented
			void				_UnlockMemory(size_t count, bool isWrite);

			bool				fUser;
			bool				fPhysical;
			size_t				fLength;
			size_t				fCount;
			size_t				fCapacity;
			iovec				fVecs[1];
};


class IORequest;


class IORequestChunk {
public:
	virtual						~IORequestChunk();

//	virtual status_t			Wait(bigtime_t timeout = B_INFINITE_TIMEOUT);

			IORequest*			Parent() const { return fParent; }

			status_t			Status() const { return fStatus; }
			void				SetStatus(status_t status)
									{ fStatus = status; }

			DoublyLinkedListLink<IORequestChunk>*
									ListLink()	{ return &fListLink; }

protected:
			IORequest*			fParent;
			status_t			fStatus;

public:
			DoublyLinkedListLink<IORequestChunk> fListLink;
};

typedef DoublyLinkedList<IORequestChunk,
	DoublyLinkedListMemberGetLink<IORequestChunk, &IORequestChunk::fListLink> >
		IORequestChunkList;


struct IOOperation : IORequestChunk, DoublyLinkedListLinkImpl<IOOperation> {
public:
			bool				Finish();
									// returns true, if it can be recycled

			void				SetRequest(IORequest* request);
			void				SetOriginalRange(off_t offset, size_t length);
									// also sets range
			void				SetRange(off_t offset, size_t length);

			off_t				Offset() const	{ return fOffset; }
			size_t				Length() const	{ return fLength; }
			off_t				OriginalOffset() const
									{ return fOriginalOffset; }
			size_t				OriginalLength() const
									{ return fOriginalLength; }

			void				SetPartialOperation(bool partialOperation);
			bool				IsPartialOperation() const
									{ return fIsPartitialOperation; }
			bool				IsWrite() const;
			bool				IsRead() const;

			bool				UsesBounceBuffer() const
									{ return fDMABuffer->UsesBounceBuffer(); }

protected:
			DMABuffer*			fDMABuffer;
			off_t				fOffset;
			size_t				fLength;
			off_t				fOriginalOffset;
			size_t				fOriginalLength;
			bool				fIsPartitialOperation;
			bool				fUsesBoundsBuffer;
};

typedef IOOperation io_operation;
typedef DoublyLinkedList<IOOperation> IOOperationList;


struct IORequest : IORequestChunk, DoublyLinkedListLinkImpl<IORequest> {
								IORequest();
	virtual						~IORequest();

	virtual	void				ChunkFinished(IORequestChunk* chunk,
									status_t status);

			status_t			Init(void* buffer, size_t length, bool write,
									uint32 flags);
			status_t			Init(iovec* vecs, size_t count, size_t length,
									bool write, uint32 flags);

			size_t				RemainingBytes() const
									{ return fRemainingBytes; }

			bool				IsWrite() const	{ return fIsWrite; }
			bool				IsRead() const	{ return !fIsWrite; }

			IOBuffer*			Buffer() const	{ return fBuffer; }
			off_t				Offset() const	{ return fOffset; }
			size_t				Length() const	{ return fLength; }

			void				Advance(size_t bySize);

			void				AddOperation(IOOperation* operation);
			void				RemoveOperation(IOOperation* operation);

			status_t			CopyData(off_t offset, void* buffer,
									size_t size);
			status_t			CopyData(const void* buffer, off_t offset,
									size_t size);

private:
			status_t			_CopyData(void* buffer, off_t offset,
									size_t size, bool copyIn);
	static	status_t			_CopySimple(void* bounceBuffer, void* external,
									size_t size, bool copyIn);
	static	status_t			_CopyPhysical(void* bounceBuffer,
									void* external, size_t size, bool copyIn);
	static	status_t			_CopyUser(void* bounceBuffer, void* external,
									size_t size, bool copyIn);

			IOBuffer*			fBuffer;
			off_t				fOffset;
			size_t				fLength;
			IORequestChunkList	fChildren;
			uint32				fFlags;
			team_id				fTeam;
			bool				fIsWrite;

			// these are for iteration
			uint32				fVecIndex;
			size_t				fVecOffset;
			size_t				fRemainingBytes;
};


typedef DoublyLinkedList<IORequest> IORequestList;


#endif	// IO_REQUESTS_H
