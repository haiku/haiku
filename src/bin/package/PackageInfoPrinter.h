/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_PRINTER_H
#define PACKAGE_INFO_PRINTER_H


#include <stdio.h>

#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/PackageInfo.h>


using namespace BPackageKit;
using BPackageKit::BHPKG::BGlobalWritableFileInfoData;
using BPackageKit::BHPKG::BPackageInfoAttributeValue;
using BPackageKit::BHPKG::BUserData;
using BPackageKit::BHPKG::BUserSettingsFileInfoData;


class PackageInfoPrinter {
public:
	void PrintPackageInfo(const BPackageInfo& info)
	{
		PrintName(info.Name());
		PrintSummary(info.Summary());
		PrintDescription(info.Description());
		PrintVendor(info.Vendor());
		PrintPackager(info.Packager());

		if (!info.BasePackage().IsEmpty())
			PrintBasePackage(info.BasePackage());

		PrintFlags(info.Flags());
		PrintArchitecture((uint32)info.Architecture());
		PrintVersion(info.Version());

		int32 count = info.CopyrightList().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintCopyright(info.CopyrightList().StringAt(i));

		count = info.LicenseList().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintLicense(info.LicenseList().StringAt(i));

		count = info.URLList().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintUrl(info.URLList().StringAt(i));

		count = info.SourceURLList().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintSourceUrl(info.SourceURLList().StringAt(i));

		count = info.ProvidesList().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintProvides(*info.ProvidesList().ItemAt(i));

		count = info.RequiresList().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintRequires(*info.RequiresList().ItemAt(i));

		count = info.SupplementsList().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintSupplements(*info.SupplementsList().ItemAt(i));

		count = info.ConflictsList().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintConflicts(*info.ConflictsList().ItemAt(i));

		count = info.FreshensList().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintFreshens(*info.FreshensList().ItemAt(i));

		count = info.ReplacesList().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintReplaces(info.ReplacesList().StringAt(i));

		count = info.GlobalWritableFileInfos().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintGlobalWritableFile(*info.GlobalWritableFileInfos().ItemAt(i));

		count = info.UserSettingsFileInfos().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintUserSettingsFile(*info.UserSettingsFileInfos().ItemAt(i));

		count = info.Users().CountItems();
		for (int32 i = 0; i < count; i++)
			PrintUser(*info.Users().ItemAt(i));

		count = info.Groups().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintGroup(info.Groups().StringAt(i));

		count = info.PostInstallScripts().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintPostInstallScript(info.PostInstallScripts().StringAt(i));

		count = info.PreUninstallScripts().CountStrings();
		for (int32 i = 0; i < count; i++)
			PrintPreUninstallScript(info.PreUninstallScripts().StringAt(i));

		if (!info.InstallPath().IsEmpty())
			PrintInstallPath(info.InstallPath());
	}

	bool PrintAttribute(const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				PrintName(value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				PrintSummary(value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				PrintDescription(value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				PrintVendor(value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				PrintPackager(value.string);
				break;

			case B_PACKAGE_INFO_BASE_PACKAGE:
				PrintBasePackage(value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				PrintFlags(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				PrintArchitecture(value.unsignedInt);
				break;

			case B_PACKAGE_INFO_VERSION:
				PrintVersion(value.version);
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				PrintCopyright(value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				PrintLicense(value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				PrintUrl(value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				PrintSourceUrl(value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				PrintProvides(value.resolvable);
				break;

			case B_PACKAGE_INFO_REQUIRES:
				PrintRequires(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				PrintSupplements(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				PrintConflicts(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_FRESHENS:
				PrintFreshens(value.resolvableExpression);
				break;

			case B_PACKAGE_INFO_REPLACES:
				PrintReplaces(value.string);
				break;

			case B_PACKAGE_INFO_GLOBAL_WRITABLE_FILES:
				PrintGlobalWritableFile(value.globalWritableFileInfo);
				break;

			case B_PACKAGE_INFO_USER_SETTINGS_FILES:
				PrintUserSettingsFile(value.userSettingsFileInfo);
				break;

			case B_PACKAGE_INFO_USERS:
				PrintUser(value.user);
				break;

			case B_PACKAGE_INFO_GROUPS:
				PrintGroup(value.string);
				break;

			case B_PACKAGE_INFO_POST_INSTALL_SCRIPTS:
				PrintPostInstallScript(value.string);
				break;

			case B_PACKAGE_INFO_PRE_UNINSTALL_SCRIPTS:
				PrintPreUninstallScript(value.string);
				break;

			case B_PACKAGE_INFO_INSTALL_PATH:
				PrintInstallPath(value.string);
				break;

			case B_PACKAGE_INFO_CHECKSUM:
				PrintChecksum(value.string);
				break;

			default:
				printf("\tunknown or future attribute: "
					"BPackageInfoAttributeID #%d\n", value.attributeID);
				return true;
		}

		return true;
	}

	void PrintName(const char* name) const
	{
		printf("\tname: %s\n", name);
	}

	void PrintSummary(const char* summary) const
	{
		printf("\tsummary: %s\n", summary);
	}

	void PrintDescription(const char* description) const
	{
		printf("\tdescription: %s\n", description);
	}

	void PrintVendor(const char* vendor) const
	{
		printf("\tvendor: %s\n", vendor);
	}

	void PrintPackager(const char* packager) const
	{
		printf("\tpackager: %s\n", packager);
	}

	void PrintBasePackage(const char* basePackage) const
	{
		printf("\tbase package: %s\n", basePackage);
	}

	void PrintFlags(uint32 flags) const
	{
		if (flags == 0)
			return;

		printf("\tflags:\n");
		if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) != 0)
			printf("\t\tapprove_license\n");
		if ((flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0)
			printf("\t\tsystem_package\n");
	}

	void PrintArchitecture(uint32 architecture) const
	{
		printf("\tarchitecture: %s\n",
			architecture < B_PACKAGE_ARCHITECTURE_ENUM_COUNT
				? BPackageInfo::kArchitectureNames[architecture]
				: "<invalid>");
	}

	void PrintVersion(const BPackageVersion& version) const
	{
		printf("\tversion: %s\n", version.ToString().String());
	}

	void PrintCopyright(const char* copyright) const
	{
		printf("\tcopyright: %s\n", copyright);
	}

	void PrintLicense(const char* license) const
	{
		printf("\tlicense: %s\n", license);
	}

	void PrintUrl(const char* url) const
	{
		printf("\tURL: %s\n", url);
	}

	void PrintSourceUrl(const char* sourceUrl) const
	{
		printf("\tsource URL: %s\n", sourceUrl);
	}

	void PrintProvides(const BPackageResolvable& provides) const
	{
		printf("\tprovides: %s", provides.Name().String());
		if (provides.Version().InitCheck() == B_OK)
			printf(" = %s", provides.Version().ToString().String());

		if (provides.CompatibleVersion().InitCheck() == B_OK) {
			printf(" (compatible >= %s)",
				provides.CompatibleVersion().ToString().String());
		}
		printf("\n");
	}

	void PrintRequires(const BPackageResolvableExpression& requires) const
	{
		_PrintResolvableExpression("requires", requires);
	}

	void PrintSupplements(const BPackageResolvableExpression& supplements)
		const
	{
		_PrintResolvableExpression("supplements", supplements);
	}

	void PrintConflicts(const BPackageResolvableExpression& conflicts) const
	{
		_PrintResolvableExpression("conflicts", conflicts);
	}

	void PrintFreshens(const BPackageResolvableExpression& freshens) const
	{
		_PrintResolvableExpression("freshens", freshens);
	}

	void PrintReplaces(const char* replaces) const
	{
		printf("\treplaces: %s\n", replaces);
	}

	void PrintGlobalWritableFile(const BGlobalWritableFileInfo& info) const
	{
		printf("\tglobal writable file: %s", info.Path().String());
		if (info.IsDirectory())
			printf( " directory");
		if (info.UpdateType() < B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT) {
			printf(" %s\n",
				BPackageInfo::kWritableFileUpdateTypes[info.UpdateType()]);
		} else
			printf("\n");
	}

	void PrintUserSettingsFile(const BUserSettingsFileInfo& info) const
	{
		printf("\tuser settings file: %s", info.Path().String());
		if (info.IsDirectory())
			printf( " directory\n");
		else if (!info.TemplatePath().IsEmpty())
			printf(" template %s\n", info.TemplatePath().String());
		else
			printf("\n");
	}

	void PrintUser(const BUser& user) const
	{
		printf("\tuser: %s\n", user.Name().String());
		if (!user.RealName().IsEmpty())
			printf("\t\treal name: %s\n", user.RealName().String());
		if (!user.Home().IsEmpty())
			printf("\t\thome:      %s\n", user.Home().String());
		if (!user.Shell().IsEmpty())
			printf("\t\tshell:     %s\n", user.Shell().String());

		int32 groupCount = user.Groups().CountStrings();
		for (int32 i = 0; i < groupCount; i++)
			printf("\t\tgroup:     %s\n", user.Groups().StringAt(i).String());
	}

	void PrintGroup(const char* group) const
	{
		printf("\tgroup: %s\n", group);
	}

	void PrintPostInstallScript(const char* script) const
	{
		printf("\tpost-install script: %s\n", script);
	}

	void PrintPreUninstallScript(const char* script) const
	{
		printf("\tpre-uninstall script: %s\n", script);
	}

	void PrintInstallPath(const char* path) const
	{
		printf("\tinstall path: %s\n", path);
	}

	void PrintChecksum(const char* checksum) const
	{
		printf("\tchecksum: %s\n", checksum);
	}

private:
	void _PrintResolvableExpression(const char* fieldName,
		const BPackageResolvableExpression& expression) const
	{
		printf("\t%s: %s\n", fieldName, expression.ToString().String());
	}
};


#endif	// PACKAGE_INFO_PRINTER_H
