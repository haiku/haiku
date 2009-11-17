/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDirectory.h"


PackageDirectory::PackageDirectory()
{
}


PackageDirectory::~PackageDirectory()
{
	while (PackageNode* child = fChildren.RemoveHead())
		delete child;
}


void
PackageDirectory::AddChild(PackageNode* node)
{
	fChildren.Add(node);
}


void
PackageDirectory::RemoveChild(PackageNode* node)
{
	fChildren.Remove(node);
}
