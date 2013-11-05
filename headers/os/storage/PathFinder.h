/*
 * Copyright 2013, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_FINDER_H
#define _PATH_FINDER_H


#include <FindDirectory.h>
#include <String.h>


class BPath;
class BStringList;
struct entry_ref;


class BPathFinder {
public:
								BPathFinder(const void* codePointer = NULL,
									const char* dependency = NULL);
								BPathFinder(const char* path,
									const char* dependency = NULL);
								BPathFinder(const entry_ref& ref,
									const char* dependency = NULL);

			status_t			SetTo(const void* codePointer = NULL,
									const char* dependency = NULL);
			status_t			SetTo(const char* path,
									const char* dependency = NULL);
			status_t			SetTo(const entry_ref& ref,
									const char* dependency = NULL);

			status_t			FindPath(path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
									BPath& path);
			status_t			FindPath(path_base_directory baseDirectory,
									const char* subPath, BPath& path);
			status_t			FindPath(path_base_directory baseDirectory,
									BPath& path);

	static	status_t			FindPaths(path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
							 		BStringList& paths);
	static	status_t			FindPaths(path_base_directory baseDirectory,
									const char* subPath, BStringList& paths);
	static	status_t			FindPaths(path_base_directory baseDirectory,
									BStringList& paths);

private:
			status_t			_SetTo(const void* codePointer,
									const char* path, const char* dependency);

private:
			const void*			fCodePointer;
			BString				fPath;
			BString				fDependency;
			status_t			fInitStatus;
			uint32				fReserved[4];
};


#endif	// _PATH_FINDER_H
