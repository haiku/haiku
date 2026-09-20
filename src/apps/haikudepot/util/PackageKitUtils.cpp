/*
 * Copyright 2022-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageKitUtils.h"

#include <Catalog.h>

#include <package/CommitTransactionResult.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageKitUtils"


using namespace BPackageKit;
using namespace BPackageKit::BPrivate;
using namespace BPackageKit::BManager::BPrivate;


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


/*!	This method will convert the exception into a string that is suitable for use in a human
 *	readable message included as part of a BAlert message.
 */
/*static*/ BString
PackageKitUtils::ExceptionToAlertString(const BFatalErrorException* fatalEx)
{
	BString result = "";

	result << fatalEx->Message();

	if (!fatalEx->Details().IsEmpty()) {
		if (!result.IsEmpty())
			result << '\n';
		BString detailsTemplate = B_TRANSLATE("Details: %Details%");
		detailsTemplate.ReplaceAll("%Details%", fatalEx->Details());
		result << detailsTemplate;
	}

	if (fatalEx->Error() != B_OK) {
		if (!result.IsEmpty())
			result << '\n';
		BString errnoStr;
		errnoStr.SetToFormat("%" B_PRId32, fatalEx->Error());
		BString errorTemplate = B_TRANSLATE("Error: %Errstr% (%Errno%)");
		errorTemplate.ReplaceAll("%Errno%", errnoStr);
		errorTemplate.ReplaceAll("%Errstr%", strerror(fatalEx->Error()));
		result << errorTemplate;
	}

	if (fatalEx->HasCommitTransactionFailed()) {
		if (!result.IsEmpty())
			result << '\n';
		BCommitTransactionResult txnResult = fatalEx->CommitTransactionResult();
		BString txnResultTemplate = B_TRANSLATE("Transaction Result: %TxnResultFullErrorMessage%");
		txnResultTemplate.ReplaceAll("%TxnResultFullErrorMessage%", txnResult.FullErrorMessage());
		result << txnResultTemplate;
	}

	if (result.IsEmpty())
		result << "???";

	return result;
}


/*!	This method will convert the exception into a string that is suitable for use in a human
 *	readable message included as part of a log message.
 */
/*static*/ BString
PackageKitUtils::ExceptionToLogString(const BFatalErrorException* fatalEx)
{
	BString result = "";

	result << fatalEx->Message();

	if (!fatalEx->Details().IsEmpty()) {
		if (!result.IsEmpty())
			result << ", ";
		result << "details: [" << fatalEx->Details() << "]";
	}

	if (fatalEx->Error() != B_OK) {
		if (!result.IsEmpty())
			result << ", ";
		result << "errno: " << fatalEx->Error() << ", errstr: " << strerror(fatalEx->Error());
	}

	if (fatalEx->HasCommitTransactionFailed()) {
		if (!result.IsEmpty())
			result << ", ";
		BCommitTransactionResult txnResult = fatalEx->CommitTransactionResult();
		result << "txn-result: " << txnResult.FullErrorMessage();
	}

	if (result.IsEmpty())
		result << "???";

	return result;
}
