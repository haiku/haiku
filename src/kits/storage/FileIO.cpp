/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <FileIO.h>

#include <errno.h>
#include <stdio.h>


BFileIO::BFileIO(FILE* file, bool takeOverOwnership)
	:
	fFile(file),
	fOwnsFile(takeOverOwnership)
{
}


BFileIO::~BFileIO()
{
	if (fOwnsFile && fFile != NULL)
		fclose(fFile);
}


ssize_t
BFileIO::Read(void* buffer, size_t size)
{
	errno = B_OK;
	ssize_t bytesRead = fread(buffer, 1, size, fFile);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFileIO::Write(const void* buffer, size_t size)
{
	errno = B_OK;
	ssize_t bytesRead = fwrite(buffer, 1, size, fFile);
	return bytesRead >= 0 ? bytesRead : errno;
}


ssize_t
BFileIO::ReadAt(off_t position, void* buffer, size_t size)
{
	// save the old position and seek to the requested one
	off_t oldPosition = _Seek(position, SEEK_SET);
	if (oldPosition < 0)
		return oldPosition;

	// read
	ssize_t result = BFileIO::Read(buffer, size);

	// seek back
	fseeko(fFile, oldPosition, SEEK_SET);

	return result;
}


ssize_t
BFileIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	// save the old position and seek to the requested one
	off_t oldPosition = _Seek(position, SEEK_SET);
	if (oldPosition < 0)
		return oldPosition;

	// write
	ssize_t result = BFileIO::Write(buffer, size);

	// seek back
	fseeko(fFile, oldPosition, SEEK_SET);

	return result;
}


off_t
BFileIO::Seek(off_t position, uint32 seekMode)
{
	if (fseeko(fFile, position, seekMode) < 0)
		return errno;

	return BFileIO::Position();
}


off_t
BFileIO::Position() const
{
	off_t result = ftello(fFile);
	return result >= 0 ? result : errno;
}


status_t
BFileIO::SetSize(off_t size)
{
	return B_UNSUPPORTED;
}


status_t
BFileIO::GetSize(off_t* _size) const
{
	// save the current position and seek to the end
	off_t position = _Seek(0, SEEK_END);
	if (position < 0)
		return position;

	// get the size (position at end) and seek back
	off_t size = _Seek(position, SEEK_SET);
	if (size < 0)
		return size;

	*_size = size;
	return B_OK;
}


off_t
BFileIO::_Seek(off_t position, uint32 seekMode) const
{
	// save the current position
	off_t oldPosition = ftello(fFile);
	if (oldPosition < 0)
		return errno;

	// seek to the requested position
	if (fseeko(fFile, position, seekMode) < 0)
		return errno;

	return oldPosition;
}
