/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Package.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"


Package::Package(PackageDomain* domain, dev_t deviceID, ino_t nodeID)
	:
	fDomain(domain),
	fName(NULL),
	fNodeID(nodeID),
	fDeviceID(deviceID)
{
}


Package::~Package()
{
	while (PackageNode* node = fNodes.RemoveHead())
		delete node;

	free(fName);
}


status_t
Package::Init(const char* name)
{
	fName = strdup(name);
	if (fName == NULL)
		RETURN_ERROR(B_NO_MEMORY);

	return B_OK;
}


void
Package::AddNode(PackageNode* node)
{
	fNodes.Add(node);
}
