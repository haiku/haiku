/*
 * Copyright 2011-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
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
			return fPackageInfo.AddCopyright(value.string);

		case B_PACKAGE_INFO_LICENSES:
			return fPackageInfo.AddLicense(value.string);

		case B_PACKAGE_INFO_PROVIDES:
			return fPackageInfo.AddProvides(value.resolvable);

		case B_PACKAGE_INFO_REQUIRES:
			return fPackageInfo.AddRequires(value.resolvableExpression);

		case B_PACKAGE_INFO_SUPPLEMENTS:
			return fPackageInfo.AddSupplements(value.resolvableExpression);

		case B_PACKAGE_INFO_CONFLICTS:
			return fPackageInfo.AddConflicts(value.resolvableExpression);

		case B_PACKAGE_INFO_FRESHENS:
			return fPackageInfo.AddFreshens(value.resolvableExpression);

		case B_PACKAGE_INFO_REPLACES:
			return fPackageInfo.AddReplaces(value.string);

		case B_PACKAGE_INFO_URLS:
			return fPackageInfo.AddURL(value.string);

		case B_PACKAGE_INFO_SOURCE_URLS:
			return fPackageInfo.AddSourceURL(value.string);

		case B_PACKAGE_INFO_CHECKSUM:
			fPackageInfo.SetChecksum(value.string);
			break;

		case B_PACKAGE_INFO_INSTALL_PATH:
			fPackageInfo.SetInstallPath(value.string);
			break;

		case B_PACKAGE_INFO_BASE_PACKAGE:
			fPackageInfo.SetBasePackage(value.string);
			break;

		case B_PACKAGE_INFO_GLOBAL_WRITABLE_FILES:
			return fPackageInfo.AddGlobalWritableFileInfo(
				value.globalWritableFileInfo);

		case B_PACKAGE_INFO_USER_SETTINGS_FILES:
			return fPackageInfo.AddUserSettingsFileInfo(value.userSettingsFileInfo);

		case B_PACKAGE_INFO_USERS:
			return fPackageInfo.AddUser(value.user);

		case B_PACKAGE_INFO_GROUPS:
			return fPackageInfo.AddGroup(value.string);

		case B_PACKAGE_INFO_POST_INSTALL_SCRIPTS:
			return fPackageInfo.AddPostInstallScript(value.string);

		case B_PACKAGE_INFO_PRE_UNINSTALL_SCRIPTS:
			fPackageInfo.AddPreUninstallScript(value.string);
			break;

		default:
			if (fErrorOutput != NULL) {
				fErrorOutput->PrintError(
					"Invalid package attribute section: unexpected package "
					"attribute id %d encountered\n", value.attributeID);
			}
			return B_NOT_SUPPORTED; // Could be a future attribute we can skip.
	}

	return B_OK;
}


void
BPackageInfoContentHandler::HandleErrorOccurred()
{
}


}	// namespace BPackageKit
