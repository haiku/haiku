// RequestBufferReplacer.cpp

#include <new>

#include <stdlib.h>
#include <string.h>

#include "RequestBufferReplacer.h"

// constructor
RequestBufferReplacer::RequestBufferReplacer()
	: fBuffer(NULL),
	  fBufferSize(0)
{
}

// destructor
RequestBufferReplacer::~RequestBufferReplacer()
{
}

// ReplaceBuffer
status_t
RequestBufferReplacer::ReplaceBuffer(Request* request)
{
	if (!request)
		return B_BAD_VALUE;

	// get the size we need to allocate
	fBuffer = NULL;
	fBufferSize = 0;
	request->ShowAround(this);
	if (fBufferSize == 0)
		return B_OK;

	// allocate the buffer
	RequestBuffer* requestBuffer = RequestBuffer::Create(fBufferSize);
	if (!requestBuffer)
		return B_NO_MEMORY;
	fBuffer = (char*)requestBuffer->GetData();

	// relocate the data
	request->ShowAround(this);

	// attach the new buffer
	request->AttachBuffer(requestBuffer);

	return B_OK;
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, bool& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, int8& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, uint8& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, int16& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, uint16& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, int32& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, uint32& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, int64& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, uint64& data)
{
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, Data& data)
{
	int32 size = data.GetSize();
	if (fBuffer) {
		if (size > 0) {
			memcpy(fBuffer, data.GetData(), size);
			data.SetTo(fBuffer, size);
			fBuffer += size;
		}
	} else
		fBufferSize += size;
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, StringData& data)
{
	Visit(member, static_cast<Data&>(data));
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member, RequestMember& subMember)
{
	subMember.ShowAround(this);
}

// Visit
void
RequestBufferReplacer::Visit(RequestMember* member,
	FlattenableRequestMember& subMember)
{
	subMember.ShowAround(this);
}

