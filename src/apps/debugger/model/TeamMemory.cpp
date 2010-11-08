/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TeamMemory.h"

#include <algorithm>

#include <OS.h>
#include <String.h>


TeamMemory::~TeamMemory()
{
}


status_t
TeamMemory::ReadMemoryString(target_addr_t address, size_t maxLength,
	BString& _string)
{
	char buffer[B_PAGE_SIZE];

	_string.Truncate(0);
	while (maxLength > 0) {
		// read at max maxLength bytes, but don't read across page bounds
		size_t toRead = std::min(maxLength,
			B_PAGE_SIZE - size_t(address % B_PAGE_SIZE));
		ssize_t bytesRead = ReadMemory(address, buffer, toRead);
		if (bytesRead < 0)
			return _string.Length() == 0 ? bytesRead : B_OK;

		if (bytesRead == 0)
			return _string.Length() == 0 ? B_BAD_ADDRESS : B_OK;

		// append the bytes read
		size_t length = strnlen(buffer, bytesRead);
		_string.Append(buffer, length);

		// stop at end of string
		if (length < (size_t)bytesRead)
			return B_OK;

		address += bytesRead;
		maxLength -= bytesRead;
	}

	return B_OK;
}

