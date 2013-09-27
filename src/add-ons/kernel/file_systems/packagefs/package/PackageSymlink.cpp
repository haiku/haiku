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
	fSymlinkPath()
{
}


PackageSymlink::~PackageSymlink()
{
}


void
PackageSymlink::SetSymlinkPath(const String& path)
{
	fSymlinkPath = path;
}


String
PackageSymlink::SymlinkPath() const
{
	return fSymlinkPath;
}
