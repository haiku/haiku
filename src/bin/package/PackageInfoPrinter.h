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

			case B_PACKAGE_INFO_INSTALL_PATH:
				PrintInstallPath(value.string);
				break;

			default:
				return false;
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

	void PrintVersion(const BPackageVersionData& version) const
	{
		printf("\tversion: ");
		_PrintPackageVersion(version);
		printf("\n");
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

	void PrintProvides(const BPackageResolvableData& provides) const
	{
		printf("\tprovides: %s", provides.name);
		if (provides.haveVersion) {
			printf(" = ");
			_PrintPackageVersion(provides.version);
		}
		if (provides.haveCompatibleVersion) {
			printf(" (compatible >= ");
			_PrintPackageVersion(provides.compatibleVersion);
			printf(")");
		}
		printf("\n");
	}

	void PrintRequires(const BPackageResolvableExpressionData& requires) const
	{
		_PrintResolvableExpression("requires", requires);
	}

	void PrintSupplements(const BPackageResolvableExpressionData& supplements)
		const
	{
		_PrintResolvableExpression("supplements", supplements);
	}

	void PrintConflicts(const BPackageResolvableExpressionData& conflicts) const
	{
		_PrintResolvableExpression("conflicts", conflicts);
	}

	void PrintFreshens(const BPackageResolvableExpressionData& freshens) const
	{
		_PrintResolvableExpression("freshens", freshens);
	}

	void PrintReplaces(const char* replaces) const
	{
		printf("\treplaces: %s\n", replaces);
	}

	void PrintGlobalWritableFile(const BGlobalWritableFileInfoData& info) const
	{
		printf("\tglobal writable file: %s", info.path);
		if (info.isDirectory)
			printf( " directory");
		if (info.updateType < B_WRITABLE_FILE_UPDATE_TYPE_ENUM_COUNT) {
			printf(" %s\n",
				BPackageInfo::kWritableFileUpdateTypes[info.updateType]);
		} else
			printf("\n");
	}

	void PrintUserSettingsFile(const BUserSettingsFileInfoData& info) const
	{
		printf("\tuser settings file: %s", info.path);
		if (info.isDirectory)
			printf( " directory\n");
		else if (info.templatePath != NULL)
			printf(" template %s\n", info.templatePath);
		else
			printf("\n");
	}

	void PrintUser(const BUserData& user) const
	{
		printf("\tuser: %s\n", user.name);
		if (user.realName != NULL)
			printf("\t\treal name: %s\n", user.realName);
		if (user.home != NULL)
			printf("\t\thome:      %s\n", user.home);
		if (user.shell != NULL)
			printf("\t\tshell:     %s\n", user.shell);
		for (size_t i = 0; i < user.groupCount; i++)
			printf("\t\tgroup:     %s\n", user.groups[i]);
	}

	void PrintGroup(const char* group) const
	{
		printf("\tgroup: %s\n", group);
	}

	void PrintPostInstallScript(const char* script) const
	{
		printf("\tpost install script: %s\n", script);
	}

	void PrintInstallPath(const char* path) const
	{
		printf("\tinstall path: %s\n", path);
	}

private:
	static void _PrintPackageVersion(const BPackageVersionData& version)
	{
		printf("%s", BPackageVersion(version).ToString().String());
	}

	void _PrintResolvableExpression(const char* fieldName,
		const BPackageResolvableExpressionData& expression) const
	{
		printf("\t%s: %s", fieldName, expression.name);
		if (expression.haveOpAndVersion) {
			printf(" %s ",
				expression.op < B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT
					? BPackageResolvableExpression::kOperatorNames[
						expression.op]
					: "<invalid operator>");
			_PrintPackageVersion(expression.version);
		}
		printf("\n");
	}
};


#endif	// PACKAGE_INFO_PRINTER_H
