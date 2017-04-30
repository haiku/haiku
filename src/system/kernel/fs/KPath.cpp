/*
 * Copyright 2004-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


/*! A simple class wrapping a path. Has a fixed-sized buffer. */


#include <fs/KPath.h>

#include <stdlib.h>
#include <string.h>

#include <team.h>
#include <vfs.h>


// debugging
#define TRACE(x) ;
//#define TRACE(x) dprintf x


KPath::KPath(size_t bufferSize)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false),
	fLazy(false),
	fFailed(false),
	fIsNull(false)
{
	SetTo(NULL, DEFAULT, bufferSize);
}


KPath::KPath(const char* path, int32 flags, size_t bufferSize)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false),
	fLazy(false),
	fFailed(false),
	fIsNull(false)
{
	SetTo(path, flags, bufferSize);
}


KPath::KPath(const KPath& other)
	:
	fBuffer(NULL),
	fBufferSize(0),
	fPathLength(0),
	fLocked(false),
	fLazy(false),
	fFailed(false),
	fIsNull(false)
{
	*this = other;
}


KPath::~KPath()
{
	free(fBuffer);
}


status_t
KPath::SetTo(const char* path, int32 flags, size_t bufferSize)
{
	if (bufferSize == 0)
		bufferSize = B_PATH_NAME_LENGTH;

	// free the previous buffer, if the buffer size differs
	if (fBuffer != NULL && fBufferSize != bufferSize) {
		free(fBuffer);
		fBuffer = NULL;
		fBufferSize = 0;
	}

	fPathLength = 0;
	fLocked = false;
	fBufferSize = bufferSize;
	fLazy = (flags & LAZY_ALLOC) != 0;
	fIsNull = path == NULL;

	if (path != NULL || !fLazy) {
		status_t status = _AllocateBuffer();
		if (status != B_OK)
			return status;
	}

	return SetPath(path, flags);
}


void
KPath::Adopt(KPath& other)
{
	free(fBuffer);

	fBuffer = other.fBuffer;
	fBufferSize = other.fBufferSize;
	fPathLength = other.fPathLength;
	fLazy = other.fLazy;
	fFailed = other.fFailed;
	fIsNull = other.fIsNull;

	other.fBuffer = NULL;
	if (!other.fLazy)
		other.fBufferSize = 0;
	other.fPathLength = 0;
	other.fFailed = false;
	other.fIsNull = other.fLazy;
}


status_t
KPath::InitCheck() const
{
	if (fBuffer != NULL || (fLazy && !fFailed && fBufferSize != 0))
		return B_OK;

	return fFailed ? B_NO_MEMORY : B_NO_INIT;
}


/*!	\brief Sets the buffer to \a path.

	\param flags Understands the following two options:
		- \c NORMALIZE
		- \c TRAVERSE_LEAF_LINK
*/
status_t
KPath::SetPath(const char* path, int32 flags)
{
	if (path == NULL && fLazy && fBuffer == NULL) {
		fIsNull = true;
		return B_OK;
	}

	if (fBuffer == NULL) {
		if (fLazy) {
			status_t status = _AllocateBuffer();
			if (status != B_OK)
				return B_NO_MEMORY;
		} else
			return B_NO_INIT;
	}

	fIsNull = false;

	if (path != NULL) {
		if ((flags & NORMALIZE) != 0) {
			// normalize path
			status_t status = _Normalize(path,
				(flags & TRAVERSE_LEAF_LINK) != 0);
			if (status != B_OK)
				return status;
		} else {
			// don't normalize path
			size_t length = strlen(path);
			if (length >= fBufferSize)
				return B_BUFFER_OVERFLOW;

			memcpy(fBuffer, path, length + 1);
			fPathLength = length;
			_ChopTrailingSlashes();
		}
	} else {
		fBuffer[0] = '\0';
		fPathLength = 0;
		if (fLazy)
			fIsNull = true;
	}
	return B_OK;
}


const char*
KPath::Path() const
{
	return fIsNull ? NULL : fBuffer;
}


/*!	\brief Locks the buffer for external changes.

	\param force In lazy mode, this will allocate a buffer when set.
		Otherwise, \c NULL will be returned if set to NULL.
*/
char*
KPath::LockBuffer(bool force)
{
	if (fBuffer == NULL && fLazy) {
		if (fIsNull && !force)
			return NULL;

		_AllocateBuffer();
	}

	if (fBuffer == NULL || fLocked)
		return NULL;

	fLocked = true;
	fIsNull = false;

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

	if (fBuffer == NULL)
		return;

	fPathLength = strnlen(fBuffer, fBufferSize);
	if (fPathLength == fBufferSize) {
		TRACE(("KPath::UnlockBuffer(): WARNING: Unterminated buffer!\n"));
		fPathLength--;
		fBuffer[fPathLength] = '\0';
	}
	_ChopTrailingSlashes();
}


char*
KPath::DetachBuffer()
{
	char* buffer = fBuffer;

	if (fBuffer != NULL) {
		fBuffer = NULL;
		fPathLength = 0;
		fLocked = false;
	}

	return buffer;
}


const char*
KPath::Leaf() const
{
	if (fBuffer == NULL)
		return NULL;

	for (int32 i = fPathLength - 1; i >= 0; i--) {
		if (fBuffer[i] == '/')
			return fBuffer + i + 1;
	}

	return fBuffer;
}


status_t
KPath::ReplaceLeaf(const char* newLeaf)
{
	const char* leaf = Leaf();
	if (leaf == NULL)
		return B_NO_INIT;

	int32 leafIndex = leaf - fBuffer;
	// chop off the current leaf (don't replace "/", though)
	if (leafIndex != 0 || fBuffer[leafIndex - 1]) {
		fBuffer[leafIndex] = '\0';
		fPathLength = leafIndex;
		_ChopTrailingSlashes();
	}

	// if a leaf was given, append it
	if (newLeaf != NULL)
		return Append(newLeaf);
	return B_OK;
}


bool
KPath::RemoveLeaf()
{
	// get the leaf -- bail out, if not initialized or only the "/" is left
	const char* leaf = Leaf();
	if (leaf == NULL || leaf == fBuffer || leaf[0] == '\0')
		return false;

	// chop off the leaf
	int32 leafIndex = leaf - fBuffer;
	fBuffer[leafIndex] = '\0';
	fPathLength = leafIndex;
	_ChopTrailingSlashes();

	return true;
}


status_t
KPath::Append(const char* component, bool isComponent)
{
	// check initialization and parameter
	if (fBuffer == NULL)
		return B_NO_INIT;
	if (component == NULL)
		return B_BAD_VALUE;
	if (fPathLength == 0)
		return SetPath(component);

	// get component length
	size_t componentLength = strlen(component);
	if (componentLength < 1)
		return B_OK;

	// if our current path is empty, we just copy the supplied one
	// compute the result path len
	bool insertSlash = isComponent && fBuffer[fPathLength - 1] != '/'
		&& component[0] != '/';
	size_t resultPathLength = fPathLength + componentLength
		+ (insertSlash ? 1 : 0);
	if (resultPathLength >= fBufferSize)
		return B_BUFFER_OVERFLOW;

	// compose the result path
	if (insertSlash)
		fBuffer[fPathLength++] = '/';
	memcpy(fBuffer + fPathLength, component, componentLength + 1);
	fPathLength = resultPathLength;
	return B_OK;
}


status_t
KPath::Normalize(bool traverseLeafLink)
{
	if (fBuffer == NULL)
		return B_NO_INIT;
	if (fPathLength == 0)
		return B_BAD_VALUE;

	return _Normalize(fBuffer, traverseLeafLink);
}


KPath&
KPath::operator=(const KPath& other)
{
	SetTo(other.fBuffer, fLazy ? KPath::LAZY_ALLOC : KPath::DEFAULT,
		other.fBufferSize);
	return *this;
}


KPath&
KPath::operator=(const char* path)
{
	SetPath(path);
	return *this;
}


bool
KPath::operator==(const KPath& other) const
{
	if (fBuffer == NULL)
		return !other.fBuffer;

	return other.fBuffer != NULL
		&& fPathLength == other.fPathLength
		&& strcmp(fBuffer, other.fBuffer) == 0;
}


bool
KPath::operator==(const char* path) const
{
	if (fBuffer == NULL)
		return path == NULL;

	return path != NULL && strcmp(fBuffer, path) == 0;
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


status_t
KPath::_AllocateBuffer()
{
	if (fBuffer == NULL && fBufferSize != 0)
		fBuffer = (char*)malloc(fBufferSize);
	if (fBuffer == NULL) {
		fFailed = true;
		return B_NO_MEMORY;
	}

	fBuffer[0] = '\0';
	fFailed = false;
	return B_OK;
}


status_t
KPath::_Normalize(const char* path, bool traverseLeafLink)
{
	status_t error = vfs_normalize_path(path, fBuffer, fBufferSize,
		traverseLeafLink,
		team_get_kernel_team_id() == team_get_current_team_id());
	if (error != B_OK) {
		// vfs_normalize_path() might have screwed up the previous
		// path -- unset it completely to avoid weird problems.
		fBuffer[0] = '\0';
		fPathLength = 0;
		return error;
	}

	fPathLength = strlen(fBuffer);
	return B_OK;
}


void
KPath::_ChopTrailingSlashes()
{
	if (fBuffer != NULL) {
		while (fPathLength > 1 && fBuffer[fPathLength - 1] == '/')
			fBuffer[--fPathLength] = '\0';
	}
}

