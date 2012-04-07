// RequestChannel.cpp

#include "RequestChannel.h"

#include <new>
#include <typeinfo>

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>
#include <ByteOrder.h>

#include "Channel.h"
#include "Compatibility.h"
#include "DebugSupport.h"
#include "Request.h"
#include "RequestDumper.h"
#include "RequestFactory.h"
#include "RequestFlattener.h"
#include "RequestUnflattener.h"

static const int32 kMaxSaneRequestSize	= 128 * 1024;	// 128 KB
static const int32 kDefaultBufferSize	= 4096;			// 4 KB

// ChannelWriter
class RequestChannel::ChannelWriter : public Writer {
public:
	ChannelWriter(Channel* channel, void* buffer, int32 bufferSize)
		: fChannel(channel),
		  fBuffer(buffer),
		  fBufferSize(bufferSize),
		  fBytesWritten(0)
	{
	}

	virtual status_t Write(const void* buffer, int32 size)
	{
		status_t error = B_OK;
		// if the data don't fit into the buffer anymore, flush the buffer
		if (fBytesWritten + size > fBufferSize) {
			error = Flush();
			if (error != B_OK)
				return error;
		}
		// if the data don't even fit into an empty buffer, just send it,
		// otherwise append it to the buffer
		if (size > fBufferSize) {
			error = fChannel->Send(buffer, size);
			if (error != B_OK)
				return error;
		} else {
			memcpy((uint8*)fBuffer + fBytesWritten, buffer, size);
			fBytesWritten += size;
		}
		return error;
	}

	status_t Flush()
	{
		if (fBytesWritten == 0)
			return B_OK;
		status_t error = fChannel->Send(fBuffer, fBytesWritten);
		if (error != B_OK)
			return error;
		fBytesWritten = 0;
		return B_OK;
	}

private:
	Channel*	fChannel;
	void*		fBuffer;
	int32		fBufferSize;
	int32		fBytesWritten;
};


// MemoryReader
class RequestChannel::MemoryReader : public Reader {
public:
	MemoryReader(void* buffer, int32 bufferSize)
		: Reader(),
		  fBuffer(buffer),
		  fBufferSize(bufferSize),
		  fBytesRead(0)
	{
	}

	virtual status_t Read(void* buffer, int32 size)
	{
		// check parameters
		if (!buffer || size < 0)
			return B_BAD_VALUE;
		if (size == 0)
			return B_OK;
		// get pointer into data buffer
		void* localBuffer;
		bool mustFree;
		status_t error = Read(size, &localBuffer, &mustFree);
		if (error != B_OK)
			return error;
		// copy data into supplied buffer
		memcpy(buffer, localBuffer, size);
		return B_OK;
	}

	virtual status_t Read(int32 size, void** buffer, bool* mustFree)
	{
		// check parameters
		if (size < 0 || !buffer || !mustFree)
			return B_BAD_VALUE;
		if (fBytesRead + size > fBufferSize)
			return B_BAD_VALUE;
		// get the data pointer
		*buffer = (uint8*)fBuffer + fBytesRead;
		*mustFree = false;
		fBytesRead += size;
		return B_OK;
	}

	bool AllBytesRead() const
	{
		return (fBytesRead == fBufferSize);
	}

private:
	void*	fBuffer;
	int32	fBufferSize;
	int32	fBytesRead;
};


// RequestHeader
struct RequestChannel::RequestHeader {
	uint32	type;
	int32	size;
};


// constructor
RequestChannel::RequestChannel(Channel* channel)
	: fChannel(channel),
	  fBuffer(NULL),
	  fBufferSize(0)
{
	// allocate the send buffer
	fBuffer = malloc(kDefaultBufferSize);
	if (fBuffer)
		fBufferSize = kDefaultBufferSize;
}

// destructor
RequestChannel::~RequestChannel()
{
	free(fBuffer);
}

// SendRequest
status_t
RequestChannel::SendRequest(Request* request)
{
	if (!request)
		RETURN_ERROR(B_BAD_VALUE);
	PRINT("%p->RequestChannel::SendRequest(): request: %p, type: %s\n", this, request, typeid(*request).name());

	// get request size
	int32 size;
	status_t error = _GetRequestSize(request, &size);
	if (error != B_OK)
		RETURN_ERROR(error);
	if (size < 0 || size > kMaxSaneRequestSize) {
		ERROR("RequestChannel::SendRequest(): ERROR: Invalid request size: "
			"%ld\n", size);
		RETURN_ERROR(B_BAD_DATA);
	}

	// write the request header
	RequestHeader header;
	header.type = B_HOST_TO_BENDIAN_INT32(request->GetType());
	header.size = B_HOST_TO_BENDIAN_INT32(size);
	ChannelWriter writer(fChannel, fBuffer, fBufferSize);
	error = writer.Write(&header, sizeof(RequestHeader));
	if (error != B_OK)
		RETURN_ERROR(error);

	// now write the request in earnest
	RequestFlattener flattener(&writer);
	request->Flatten(&flattener);
	error = flattener.GetStatus();
	if (error != B_OK)
		RETURN_ERROR(error);
	error = writer.Flush();
	RETURN_ERROR(error);
}

// ReceiveRequest
status_t
RequestChannel::ReceiveRequest(Request** _request)
{
	if (!_request)
		RETURN_ERROR(B_BAD_VALUE);

	// get the request header
	RequestHeader header;
	status_t error = fChannel->Receive(&header, sizeof(RequestHeader));
	if (error != B_OK)
		RETURN_ERROR(error);
	header.type = B_HOST_TO_BENDIAN_INT32(header.type);
	header.size = B_HOST_TO_BENDIAN_INT32(header.size);
	if (header.size < 0 || header.size > kMaxSaneRequestSize) {
		ERROR("RequestChannel::ReceiveRequest(): ERROR: Invalid request size: "
			"%ld\n", header.size);
		RETURN_ERROR(B_BAD_DATA);
	}

	// create the request
	Request* request;
	error = RequestFactory::CreateRequest(header.type, &request);
	if (error != B_OK)
		RETURN_ERROR(error);
	ObjectDeleter<Request> requestDeleter(request);

	// allocate a buffer for the data and read them
	if (header.size > 0) {
		RequestBuffer* requestBuffer = RequestBuffer::Create(header.size);
		if (!requestBuffer)
			RETURN_ERROR(B_NO_MEMORY);
		request->AttachBuffer(requestBuffer);

		// receive the data
		error = fChannel->Receive(requestBuffer->GetData(), header.size);
		if (error != B_OK)
			RETURN_ERROR(error);

		// unflatten the request
		MemoryReader reader(requestBuffer->GetData(), header.size);
		RequestUnflattener unflattener(&reader);
		request->Unflatten(&unflattener);
		error = unflattener.GetStatus();
		if (error != B_OK)
			RETURN_ERROR(error);
		if (!reader.AllBytesRead())
			RETURN_ERROR(B_BAD_DATA);
	}

	requestDeleter.Detach();
	*_request = request;
	PRINT("%p->RequestChannel::ReceiveRequest(): request: %p, type: %s\n", this, request, typeid(*request).name());
	return B_OK;
}

// _GetRequestSize
status_t
RequestChannel::_GetRequestSize(Request* request, int32* size)
{
	DummyWriter dummyWriter;
	RequestFlattener flattener(&dummyWriter);
	request->ShowAround(&flattener);
	status_t error = flattener.GetStatus();
	if (error != B_OK)
		return error;
	*size = flattener.GetBytesWritten();
	return error;
}

