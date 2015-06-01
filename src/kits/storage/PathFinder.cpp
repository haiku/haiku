/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <PathFinder.h>

#include <AutoDeleter.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StringList.h>


// NOTE: The package kit specific part of BPathFinder (BResolvableExpression
// constructor and SetTo()) is implemented in the package kit.


BPathFinder::BPathFinder(const void* codePointer, const char* dependency)
{
	_SetTo(codePointer, NULL, dependency);
}


BPathFinder::BPathFinder(const char* path, const char* dependency)
{
	_SetTo(NULL, path, dependency);
}


BPathFinder::BPathFinder(const entry_ref& ref, const char* dependency)
{
	SetTo(ref, dependency);
}


status_t
BPathFinder::SetTo(const void* codePointer, const char* dependency)
{
	return _SetTo(codePointer, NULL, dependency);
}


status_t
BPathFinder::SetTo(const char* path, const char* dependency)
{
	return _SetTo(NULL, path, dependency);
}


status_t
BPathFinder::SetTo(const entry_ref& ref, const char* dependency)
{
	BPath path;
	fInitStatus = path.SetTo(&ref);
	if (fInitStatus != B_OK)
		return fInitStatus;

	return _SetTo(NULL, path.Path(), dependency);
}


status_t
BPathFinder::FindPath(const char* architecture,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	BPath& _path)
{
	_path.Unset();

	if (fInitStatus != B_OK)
		return fInitStatus;

	const char* dependency = fDependency.IsEmpty()
		? NULL : fDependency.String();

	char pathBuffer[B_PATH_NAME_LENGTH];
	status_t error;

	if (!fPath.IsEmpty()) {
		error = find_path_for_path_etc(fPath, dependency, architecture,
			baseDirectory, subPath, flags, pathBuffer, sizeof(pathBuffer));
	} else {
		error = find_path_etc(fCodePointer, dependency, architecture,
			baseDirectory, subPath, flags, pathBuffer, sizeof(pathBuffer));
	}

	if (error != B_OK)
		return error;

	return _path.SetTo(pathBuffer);
}


status_t
BPathFinder::FindPath(path_base_directory baseDirectory, const char* subPath,
	uint32 flags, BPath& _path)
{
	return FindPath(NULL, baseDirectory, subPath, flags, _path);
}


status_t
BPathFinder::FindPath(path_base_directory baseDirectory, const char* subPath,
	BPath& _path)
{
	return FindPath(NULL, baseDirectory, subPath, 0, _path);
}


status_t
BPathFinder::FindPath(path_base_directory baseDirectory, BPath& _path)
{
	return FindPath(NULL, baseDirectory, NULL, 0, _path);
}


/*static*/ status_t
BPathFinder::FindPaths(const char* architecture,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	BStringList& _paths)
{
	_paths.MakeEmpty();

	// get the paths
	char** pathArray;
	size_t pathCount;
	status_t error = find_paths_etc(architecture, baseDirectory, subPath, flags,
		&pathArray, &pathCount);
	if (error != B_OK)
		return error;

	MemoryDeleter pathArrayDeleter(pathArray);

	// add them to BStringList
	for (size_t i = 0; i < pathCount; i++) {
		BString path(pathArray[i]);
		if (path.IsEmpty() || !_paths.Add(path)) {
			_paths.MakeEmpty();
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


/*static*/ status_t
BPathFinder::FindPaths(path_base_directory baseDirectory, const char* subPath,
	uint32 flags, BStringList& _paths)
{
	return FindPaths(NULL, baseDirectory, subPath, flags, _paths);
}


/*static*/ status_t
BPathFinder::FindPaths(path_base_directory baseDirectory, const char* subPath,
	BStringList& _paths)
{
	return FindPaths(NULL, baseDirectory, subPath, 0, _paths);
}


/*static*/ status_t
BPathFinder::FindPaths(path_base_directory baseDirectory, BStringList& _paths)
{
	return FindPaths(NULL, baseDirectory, NULL, 0, _paths);
}


status_t
BPathFinder::_SetTo(const void* codePointer, const char* path,
	const char* dependency)
{
	fCodePointer = codePointer;
	fPath = path;
	fDependency = dependency;

	if ((path != NULL && path[0] != '\0' && fPath.IsEmpty())
		|| (dependency != NULL && dependency[0] != '\0'
			&& fDependency.IsEmpty())) {
		fInitStatus = B_NO_MEMORY;
	} else
		fInitStatus = B_OK;

	return fInitStatus;
}
