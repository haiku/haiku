/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageNode.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"


PackageNode::PackageNode(Package* package, mode_t mode)
	:
	fPackage(package),
	fParent(NULL),
	fName(NULL),
	fMode(mode),
	fUserID(0),
	fGroupID(0)
{
}


PackageNode::~PackageNode()
{
	free(fName);
}


status_t
PackageNode::Init(PackageDirectory* parent, const char* name)
{
	fParent = parent;
	fName = strdup(name);
	if (fName == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


status_t
PackageNode::VFSInit(dev_t deviceID, ino_t nodeID)
{
	return B_OK;
}


void
PackageNode::VFSUninit()
{
}


off_t
PackageNode::FileSize() const
{
	return 0;
}
