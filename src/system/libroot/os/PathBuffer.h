/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PATH_BUFFER_H
#define PATH_BUFFER_H


#include <string.h>

#include <algorithm>


namespace {

struct PathBuffer {
	PathBuffer()
		:
		fBuffer(NULL),
		fSize(0),
		fLength(0)
	{
	}

	PathBuffer(char* buffer, size_t size, size_t length = 0)
	{
		SetTo(buffer, size, length);
	}

	void SetTo(char* buffer, size_t size, size_t length = 0)
	{
		fBuffer = buffer;
		fSize = size;
		fLength = length;
		if (fLength < fSize)
			fBuffer[fLength] = '\0';
	}

	bool Append(const char* toAppend, size_t length)
	{
		if (length > 0 && fLength + 1 < fSize) {
			size_t toCopy = std::min(length, fSize - fLength - 1);
			memcpy(fBuffer + fLength, toAppend, toCopy);
			fBuffer[fLength + toCopy] = '\0';
		}

		fLength += length;
		return fLength < fSize;
	}

	bool Append(const char* toAppend)
	{
		return Append(toAppend, strlen(toAppend));
	}

	bool Append(char c)
	{
		return Append(&c, 1);
	}

	size_t Length() const
	{
		return fLength;
	}

private:
	char*	fBuffer;
	size_t	fSize;
	size_t	fLength;
};

}


#endif	// PATH_BUFFER_H
