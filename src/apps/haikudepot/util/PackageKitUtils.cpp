/*
 * Copyright 2022-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageKitUtils.h"


using namespace BPackageKit;


/*static*/ status_t
PackageKitUtils::DeriveLocalFilePath(const PackageInfoRef package, BPath& path)
{
	if (!package.IsSet())
		return B_ERROR;

	PackageLocalInfoRef localInfo = package->LocalInfo();

	if (localInfo.IsSet() && localInfo->IsLocalFile()) {
		path.SetTo(localInfo->LocalFilePath());
		return B_OK;
	}

	path.Unset();
	BPackageInstallationLocation installationLocation = DeriveInstallLocation(package);
	directory_which which;
	status_t result = _DeriveDirectoryWhich(installationLocation, &which);

	if (result == B_OK)
		result = find_directory(which, &path);

	if (result == B_OK)
		path.Append(localInfo->FileName());

	return result;
}


/*static*/ status_t
PackageKitUtils::_DeriveDirectoryWhich(BPackageInstallationLocation location,
	directory_which* which)
{
	switch (location) {
		case B_PACKAGE_INSTALLATION_LOCATION_SYSTEM:
			*which = B_SYSTEM_PACKAGES_DIRECTORY;
			break;
		case B_PACKAGE_INSTALLATION_LOCATION_HOME:
			*which = B_USER_PACKAGES_DIRECTORY;
			break;
		default:
			debugger("illegal state: unsupported package installation location");
			return B_BAD_VALUE;
	}
	return B_OK;
}


/*static*/ BPackageInstallationLocation
PackageKitUtils::DeriveInstallLocation(const PackageInfoRef package)
{
	if (package.IsSet()) {

		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet()) {
			const PackageInstallationLocationSet& locations = localInfo->InstallationLocations();

			// If the package is already installed, return its first installed location
			if (locations.size() != 0)
				return static_cast<BPackageInstallationLocation>(*locations.begin());
		}
	}

	return B_PACKAGE_INSTALLATION_LOCATION_SYSTEM;
}


/*static*/ PackageInfoRef
PackageKitUtils::CreatePackageInfo(const BPackageInfo& info)
{
	PackageCoreInfoBuilder coreInfoBuilder = PackageCoreInfoBuilder()
												 .WithArchitecture(info.ArchitectureName())
												 .WithVersion(new PackageVersion(info.Version()))
												 .WithPublisher(CreatePublisherInfo(info));

	PackageLocalizedTextBuilder localizedTextBuilder = PackageLocalizedTextBuilder()
														   .WithTitle(info.Name())
														   .WithSummary(info.Summary())
														   .WithDescription(info.Description());

	// TODO: Retrieve local file size
	PackageLocalInfoBuilder localInfoBuilder
		= PackageLocalInfoBuilder().WithFlags(info.Flags()).WithFileName(info.FileName());

	return PackageInfoBuilder(info.Name())
		.WithCoreInfo(coreInfoBuilder.BuildRef())
		.WithLocalizedText(localizedTextBuilder.BuildRef())
		.WithLocalInfo(localInfoBuilder.BuildRef())
		.BuildRef();
}


/*static*/ PackagePublisherInfoRef
PackageKitUtils::CreatePublisherInfo(const BPackageInfo& info)
{
	BString publisherURL;
	if (info.URLList().CountStrings() > 0)
		publisherURL = info.URLList().StringAt(0);

	BString publisherName = info.Vendor();
	const BStringList& copyrightList = info.CopyrightList();
	if (!copyrightList.IsEmpty()) {
		publisherName = "";

		for (int32 i = 0; i < copyrightList.CountStrings(); i++) {
			if (!publisherName.IsEmpty())
				publisherName << ", ";
			publisherName << copyrightList.StringAt(i);
		}
	}
	if (!publisherName.IsEmpty())
		publisherName.Prepend("© ");

	PackagePublisherInfoRef result(new PackagePublisherInfo(publisherName, publisherURL), true);
	return result;
}
