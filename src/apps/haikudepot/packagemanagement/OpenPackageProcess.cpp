/*
 * Copyright 2013-2021, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 *		Julian Harnath <julian.harnath@rwth-aachen.de>
 *		Andrew Lindesay <apl@lindesay.co.nz>
 *
 * Note that this file has been re-factored from `PackageManager.cpp` and
 * authors have been carried across in 2021.
 */


#include "OpenPackageProcess.h"

#include <Catalog.h>
#include <FindDirectory.h>
#include <Roster.h>

#include <package/PackageDefs.h>
#include <package/hpkg/NoErrorOutput.h>
#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>

#include "Logger.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OpenPackageProcess"

using namespace BPackageKit;

using BPackageKit::BHPKG::BNoErrorOutput;
using BPackageKit::BHPKG::BPackageContentHandler;
using BPackageKit::BHPKG::BPackageEntry;
using BPackageKit::BHPKG::BPackageEntryAttribute;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BPackageReader;

// #pragma mark - DeskbarLinkFinder


class DeskbarLinkFinder : public BPackageContentHandler {
public:
	DeskbarLinkFinder(std::vector<DeskbarLink>& foundLinks)
		:
		fDeskbarLinks(foundLinks)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		BString path = MakePath(entry);
		if (path.FindFirst("data/deskbar/menu") == 0
				&& entry->SymlinkPath() != NULL) {
			HDINFO("found deskbar entry: %s -> %s",
				path.String(), entry->SymlinkPath());
			fDeskbarLinks.push_back(DeskbarLink(path, entry->SymlinkPath()));
		}
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

	BString MakePath(const BPackageEntry* entry)
	{
		BString path;
		while (entry != NULL) {
			if (!path.IsEmpty())
				path.Prepend('/', 1);
			path.Prepend(entry->Name());
			entry = entry->Parent();
		}
		return path;
	}

private:
	std::vector<DeskbarLink>&	fDeskbarLinks;
};


// #pragma mark - OpenPackageProcess


OpenPackageProcess::OpenPackageProcess(PackageInfoRef package, Model* model,
		const DeskbarLink& link)
	:
	AbstractPackageProcess(package, model),
	fDeskbarLink(link)
{
	fDescription = _DeriveDescription();
}


OpenPackageProcess::~OpenPackageProcess()
{
}


const char*
OpenPackageProcess::Name() const
{
	return "OpenPackageProcess";
}


const char*
OpenPackageProcess::Description() const
{
	return fDescription;
}


status_t
OpenPackageProcess::RunInternal()
{
	status_t status;
	BPath path;
	if (fDeskbarLink.Link().FindFirst('/') == 0) {
		status = path.SetTo(fDeskbarLink.Link());
		HDINFO("trying to launch (absolute link): %s", path.Path());
	} else {
		int32 location = InstallLocation();
		if (location == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
			status = find_directory(B_SYSTEM_DIRECTORY, &path);
			if (status != B_OK)
				return status;
		} else if (location == B_PACKAGE_INSTALLATION_LOCATION_HOME) {
			status = find_directory(B_USER_DIRECTORY, &path);
			if (status != B_OK)
				return status;
		} else {
			return B_ERROR;
		}

		status = path.Append(fDeskbarLink.Path());
		if (status == B_OK)
			status = path.GetParent(&path);
		if (status == B_OK) {
			status = path.Append(fDeskbarLink.Link(), true);
			HDINFO("trying to launch: %s", path.Path());
		}
	}

	entry_ref ref;
	if (status == B_OK)
		status = get_ref_for_path(path.Path(), &ref);

	if (status == B_OK)
		status = be_roster->Launch(&ref);

	return status;
}


BString
OpenPackageProcess::_DeriveDescription()
{
	BString target = fDeskbarLink.Link();
	int32 lastPathSeparator = target.FindLast('/');
	if (lastPathSeparator > 0 && lastPathSeparator + 1 < target.Length())
		target.Remove(0, lastPathSeparator + 1);

	BString result = B_TRANSLATE("Opening \"%DeskbarLink%\"");
	result.ReplaceAll("%DeskbarLink%", target);
	return result;
}


/*static*/ bool
OpenPackageProcess::FindAppToLaunch(const PackageInfoRef& package,
		std::vector<DeskbarLink>& foundLinks)
{
	if (!package.IsSet())
		return false;

	int32 installLocation = AbstractPackageProcess::InstallLocation(package);

	BPath packagePath;
	if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_SYSTEM) {
		if (find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &packagePath)
			!= B_OK) {
			return false;
		}
	} else if (installLocation == B_PACKAGE_INSTALLATION_LOCATION_HOME) {
		if (find_directory(B_USER_PACKAGES_DIRECTORY, &packagePath)
			!= B_OK) {
			return false;
		}
	} else {
		HDINFO("OpenPackageAction::FindAppToLaunch(): "
			"unknown install location");
		return false;
	}

	packagePath.Append(package->FileName());

	BNoErrorOutput errorOutput;
	BPackageReader reader(&errorOutput);

	status_t status = reader.Init(packagePath.Path());
	if (status != B_OK) {
		HDINFO("OpenPackageAction::FindAppToLaunch(): "
			"failed to init BPackageReader(%s): %s",
			packagePath.Path(), strerror(status));
		return false;
	}

	// Scan package contents for Deskbar links
	DeskbarLinkFinder contentHandler(foundLinks);
	status = reader.ParseContent(&contentHandler);
	if (status != B_OK) {
		HDINFO("OpenPackageAction::FindAppToLaunch(): "
			"failed parse package contents (%s): %s",
			packagePath.Path(), strerror(status));
		return false;
	}

	return !foundLinks.empty();
}
