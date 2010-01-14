// Request.cpp

#include "Request.h"

#include <new>

#include <stdlib.h>

#include "DebugSupport.h"
#include "RequestFlattener.h"
#include "RequestUnflattener.h"

// RequestMember

// constructor
RequestMember::RequestMember()
{
}

// destructor
RequestMember::~RequestMember()
{
}


// #pragma mark -

// FlattenableRequestMember

// constructor
FlattenableRequestMember::FlattenableRequestMember()
{
}

// destructor
FlattenableRequestMember::~FlattenableRequestMember()
{
}


// #pragma mark -

// Create
RequestBuffer*
RequestBuffer::Create(uint32 dataSize)
{
	void* buffer = malloc(sizeof(RequestBuffer) + dataSize);
	if (!buffer)
		return NULL;

	return new(buffer) RequestBuffer;
}

// Delete
void
RequestBuffer::Delete(RequestBuffer* buffer)
{
	if (buffer) {
		buffer->~RequestBuffer();
		free(buffer);
	}
}

// GetData
void*
RequestBuffer::GetData()
{
	return ((char*)this + sizeof(RequestBuffer));
}

// GetData
const void*
RequestBuffer::GetData() const
{
	return ((char*)this + sizeof(RequestBuffer));
}


// #pragma mark -

// Request

// constructor
Request::Request(uint32 type)
	: FlattenableRequestMember(),
	  fType(type),
	  fBuffers()
{
}

// destructor
Request::~Request()
{
	while (RequestBuffer* buffer = fBuffers.GetFirst()) {
		fBuffers.Remove(buffer);
		RequestBuffer::Delete(buffer);
	}
}

// GetType
uint32
Request::GetType() const
{
	return fType;
}

// AttachBuffer
void
Request::AttachBuffer(RequestBuffer* buffer)
{
	if (!buffer)
		return;

	fBuffers.Insert(buffer);

}

// Flatten
status_t
Request::Flatten(RequestFlattener* flattener)
{
	ShowAround(flattener);
	return B_OK;
}

// Unflatten
status_t
Request::Unflatten(RequestUnflattener* unflattener)
{
	ShowAround(unflattener);
	return B_OK;
}


// #pragma mark -

// RequestMemberVisitor

// constructor
RequestMemberVisitor::RequestMemberVisitor()
{
}

// destructor
RequestMemberVisitor::~RequestMemberVisitor()
{
}

