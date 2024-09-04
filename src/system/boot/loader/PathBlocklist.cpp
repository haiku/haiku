/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/PathBlocklist.h>

#include <stdlib.h>

#include <algorithm>


// #pragma mark - BlockedPath


BlockedPath::BlockedPath()
	:
	fPath(NULL),
	fLength(0),
	fCapacity(0)
{
}


BlockedPath::~BlockedPath()
{
	free(fPath);
}


bool
BlockedPath::SetTo(const char* path)
{
	size_t length = strlen(path);
	if (length > 0 && path[length - 1] == '/')
		length--;

	if (!_Resize(length, false))
		return false;

	if (length > 0) {
		memcpy(fPath, path, length);
		fPath[length] = '\0';
	}

	return true;
}


bool
BlockedPath::Append(const char* component)
{
	size_t componentLength = strlen(component);
	if (componentLength > 0 && component[componentLength - 1] == '/')
		componentLength--;
	if (componentLength == 0)
		return true;

	size_t oldLength = fLength;
	size_t length = (fLength > 0 ? fLength + 1 : 0) + componentLength;
	if (!_Resize(length, true))
		return false;

	if (oldLength > 0)
		fPath[oldLength++] = '/';
	memcpy(fPath + oldLength, component, componentLength);
	return true;
}


bool
BlockedPath::_Resize(size_t length, bool keepData)
{
	if (length == 0) {
		free(fPath);
		fPath = NULL;
		fLength = 0;
		fCapacity = 0;
		return true;
	}

	if (length < fCapacity) {
		fPath[length] = '\0';
		fLength = length;
		return true;
	}

	size_t capacity = std::max(length + 1, 2 * fCapacity);
	capacity = std::max(capacity, size_t(32));

	char* path;
	if (fLength > 0 && keepData) {
		path = (char*)realloc(fPath, capacity);
		if (path == NULL)
			return false;
	} else {
		path = (char*)malloc(capacity);
		if (path == NULL)
			return false;
		free(fPath);
	}

	fPath = path;
	fPath[length] = '\0';
	fLength = length;
	fCapacity = capacity;
	return true;
}


// #pragma mark - PathBlocklist


PathBlocklist::PathBlocklist()
{
}


PathBlocklist::~PathBlocklist()
{
	MakeEmpty();
}


bool
PathBlocklist::Add(const char* path)
{
	BlockedPath* blockedPath = _FindPath(path);
	if (blockedPath != NULL)
		return true;

	blockedPath = new(std::nothrow) BlockedPath;
	if (blockedPath == NULL || !blockedPath->SetTo(path)) {
		delete blockedPath;
		return false;
	}

	fPaths.Add(blockedPath);
	return true;
}


void
PathBlocklist::Remove(const char* path)
{
	BlockedPath* blockedPath = _FindPath(path);
	if (blockedPath != NULL) {
		fPaths.Remove(blockedPath);
		delete blockedPath;
	}
}


bool
PathBlocklist::Contains(const char* path) const
{
	return _FindPath(path) != NULL;
}


void
PathBlocklist::MakeEmpty()
{
	while (BlockedPath* blockedPath = fPaths.RemoveHead())
		delete blockedPath;
}


BlockedPath*
PathBlocklist::_FindPath(const char* path) const
{
	for (PathList::ConstIterator it = fPaths.GetIterator(); it.HasNext();) {
		BlockedPath* blockedPath = it.Next();
		if (*blockedPath == path)
			return blockedPath;
	}

	return NULL;
}
