// KPath.cpp

#include <stdlib.h>
#include <string.h>

#include <KPath.h>

// debugging
#define DBG(x)
//#define DBG(x) x
#define OUT dprintf

// constructor
KPath::KPath(int32 bufferSize)
	: fBuffer(NULL),
	  fBufferSize(0),
	  fPathLength(0),
	  fLocked(false)
{
	SetTo(NULL, bufferSize);
}

// constructor
KPath::KPath(const char* path, int32 bufferSize)
	: fBuffer(NULL),
	  fBufferSize(0),
	  fPathLength(0),
	  fLocked(false)
{
	SetTo(path, bufferSize);
}

// copy constructor
KPath::KPath(const KPath& other)
	: fBuffer(NULL),
	  fBufferSize(0),
	  fPathLength(0),
	  fLocked(false)
{
	*this = other;
}

// destructor
KPath::~KPath()
{
	free(fBuffer);
}

// SetTo
status_t
KPath::SetTo(const char* path, int32 bufferSize)
{
	if (bufferSize <= 0)
		bufferSize = B_PATH_NAME_LENGTH;
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
		return B_NO_MEMORY;
	if (fBuffer) {
		fBufferSize = bufferSize;
		fBuffer[0] = '\0';
	}
	return SetPath(path);
}

// InitCheck
status_t
KPath::InitCheck() const
{
	return (fBuffer ? B_OK : B_NO_MEMORY);
}

// SetPath
status_t
KPath::SetPath(const char *path)
{
	if (!fBuffer)
		return B_NO_INIT;
	if (path) {
		int32 len = strlen(path);
		if (len >= fBufferSize)
			return B_BUFFER_OVERFLOW;
		memcpy(fBuffer, path, len + 1);
		fPathLength = len;
		_ChopTrailingSlashes();
	} else {
		fBuffer[0] = '\0';
		fPathLength = 0;
	}
	return B_OK;
}

// Path
const char*
KPath::Path() const
{
	return fBuffer;
}

// Length
int32
KPath::Length() const
{
	return fPathLength;
}

// BufferSize
int32
KPath::BufferSize() const
{
	return fBufferSize;
}

// LockBuffer
char *
KPath::LockBuffer()
{
	if (!fBuffer || fLocked)
		return NULL;
	fLocked = true;
	return fBuffer;
}

// UnlockBuffer
void
KPath::UnlockBuffer()
{
	if (!fLocked) {
		DBG(OUT("KPath::UnlockBuffer(): ERROR: Buffer not locked!\n"));
		return;
	}
	fLocked = false;
	fPathLength = strnlen(fBuffer, fBufferSize);
	if (fPathLength == fBufferSize) {
		DBG(OUT("KPath::UnlockBuffer(): WARNING: Unterminated buffer!\n"));
		fPathLength--;
		fBuffer[fPathLength] = '\0';
	}
	_ChopTrailingSlashes();
}

// Leaf
const char *
KPath::Leaf() const
{
	if (!fBuffer)
		return NULL;
	// only "/" has trailing slashes -- then we have to return the complete
	// buffer, as we have to do in case there are no slashes at all
	if (fPathLength != 1 || fBuffer[0] != '/') {
		for (int32 i = fPathLength - 1; i >= 0; i--) {
			if (fBuffer[i] == '/')
				return fBuffer + i + 1;
		}
	}
	return fBuffer;
}

// ReplaceLeaf
status_t
KPath::ReplaceLeaf(const char *newLeaf)
{
	const char *leaf = Leaf();
	if (!leaf)
		return B_NO_INIT;
	int32 leafIndex = leaf - fBuffer;
	// chop off the current leaf (don't replace "/", though)
	if (leafIndex != 0 || fBuffer[leafIndex - 1]) {
		fBuffer[leafIndex] = '\0';
		fPathLength = leafIndex;
		_ChopTrailingSlashes();
	}
	// if a leaf was given, append it
	if (newLeaf)
		return Append(newLeaf);
	return B_OK;
}

// Append
status_t
KPath::Append(const char *component, bool isComponent)
{
	// check initialization and parameter
	if (!fBuffer)
		return B_NO_INIT;
	if (!component)
		return B_BAD_VALUE;
	if (fPathLength == 0)
		return SetPath(component);
	// get component length
	int32 componentLen = strlen(component);
	if (componentLen < 1)
		return B_OK;
	// if our current path is empty, we just copy the supplied one
	// compute the result path len
	bool insertSlash = isComponent && fBuffer[fPathLength - 1] != '/'
		&& component[0] != '/';
	int32 resultPathLen = fPathLength + componentLen + (insertSlash ? 1 : 0);
	if (resultPathLen >= fBufferSize)
		return B_BUFFER_OVERFLOW;
	// compose the result path
	if (insertSlash)
		fBuffer[fPathLength++] = '/';
	memcpy(fBuffer + fPathLength, component, componentLen + 1);
	fPathLength = resultPathLen;
	return B_OK;
}

// =
KPath&
KPath::operator=(const KPath& other)
{
	SetTo(other.fBuffer, other.fBufferSize);
	return *this;
}

// =
KPath&
KPath::operator=(const char* path)
{
	SetTo(path);
	return *this;
}

// ==
bool
KPath::operator==(const KPath& other) const
{
	if (!fBuffer)
		return (!other.fBuffer);
	return (other.fBuffer
		&& fPathLength == other.fPathLength
		&& strcmp(fBuffer, other.fBuffer) == 0);
}

// ==
bool
KPath::operator==(const char* path) const
{
	if (!fBuffer)
		return (!path);
	return (path && strcmp(fBuffer, path) == 0);
}

// !=
bool
KPath::operator!=(const KPath& other) const
{
	return !(*this == other);
}

// !=
bool
KPath::operator!=(const char* path) const
{
	return !(*this == path);
}

// _ChopTrailingSlashes
void
KPath::_ChopTrailingSlashes()
{
	if (fBuffer) {
		while (fPathLength > 1 && fBuffer[fPathLength - 1] == '/')
			fBuffer[--fPathLength] = '\0';
	}
}

