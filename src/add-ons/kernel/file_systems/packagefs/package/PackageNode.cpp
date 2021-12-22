/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageNode.h"

#include <stdlib.h>
#include <string.h>

#include "DebugSupport.h"
#include "Package.h"
#include "Utils.h"


PackageNode::PackageNode(Package* package, mode_t mode)
	:
	fPackage(package),
	fParent(NULL),
	fName(),
	fMode(mode),
	fUserID(0),
	fGroupID(0)
{
}


PackageNode::~PackageNode()
{
	while (PackageNodeAttribute* attribute = fAttributes.RemoveHead())
		delete attribute;
}


BReference<Package>
PackageNode::GetPackage() const
{
	return fPackage.GetReference();
}


status_t
PackageNode::Init(PackageDirectory* parent, const String& name)
{
	fParent = parent;
	fName = name;
	return B_OK;
}


status_t
PackageNode::VFSInit(dev_t deviceID, ino_t nodeID)
{
	BReference<Package> package(GetPackage());

	// open the package
	int fd = package->Open();
	if (fd < 0)
		RETURN_ERROR(fd);

	package->AcquireReference();
	return B_OK;
}


void
PackageNode::VFSUninit()
{
	BReference<Package> package(GetPackage());
	package->Close();
	package->ReleaseReference();
}


off_t
PackageNode::FileSize() const
{
	return 0;
}


void
PackageNode::AddAttribute(PackageNodeAttribute* attribute)
{
	fAttributes.Add(attribute);
}


void
PackageNode::RemoveAttribute(PackageNodeAttribute* attribute)
{
	fAttributes.Remove(attribute);
}


PackageNodeAttribute*
PackageNode::FindAttribute(const StringKey& name) const
{
	for (PackageNodeAttributeList::ConstIterator it = fAttributes.GetIterator();
			PackageNodeAttribute* attribute = it.Next();) {
		if (name == attribute->Name())
			return attribute;
	}

	return NULL;
}


void
PackageNode::UnsetIndexCookie(void* attributeCookie)
{
	((PackageNodeAttribute*)attributeCookie)->SetIndexCookie(NULL);
}


bool
PackageNode::HasPrecedenceOver(const PackageNode* other) const
{
	uint32 packageFlags = 0, otherPackageFlags = 0;
	BReference<Package> package(GetPackage()), otherPackage(other->GetPackage());
	if (package)
		packageFlags = package->Flags();
	if (otherPackage)
		otherPackageFlags = otherPackage->Flags();

	const bool isSystemPkg = (packageFlags
			& BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0,
		otherIsSystemPkg = (otherPackageFlags
			& BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0;
	if (isSystemPkg && !otherIsSystemPkg)
		return true;
	if (!isSystemPkg && otherIsSystemPkg)
		return false;
	return fModifiedTime > other->fModifiedTime;
}
