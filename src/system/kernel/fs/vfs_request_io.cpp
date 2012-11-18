/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

// included by vfs.cpp


//#define TRACE_VFS_REQUEST_IO
#ifdef TRACE_VFS_REQUEST_IO
#	define TRACE_RIO(x...) dprintf(x)
#else
#	define TRACE_RIO(x...) do {} while (false)
#endif


#include <heap.h>


// #pragma mark - AsyncIOCallback


AsyncIOCallback::~AsyncIOCallback()
{
}


/* static */ status_t
AsyncIOCallback::IORequestCallback(void* data, io_request* request,
	status_t status, bool partialTransfer, generic_size_t transferEndOffset)
{
	((AsyncIOCallback*)data)->IOFinished(status, partialTransfer,
		transferEndOffset);
	return B_OK;
}


// #pragma mark - StackableAsyncIOCallback


StackableAsyncIOCallback::StackableAsyncIOCallback(AsyncIOCallback* next)
	:
	fNextCallback(next)
{
}


// #pragma mark -


struct iterative_io_cookie {
	struct vnode*					vnode;
	file_descriptor*				descriptor;
	iterative_io_get_vecs			get_vecs;
	iterative_io_finished			finished;
	void*							cookie;
	off_t							request_offset;
	io_request_finished_callback	next_finished_callback;
	void*							next_finished_cookie;
};


class DoIO {
public:
	DoIO(bool write)
		:
		fWrite(write)
	{
	}

	virtual	~DoIO()
	{
	}

	virtual status_t IO(off_t offset, void* buffer, size_t* length) = 0;

protected:
	bool	fWrite;
};


class CallbackIO : public DoIO {
public:
	CallbackIO(bool write,
			status_t (*doIO)(void* cookie, off_t offset, void* buffer,
				size_t* length),
			void* cookie)
		:
		DoIO(write),
		fDoIO(doIO),
		fCookie(cookie)
	{
	}

	virtual status_t IO(off_t offset, void* buffer, size_t* length)
	{
		return fDoIO(fCookie, offset, buffer, length);
	}

private:
	status_t (*fDoIO)(void*, off_t, void*, size_t*);
	void*		fCookie;
};


class VnodeIO : public DoIO {
public:
	VnodeIO(bool write, struct vnode* vnode, void* cookie)
		:
		DoIO(write),
		fVnode(vnode),
		fCookie(cookie)
	{
	}

	virtual status_t IO(off_t offset, void* buffer, size_t* length)
	{
		iovec vec;
		vec.iov_base = buffer;
		vec.iov_len = *length;

		if (fWrite) {
			return FS_CALL(fVnode, write_pages, fCookie, offset, &vec, 1,
				length);
		}

		return FS_CALL(fVnode, read_pages, fCookie, offset, &vec, 1, length);
	}

private:
	struct vnode*	fVnode;
	void*			fCookie;
};


static status_t
do_iterative_fd_io_iterate(void* _cookie, io_request* request,
	bool* _partialTransfer)
{
	TRACE_RIO("[%ld] do_iterative_fd_io_iterate(request: %p)\n",
		find_thread(NULL), request);

	static const size_t kMaxSubRequests = 8;

	iterative_io_cookie* cookie = (iterative_io_cookie*)_cookie;

	request->DeleteSubRequests();

	off_t requestOffset = cookie->request_offset;
	size_t requestLength = request->Length()
		- (requestOffset - request->Offset());

	// get the next file vecs
	file_io_vec vecs[kMaxSubRequests];
	size_t vecCount = kMaxSubRequests;
	status_t error = cookie->get_vecs(cookie->cookie, request, requestOffset,
		requestLength, vecs, &vecCount);
	if (error != B_OK && error != B_BUFFER_OVERFLOW)
		return error;
	if (vecCount == 0) {
		*_partialTransfer = true;
		return B_OK;
	}
	TRACE_RIO("[%ld]  got %zu file vecs\n", find_thread(NULL), vecCount);

	// create subrequests for the file vecs we've got
	size_t subRequestCount = 0;
	for (size_t i = 0; i < vecCount && subRequestCount < kMaxSubRequests; i++) {
		off_t vecOffset = vecs[i].offset;
		off_t vecLength = min_c(vecs[i].length, requestLength);
		TRACE_RIO("[%ld]    vec %lu offset: %lld, length: %lld\n",
			find_thread(NULL), i, vecOffset, vecLength);

		while (vecLength > 0 && subRequestCount < kMaxSubRequests) {
			TRACE_RIO("[%ld]    creating subrequest: offset: %lld, length: "
				"%lld\n", find_thread(NULL), vecOffset, vecLength);
			IORequest* subRequest;
			error = request->CreateSubRequest(requestOffset, vecOffset,
				vecLength, subRequest);
			if (error != B_OK)
				break;

			subRequestCount++;

			size_t lengthProcessed = subRequest->Length();
			vecOffset += lengthProcessed;
			vecLength -= lengthProcessed;
			requestOffset += lengthProcessed;
			requestLength -= lengthProcessed;
		}
	}

	// Only if we couldn't create any subrequests, we fail.
	if (error != B_OK && subRequestCount == 0)
		return error;

	// Reset the error code for the loop below
	error = B_OK;

	request->Advance(requestOffset - cookie->request_offset);
	cookie->request_offset = requestOffset;

	// Schedule the subrequests.
	IORequest* nextSubRequest = request->FirstSubRequest();
	while (nextSubRequest != NULL) {
		IORequest* subRequest = nextSubRequest;
		nextSubRequest = request->NextSubRequest(subRequest);

		if (error == B_OK) {
			TRACE_RIO("[%ld]  scheduling subrequest: %p\n", find_thread(NULL),
				subRequest);
			error = vfs_vnode_io(cookie->vnode, cookie->descriptor->cookie,
				subRequest);
		} else {
			// Once scheduling a subrequest failed, we cancel all subsequent
			// subrequests.
			subRequest->SetStatusAndNotify(B_CANCELED);
		}
	}

	// TODO: Cancel the subrequests that were scheduled successfully.

	return B_OK;
}


static status_t
do_iterative_fd_io_finish(void* _cookie, io_request* request, status_t status,
	bool partialTransfer, generic_size_t transferEndOffset)
{
	iterative_io_cookie* cookie = (iterative_io_cookie*)_cookie;

	if (cookie->finished != NULL) {
		cookie->finished(cookie->cookie, request, status, partialTransfer,
			transferEndOffset);
	}

	put_fd(cookie->descriptor);

	if (cookie->next_finished_callback != NULL) {
		cookie->next_finished_callback(cookie->next_finished_cookie, request,
			status, partialTransfer, transferEndOffset);
	}

	delete cookie;

	return B_OK;
}


static status_t
do_synchronous_iterative_vnode_io(struct vnode* vnode, void* openCookie,
	io_request* request, iterative_io_get_vecs getVecs,
	iterative_io_finished finished, void* cookie)
{
	IOBuffer* buffer = request->Buffer();
	VnodeIO io(request->IsWrite(), vnode, openCookie);

	iovec vector;
	void* virtualVecCookie = NULL;
	off_t offset = request->Offset();
	generic_size_t length = request->Length();

	status_t error = B_OK;

	for (; error == B_OK && length > 0
			&& buffer->GetNextVirtualVec(virtualVecCookie, vector) == B_OK;) {
		uint8* vecBase = (uint8*)vector.iov_base;
		generic_size_t vecLength = min_c(vector.iov_len, length);

		while (error == B_OK && vecLength > 0) {
			file_io_vec fileVecs[8];
			size_t fileVecCount = 8;
			error = getVecs(cookie, request, offset, vecLength, fileVecs,
				&fileVecCount);
			if (error != B_OK || fileVecCount == 0)
				break;

			for (size_t i = 0; i < fileVecCount; i++) {
				const file_io_vec& fileVec = fileVecs[i];
				size_t toTransfer = min_c(fileVec.length, (off_t)length);
				size_t transferred = toTransfer;
				error = io.IO(fileVec.offset, vecBase, &transferred);
				if (error != B_OK)
					break;

				offset += transferred;
				length -= transferred;
				vecBase += transferred;
				vecLength -= transferred;

				if (transferred != toTransfer)
					break;
			}
		}
	}

	buffer->FreeVirtualVecCookie(virtualVecCookie);

	bool partial = length > 0;
	size_t bytesTransferred = request->Length() - length;
	request->SetTransferredBytes(partial, bytesTransferred);
	finished(cookie, request, error, partial, bytesTransferred);
	request->SetStatusAndNotify(error);
	return error;
}


static status_t
synchronous_io(io_request* request, DoIO& io)
{
	TRACE_RIO("[%" B_PRId32 "] synchronous_io(request: %p (offset: %" B_PRIdOFF
		", length: %" B_PRIuGENADDR "))\n", find_thread(NULL), request,
		request->Offset(), request->Length());

	IOBuffer* buffer = request->Buffer();

	iovec vector;
	void* virtualVecCookie = NULL;
	off_t offset = request->Offset();
	generic_size_t length = request->Length();

	for (; length > 0
			&& buffer->GetNextVirtualVec(virtualVecCookie, vector) == B_OK;) {
		void* vecBase = (void*)(addr_t)vector.iov_base;
		size_t vecLength = min_c(vector.iov_len, length);

		TRACE_RIO("[%ld]   I/O: offset: %lld, vecBase: %p, length: %lu\n",
			find_thread(NULL), offset, vecBase, vecLength);

		size_t transferred = vecLength;
		status_t error = io.IO(offset, vecBase, &transferred);
		if (error != B_OK) {
			TRACE_RIO("[%ld]   I/O failed: %#lx\n", find_thread(NULL), error);
			buffer->FreeVirtualVecCookie(virtualVecCookie);
			request->SetStatusAndNotify(error);
			return error;
		}

		offset += transferred;
		length -= transferred;

		if (transferred != vecLength)
			break;
	}

	TRACE_RIO("[%ld] synchronous_io() succeeded\n", find_thread(NULL));

	buffer->FreeVirtualVecCookie(virtualVecCookie);
	request->SetTransferredBytes(length > 0, request->Length() - length);
	request->SetStatusAndNotify(B_OK);
	return B_OK;
}


// #pragma mark - kernel private API


status_t
vfs_vnode_io(struct vnode* vnode, void* cookie, io_request* request)
{
	status_t result = B_ERROR;
	if (!HAS_FS_CALL(vnode, io)
		|| (result = FS_CALL(vnode, io, cookie, request)) == B_UNSUPPORTED) {
		// no io() call -- fall back to synchronous I/O
		VnodeIO io(request->IsWrite(), vnode, cookie);
		return synchronous_io(request, io);
	}

	return result;
}


status_t
vfs_synchronous_io(io_request* request,
	status_t (*doIO)(void* cookie, off_t offset, void* buffer, size_t* length),
	void* cookie)
{
	CallbackIO io(request->IsWrite(), doIO, cookie);
	return synchronous_io(request, io);
}


status_t
vfs_asynchronous_read_pages(struct vnode* vnode, void* cookie, off_t pos,
	const generic_io_vec* vecs, size_t count, generic_size_t numBytes,
	uint32 flags, AsyncIOCallback* callback)
{
	IORequest* request = IORequest::Create((flags & B_VIP_IO_REQUEST) != 0);
	if (request == NULL) {
		callback->IOFinished(B_NO_MEMORY, true, 0);
		return B_NO_MEMORY;
	}

	status_t status = request->Init(pos, vecs, count, numBytes, false,
		flags | B_DELETE_IO_REQUEST);
	if (status != B_OK) {
		delete request;
		callback->IOFinished(status, true, 0);
		return status;
	}

	request->SetFinishedCallback(&AsyncIOCallback::IORequestCallback,
		callback);

	return vfs_vnode_io(vnode, cookie, request);
}


status_t
vfs_asynchronous_write_pages(struct vnode* vnode, void* cookie, off_t pos,
	const generic_io_vec* vecs, size_t count, generic_size_t numBytes,
	uint32 flags, AsyncIOCallback* callback)
{
	IORequest* request = IORequest::Create((flags & B_VIP_IO_REQUEST) != 0);
	if (request == NULL) {
		callback->IOFinished(B_NO_MEMORY, true, 0);
		return B_NO_MEMORY;
	}

	status_t status = request->Init(pos, vecs, count, numBytes, true,
		flags | B_DELETE_IO_REQUEST);
	if (status != B_OK) {
		delete request;
		callback->IOFinished(status, true, 0);
		return status;
	}

	request->SetFinishedCallback(&AsyncIOCallback::IORequestCallback,
		callback);

	return vfs_vnode_io(vnode, cookie, request);
}


// #pragma mark - public API


status_t
do_fd_io(int fd, io_request* request)
{
	struct vnode* vnode;
	file_descriptor* descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL) {
		request->SetStatusAndNotify(B_FILE_ERROR);
		return B_FILE_ERROR;
	}

	CObjectDeleter<file_descriptor> descriptorPutter(descriptor, put_fd);

	return vfs_vnode_io(vnode, descriptor->cookie, request);
}


status_t
do_iterative_fd_io(int fd, io_request* request, iterative_io_get_vecs getVecs,
	iterative_io_finished finished, void* cookie)
{
	TRACE_RIO("[%" B_PRId32 "] do_iterative_fd_io(fd: %d, request: %p "
		"(offset: %" B_PRIdOFF ", length: %" B_PRIuGENADDR "))\n",
		find_thread(NULL), fd, request, request->Offset(), request->Length());

	struct vnode* vnode;
	file_descriptor* descriptor = get_fd_and_vnode(fd, &vnode, true);
	if (descriptor == NULL) {
		finished(cookie, request, B_FILE_ERROR, true, 0);
		request->SetStatusAndNotify(B_FILE_ERROR);
		return B_FILE_ERROR;
	}

	CObjectDeleter<file_descriptor> descriptorPutter(descriptor, put_fd);

	if (!HAS_FS_CALL(vnode, io)) {
		// no io() call -- fall back to synchronous I/O
		return do_synchronous_iterative_vnode_io(vnode, descriptor->cookie,
			request, getVecs, finished, cookie);
	}

	iterative_io_cookie* iterationCookie
		= (request->Flags() & B_VIP_IO_REQUEST) != 0
			? new(malloc_flags(HEAP_PRIORITY_VIP)) iterative_io_cookie
			: new(std::nothrow) iterative_io_cookie;
	if (iterationCookie == NULL) {
		// no memory -- fall back to synchronous I/O
		return do_synchronous_iterative_vnode_io(vnode, descriptor->cookie,
			request, getVecs, finished, cookie);
	}

	iterationCookie->vnode = vnode;
	iterationCookie->descriptor = descriptor;
	iterationCookie->get_vecs = getVecs;
	iterationCookie->finished = finished;
	iterationCookie->cookie = cookie;
	iterationCookie->request_offset = request->Offset();
	iterationCookie->next_finished_callback = request->FinishedCallback(
		&iterationCookie->next_finished_cookie);

	request->SetFinishedCallback(&do_iterative_fd_io_finish, iterationCookie);
	request->SetIterationCallback(&do_iterative_fd_io_iterate, iterationCookie);

	descriptorPutter.Detach();
		// From now on the descriptor is put by our finish callback.

	bool partialTransfer = false;
	status_t error = do_iterative_fd_io_iterate(iterationCookie, request,
		&partialTransfer);
	if (error != B_OK || partialTransfer) {
		if (partialTransfer) {
			request->SetTransferredBytes(partialTransfer,
				request->TransferredBytes());
		}

		request->SetStatusAndNotify(error);
		return error;
	}

	return B_OK;
}
