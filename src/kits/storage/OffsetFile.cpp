//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file OffsetFile.cpp
	OffsetFile implementation.
*/

#include <stdio.h>

#include "OffsetFile.h"

namespace BPrivate {
namespace Storage {

// constructor
OffsetFile::OffsetFile()
	: fFile(NULL),
	  fOffset(0),
	  fCurrentPosition(0)
{
}

// constructor
OffsetFile::OffsetFile(BFile *file, off_t offset)
	: fFile(NULL),
	  fOffset(0),
	  fCurrentPosition(0)
{
	SetTo(file, offset);
}

// destructor
OffsetFile::~OffsetFile()
{
}

// SetTo
status_t
OffsetFile::SetTo(BFile *file, off_t offset)
{
	Unset();
	fFile = file;
	fOffset = offset;
	return fFile->InitCheck();
}

// Unset
void
OffsetFile::Unset()
{
	fFile = NULL;
	fOffset = 0;
	fCurrentPosition = 0;
}

// InitCheck
status_t
OffsetFile::InitCheck() const
{
	return (fFile ? fFile->InitCheck() : B_NO_INIT);
}

// File
BFile *
OffsetFile::File() const
{
	return fFile;
}

// ReadAt
ssize_t
OffsetFile::ReadAt(off_t pos, void *buffer, size_t size)
{
	status_t error = InitCheck();
	ssize_t result = 0;
	if (error == B_OK)
		result = fFile->ReadAt(pos + fOffset, buffer, size);
	return (error == B_OK ? result : error);
}

// WriteAt
ssize_t
OffsetFile::WriteAt(off_t pos, const void *buffer, size_t size)
{
	status_t error = InitCheck();
	ssize_t result = 0;
	if (error == B_OK)
		result = fFile->WriteAt(pos + fOffset, buffer, size);
	return (error == B_OK ? result : error);
}

// Seek
off_t
OffsetFile::Seek(off_t position, uint32 seekMode)
{
	off_t result = B_BAD_VALUE;
	status_t error = InitCheck();
	if (error == B_OK) {
		switch (seekMode) {
			case SEEK_SET:
				if (position >= 0)
					result = fCurrentPosition = position;
				break;
			case SEEK_END:
			{
				off_t size;
				error = GetSize(&size);
				if (error == B_OK) {
					if (size + position >= 0)
						result = fCurrentPosition = size + position;
				}
				break;
			}
			case SEEK_CUR:
				if (fCurrentPosition + position >= 0)
					result = fCurrentPosition += position;
				break;
			default:
				break;
		}
	}
	return (error == B_OK ? result : error);
}

// Position
off_t
OffsetFile::Position() const
{
	return fCurrentPosition;
}

// SetSize
status_t
OffsetFile::SetSize(off_t size)
{
	status_t error = (size >= 0 ? B_OK : B_BAD_VALUE );
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK)
		error = fFile->SetSize(size + fOffset);
	return error;
}

// GetSize
status_t
OffsetFile::GetSize(off_t *size) const
{
	status_t error = (size ? B_OK : B_BAD_VALUE );
	if (error == B_OK)
		error = InitCheck();
	if (error == B_OK)
		error = fFile->GetSize(size);
	if (error == B_OK) {
		*size -= fOffset;
		if (*size < 0)
			*size = 0;
	}
	return error;
}

// Offset
off_t
OffsetFile::Offset() const
{
	return fOffset;
}

};	// namespace Storage
};	// namespace BPrivate
