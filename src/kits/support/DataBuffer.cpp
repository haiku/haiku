//------------------------------------------------------------------------------
//	DataBuffer.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <string.h>

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <DataBuffer.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

//------------------------------------------------------------------------------
BDataBuffer::BDataBuffer(size_t len)
{
	BDataReference::Create(len, fDataRef);
}
//------------------------------------------------------------------------------
BDataBuffer::BDataBuffer(void* data, size_t len)
{
	BDataReference::Create(data, len, fDataRef);
}
//------------------------------------------------------------------------------
BDataBuffer::BDataBuffer(const BDataBuffer& rhs)
{
	*this = rhs;
}
//------------------------------------------------------------------------------
BDataBuffer::~BDataBuffer()
{
	if (fDataRef)
	{
		fDataRef->Release(fDataRef);
	}
}
//------------------------------------------------------------------------------
BDataBuffer& BDataBuffer::operator=(const BDataBuffer& rhs)
{
	if (this != &rhs)
	{
		rhs.fDataRef->Acquire(fDataRef);
	}

	return *this;
}
//------------------------------------------------------------------------------
size_t BDataBuffer::BufferSize() const
{
	return fDataRef->Size();
}
//------------------------------------------------------------------------------
const void* BDataBuffer::Buffer()
{
	return (const void*)fDataRef->Data();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Create(void* data, size_t len,
										 BDataReference*& ref)
{
	BDataReference* temp = new BDataReference(data, len);
	temp->Acquire(ref);
}
//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Create(size_t len, BDataReference*& ref)
{
	BDataReference* temp = new BDataReference(len);
	temp->Acquire(ref);
}
//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Acquire(BDataReference*& ref)
{
	ref = this;
	++count;
}
//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Release(BDataReference*& ref)
{
	ref = NULL;
	--count;
	if (count <= 0)
	{
		delete this;
	}
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::BDataReference(void* data, size_t len)
	:	data(NULL), size(len), count(0)
{
	data = new char[len];
	memcpy((void*)data, data, len);
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::BDataReference(size_t len)
	:	data(NULL), size(len), count(0)
{
	data = new char[len];
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::~BDataReference()
{
	if (data)
	{
		delete[] data;
		data = NULL;
	}
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

/*
 * $Log $
 *
 * $Id  $
 *
 */

