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

status_t __find_path(const void* codePointer, const char* dependency,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize);

status_t __find_path_for_path(const char* path, const char* dependency,
	path_base_directory baseDirectory, const char* subPath, uint32 flags,
	char* pathBuffer, size_t bufferSize);

status_t __find_paths(path_base_directory baseDirectory, const char* subPath,
	uint32 flags, char*** _paths, size_t* _pathCount);


__END_DECLS


#endif	/* _SYSTEM_FIND_DIRECTORY_PRIVATE_H */
