/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDirectory.h"


PackageDirectory::PackageDirectory(Package* package, mode_t mode)
	:
	PackageNode(package, mode)
{
}


PackageDirectory::~PackageDirectory()
{
	while (PackageNode* child = fChildren.RemoveHead())
		child->ReleaseReference();
}


void
PackageDirectory::AddChild(PackageNode* node)
{
	fChildren.Add(node);
	node->AcquireReference();
}


void
PackageDirectory::RemoveChild(PackageNode* node)
{
	fChildren.Remove(node);
	node->ReleaseReference();
}
