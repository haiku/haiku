//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file ResourcesItem.cpp
	ResourceItem implementation.
*/

#include "ResourceItem.h"

#include <stdio.h>
#include <string.h>

#include <DataIO.h>

namespace BPrivate {
namespace Storage {

// constructor
ResourceItem::ResourceItem()
			: BMallocIO(),
			  fOffset(0),
			  fInitialSize(0),
			  fType(0),
			  fID(0),
			  fName(),
			  fIsLoaded(false),
			  fIsModified(false)
{
	SetBlockSize(1);
}

// destructor
ResourceItem::~ResourceItem()
{
}

// WriteAt
ssize_t
ResourceItem::WriteAt(off_t pos, const void *buffer, size_t size)
{
	ssize_t result = BMallocIO::WriteAt(pos, buffer, size);
	if (result >= 0)
		SetModified(true);
	return result;
}

// SetSize
status_t
ResourceItem::SetSize(off_t size)
{
	status_t error = BMallocIO::SetSize(size);
	if (error == B_OK)
		SetModified(true);
	return error;
}

// SetLocation
void
ResourceItem::SetLocation(int32 offset, size_t initialSize)
{
	SetOffset(offset);
	fInitialSize = initialSize;
}

// SetIdentity
void
ResourceItem::SetIdentity(type_code type, int32 id, const char *name)
{
	fType = type;
	fID = id;
	fName = name;
}

// SetOffset
void
ResourceItem::SetOffset(int32 offset)
{
	fOffset = offset;
}

// Offset
int32
ResourceItem::Offset() const
{
	return fOffset;
}

// InitialSize
size_t
ResourceItem::InitialSize() const
{
	return fInitialSize;
}

// DataSize
size_t
ResourceItem::DataSize() const
{
	if (IsModified())
		return BufferLength();
	return fInitialSize;
}

// SetType
void
ResourceItem::SetType(type_code type)
{
	fType = type;
}

// Type
type_code
ResourceItem::Type() const
{
	return fType;
}

// SetID
void
ResourceItem::SetID(int32 id)
{
	fID = id;
}

// ID
int32
ResourceItem::ID() const
{
	return fID;
}

// SetName
void
ResourceItem::SetName(const char *name)
{
	fName = name;
}

// Name
const char *
ResourceItem::Name() const
{
	return fName.String();
}

// Data
void *
ResourceItem::Data() const
{
	// Since MallocIO may have a NULL buffer, if the data size is 0,
	// we return a pointer to ourselves in this case. This ensures, that
	// the resource item still can be uniquely identified by its data pointer.
	if (DataSize() == 0)
		return const_cast<ResourceItem*>(this);
	return const_cast<void*>(Buffer());
}

// SetLoaded
void
ResourceItem::SetLoaded(bool loaded)
{
	fIsLoaded = loaded;
}

// IsLoaded
bool
ResourceItem::IsLoaded() const
{
	return (BufferLength() > 0 || fIsLoaded);
}

// SetModified
void
ResourceItem::SetModified(bool modified)
{
	fIsModified = modified;
}

// IsModified
bool
ResourceItem::IsModified() const
{
	return fIsModified;
}


};	// namespace Storage
};	// namespace BPrivate




