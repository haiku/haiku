/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageInfoContentHandler.h>

#include <package/PackageInfo.h>
#include <package/hpkg/ErrorOutput.h>
#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


using namespace BHPKG;


BPackageInfoContentHandler::BPackageInfoContentHandler(
	BPackageInfo& packageInfo, BErrorOutput* errorOutput)
	:
	fPackageInfo(packageInfo),
	fErrorOutput(errorOutput)
{
}


BPackageInfoContentHandler::~BPackageInfoContentHandler()
{
}


status_t
BPackageInfoContentHandler::HandleEntry(BPackageEntry* entry)
{
	return B_OK;
}


status_t
BPackageInfoContentHandler::HandleEntryAttribute(BPackageEntry* entry,
	BPackageEntryAttribute* attribute)
{
	return B_OK;
}


status_t
BPackageInfoContentHandler::HandleEntryDone(BPackageEntry* entry)
{
	return B_OK;
}


status_t
BPackageInfoContentHandler::HandlePackageAttribute(
	const BPackageInfoAttributeValue& value)
{
	switch (value.attributeID) {
		case B_PACKAGE_INFO_NAME:
			fPackageInfo.SetName(value.string);
			break;

		case B_PACKAGE_INFO_SUMMARY:
			fPackageInfo.SetSummary(value.string);
			break;

		case B_PACKAGE_INFO_DESCRIPTION:
			fPackageInfo.SetDescription(value.string);
			break;

		case B_PACKAGE_INFO_VENDOR:
			fPackageInfo.SetVendor(value.string);
			break;

		case B_PACKAGE_INFO_PACKAGER:
			fPackageInfo.SetPackager(value.string);
			break;

		case B_PACKAGE_INFO_FLAGS:
			fPackageInfo.SetFlags(value.unsignedInt);
			break;

		case B_PACKAGE_INFO_ARCHITECTURE:
			fPackageInfo.SetArchitecture(
				(BPackageArchitecture)value.unsignedInt);
			break;

		case B_PACKAGE_INFO_VERSION:
			fPackageInfo.SetVersion(value.version);
			break;

		case B_PACKAGE_INFO_COPYRIGHTS:
			fPackageInfo.AddCopyright(value.string);
			break;

		case B_PACKAGE_INFO_LICENSES:
			fPackageInfo.AddLicense(value.string);
			break;

		case B_PACKAGE_INFO_PROVIDES:
			fPackageInfo.AddProvides(value.resolvable);
			break;

		case B_PACKAGE_INFO_REQUIRES:
			fPackageInfo.AddRequires(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_SUPPLEMENTS:
			fPackageInfo.AddSupplements(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_CONFLICTS:
			fPackageInfo.AddConflicts(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_FRESHENS:
			fPackageInfo.AddFreshens(value.resolvableExpression);
			break;

		case B_PACKAGE_INFO_REPLACES:
			fPackageInfo.AddReplaces(value.string);
			break;

		case B_PACKAGE_INFO_URLS:
			fPackageInfo.AddURL(value.string);
			break;

		case B_PACKAGE_INFO_SOURCE_URLS:
			fPackageInfo.AddSourceURL(value.string);
			break;

		case B_PACKAGE_INFO_CHECKSUM:
			fPackageInfo.SetChecksum(value.string);
			break;

		case B_PACKAGE_INFO_INSTALL_PATH:
			fPackageInfo.SetInstallPath(value.string);
			break;

		default:
			if (fErrorOutput != NULL) {
				fErrorOutput->PrintError(
					"Invalid package attribute section: unexpected package "
					"attribute id %d encountered\n", value.attributeID);
			}
			return B_BAD_DATA;
	}

	return B_OK;
}


void
BPackageInfoContentHandler::HandleErrorOccurred()
{
}


}	// namespace BPackageKit
