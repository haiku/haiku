/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinksDirectory.h"

#include <AutoDeleter.h>

#include "DebugSupport.h"
#include "PackageLinkDirectory.h"
#include "PackageLinksListener.h"
#include "Utils.h"


PackageLinksDirectory::PackageLinksDirectory()
	:
	Directory(0),
	fListener(NULL)
		// the ID needs to be assigned later, when added to a volume
{
	get_real_time(fModifiedTime);
}


PackageLinksDirectory::~PackageLinksDirectory()
{
}


timespec
PackageLinksDirectory::ModifiedTime() const
{
	return fModifiedTime;
}


status_t
PackageLinksDirectory::AddPackage(Package* package)
{
	// Create a package link directory -- there might already be one, but since
	// that's unlikely, we don't bother to check and recheck later.
	PackageLinkDirectory* linkDirectory
		= new(std::nothrow) PackageLinkDirectory;
	if (linkDirectory == NULL)
		return B_NO_MEMORY;
	BReference<PackageLinkDirectory> linkDirectoryReference(linkDirectory,
		true);

	status_t error = linkDirectory->Init(this, package);
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the link directory
	NodeWriteLocker writeLocker(this);
	if (Node* child = FindChild(linkDirectory->Name())) {
		// There already is an entry with the name.
		PackageLinkDirectory* otherLinkDirectory
			= dynamic_cast<PackageLinkDirectory*>(child);
		if (otherLinkDirectory == NULL)
			RETURN_ERROR(B_BAD_VALUE);

		// There's already a package link directory. Delete the one we created
		// and add the package to the pre-existing one.
		linkDirectory->RemovePackage(package, NULL);
		linkDirectory = otherLinkDirectory;
		linkDirectory->AddPackage(package, fListener);
	} else {
		// No entry is in the way, so just add the link directory.
		AddChild(linkDirectory);

		if (fListener != NULL)
			linkDirectory->NotifyDirectoryAdded(fListener);
	}

	return B_OK;
}


void
PackageLinksDirectory::RemovePackage(Package* package)
{
	NodeWriteLocker writeLocker(this);

	// get the package's link directory and remove the package from it
	PackageLinkDirectory* linkDirectory = package->LinkDirectory();
	if (linkDirectory == NULL)
		return;

	BReference<PackageLinkDirectory> linkDirectoryReference(linkDirectory);

	linkDirectory->RemovePackage(package, fListener);

	// if empty, remove the link directory itself
	if (linkDirectory->IsEmpty()) {
		if (fListener != NULL) {
			NodeWriteLocker linkDirectoryWriteLocker(linkDirectory);
			fListener->PackageLinkNodeRemoved(linkDirectory);
		}

		RemoveChild(linkDirectory);
	}
}


void
PackageLinksDirectory::UpdatePackageDependencies(Package* package)
{
	NodeWriteLocker writeLocker(this);

	PackageLinkDirectory* linkDirectory = package->LinkDirectory();
	if (linkDirectory == NULL)
		return;

	linkDirectory->UpdatePackageDependencies(package, fListener);
}

