/*
 *	Copyright (c) 2001-2008, Haiku
 *	Distributed under the terms of the MIT license
 *
 *	Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */


#include <BufferIO.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


BBufferIO::BBufferIO(BPositionIO* stream, size_t bufferSize, bool ownsStream)
	:
	fBufferStart(0),
	fStream(stream),
	fBuffer(NULL),
	fBufferUsed(0),
	fBufferIsDirty(false),
	fOwnsStream(ownsStream)

{
	fBufferSize = max_c(bufferSize, 512);
	fPosition = stream->Position();

	// What can we do if this malloc fails ?
	// I think R5 uses new, but doesn't catch the thrown exception
	// (if you specify a very big buffer, the application just
	// terminates with abort).
	fBuffer = (char*)malloc(fBufferSize);
}


BBufferIO::~BBufferIO()
{
	if (fBufferIsDirty) {
		// Write pending changes to the stream
		Flush();
	}

	free(fBuffer);

	if (fOwnsStream)
		delete fStream;
}


ssize_t
BBufferIO::ReadAt(off_t pos, void* buffer, size_t size)
{
	// We refuse to crash, even if
	// you were lazy and didn't give a valid
	// stream on construction.
	if (fStream == NULL)
		return B_NO_INIT;
	if (buffer == NULL)
		return B_BAD_VALUE;

	// If the amount of data we want doesn't fit in the buffer, just
	// read it directly from the disk (and don't touch the buffer).
	if (size > fBufferSize || fBuffer == NULL) {
		if (fBufferIsDirty)
			Flush();
		return fStream->ReadAt(pos, buffer, size);
	}

	// If the data we are looking for is not in the buffer...
	if (size > fBufferUsed
		|| pos < fBufferStart
		|| pos > fBufferStart + (off_t)fBufferUsed
		|| pos + size > fBufferStart + fBufferUsed) {
		if (fBufferIsDirty) {
			// If there are pending writes, do them.
			Flush();
		}

		// ...cache as much as we can from the stream
		ssize_t sizeRead = fStream->ReadAt(pos, fBuffer, fBufferSize);
		if (sizeRead < 0)
			return sizeRead;

		fBufferUsed = sizeRead;
		if (fBufferUsed > 0) {
			// The data is buffered starting from this offset
			fBufferStart = pos;
		}
	}

	size = min_c(size, fBufferUsed);

	// copy data from the cache to the given buffer
	memcpy(buffer, fBuffer + pos - fBufferStart, size);

	return size;
}


ssize_t
BBufferIO::WriteAt(off_t pos, const void* buffer, size_t size)
{
	if (fStream == NULL)
		return B_NO_INIT;
	if (buffer == NULL)
		return B_BAD_VALUE;

	// If data doesn't fit into the buffer, write it directly to the stream
	if (size > fBufferSize || fBuffer == NULL)
		return fStream->WriteAt(pos, buffer, size);

	// If we have cached data in the buffer, whose offset into the stream
	// is > 0, and the buffer isn't dirty, drop the data.
	if (!fBufferIsDirty && fBufferStart > pos) {
		fBufferStart = 0;
		fBufferUsed = 0;
	}

	// If we want to write beyond the cached data...
	if (pos > fBufferStart + (off_t)fBufferUsed
		|| pos < fBufferStart) {
		ssize_t read;
		off_t where = pos;

		// Can we just cache from the beginning?
		if (pos + size <= fBufferSize)
			where = 0;

		// ...cache more.
		read = fStream->ReadAt(where, fBuffer, fBufferSize);
		if (read > 0) {
			fBufferUsed = read;
			fBufferStart = where;
		}
	}

	memcpy(fBuffer + pos - fBufferStart, buffer, size);

	fBufferIsDirty = true;
	fBufferUsed = max_c((size + pos), fBufferUsed);

	return size;
}


off_t
BBufferIO::Seek(off_t position, uint32 seekMode)
{
	if (fStream == NULL)
		return B_NO_INIT;

	off_t newPosition = fPosition;

	switch (seekMode) {
		case SEEK_CUR:
			newPosition += position;
			break;
		case SEEK_SET:
			newPosition = position;
			break;
		case SEEK_END:
		{
			off_t size;
			status_t status = fStream->GetSize(&size);
			if (status != B_OK)
				return status;

			newPosition = size - position;
			break;
		}
	}

	if (newPosition < 0)
		return B_BAD_VALUE;

	fPosition = newPosition;
	return newPosition;
}


off_t
BBufferIO::Position() const
{
	return fPosition;
}


status_t
BBufferIO::SetSize(off_t size)
{
	if (fStream == NULL)
		return B_NO_INIT;

	return fStream->SetSize(size);
}


status_t
BBufferIO::Flush()
{
	if (!fBufferIsDirty)
		return B_OK;

	// Write the cached data to the stream
	ssize_t bytesWritten = fStream->WriteAt(fBufferStart, fBuffer, fBufferUsed);
	if (bytesWritten > 0)
		fBufferIsDirty = false;

	return (bytesWritten < 0) ? bytesWritten : B_OK;
}


BPositionIO*
BBufferIO::Stream() const
{
	return fStream;
}


size_t
BBufferIO::BufferSize() const
{
	return fBufferSize;
}


bool
BBufferIO::OwnsStream() const
{
	return fOwnsStream;
}


void
BBufferIO::SetOwnsStream(bool ownsStream)
{
	fOwnsStream = ownsStream;
}


void
BBufferIO::PrintToStream() const
{
	printf("stream %p\n", fStream);
	printf("buffer %p\n", fBuffer);
	printf("start  %" B_PRId64 "\n", fBufferStart);
	printf("used   %ld\n", fBufferUsed);
	printf("phys   %ld\n", fBufferSize);
	printf("dirty  %s\n", (fBufferIsDirty) ? "true" : "false");
	printf("owns   %s\n", (fOwnsStream) ? "true" : "false");
}


//	#pragma mark -


// These functions are here to maintain future binary
// compatibility.
status_t BBufferIO::_Reserved_BufferIO_0(void*) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_1(void*) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_2(void*) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_3(void*) { return B_ERROR; }
status_t BBufferIO::_Reserved_BufferIO_4(void*) { return B_ERROR; }
