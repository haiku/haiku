// OffsetFile.cpp

#include <stdio.h>

#include "OffsetFile.h"

// constructor
OffsetFile::OffsetFile()
	: fFile(),
	  fOffset(0),
	  fCurrentPosition(0)
{
}

// constructor
OffsetFile::OffsetFile(const BFile& file, off_t offset)
	: fFile(),
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
OffsetFile::SetTo(const BFile& file, off_t offset)
{
	Unset();
	fFile = file;
	fOffset = offset;
	return fFile.InitCheck();
}

// Unset
void
OffsetFile::Unset()
{
	fFile.Unset();
	fOffset = 0;
	fCurrentPosition = 0;
}

// InitCheck
status_t
OffsetFile::InitCheck() const
{
	return fFile.InitCheck();
}

// Read
//ssize_t
//OffsetFile::Read(void *buffer, size_t size)
//{
//	return fFile.Read(buffer, size);
//}

// Write
//ssize_t
//OffsetFile::Write(const void *buffer, size_t size)
//{
//	return fFile.Write(buffer, size);
//}

// ReadAt
ssize_t
OffsetFile::ReadAt(off_t pos, void *buffer, size_t size)
{
//printf("ReadAt(%Lx + %Lx, %lu)\n", pos, fOffset, size);
	return fFile.ReadAt(pos + fOffset, buffer, size);
}

// WriteAt
ssize_t
OffsetFile::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return fFile.WriteAt(pos + fOffset, buffer, size);
}

// Seek
off_t
OffsetFile::Seek(off_t position, uint32 seekMode)
{
//	off_t result = fFile.Seek(position + fOffset, seekMode);
//	if (result >= 0)
//		result -= fOffset;
//	return result;
	off_t result = B_BAD_VALUE;
	switch (seekMode) {
		case SEEK_SET:
			if (position >= 0)
				result = fCurrentPosition = position;
			break;
		case SEEK_END:
		{
			off_t size;
			status_t error = GetSize(&size);
			if (size + position >= 0)
				result = fCurrentPosition = size + position;
			else
				result = error;
			break;
		}
		case SEEK_CUR:
			if (fCurrentPosition + position >= 0)
				result = fCurrentPosition += position;
			break;
		default:
			break;
	}
	return result;
}

// Position
off_t
OffsetFile::Position() const
{
//	off_t result = fFile.Position();
//	if (result >= 0)
//		result -= fOffset;
//	return result;
	return fCurrentPosition;
}

// SetSize
status_t
OffsetFile::SetSize(off_t size)
{
	status_t error = (size >= 0 ? B_OK : B_BAD_VALUE );
	if (error == B_OK)
		error = fFile.SetSize(size + fOffset);
	return error;
}

// GetSize
status_t
OffsetFile::GetSize(off_t* size)
{
	status_t error = fFile.GetSize(size);
	if (error == B_OK)
		*size -= fOffset;
	return error;
}

// GetOffset
off_t
OffsetFile::GetOffset() const
{
	return fOffset;
}

