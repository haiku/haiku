/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageDirectory.h"
#include "Package.h"

#include "ClassCache.h"


CLASS_CACHE(PackageDirectory);


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


bool
PackageDirectory::HasPrecedenceOver(const PackageDirectory* other) const
{
	// If one of us has the SYSTEM_PACKAGE flag and the other doesn't,
	// let PackageNode take care of the comparison.
	uint32 packageFlags = 0, otherPackageFlags = 0;
	BReference<Package> package(GetPackage()), otherPackage(other->GetPackage());
	if (package)
		packageFlags = package->Flags();
	if (otherPackage)
		otherPackageFlags = otherPackage->Flags();
	if ((packageFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE)
			!= (otherPackageFlags
				& BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE)) {
		return PackageNode::HasPrecedenceOver(other);
	}

	const int32 attrs = fAttributes.Count(),
		otherAttrs = other->fAttributes.Count();
	if (attrs != otherAttrs)
		return attrs > otherAttrs;
	return PackageNode::HasPrecedenceOver(other);
}
