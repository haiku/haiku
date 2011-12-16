/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <BufferedDataIO.h>

#include <new>

#include <stdio.h>
#include <string.h>


//#define TRACE_DATA_IO
#ifdef TRACE_DATA_IO
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


BBufferedDataIO::BBufferedDataIO(BDataIO& stream, size_t bufferSize,
	bool ownsStream, bool partialReads)
	:
	fStream(stream),
	fPosition(0),
	fSize(0),
	fDirty(false),
	fOwnsStream(ownsStream),
	fPartialReads(partialReads)
{
	fBufferSize = max_c(bufferSize, 512);
	fBuffer = new(std::nothrow) uint8[fBufferSize];
}


BBufferedDataIO::~BBufferedDataIO()
{
	Flush();
	delete[] fBuffer;

	if (fOwnsStream)
		delete &fStream;
}


status_t
BBufferedDataIO::InitCheck() const
{
	return fBuffer == NULL ? B_NO_MEMORY : B_OK;
}


BDataIO*
BBufferedDataIO::Stream() const
{
	return &fStream;
}


size_t
BBufferedDataIO::BufferSize() const
{
	return fBufferSize;
}


bool
BBufferedDataIO::OwnsStream() const
{
	return fOwnsStream;
}


void
BBufferedDataIO::SetOwnsStream(bool ownsStream)
{
	fOwnsStream = ownsStream;
}


status_t
BBufferedDataIO::Flush()
{
	if (!fDirty)
		return B_OK;

	size_t bytesWritten = fStream.Write(fBuffer + fPosition, fSize);
	if (bytesWritten == fSize) {
		fDirty = false;
		fPosition = 0;
		fSize = 0;
		return B_OK;
	} else if (bytesWritten >= 0) {
		fSize -= bytesWritten;
		fPosition += bytesWritten;
		return B_ERROR;
	}

	return bytesWritten;
}


ssize_t
BBufferedDataIO::Read(void* buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	TRACE("%p::Read(size %lu)\n", this, size);

	size_t bytesRead = 0;

	if (fSize > 0) {
		// fill the part of the stream we already have
		bytesRead = min_c(size, fSize);
		TRACE("%p: read %lu bytes we already have in the buffer.\n", this,
			bytesRead);
		memcpy(buffer, fBuffer + fPosition, bytesRead);

		buffer = (void*)((uint8_t*)buffer + bytesRead);
		size -= bytesRead;
		fPosition += bytesRead;
		fSize -= bytesRead;

		if (fPartialReads)
			return bytesRead;
	}

	if (size > fBufferSize || fBuffer == NULL) {
		// request is larger than our buffer, just fill it directly
		return fStream.Read(buffer, size);
	}

	if (size > 0) {
		// retrieve next buffer

		status_t status = Flush();
		if (status != B_OK)
			return status;

		TRACE("%p: read %" B_PRIuSIZE " bytes from stream\n", this, fBufferSize);
		fSize = fStream.Read(fBuffer, fBufferSize);
		TRACE("%p: retrieved %" B_PRIuSIZE " bytes from stream\n", this, fSize);
		fPosition = 0;

		// Copy the remaining part
		size_t copy = min_c(size, fSize);
		memcpy(buffer, fBuffer, copy);
		TRACE("%p: copy %" B_PRIuSIZE" bytes to buffer\n", this, copy);

		bytesRead += copy;
		fPosition = copy;
		fSize -= copy;
	}

	return bytesRead;
}


ssize_t
BBufferedDataIO::Write(const void* buffer, size_t size)
{
	if (buffer == NULL)
		return B_BAD_VALUE;

	TRACE("%p::Write(size %lu)\n", this, size);

	if (!fDirty) {
		// Throw away a read-only buffer if necessary
		TRACE("%p: throw away previous buffer.\n", this);
		fPosition = 0;
		fSize = 0;
	}

	size_t bytesWritten = 0;

	if (size > fBufferSize || fBuffer == NULL) {
		// request is larger than our buffer, just fill it directly
		bytesWritten = fSize;

		status_t status = Flush();
		if (status != B_OK)
			return status;

		ssize_t streamWritten = fStream.Write(buffer, size);
		if (streamWritten >= 0)
			return bytesWritten + streamWritten;

		return streamWritten;
	}

	bytesWritten = min_c(size, fBufferSize - fSize - fPosition);
	TRACE("%p: write %" B_PRIuSIZE " bytes to the buffer.\n", this,
		bytesWritten);
	memcpy(fBuffer + fPosition + fSize, buffer, bytesWritten);
	fSize += bytesWritten;
	size -= bytesWritten;

	if (size > 0) {
		status_t status = Flush();
		if (status != B_OK)
			return status;

		memcpy(fBuffer, (uint8*)buffer + bytesWritten, size);
		fPosition = 0;
		fSize = size;
		fDirty = true;
		bytesWritten += size;
	}

	return bytesWritten;
}


//	#pragma mark - FBC


status_t BBufferedDataIO::_Reserved0(void*) { return B_ERROR; }
status_t BBufferedDataIO::_Reserved1(void*) { return B_ERROR; }
status_t BBufferedDataIO::_Reserved2(void*) { return B_ERROR; }
status_t BBufferedDataIO::_Reserved3(void*) { return B_ERROR; }
status_t BBufferedDataIO::_Reserved4(void*) { return B_ERROR; }
