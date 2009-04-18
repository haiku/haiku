/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <DebugEventStream.h>

#include <stdlib.h>
#include <string.h>

#include <DataIO.h>


#define INPUT_BUFFER_SIZE	(128 * 1024)


BDebugEventInputStream::BDebugEventInputStream()
	:
	fStream(NULL),
	fFlags(0),
	fEventMask(0),
	fBuffer(NULL),
	fBufferCapacity(0),
	fBufferSize(0),
	fBufferPosition(0)
{
}


BDebugEventInputStream::~BDebugEventInputStream()
{
	Unset();

	free(fBuffer);
}


status_t
BDebugEventInputStream::SetTo(BDataIO* stream)
{
	Unset();

	// set the new values
	if (stream == NULL)
		return B_BAD_VALUE;

	fStream = stream;

	// allocate a buffer
	if (fBuffer == NULL) {
		fBuffer = (uint8*)malloc(INPUT_BUFFER_SIZE);
		if (fBuffer == NULL) {
			Unset();
			return B_NO_MEMORY;
		}

		fBufferCapacity = INPUT_BUFFER_SIZE;
		fBufferSize = 0;
	}

	fBufferPosition = 0;

	// read the header
	debug_event_stream_header header;
	ssize_t bytesRead = _Read(&header, sizeof(header));
	if (bytesRead < 0) {
		Unset();
		return bytesRead;
	}
	if ((size_t)bytesRead != sizeof(header)) {
		Unset();
		return B_BAD_DATA;
	}

	// check the header
	if (strlcpy(header.signature, B_DEBUG_EVENT_STREAM_SIGNATURE,
			sizeof(header.signature)) != 0
		|| header.version != B_DEBUG_EVENT_STREAM_VERSION
		|| (header.flags & B_DEBUG_EVENT_STREAM_FLAG_HOST_ENDIAN) == 0) {
		// TODO: Support non-host endianess!
		Unset();
		return B_BAD_DATA;
	}

	fFlags = header.flags;
	fEventMask = header.event_mask;

	return B_OK;
}


void
BDebugEventInputStream::Unset()
{
	fStream = NULL;
	fFlags = 0;
	fEventMask = 0;
}


ssize_t
BDebugEventInputStream::ReadNextEvent(
	const system_profiler_event_header** _header)
{
	// get the next header
	status_t error = _GetData(sizeof(system_profiler_event_header));
	if (error != B_OK) {
		if (error == B_BAD_DATA && fBufferSize == 0)
			return B_ENTRY_NOT_FOUND;
		return error;
	}

	system_profiler_event_header* header
		= (system_profiler_event_header*)(fBuffer + fBufferPosition);

	// get the data
	size_t size = header->size;
	size_t totalSize = sizeof(system_profiler_event_header) + size;
	if (header->size > 0) {
		error = _GetData(totalSize);
		if (error != B_OK)
			return error;
	}

	// header might have moved when getting the data
	header = (system_profiler_event_header*)(fBuffer + fBufferPosition);

	// skip the event in the buffer
	fBufferSize -= totalSize;
	fBufferPosition += totalSize;

	*_header = header;
	return size;
}


ssize_t
BDebugEventInputStream::_Read(void* _buffer, size_t size)
{
	uint8* buffer = (uint8*)_buffer;
	size_t totalBytesRead = 0;
	ssize_t bytesRead = 0;

	while (size > 0 && (bytesRead = fStream->Read(buffer, size)) > 0) {
		totalBytesRead += bytesRead;
		buffer += bytesRead;
		size -= bytesRead;
	}

	if (bytesRead < 0)
		return bytesRead;

	return totalBytesRead;
}


status_t
BDebugEventInputStream::_GetData(size_t size)
{
	if (fBufferSize >= size)
		return B_OK;

	if (size > fBufferCapacity)
		return B_BUFFER_OVERFLOW;

	// move remaining data to the start of the buffer
	if (fBufferSize > 0 && fBufferPosition > 0) {
		memmove(fBuffer, fBuffer + fBufferPosition, fBufferSize);
		fBufferPosition = 0;
	}

	// read more data
	ssize_t bytesRead = _Read(fBuffer + fBufferPosition,
		fBufferCapacity - fBufferSize);
	if (bytesRead < 0)
		return bytesRead;

	fBufferSize += bytesRead;

	return fBufferSize >= size ? B_OK : B_BAD_DATA;
}


// #pragma mark - BDebugEventOutputStream


BDebugEventOutputStream::BDebugEventOutputStream()
	:
	fStream(NULL),
	fFlags(0)
{
}


BDebugEventOutputStream::~BDebugEventOutputStream()
{
	Unset();
}


status_t
BDebugEventOutputStream::SetTo(BDataIO* stream, uint32 flags, uint32 eventMask)
{
	Unset();

	// set the new values
	if (stream == NULL)
		return B_BAD_VALUE;

	fStream = stream;
	fFlags = /*(flags & B_DEBUG_EVENT_STREAM_FLAG_ZIPPED)
		|*/ B_DEBUG_EVENT_STREAM_FLAG_HOST_ENDIAN;
		// TODO: Support zipped data!

	// init and write the header
	debug_event_stream_header header;
	memset(header.signature, 0, sizeof(header.signature));
	strlcpy(header.signature, B_DEBUG_EVENT_STREAM_SIGNATURE,
		sizeof(header.signature));
	header.version = B_DEBUG_EVENT_STREAM_VERSION;
	header.flags = fFlags;
	header.event_mask = eventMask;

	ssize_t written = fStream->Write(&header, sizeof(header));
	if (written < 0) {
		Unset();
		return written;
	}
	if ((size_t)written != sizeof(header)) {
		Unset();
		return B_FILE_ERROR;
	}

	return B_OK;
}


void
BDebugEventOutputStream::Unset()
{
	Flush();
	fStream = NULL;
	fFlags = 0;
}


status_t
BDebugEventOutputStream::Write(const void* buffer, size_t size)
{
	if (size == 0)
		return B_OK;
	if (buffer == NULL)
		return B_BAD_VALUE;

	ssize_t written = fStream->Write(buffer, size);
	if (written < 0)
		return written;
	if ((size_t)written != size)
		return B_FILE_ERROR;

	return B_OK;
}


status_t
BDebugEventOutputStream::Flush()
{
	return B_OK;
}
