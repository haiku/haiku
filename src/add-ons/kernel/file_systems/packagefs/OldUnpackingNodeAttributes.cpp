/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "OldUnpackingNodeAttributes.h"

#include "PackageNode.h"


OldUnpackingNodeAttributes::OldUnpackingNodeAttributes(
	PackageNode* packageNode)
	:
	fPackageNode(packageNode)
{
}


timespec
OldUnpackingNodeAttributes::ModifiedTime() const
{
	if (fPackageNode != NULL)
		return fPackageNode->ModifiedTime();

	timespec time = { 0, 0 };
	return time;
}


off_t
OldUnpackingNodeAttributes::FileSize() const
{
	return fPackageNode != NULL ? fPackageNode->FileSize() : 0;
}


void*
OldUnpackingNodeAttributes::IndexCookieForAttribute(const char* name) const
{
	return fPackageNode != NULL
		? fPackageNode->IndexCookieForAttribute(name) : NULL;
}
