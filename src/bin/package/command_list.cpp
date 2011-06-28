/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <package/hpkg/PackageContentHandler.h>
#include <package/hpkg/PackageEntry.h>
#include <package/hpkg/PackageEntryAttribute.h>
#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/PackageReader.h>

#include <package/PackageInfo.h>

#include "package.h"
#include "StandardErrorOutput.h"


using namespace BPackageKit::BHPKG;
using namespace BPackageKit;


struct PackageContentListHandler : BPackageContentHandler {
	PackageContentListHandler(bool listAttributes)
		:
		fLevel(0),
		fListAttribute(listAttributes)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
		fLevel++;

		int indentation = (fLevel - 1) * 2;
		printf("%*s", indentation, "");

		// name and size
		printf("%-*s", indentation < 32 ? 32 - indentation : 0, entry->Name());
		printf("  %8llu", (unsigned long long)entry->Data().UncompressedSize());

		// time
		struct tm* time = localtime(&entry->ModifiedTime().tv_sec);
		printf("  %04d-%02d-%02d %02d:%02d:%02d",
			1900 + time->tm_year, time->tm_mon + 1, time->tm_mday,
			time->tm_hour, time->tm_min, time->tm_sec);

		// file type
		mode_t mode = entry->Mode();
		if (S_ISREG(mode))
			printf("  -");
		else if (S_ISDIR(mode))
			printf("  d");
		else if (S_ISLNK(mode))
			printf("  l");
		else
			printf("  ?");

		// permissions
		char buffer[4];
		printf("%s", _PermissionString(buffer, mode >> 6,
			(mode & S_ISUID) != 0));
		printf("%s", _PermissionString(buffer, mode >> 3,
			(mode & S_ISGID) != 0));
		printf("%s", _PermissionString(buffer, mode, false));

		// print the symlink path
		if (S_ISLNK(mode))
			printf("  -> %s", entry->SymlinkPath());

		printf("\n");
		return B_OK;
	}

	virtual status_t HandleEntryAttribute(BPackageEntry* entry,
		BPackageEntryAttribute* attribute)
	{
		if (!fListAttribute)
			return B_OK;

		int indentation = fLevel * 2;
		printf("%*s<", indentation, "");
		printf("%-*s  %8llu", indentation < 31 ? 31 - indentation : 0,
			attribute->Name(),
			(unsigned long long)attribute->Data().UncompressedSize());

		uint32 type = attribute->Type();
		if (isprint(type & 0xff) && isprint((type >> 8) & 0xff)
			 && isprint((type >> 16) & 0xff) && isprint(type >> 24)) {
			printf("  '%c%c%c%c'", int(type >> 24), int((type >> 16) & 0xff),
				int((type >> 8) & 0xff), int(type & 0xff));
		} else
			printf("  %#" B_PRIx32, type);

		printf(">\n");
		return B_OK;
	}

	virtual status_t HandleEntryDone(BPackageEntry* entry)
	{
		fLevel--;
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				printf("package-attributes:\n");
				printf("\tname: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				printf("\tsummary: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				printf("\tdescription: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				printf("\tvendor: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				printf("\tpackager: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				if (value.unsignedInt == 0)
					break;
				printf("\tflags:\n");
				if ((value.unsignedInt & B_PACKAGE_FLAG_APPROVE_LICENSE) != 0)
					printf("\t\tapprove_license\n");
				if ((value.unsignedInt & B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0)
					printf("\t\tsystem_package\n");
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				printf("\tarchitecture: %s\n",
					BPackageInfo::kArchitectureNames[value.unsignedInt]);
				break;

			case B_PACKAGE_INFO_VERSION:
				printf("\tversion: ");
				_PrintPackageVersion(value.version);
				printf("\n");
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				printf("\tcopyright: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				printf("\tlicense: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				printf("\tURL: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				printf("\tsource URL: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				printf("\tprovides: %s", value.resolvable.name);
				if (value.resolvable.haveVersion) {
					printf(" = ");
					_PrintPackageVersion(value.resolvable.version);
				}
				if (value.resolvable.haveCompatibleVersion) {
					printf(" (compatible >= ");
					_PrintPackageVersion(value.resolvable.compatibleVersion);
					printf(")");
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_REQUIRES:
				printf("\trequires: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				printf("\tsupplements: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				printf("\tconflicts: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_FRESHENS:
				printf("\tfreshens: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_REPLACES:
				printf("\treplaces: %s\n", value.string);
				break;

			default:
				printf(
					"*** Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", value.attributeID);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	static const char* _PermissionString(char* buffer, uint32 mode, bool sticky)
	{
		buffer[0] = (mode & 0x4) != 0 ? 'r' : '-';
		buffer[1] = (mode & 0x2) != 0 ? 'w' : '-';

		if ((mode & 0x1) != 0)
			buffer[2] = sticky ? 's' : 'x';
		else
			buffer[2] = '-';

		buffer[3] = '\0';
		return buffer;
	}

	static void _PrintPackageVersion(const BPackageVersionData& version)
	{
		printf("%s", version.major);
		if (version.minor != NULL && version.minor[0] != '\0')
			printf(".%s", version.minor);
		if (version.micro != NULL && version.micro[0] != '\0')
			printf(".%s", version.micro);
		if (version.preRelease != NULL && version.preRelease[0] != '\0')
			printf("-%s", version.preRelease);
		if (version.release > 0)
			printf("-%d", version.release);
	}

private:
	int		fLevel;
	bool	fListAttribute;
};


int
command_list(int argc, const char* const* argv)
{
	bool listAttributes = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+ha", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'a':
				listAttributes = true;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the package file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// open package
	StandardErrorOutput errorOutput;
	BPackageReader packageReader(&errorOutput);
	status_t error = packageReader.Init(packageFileName);
	if (error != B_OK)
		return 1;

	// list
	PackageContentListHandler handler(listAttributes);
	error = packageReader.ParseContent(&handler);
	if (error != B_OK)
		return 1;

	return 0;
}
