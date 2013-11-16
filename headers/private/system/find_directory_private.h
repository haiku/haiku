/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_FIND_DIRECTORY_PRIVATE_H
#define _SYSTEM_FIND_DIRECTORY_PRIVATE_H


#include <sys/cdefs.h>

#include <FindDirectory.h>


__BEGIN_DECLS


status_t __find_directory(directory_which which, dev_t device, bool createIt,
	char *returnedPath, int32 pathLength);

status_t __find_path(const void* codePointer, path_base_directory baseDirectory,
	const char* subPath, char* pathBuffer, size_t bufferSize);

status_t __find_path_etc(const void* codePointer, const char* dependency,
	const char* architecture, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize);

status_t __find_path_for_path(const char* path,
	path_base_directory baseDirectory, const char* subPath, char* pathBuffer,
	size_t bufferSize);

status_t __find_path_for_path_etc(const char* path, const char* dependency,
	const char* architecture, path_base_directory baseDirectory,
	const char* subPath, uint32 flags, char* pathBuffer, size_t bufferSize);

status_t __find_paths(path_base_directory baseDirectory, const char* subPath,
	char*** _paths, size_t* _pathCount);

status_t __find_paths_etc(const char* architecture,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char*** _paths, size_t* _pathCount);

const char* __guess_secondary_architecture_from_path(const char* path,
	const char* const* secondaryArchitectures,
	size_t secondaryArchitectureCount);


__END_DECLS


#endif	/* _SYSTEM_FIND_DIRECTORY_PRIVATE_H */
