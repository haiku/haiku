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
BDataBuffer::BDataBuffer(void* data, size_t len, bool copy)
{
	BDataReference::Create(data, len, fDataRef, copy);
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
const void* BDataBuffer::Buffer() const
{
	return (const void*)fDataRef->Data();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Create(void* data, size_t len,
										 BDataReference*& ref, bool copy)
{
	BDataReference* temp = new BDataReference(data, len, copy);
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
	++fCount;
}
//------------------------------------------------------------------------------
void BDataBuffer::BDataReference::Release(BDataReference*& ref)
{
	ref = NULL;
	--fCount;
	if (fCount <= 0)
	{
		delete this;
	}
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::BDataReference(void* data, size_t len, bool copy)
	:	fData(NULL), fSize(len), fCount(0)
{
	if (copy)
	{
		fData = new char[fSize];
		memcpy((void*)fData, data, fSize);
	}
	else
	{
		fData = (char*)data;
	}
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::BDataReference(size_t len)
	:	fData(NULL), fSize(len), fCount(0)
{
	fData = new char[fSize];
}
//------------------------------------------------------------------------------
BDataBuffer::BDataReference::~BDataReference()
{
	if (fData)
	{
		delete[] fData;
		fData = NULL;
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

