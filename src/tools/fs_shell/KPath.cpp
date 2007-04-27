/*
 * Copyright 2004-2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

/** A simple class wrapping a path. Has a fixed-sized buffer. */

#include "KPath.h"

#include <stdlib.h>

#include "fssh_string.h"
#include "vfs.h"


// debugging
#define TRACE(x) ;
//#define TRACE(x) dprintf x


KPath::KPath(fssh_size_t bufferSize)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false)
{
	SetTo(NULL, bufferSize);
}


KPath::KPath(const char* path, bool normalize, fssh_size_t bufferSize)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false)
{
	SetTo(path, normalize, bufferSize);
}


KPath::KPath(const KPath& other)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false)
{
	*this = other;
}


KPath::~KPath()
{
	free(fBuffer);
}


fssh_status_t
KPath::SetTo(const char* path, bool normalize, fssh_size_t bufferSize)
{
	if (bufferSize == 0)
		bufferSize = FSSH_B_PATH_NAME_LENGTH;

	// free the previous buffer, if the buffer size differs
	if (fBuffer && fBufferSize != bufferSize) {
		free(fBuffer);
		fBuffer = NULL;
		fBufferSize = 0;
	}
	fPathLength = 0;
	fLocked = false;

	// allocate buffer
	if (!fBuffer)
		fBuffer = (char*)malloc(bufferSize);
	if (!fBuffer)
		return FSSH_B_NO_MEMORY;
	if (fBuffer) {
		fBufferSize = bufferSize;
		fBuffer[0] = '\0';
	}
	return SetPath(path, normalize);
}


fssh_status_t
KPath::InitCheck() const
{
	return fBuffer ? FSSH_B_OK : FSSH_B_NO_MEMORY;
}


fssh_status_t
KPath::SetPath(const char *path, bool normalize)
{
	if (!fBuffer)
		return FSSH_B_NO_INIT;

	if (path) {
		if (normalize) {
			// normalize path
			fssh_status_t error = vfs_normalize_path(path, fBuffer, fBufferSize,
				true);
			if (error != FSSH_B_OK) {
				SetPath(NULL);
				return error;
			}
			fPathLength = fssh_strlen(fBuffer);
		} else {
			// don't normalize path
			fssh_size_t length = fssh_strlen(path);
			if (length >= fBufferSize)
				return FSSH_B_BUFFER_OVERFLOW;

			fssh_memcpy(fBuffer, path, length + 1);
			fPathLength = length;
			_ChopTrailingSlashes();
		}
	} else {
		fBuffer[0] = '\0';
		fPathLength = 0;
	}
	return FSSH_B_OK;
}


const char*
KPath::Path() const
{
	return fBuffer;
}


char *
KPath::LockBuffer()
{
	if (!fBuffer || fLocked)
		return NULL;

	fLocked = true;
	return fBuffer;
}


void
KPath::UnlockBuffer()
{
	if (!fLocked) {
		TRACE(("KPath::UnlockBuffer(): ERROR: Buffer not locked!\n"));
		return;
	}
	fLocked = false;
	fPathLength = fssh_strnlen(fBuffer, fBufferSize);
	if (fPathLength == fBufferSize) {
		TRACE(("KPath::UnlockBuffer(): WARNING: Unterminated buffer!\n"));
		fPathLength--;
		fBuffer[fPathLength] = '\0';
	}
	_ChopTrailingSlashes();
}


const char *
KPath::Leaf() const
{
	if (!fBuffer)
		return NULL;

	// only "/" has trailing slashes -- then we have to return the complete
	// buffer, as we have to do in case there are no slashes at all
	if (fPathLength != 1 || fBuffer[0] != '/') {
		for (int32_t i = fPathLength - 1; i >= 0; i--) {
			if (fBuffer[i] == '/')
				return fBuffer + i + 1;
		}
	}
	return fBuffer;
}


fssh_status_t
KPath::ReplaceLeaf(const char *newLeaf)
{
	const char *leaf = Leaf();
	if (!leaf)
		return FSSH_B_NO_INIT;

	int32_t leafIndex = leaf - fBuffer;
	// chop off the current leaf (don't replace "/", though)
	if (leafIndex != 0 || fBuffer[leafIndex - 1]) {
		fBuffer[leafIndex] = '\0';
		fPathLength = leafIndex;
		_ChopTrailingSlashes();
	}

	// if a leaf was given, append it
	if (newLeaf)
		return Append(newLeaf);
	return FSSH_B_OK;
}


fssh_status_t
KPath::Append(const char *component, bool isComponent)
{
	// check initialization and parameter
	if (!fBuffer)
		return FSSH_B_NO_INIT;
	if (!component)
		return FSSH_B_BAD_VALUE;
	if (fPathLength == 0)
		return SetPath(component);

	// get component length
	fssh_size_t componentLength = fssh_strlen(component);
	if (componentLength < 1)
		return FSSH_B_OK;

	// if our current path is empty, we just copy the supplied one
	// compute the result path len
	bool insertSlash = isComponent && fBuffer[fPathLength - 1] != '/'
		&& component[0] != '/';
	fssh_size_t resultPathLength = fPathLength + componentLength + (insertSlash ? 1 : 0);
	if (resultPathLength >= fBufferSize)
		return FSSH_B_BUFFER_OVERFLOW;

	// compose the result path
	if (insertSlash)
		fBuffer[fPathLength++] = '/';
	fssh_memcpy(fBuffer + fPathLength, component, componentLength + 1);
	fPathLength = resultPathLength;
	return FSSH_B_OK;
}


KPath&
KPath::operator=(const KPath& other)
{
	SetTo(other.fBuffer, other.fBufferSize);
	return *this;
}


KPath&
KPath::operator=(const char* path)
{
	SetTo(path);
	return *this;
}


bool
KPath::operator==(const KPath& other) const
{
	if (!fBuffer)
		return !other.fBuffer;

	return (other.fBuffer
		&& fPathLength == other.fPathLength
		&& fssh_strcmp(fBuffer, other.fBuffer) == 0);
}


bool
KPath::operator==(const char* path) const
{
	if (!fBuffer)
		return (!path);

	return path && !fssh_strcmp(fBuffer, path);
}


bool
KPath::operator!=(const KPath& other) const
{
	return !(*this == other);
}


bool
KPath::operator!=(const char* path) const
{
	return !(*this == path);
}


void
KPath::_ChopTrailingSlashes()
{
	if (fBuffer) {
		while (fPathLength > 1 && fBuffer[fPathLength - 1] == '/')
			fBuffer[--fPathLength] = '\0';
	}
}

