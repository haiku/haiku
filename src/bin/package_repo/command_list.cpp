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
#include <package/hpkg/StandardErrorOutput.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>

#include "package_repo.h"
#include "PackageInfoPrinter.h"


using namespace BPackageKit::BHPKG;
using namespace BPackageKit;

struct RepositoryContentListHandler : BRepositoryContentHandler {
	RepositoryContentListHandler(bool verbose)
		:
		fPrinter(),
		fLevel(0),
		fVerbose(verbose)
	{
	}

	virtual status_t HandlePackage(const char* packageName)
	{
		return B_OK;
	}

	virtual status_t HandlePackageAttribute(
		const BPackageInfoAttributeValue& value)
	{
		if (value.attributeID == B_PACKAGE_INFO_NAME) {
			if (fVerbose) {
				printf("package-attributes:\n");
				fPrinter.PrintName(value.string);
			} else
				printf("\t%s\n", value.string);
		} else {
			if (fVerbose && !fPrinter.PrintAttribute(value)) {
				printf("*** Invalid package attribute section: unexpected "
					"package attribute id %d encountered\n", value.attributeID);
				return B_BAD_DATA;
			}
		}

		return B_OK;
	}

	virtual status_t HandlePackageDone(const char* packageName)
	{
		return B_OK;
	}

	virtual status_t HandleRepositoryInfo(const BRepositoryInfo& repositoryInfo)
	{
		printf("repository-info:\n");
		printf("\tname: %s\n", repositoryInfo.Name().String());
		printf("\tsummary: %s\n", repositoryInfo.Summary().String());
		printf("\tbase-url: %s\n", repositoryInfo.BaseURL().String());
		printf("\tidentifier: %s\n", repositoryInfo.Identifier().String());
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
		printf("packages:\n");

		return B_OK;
	}

	virtual void HandleErrorOccurred()
	{
	}

private:
	static void _PrintPackageVersion(const BPackageVersionData& version)
	{
		printf("%s", BPackageVersion(version).ToString().String());
	}

private:
	PackageInfoPrinter	fPrinter;
	int					fLevel;
	bool				fVerbose;
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
	BStandardErrorOutput errorOutput;
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
