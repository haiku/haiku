/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <DebugEventStream.h>

#include <stdlib.h>
#include <string.h>

#include <DataIO.h>

#include <system_profiler_defs.h>


#define INPUT_BUFFER_SIZE	(128 * 1024)


BDebugEventInputStream::BDebugEventInputStream()
	:
	fStream(NULL),
	fFlags(0),
	fEventMask(0),
	fBuffer(NULL),
	fBufferCapacity(0),
	fBufferSize(0),
	fBufferPosition(0),
	fStreamPosition(0),
	fOwnsBuffer(false)
{
}


BDebugEventInputStream::~BDebugEventInputStream()
{
	Unset();

	if (fOwnsBuffer)
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

		fOwnsBuffer = true;
		fBufferCapacity = INPUT_BUFFER_SIZE;
		fBufferSize = 0;
	}

	return _Init();
}


status_t
BDebugEventInputStream::SetTo(const void* data, size_t size,
	bool takeOverOwnership)
{
	Unset();

	if (data == NULL || size == 0)
		return B_BAD_VALUE;

	if (fBuffer != NULL) {
		if (fOwnsBuffer)
			free(fBuffer);
		fBuffer = NULL;
		fBufferCapacity = 0;
		fBufferSize = 0;
	}

	fBuffer = (uint8*)data;
	fBufferCapacity = fBufferSize = size;
	fOwnsBuffer = takeOverOwnership;

	return _Init();
}


void
BDebugEventInputStream::Unset()
{
	fStream = NULL;
	fFlags = 0;
	fEventMask = 0;

	// If we have a buffer that we own and has the right size, we keep it.
	if (fOwnsBuffer) {
		if (fBuffer != NULL && fBufferSize != INPUT_BUFFER_SIZE) {
			free(fBuffer);
			fBuffer = NULL;
			fBufferCapacity = 0;
			fBufferSize = 0;
		}
	} else {
		fBuffer = NULL;
		fBufferCapacity = 0;
		fBufferSize = 0;
	}
}


status_t
BDebugEventInputStream::Seek(off_t streamOffset)
{
	// TODO: Support for streams, at least for BPositionIOs.
	if (fStream != NULL)
		return B_UNSUPPORTED;

	if (streamOffset < 0 || streamOffset > (off_t)fBufferCapacity)
		return B_BUFFER_OVERFLOW;

	fStreamPosition = 0;
	fBufferPosition = streamOffset;
	fBufferSize = fBufferCapacity - streamOffset;

	return B_OK;
}


/*!	\brief Returns the next event in the stream.

	At the end of the stream \c 0 is returned and \c *_buffer is set to \c NULL.
	For events that don't have data associated with them, \c *_buffer will still
	be non-NULL, even if dereferencing that address is not allowed.

	\param _event Pointer to a pre-allocated location where the event ID shall
		be stored.
	\param _cpu Pointer to a pre-allocated location where the CPU index shall
		be stored.
	\param _buffer Pointer to a pre-allocated location where the pointer to the
		event data shall be stored.
	\param _streamOffset Pointer to a pre-allocated location where the event
		header's offset relative to the beginning of the stream shall be stored.
		May be \c NULL.
	\return A negative error code in case an error occurred while trying to read
		the info, the size of the data associated with the event otherwise.
*/
ssize_t
BDebugEventInputStream::ReadNextEvent(uint32* _event, uint32* _cpu,
	const void** _buffer, off_t* _streamOffset)
{
	// get the next header
	status_t error = _GetData(sizeof(system_profiler_event_header));
	if (error != B_OK) {
		if (error == B_BAD_DATA && fBufferSize == 0) {
			*_buffer = NULL;
			return 0;
		}
		return error;
	}

	system_profiler_event_header header
		= *(system_profiler_event_header*)(fBuffer + fBufferPosition);

	off_t streamOffset = fStreamPosition + fBufferPosition;

	// skip the header in the buffer
	fBufferSize -= sizeof(system_profiler_event_header);
	fBufferPosition += sizeof(system_profiler_event_header);

	// get the data
	if (header.size > 0) {
		error = _GetData(header.size);
		if (error != B_OK)
			return error;
	}

	*_event = header.event;
	*_cpu = header.cpu;
	*_buffer = fBuffer + fBufferPosition;
	if (_streamOffset)
		*_streamOffset = streamOffset;

	// skip the event in the buffer
	fBufferSize -= header.size;
	fBufferPosition += header.size;

	return header.size;
}


status_t
BDebugEventInputStream::_Init()
{
	fStreamPosition = 0;
	fBufferPosition = 0;

	// get the header
	status_t error = _GetData(sizeof(debug_event_stream_header));
	if (error != B_OK) {
		Unset();
		return error;
	}
	const debug_event_stream_header& header
		= *(const debug_event_stream_header*)(fBuffer + fBufferPosition);

	fBufferPosition += sizeof(debug_event_stream_header);
	fBufferSize -= sizeof(debug_event_stream_header);

	// check the header
	if (strncmp(header.signature, B_DEBUG_EVENT_STREAM_SIGNATURE,
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
	if (fBufferSize > 0 && fBufferPosition > 0)
		memmove(fBuffer, fBuffer + fBufferPosition, fBufferSize);
	fStreamPosition += fBufferPosition;
	fBufferPosition = 0;

	// read more data
	if (fStream != NULL) {
		ssize_t bytesRead = _Read(fBuffer + fBufferSize,
			fBufferCapacity - fBufferSize);
		if (bytesRead < 0)
			return bytesRead;

		fBufferSize += bytesRead;
	}

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
