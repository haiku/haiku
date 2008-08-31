/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef IO_REQUESTS_H
#define IO_REQUESTS_H

#include <sys/uio.h>

#include <new>

#include <SupportDefs.h>

#include <condition_variable.h>
#include <lock.h>
#include <util/DoublyLinkedList.h>

#include "dma_resources.h"


#define B_PHYSICAL_IO_REQUEST	0x01	/* buffer points to physical memory */
#define B_VIP_IO_REQUEST		0x02	/* used by the page writer -- make sure
										   allocations won't fail */
#define B_DELETE_IO_REQUEST		0x04	/* delete request when finished */

struct DMABuffer;
struct IOOperation;

typedef struct IOOperation io_operation;

class IOBuffer : public DoublyLinkedListLinkImpl<IOBuffer> {
public:
	static	IOBuffer*			Create(uint32 count, bool vip);
			void				Delete();

			bool				IsVirtual() const { return !fPhysical; }
			bool				IsPhysical() const { return fPhysical; }
			bool				IsUser() const { return fUser; }

			void				SetVecs(size_t firstVecOffset,
									const iovec* vecs, uint32 count,
									size_t length, uint32 flags);

			void				SetPhysical(bool physical)
									{ fPhysical = physical; }
			void				SetUser(bool user) { fUser = user; }
			void				SetLength(size_t length) { fLength = length; }
			void				SetVecCount(uint32 count) { fVecCount = count; }

			size_t				Length() const { return fLength; }

			iovec*				Vecs() { return fVecs; }
			iovec&				VecAt(size_t index) { return fVecs[index]; }
			size_t				VecCount() const { return fVecCount; }
			size_t				Capacity() const { return fCapacity; }

			status_t			LockMemory(team_id team, bool isWrite);
			void				UnlockMemory(team_id team, bool isWrite);
			bool				IsMemoryLocked() const
									{ return fMemoryLocked; }

			void				Dump() const;

private:
								IOBuffer();
								~IOBuffer();
									// not implemented
			void				_UnlockMemory(team_id team, size_t count,
									bool isWrite);

			bool				fUser;
			bool				fPhysical;
			bool				fVIP;
			bool				fMemoryLocked;
			size_t				fLength;
			size_t				fVecCount;
			size_t				fCapacity;
			iovec				fVecs[1];
};


class IORequest;
class IORequestOwner;


class IORequestChunk {
public:
								IORequestChunk();
	virtual						~IORequestChunk();

			IORequest*			Parent() const { return fParent; }
			void				SetParent(IORequest* parent)
									{ fParent = parent; }

			status_t			Status() const { return fStatus; }

			DoublyLinkedListLink<IORequestChunk>*
									ListLink()	{ return &fListLink; }

			void				operator delete(void* address, size_t size);

protected:
			void				SetStatus(status_t status)
									{ fStatus = status; }
			void				ResetStatus()
									{ fStatus = 1; }

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

			status_t			Prepare(IORequest* request);
			void				SetOriginalRange(off_t offset, size_t length);
									// also sets range
			void				SetRange(off_t offset, size_t length);

			void				SetStatus(status_t status)
									{ IORequestChunk::SetStatus(status); }

			off_t				Offset() const;
			size_t				Length() const;
			off_t				OriginalOffset() const
									{ return fOriginalOffset; }
			size_t				OriginalLength() const
									{ return fOriginalLength; }

			size_t				TransferredBytes() const
									{ return fTransferredBytes; }
			void				SetTransferredBytes(size_t bytes)
									{ fTransferredBytes = bytes; }

			iovec*				Vecs() const;
			uint32				VecCount() const;

			void				SetPartial(bool partialBegin, bool partialEnd);
			bool				HasPartialBegin() const
									{ return fPartialBegin; }
			bool				HasPartialEnd() const
									{ return fPartialEnd; }
			bool				IsWrite() const;
			bool				IsRead() const;

			void				SetBlockSize(size_t blockSize)
									{ fBlockSize  = blockSize; }

			bool				UsesBounceBuffer() const
									{ return fUsesBounceBuffer; }
			void				SetUsesBounceBuffer(bool uses)
									{ fUsesBounceBuffer = uses; }

			DMABuffer*			Buffer() const { return fDMABuffer; }
			void				SetBuffer(DMABuffer* buffer)
									{ fDMABuffer = buffer; }

			void				Dump() const;

protected:
			void				_PrepareVecs();
			status_t			_CopyPartialBegin(bool isWrite,
									bool& partialBlockOnly);
			status_t			_CopyPartialEnd(bool isWrite);

			DMABuffer*			fDMABuffer;
			off_t				fOffset;
			off_t				fOriginalOffset;
			size_t				fLength;
			size_t				fOriginalLength;
			size_t				fTransferredBytes;
			size_t				fBlockSize;
			uint16				fSavedVecIndex;
			uint16				fSavedVecLength;
			uint8				fPhase;
			bool				fPartialBegin;
			bool				fPartialEnd;
			bool				fUsesBounceBuffer;
};


typedef IOOperation io_operation;
typedef DoublyLinkedList<IOOperation> IOOperationList;

typedef struct IORequest io_request;
typedef status_t (*io_request_finished_callback)(void* data,
			io_request* request, status_t status, bool partialTransfer,
			size_t transferEndOffset);
			// TODO: Return type: status_t -> void
typedef status_t (*io_request_iterate_callback)(void* data,
			io_request* request, bool* _partialTransfer);


struct IORequest : IORequestChunk, DoublyLinkedListLinkImpl<IORequest> {
								IORequest();
	virtual						~IORequest();

	static	IORequest*			Create(bool vip);

			status_t			Init(off_t offset, void* buffer, size_t length,
									bool write, uint32 flags);
			status_t			Init(off_t offset, const iovec* vecs,
									size_t count, size_t length, bool write,
									uint32 flags)
									{ return Init(offset, 0, vecs, count,
										length, write, flags); }
			status_t			Init(off_t offset, size_t firstVecOffset,
									const iovec* vecs, size_t count,
									size_t length, bool write, uint32 flags);

			void				SetOwner(IORequestOwner* owner)
									{ fOwner = owner; }
			IORequestOwner*		Owner() const	{ return fOwner; }

			status_t			CreateSubRequest(off_t parentOffset,
									off_t offset, size_t length,
									IORequest*& subRequest);
			void				DeleteSubRequests();

			void				SetFinishedCallback(
									io_request_finished_callback callback,
									void* cookie);
			void				SetIterationCallback(
									io_request_iterate_callback callback,
									void* cookie);
			io_request_finished_callback FinishedCallback(
									void** _cookie = NULL) const;

			status_t			Wait(uint32 flags = 0, bigtime_t timeout = 0);

			bool				IsFinished() const
									{ return fStatus != 1
										&& fPendingChildren == 0; }
			void				NotifyFinished();
			bool				HasCallbacks() const;
			void				SetStatusAndNotify(status_t status);

			void				OperationFinished(IOOperation* operation,
									status_t status, bool partialTransfer,
									size_t transferEndOffset);
			void				SubRequestFinished(IORequest* request,
									status_t status, bool partialTransfer,
									size_t transferEndOffset);
			void				SetUnfinished();

			size_t				RemainingBytes() const
									{ return fRemainingBytes; }
			size_t				TransferredBytes() const
									{ return fTransferSize; }
			bool				IsPartialTransfer() const
									{ return fPartialTransfer; }
			void				SetTransferredBytes(bool partialTransfer,
									size_t transferredBytes);

			bool				IsWrite() const	{ return fIsWrite; }
			bool				IsRead() const	{ return !fIsWrite; }
			team_id				Team() const	{ return fTeam; }
			thread_id			Thread() const	{ return fThread; }
			uint32				Flags() const	{ return fFlags; }

			IOBuffer*			Buffer() const	{ return fBuffer; }
			off_t				Offset() const	{ return fOffset; }
			size_t				Length() const	{ return fLength; }

			void				SetOffset(off_t offset)	{ fOffset = offset; }

			uint32				VecIndex() const	{ return fVecIndex; }
			size_t				VecOffset() const	{ return fVecOffset; }

			void				Advance(size_t bySize);

			IORequest*			FirstSubRequest();
			IORequest*			NextSubRequest(IORequest* previous);

			void				AddOperation(IOOperation* operation);
			void				RemoveOperation(IOOperation* operation);

			status_t			CopyData(off_t offset, void* buffer,
									size_t size);
			status_t			CopyData(const void* buffer, off_t offset,
									size_t size);

			void				Dump() const;

private:
			status_t			_CopyData(void* buffer, off_t offset,
									size_t size, bool copyIn);
	static	status_t			_CopySimple(void* bounceBuffer, void* external,
									size_t size, team_id team, bool copyIn);
	static	status_t			_CopyPhysical(void* bounceBuffer,
									void* external, size_t size, team_id team,
									bool copyIn);
	static	status_t			_CopyUser(void* bounceBuffer, void* external,
									size_t size, team_id team, bool copyIn);

			mutex				fLock;
			IORequestOwner*		fOwner;
			IOBuffer*			fBuffer;
			off_t				fOffset;
			size_t				fLength;
			size_t				fTransferSize;
									// After all subrequests/operations have
									// finished, number of contiguous bytes at
									// the beginning of the request that have
									// actually been transferred.
			size_t				fRelativeParentOffset;
									// offset of this request relative to its
									// parent
			IORequestChunkList	fChildren;
			int32				fPendingChildren;
			uint32				fFlags;
			team_id				fTeam;
			thread_id			fThread;
			bool				fIsWrite;
			bool				fPartialTransfer;

			io_request_finished_callback	fFinishedCallback;
			void*				fFinishedCookie;
			io_request_iterate_callback	fIterationCallback;
			void*				fIterationCookie;
			ConditionVariable	fFinishedCondition;

			// these are for iteration
			uint32				fVecIndex;
			size_t				fVecOffset;
			size_t				fRemainingBytes;
};


typedef DoublyLinkedList<IORequest> IORequestList;


// allocator for VIP I/O request memory
void* vip_io_request_malloc(size_t size);
void vip_io_request_free(void* address);

void io_request_free(void* address);
	// frees regardless of whether allocated with vip_io_request_malloc() or
	// malloc()

void vip_io_request_allocator_init();


static const struct vip_io_alloc_t {
} vip_io_alloc = {};


inline void*
operator new(size_t size, const vip_io_alloc_t& vip_io_alloc) throw ()
{
	return vip_io_request_malloc(size);
}


inline void*
operator new[](size_t size, const vip_io_alloc_t& vip_io_alloc) throw ()
{
	return vip_io_request_malloc(size);
}


#endif	// IO_REQUESTS_H
