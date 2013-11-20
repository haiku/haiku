/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/PathBlacklist.h>

#include <stdlib.h>

#include <algorithm>


// #pragma mark - BlacklistedPath


BlacklistedPath::BlacklistedPath()
	:
	fPath(NULL),
	fLength(0),
	fCapacity(0)
{
}


BlacklistedPath::~BlacklistedPath()
{
	free(fPath);
}


bool
BlacklistedPath::SetTo(const char* path)
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
BlacklistedPath::Append(const char* component)
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
BlacklistedPath::_Resize(size_t length, bool keepData)
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


// #pragma mark - PathBlacklist


PathBlacklist::PathBlacklist()
{
}


PathBlacklist::~PathBlacklist()
{
	MakeEmpty();
}


bool
PathBlacklist::Add(const char* path)
{
	BlacklistedPath* blacklistedPath = _FindPath(path);
	if (blacklistedPath != NULL)
		return true;

	blacklistedPath = new(std::nothrow) BlacklistedPath;
	if (blacklistedPath == NULL || !blacklistedPath->SetTo(path)) {
		delete blacklistedPath;
		return false;
	}

	fPaths.Add(blacklistedPath);
	return true;
}


void
PathBlacklist::Remove(const char* path)
{
	BlacklistedPath* blacklistedPath = _FindPath(path);
	if (blacklistedPath != NULL) {
		fPaths.Remove(blacklistedPath);
		delete blacklistedPath;
	}
}


bool
PathBlacklist::Contains(const char* path) const
{
	return _FindPath(path) != NULL;
}


void
PathBlacklist::MakeEmpty()
{
	while (BlacklistedPath* blacklistedPath = fPaths.RemoveHead())
		delete blacklistedPath;
}


BlacklistedPath*
PathBlacklist::_FindPath(const char* path) const
{
	for (PathList::Iterator it = fPaths.GetIterator(); it.HasNext();) {
		BlacklistedPath* blacklistedPath = it.Next();
		if (*blacklistedPath == path)
			return blacklistedPath;
	}

	return NULL;
}
