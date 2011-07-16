/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/hpkg/RepositoryContentHandler.h>
#include <package/hpkg/RepositoryReader.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>

#include "package.h"
#include "StandardErrorOutput.h"


using namespace BPackageKit::BHPKG;
using namespace BPackageKit;

struct RepositoryContentListHandler : BRepositoryContentHandler {
	RepositoryContentListHandler(bool verbose)
		:
		fLevel(0),
		fVerbose(verbose)
	{
	}

	virtual status_t HandleEntry(BPackageEntry* entry)
	{
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
		switch (value.attributeID) {
			case B_PACKAGE_INFO_NAME:
				if (fVerbose) {
					printf("package-attributes:\n");
					printf("\tname: %s\n", value.string);
				} else
					printf("package: %s", value.string);
				break;

			case B_PACKAGE_INFO_SUMMARY:
				if (fVerbose)
					printf("\tsummary: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_DESCRIPTION:
				if (fVerbose)
					printf("\tdescription: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_VENDOR:
				if (fVerbose)
					printf("\tvendor: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_PACKAGER:
				if (fVerbose)
					printf("\tpackager: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_FLAGS:
				if (value.unsignedInt == 0 || !fVerbose)
					break;
				printf("\tflags:\n");
				if ((value.unsignedInt & B_PACKAGE_FLAG_APPROVE_LICENSE) != 0)
					printf("\t\tapprove_license\n");
				if ((value.unsignedInt & B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0)
					printf("\t\tsystem_package\n");
				break;

			case B_PACKAGE_INFO_ARCHITECTURE:
				if (fVerbose) {
					printf("\tarchitecture: %s\n",
						BPackageInfo::kArchitectureNames[value.unsignedInt]);
				}
				break;

			case B_PACKAGE_INFO_VERSION:
				if (!fVerbose)
					printf("(");
				_PrintPackageVersion(value.version);
				if (!fVerbose)
					printf(")\n");
				break;

			case B_PACKAGE_INFO_COPYRIGHTS:
				if (fVerbose)
					printf("\tcopyright: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_LICENSES:
				if (fVerbose)
					printf("\tlicense: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_URLS:
				if (fVerbose)
					printf("\tURL: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_SOURCE_URLS:
				if (fVerbose)
					printf("\tsource URL: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_PROVIDES:
				if (!fVerbose)
					break;
				printf("\tprovides: %s", value.resolvable.name);
				if (value.resolvable.haveVersion) {
					printf(" = ");
					_PrintPackageVersion(value.resolvable.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_REQUIRES:
				if (!fVerbose)
					break;
				printf("\trequires: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_SUPPLEMENTS:
				if (!fVerbose)
					break;
				printf("\tsupplements: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_CONFLICTS:
				if (!fVerbose)
					break;
				printf("\tconflicts: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_FRESHENS:
				if (!fVerbose)
					break;
				printf("\tfreshens: %s", value.resolvableExpression.name);
				if (value.resolvableExpression.haveOpAndVersion) {
					printf(" %s ", BPackageResolvableExpression::kOperatorNames[
							value.resolvableExpression.op]);
					_PrintPackageVersion(value.resolvableExpression.version);
				}
				printf("\n");
				break;

			case B_PACKAGE_INFO_REPLACES:
				if (!fVerbose)
					break;
				printf("\treplaces: %s\n", value.string);
				break;

			case B_PACKAGE_INFO_CHECKSUM:
				printf("\tchecksum: %s\n", value.string);
				break;

			default:
				printf(
					"*** Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", value.attributeID);
				return B_BAD_DATA;
		}

		return B_OK;
	}

	virtual status_t HandleRepositoryInfo(const BRepositoryInfo& repositoryInfo)
	{
		printf("repository-info:\n");
		printf("\tname: %s\n", repositoryInfo.Name().String());
		printf("\tsummary: %s\n", repositoryInfo.Summary().String());
		printf("\turl: %s\n", repositoryInfo.OriginalBaseURL().String());
		printf("\tvendor: %s\n", repositoryInfo.Vendor().String());
		printf("\tpriority: %u\n", repositoryInfo.Priority());
		printf("\tarchitecture: %s\n",
			BPackageInfo::kArchitectureNames[repositoryInfo.Architecture()]);
		const BStringList licenseNames = repositoryInfo.LicenseNames();
		if (!licenseNames.IsEmpty()) {
			printf("\tlicenses:\n");
			for (int i = 0; i < licenseNames.CountStrings(); ++i)
				printf("\t\t%s\n", licenseNames.StringAt(i).String());
		}

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
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
	bool	fVerbose;
};


int
command_list(int argc, const char* const* argv)
{
	bool verbose = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+hv", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'h':
				print_usage_and_exit(false);
				break;

			case 'v':
				verbose = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// One argument should remain -- the repository file name.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* repositoryFileName = argv[optind++];

	// open repository
	StandardErrorOutput errorOutput;
	BRepositoryReader repositoryReader(&errorOutput);
	status_t error = repositoryReader.Init(repositoryFileName);
	if (error != B_OK)
		return 1;

	// list
	RepositoryContentListHandler handler(verbose);
	error = repositoryReader.ParseContent(&handler);
	if (error != B_OK)
		return 1;

	return 0;
}
