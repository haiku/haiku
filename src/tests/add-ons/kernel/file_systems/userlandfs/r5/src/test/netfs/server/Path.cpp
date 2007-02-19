// Path.cpp

#include <stdlib.h>
#include <string.h>

#include <StorageDefs.h>

#include "Path.h"

// constructor
Path::Path()
	: fBuffer(NULL),
	  fBufferSize(0),
	  fLength(0)
{
}

// destructor
Path::~Path()
{
	free(fBuffer);
}

// SetTo
status_t
Path::SetTo(const char* path, const char* leaf)
{
	if (!path)
		return B_BAD_VALUE;

	// get the base path len
	int32 len = strlen(path);
	if (len == 0)
		return B_BAD_VALUE;

	// get leaf len and check, if a separator needs to be inserted
	bool insertSeparator = false;
	int32 leafLen = 0;
	if (leaf) {
		leafLen = strlen(leaf);
		if (leafLen > 0)
			insertSeparator = (path[len - 1] != '/' && leaf[0] != '/');
	}

	// compute the resulting length and resize the buffer
	int32 wholeLen = len + leafLen + (insertSeparator ? 1 : 0);
	status_t error = _Resize(wholeLen + 1);
	if (error != B_OK)
		return error;

	// copy path
	memcpy(fBuffer, path, len);

	// insert separator
	if (insertSeparator)
		fBuffer[len++] = '/';

	// append leaf
	if (leafLen > 0)
		memcpy(fBuffer + len, leaf, leafLen);

	// null terminate
	fBuffer[wholeLen] = '\0';
	fLength = wholeLen;
	return B_OK;
}

// Append
status_t
Path::Append(const char* leaf)
{
	if (!leaf)
		return B_BAD_VALUE;

	if (fLength == 0)
		return SetTo(leaf);

	// get the leaf len
	int32 leafLen = strlen(leaf);
	if (leafLen == 0)
		return B_BAD_VALUE;

	// check, if we need a separator
	bool insertSeparator = (fBuffer[fLength - 1] != '/' && leaf[0] != '/');

	// compute the resulting length and resize the buffer
	int32 wholeLen = fLength + leafLen + (insertSeparator ? 1 : 0);
	status_t error = _Resize(wholeLen + 1);
	if (error != B_OK)
		return error;

	// insert separator
	if (insertSeparator)
		fBuffer[fLength++] = '/';

	// append leaf
	if (leafLen > 0)
		memcpy(fBuffer + fLength, leaf, leafLen + 1);

	fLength = wholeLen;
	return B_OK;
}

// GetPath
const char*
Path::GetPath() const
{
	return (fLength == 0 ? NULL : fBuffer);
}

// GetLength
int32
Path::GetLength() const
{
	return fLength;
}

// _Resize
status_t
Path::_Resize(int32 minLen)
{
	// align to multiples of B_PATH_NAME_LENGTH
	minLen = (minLen + B_PATH_NAME_LENGTH - 1)
		/ B_PATH_NAME_LENGTH * B_PATH_NAME_LENGTH;

	if (minLen != fBufferSize) {
		char* buffer = (char*)realloc(fBuffer, minLen);
		if (!buffer)
			return B_NO_MEMORY;

		fBuffer = buffer;
		fBufferSize = minLen;
	}

	return B_OK;
}
