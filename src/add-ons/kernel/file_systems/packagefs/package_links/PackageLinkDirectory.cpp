/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinkDirectory.h"

#include <new>

#include <NodeMonitor.h>

#include <AutoDeleter.h>

#include "AutoPackageAttributeDirectoryCookie.h"
#include "DebugSupport.h"
#include "PackageLinksListener.h"
#include "StringConstants.h"
#include "Utils.h"
#include "Version.h"
#include "Volume.h"


PackageLinkDirectory::PackageLinkDirectory()
	:
	Directory(0),
		// the ID needs to be assigned later, when added to a volume
	fSelfLink(NULL),
	fSettingsLink(NULL)
{
	get_real_time(fModifiedTime);
}


PackageLinkDirectory::~PackageLinkDirectory()
{
	if (fSelfLink != NULL)
		fSelfLink->ReleaseReference();
	if (fSettingsLink != NULL)
		fSettingsLink->ReleaseReference();

	while (DependencyLink* link = fDependencyLinks.RemoveHead())
		link->ReleaseReference();
}


status_t
PackageLinkDirectory::Init(Directory* parent, Package* package)
{
	// init the directory/node
	status_t error = Init(parent, package->VersionedName());
	if (error != B_OK)
		RETURN_ERROR(error);

	// add the package
	AddPackage(package, NULL);

	return B_OK;
}


status_t
PackageLinkDirectory::Init(Directory* parent, const String& name)
{
	return Directory::Init(parent, name);
}


timespec
PackageLinkDirectory::ModifiedTime() const
{
	return fModifiedTime;
}


status_t
PackageLinkDirectory::OpenAttributeDirectory(AttributeDirectoryCookie*& _cookie)
{
	Package* package = fPackages.Head();
	if (package == NULL)
		return B_ENTRY_NOT_FOUND;

	AutoPackageAttributeDirectoryCookie* cookie = new(std::nothrow)
		AutoPackageAttributeDirectoryCookie();
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
PackageLinkDirectory::OpenAttribute(const StringKey& name, int openMode,
	AttributeCookie*& _cookie)
{
	Package* package = fPackages.Head();
	if (package == NULL)
		return B_ENTRY_NOT_FOUND;

	return AutoPackageAttributes::OpenCookie(package, name, openMode, _cookie);
}


void
PackageLinkDirectory::AddPackage(Package* package,
	PackageLinksListener* listener)
{
	NodeWriteLocker writeLocker(this);

	// Find the insertion point in the list. We sort by mount type -- the more
	// specific the higher the priority.
	MountType mountType = package->Volume()->MountType();
	Package* otherPackage = NULL;
	for (PackageList::Iterator it = fPackages.GetIterator();
			(otherPackage = it.Next()) != NULL;) {
		if (otherPackage->Volume()->MountType() <= mountType)
			break;
	}

	fPackages.InsertBefore(otherPackage, package);
	package->SetLinkDirectory(this);

	if (package == fPackages.Head())
		_Update(listener);
}


void
PackageLinkDirectory::RemovePackage(Package* package,
	PackageLinksListener* listener)
{
	ASSERT(package->LinkDirectory() == this);

	NodeWriteLocker writeLocker(this);

	bool firstPackage = package == fPackages.Head();

	package->SetLinkDirectory(NULL);
	fPackages.Remove(package);

	if (firstPackage)
		_Update(listener);
}


void
PackageLinkDirectory::UpdatePackageDependencies(Package* package,
	PackageLinksListener* listener)
{
	ASSERT(package->LinkDirectory() == this);

	NodeWriteLocker writeLocker(this);

	// We only need to update, if that head package is affected.
	if (package != fPackages.Head())
		return;

	_UpdateDependencies(listener);
}


status_t
PackageLinkDirectory::_Update(PackageLinksListener* listener)
{
	// Always remove all dependency links -- if there's still a package, they
	// will be re-created below.
	while (DependencyLink* link = fDependencyLinks.RemoveHead())
		_RemoveLink(link, listener);

	// check, if empty
	Package* package = fPackages.Head();
	if (package == NULL) {
		// remove self and settings link
		_RemoveLink(fSelfLink, listener);
		fSelfLink = NULL;

		_RemoveLink(fSettingsLink, listener);
		fSettingsLink = NULL;

		return B_OK;
	}

	// create/update self and settings link
	status_t error = _CreateOrUpdateLink(fSelfLink, package,
		Link::TYPE_INSTALLATION_LOCATION, StringConstants::Get().kSelfLinkName,
		listener);
	if (error != B_OK)
		RETURN_ERROR(error);

	error = _CreateOrUpdateLink(fSettingsLink, package, Link::TYPE_SETTINGS,
		StringConstants::Get().kSettingsLinkName, listener);
	if (error != B_OK)
		RETURN_ERROR(error);

	// update the dependency links
	return _UpdateDependencies(listener);
}


status_t
PackageLinkDirectory::_UpdateDependencies(PackageLinksListener* listener)
{
	Package* package = fPackages.Head();
	if (package == NULL)
		return B_OK;

	// Iterate through the package's dependencies
	for (DependencyList::ConstIterator it
				= package->Dependencies().GetIterator();
			Dependency* dependency = it.Next();) {
		Resolvable* resolvable = dependency->Resolvable();
		Package* resolvablePackage = resolvable != NULL
			? resolvable->Package() : NULL;

		Node* node = FindChild(dependency->FileName());
		if (node != NULL) {
			// link already exists -- update
			DependencyLink* link = static_cast<DependencyLink*>(node);

			NodeWriteLocker linkLocker(link);
			link->Update(resolvablePackage, listener);
		} else {
			// no link for the dependency yet -- create one
			DependencyLink* link = new(std::nothrow) DependencyLink(
				resolvablePackage);
			if (link == NULL)
				return B_NO_MEMORY;

			status_t error = link->Init(this, dependency->FileName());
			if (error != B_OK) {
				delete link;
				RETURN_ERROR(error);
			}

			AddChild(link);
			fDependencyLinks.Add(link);

			if (listener != NULL) {
				NodeWriteLocker linkLocker(link);
				listener->PackageLinkNodeAdded(link);
			}
		}
	}

	return B_OK;
}


void
PackageLinkDirectory::_RemoveLink(Link* link, PackageLinksListener* listener)
{
	if (link != NULL) {
		NodeWriteLocker linkLocker(link);
		if (listener != NULL)
			listener->PackageLinkNodeRemoved(link);

		RemoveChild(link);
		linkLocker.Unlock();
		link->ReleaseReference();
	}
}


status_t
PackageLinkDirectory::_CreateOrUpdateLink(Link*& link, Package* package,
	Link::Type type, const String& name, PackageLinksListener* listener)
{
	if (link == NULL) {
		link = new(std::nothrow) Link(package, type);
		if (link == NULL)
			return B_NO_MEMORY;

		status_t error = link->Init(this, name);
		if (error != B_OK)
			RETURN_ERROR(error);

		AddChild(link);

		if (listener != NULL) {
			NodeWriteLocker lLinkLocker(link);
			listener->PackageLinkNodeAdded(link);
		}
	} else {
		NodeWriteLocker lLinkLocker(link);
		link->Update(package, listener);
	}


	return B_OK;
}
