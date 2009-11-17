/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "File.h"

#include "PackageFile.h"


File::File(ino_t id)
	:
	Node(id)
{
	fMode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
}


File::~File()
{
}


status_t
File::Init(Directory* parent, const char* name)
{
	return Node::Init(parent, name);
}


status_t
File::AddPackageNode(PackageNode* packageNode)
{
	if (!S_ISREG(packageNode->Mode()))
		return B_BAD_VALUE;

// TODO:...

	return B_OK;
}
