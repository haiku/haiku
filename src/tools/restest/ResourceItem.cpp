// ResourceItem.cpp

#include "ResourceItem.h"

#include <stdio.h>
#include <string.h>

#include <DataIO.h>

// constructor
ResourceItem::ResourceItem()
			: fOffset(0),
			  fSize(0),
			  fType(0),
			  fID(0),
			  fName(),
			  fData(NULL)
{
}

// destructor
ResourceItem::~ResourceItem()
{
	UnsetData();
}

// SetLocation
void
ResourceItem::SetLocation(roff_t offset, roff_t size)
{
	SetOffset(offset);
	SetSize(size);
}

// SetIdentity
void
ResourceItem::SetIdentity(type_code type, int32 id, const char* name)
{
	fType = type;
	fID = id;
	fName = name;
}

// SetOffset
void
ResourceItem::SetOffset(roff_t offset)
{
	fOffset = offset;
}

// GetOffset
ResourceItem::roff_t
ResourceItem::GetOffset() const
{
	return fOffset;
}

// SetSize
void
ResourceItem::SetSize(roff_t size)
{
	if (size != fSize) {
		UnsetData();
		fSize = size;
	}
}

// GetSize
ResourceItem::roff_t
ResourceItem::GetSize() const
{
	return fSize;
}

// SetType
void
ResourceItem::SetType(type_code type)
{
	fType = type;
}

// GetType
type_code
ResourceItem::GetType() const
{
	return fType;
}

// SetID
void
ResourceItem::SetID(int32 id)
{
	fID = id;
}

// GetID
int32
ResourceItem::GetID() const
{
	return fID;
}

// SetName
void
ResourceItem::SetName(const char* name)
{
	fName = name;
}

// GetName
const char*
ResourceItem::GetName() const
{
	return fName.String();
}

// SetData
void
ResourceItem::SetData(const void* data, roff_t size)
{
	if (size < 0)
		size = fSize;
	// set the new data
	if (data) {
		AllocData(size);
		if (fSize > 0)
			memcpy(fData, data, fSize);
	} else
		UnsetData();
}

// UnsetData
void
ResourceItem::UnsetData()
{
	if (fData) {
		delete[] (char*)fData;
		fData = NULL;
	}
}

// AllocData
void*
ResourceItem::AllocData(roff_t size)
{
	if (size >= 0)
		SetSize(size);
	if (!fData && fSize > 0)
		fData = new char*[fSize];
	return fData;
}

// GetData
void*
ResourceItem::GetData() const
{
	return fData;
}

// LoadData
status_t
ResourceItem::LoadData(BPositionIO& file, roff_t position, roff_t size)
{
	status_t error = B_OK;
	AllocData(size);
	if (position >= 0)
		fOffset = position;
	if (fData && fSize) {
		ssize_t read = file.ReadAt(fOffset, fData, fSize);
		if (read < 0)
			error = read;
		else if (read < fSize)
			error = B_ERROR;
	}
	return error;
}

// WriteData
status_t
ResourceItem::WriteData(BPositionIO& file) const
{
	status_t error = (fData ? B_OK : B_BAD_VALUE);
	if (error == B_OK && fSize > 0) {
		ssize_t written = file.WriteAt(fOffset, fData, fSize);
		if (written < 0)
			error = written;
		else if (written < fSize)
			error = B_FILE_ERROR;
	}
	return error;
}

// PrintToStream
void
ResourceItem::PrintToStream()
{
	char typeName[4] = { fType >> 24, (fType >> 16) & 0xff,
						 (fType >> 8) & 0xff, fType & 0xff };
	printf("resource: offset: 0x%08lx (%10ld)\n", fOffset, fOffset);
	printf("          size  : 0x%08lx (%10ld)\n", fSize, fSize);
	printf("          type  : '%.4s'     (0x%8lx)\n", typeName, fType);
	printf("          id    : %ld\n", fID);
	printf("          name  : `%s'\n", fName.String());
}

