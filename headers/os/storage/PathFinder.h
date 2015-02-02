/*
 * Copyright 2013-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PATH_FINDER_H
#define _PATH_FINDER_H


#include <FindDirectory.h>
#include <StringList.h>


class BPath;
struct entry_ref;


namespace BPackageKit {
	class BPackageResolvableExpression;
}


class BPathFinder {
public:
			typedef BPackageKit::BPackageResolvableExpression
				BResolvableExpression;

public:
								BPathFinder(const void* codePointer = NULL,
									const char* dependency = NULL);
								BPathFinder(const char* path,
									const char* dependency = NULL);
								BPathFinder(const entry_ref& ref,
									const char* dependency = NULL);
								BPathFinder(
									const BResolvableExpression& expression,
									const char* dependency = NULL);
									// requires libpackage

			status_t			SetTo(const void* codePointer = NULL,
									const char* dependency = NULL);
			status_t			SetTo(const char* path,
									const char* dependency = NULL);
			status_t			SetTo(const entry_ref& ref,
									const char* dependency = NULL);
			status_t			SetTo(const BResolvableExpression& expression,
									const char* dependency = NULL);
									// requires libpackage

			status_t			FindPath(const char* architecture,
									path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
									BPath& _path);
			status_t			FindPath(path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
									BPath& _path);
			status_t			FindPath(path_base_directory baseDirectory,
									const char* subPath, BPath& _path);
			status_t			FindPath(path_base_directory baseDirectory,
									BPath& _path);

	static	status_t			FindPaths(const char* architecture,
									path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
							 		BStringList& _paths);
	static	status_t			FindPaths(path_base_directory baseDirectory,
									const char* subPath, uint32 flags,
							 		BStringList& _paths);
	static	status_t			FindPaths(path_base_directory baseDirectory,
									const char* subPath, BStringList& _paths);
	static	status_t			FindPaths(path_base_directory baseDirectory,
									BStringList& _paths);

private:
			status_t			_SetTo(const void* codePointer,
									const char* path, const char* dependency);

private:
			const void*			fCodePointer;
			BString				fPath;
			BString				fDependency;
			status_t			fInitStatus;
			addr_t				fReserved[4];
};


#endif	// _PATH_FINDER_H
