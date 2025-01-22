/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageUtils.h"

#include "Logger.h"

/*!	This method will obtain the title from the package if this is possible or
	otherwise it will return the name of the package.
*/

/*static*/ void
PackageUtils::TitleOrName(const PackageInfoRef& package, BString& title)
{
	PackageUtils::Title(package, title);
	if (title.IsEmpty() && package.IsSet())
		title.SetTo(package->Name());
}


/*static*/ void
PackageUtils::Title(const PackageInfoRef& package, BString& title)
{
	if (package.IsSet()) {
		PackageLocalizedTextRef localizedText = package->LocalizedText();

		if (localizedText.IsSet())
			title.SetTo(localizedText->Title());
		else
			title.SetTo("");
	} else {
		title.SetTo("");
	}
}


/*static*/ void
PackageUtils::Summary(const PackageInfoRef& package, BString& summary)
{
	if (package.IsSet()) {
		PackageLocalizedTextRef localizedText = package->LocalizedText();

		if (localizedText.IsSet())
			summary.SetTo(localizedText->Summary());
		else
			summary.SetTo("");
	} else {
		summary.SetTo("");
	}
}


/*static*/ const BString
PackageUtils::DepotName(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageCoreInfoRef coreInfo = package->CoreInfo();

		if (coreInfo.IsSet())
			return coreInfo->DepotName();
	}

	return BString();
}


/*static*/ PackageVersionRef
PackageUtils::Version(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageCoreInfoRef coreInfo = package->CoreInfo();

		if (coreInfo.IsSet())
			return coreInfo->Version();
	}

	return PackageVersionRef();
}


/*static*/ const BString
PackageUtils::Architecture(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageCoreInfoRef coreInfo = package->CoreInfo();

		if (coreInfo.IsSet())
			return coreInfo->Architecture();
	}

	return "";
}


/*static*/ const BString
PackageUtils::PublisherName(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageCoreInfoRef coreInfo = package->CoreInfo();

		if (coreInfo.IsSet()) {
			PackagePublisherInfoRef publisherInfo = coreInfo->Publisher();

			if (publisherInfo.IsSet())
				return publisherInfo->Name();
		}
	}

	return "";
}


/*static*/ bool
PackageUtils::IsProminent(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageClassificationInfoRef classificationInfo = package->PackageClassificationInfo();

		if (classificationInfo.IsSet())
			return classificationInfo->IsProminent();
	}

	return false;
}


/*static*/ bool
PackageUtils::IsNativeDesktop(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageClassificationInfoRef classificationInfo = package->PackageClassificationInfo();

		if (classificationInfo.IsSet())
			return classificationInfo->IsNativeDesktop();
	}

	return false;
}


/*static*/ PackageState
PackageUtils::State(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->State();
	}

	return NONE;
}


/*static*/ off_t
PackageUtils::Size(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->Size();
	}

	return 0;
}


/*static*/ bool
PackageUtils::Viewed(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->Viewed();
	}

	return false;
}


/*static*/ bool
PackageUtils::IsActivatedOrLocalFile(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->IsLocalFile() || localInfo->State() == ACTIVATED;
	}

	return false;
}


/*static*/ float
PackageUtils::DownloadProgress(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->DownloadProgress();
	}

	return 0.0f;
}


/*static*/ int32
PackageUtils::Flags(const PackageInfoRef& package)
{
	if (package.IsSet()) {
		PackageLocalInfoRef localInfo = package->LocalInfo();

		if (localInfo.IsSet())
			return localInfo->Flags();
	}

	return false;
}


/*static*/ const char*
PackageUtils::StateToString(PackageState state)
{
	switch (state) {
		case NONE:
			return "NONE";
		case INSTALLED:
			return "INSTALLED";
		case DOWNLOADING:
			return "DOWNLOADING";
		case ACTIVATED:
			return "ACTIVATED";
		case UNINSTALLED:
			return "UNINSTALLED";
		case PENDING:
			return "PENDING";
		default:
			debugger("unknown package state");
			return "???";
	}
}
