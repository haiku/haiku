/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Symlink.h"

#include "PackageSymlink.h"


Symlink::Symlink(ino_t id)
	:
	Node(id)
{
	fMode = S_IFLNK | S_IRUSR | S_IRGRP | S_IROTH;
}


Symlink::~Symlink()
{
}


status_t
Symlink::Init(Directory* parent, const char* name)
{
	return Node::Init(parent, name);
}


status_t
Symlink::AddPackageNode(PackageNode* packageNode)
{
	if (!S_ISLNK(packageNode->Mode()))
		return B_BAD_VALUE;

// TODO:...

	return B_OK;
}
