/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageSymlink.h"

#include <stdlib.h>
#include <string.h>


PackageSymlink::PackageSymlink(Package* package, mode_t mode)
	:
	PackageLeafNode(package, mode),
	fSymlinkPath(NULL)
{
}


PackageSymlink::~PackageSymlink()
{
	free(fSymlinkPath);
}


status_t
PackageSymlink::SetSymlinkPath(const char* path)
{
	if (path == NULL)
		return B_OK;

	fSymlinkPath = strdup(path);
	return fSymlinkPath != NULL ? B_OK : B_NO_MEMORY;
}


const char*
PackageSymlink::SymlinkPath() const
{
	return fSymlinkPath;
}
