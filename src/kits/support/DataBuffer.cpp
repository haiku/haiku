//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		DataBuffer.cpp
//	Author(s):		Erik Jaesler <erik@cgsoftware.com>
//					
//	Description:	Provides the backing store for BMessages
//					
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
BDataBuffer::BDataBuffer(const void* data, size_t len, bool copy)
{
	BDataReference::Create(data, len, fDataRef, copy);
}
//------------------------------------------------------------------------------
BDataBuffer::BDataBuffer(const BDataBuffer& rhs, bool copy)
{
	if (this != &rhs)
	{
		if (copy)
		{
			BDataReference::Create(rhs.Buffer(), rhs.BufferSize(),
								   fDataRef, copy);
		}
		else
		{
			rhs.fDataRef->Acquire(fDataRef);
		}
	}
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
void BDataBuffer::BDataReference::Create(const void* data, size_t len,
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
BDataBuffer::BDataReference::BDataReference(const void* data, size_t len,
											bool copy)
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

