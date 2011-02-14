/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Entry.h>
#include <Path.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/RepositoryWriter.h>
#include <package/PackageInfo.h>
#include <package/RepositoryInfo.h>

#include "package.h"
#include "StandardErrorOutput.h"


using BPackageKit::BHPKG::BRepositoryWriterListener;
using BPackageKit::BHPKG::BRepositoryWriter;
using namespace BPackageKit;


class RepositoryWriterListener	: public BRepositoryWriterListener {
public:
	RepositoryWriterListener(bool verbose, bool quiet)
		: fVerbose(verbose), fQuiet(quiet)
	{
	}

	virtual void PrintErrorVarArgs(const char* format, va_list args)
	{
		vfprintf(stderr, format, args);
	}

	virtual void OnPackageAdded(const BPackageInfo& packageInfo)
	{
		if (fQuiet)
			return;

		printf("%s (%s)\n", packageInfo.Name().String(),
			packageInfo.Version().ToString().String());
		if (fVerbose) {
			printf("\tsummary:  %s\n", packageInfo.Summary().String());
			printf("\tvendor:   %s\n", packageInfo.Vendor().String());
			printf("\tpackager: %s\n", packageInfo.Packager().String());
			printf("\tchecksum: %s\n", packageInfo.Checksum().String());
			if (uint32 flags = packageInfo.Flags()) {
				printf("\tflags:\n");
				if ((flags & B_PACKAGE_FLAG_APPROVE_LICENSE) != 0)
					printf("\t\tapprove_license\n");
				if ((flags & B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0)
					printf("\t\tsystem_package\n");
			}
		} else
			printf("\tchecksum: %s\n", packageInfo.Checksum().String());
	}

	virtual void OnRepositoryInfoSectionDone(uint32 uncompressedSize)
	{
		if (fQuiet || !fVerbose)
			return;

		printf("----- Repository Info Section --------------------\n");
		printf("repository info size:    %10lu (uncompressed)\n",
			uncompressedSize);
	}

	virtual void OnPackageAttributesSectionDone(uint32 stringCount,
		uint32 uncompressedSize)
	{
		if (fQuiet || !fVerbose)
			return;

		printf("----- Package Attribute Section -------------------\n");
		printf("string count:            %10lu\n", stringCount);
		printf("package attributes size: %10lu (uncompressed)\n",
			uncompressedSize);
	}

	virtual void OnRepositoryDone(uint32 headerSize, uint32 repositoryInfoSize,
		uint32 licenseCount, uint32 packageCount, uint32 packageAttributesSize,
		uint64 totalSize)
	{
		if (fQuiet)
			return;

		printf("----- Package Repository Info -----\n");
		if (fVerbose)
			printf("embedded license count   %10lu\n", licenseCount);
		printf("package count            %10lu\n", packageCount);
		printf("-----------------------------------\n");
		printf("header size:             %10lu\n", headerSize);
		printf("repository header size:  %10lu\n", repositoryInfoSize);
		printf("package attributes size: %10lu\n", packageAttributesSize);
		printf("total size:              %10llu\n", totalSize);
		printf("-----------------------------------\n");
	}

private:
	bool fVerbose;
	bool fQuiet;
};


int
command_create(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;
	bool quiet = false;
	bool verbose = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:hqv", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'q':
				quiet = true;
				break;

			case 'v':
				verbose = true;
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are the repository info file plus one or more
	// package files, i.e. at least two more arguments.
	if (optind + 2 >= argc)
		print_usage_and_exit(true);

	const char* repositoryInfoFileName = argv[optind++];
	const char* const* packageFileNames = argv + optind;

	RepositoryWriterListener listener(verbose, quiet);

	BEntry repositoryInfoEntry(repositoryInfoFileName);
	if (!repositoryInfoEntry.Exists()) {
		listener.PrintError(
			"Error: given repository-info file '%s' doesn't exist!\n",
			repositoryInfoFileName);
		return 1;
	}

	// determine path for 'repo' file from given info file
	BEntry repositoryParentEntry;
	repositoryInfoEntry.GetParent(&repositoryParentEntry);
	BPath repositoryPath;
	if (repositoryParentEntry.GetPath(&repositoryPath) != B_OK) {
		listener.PrintError(
			"Error: can't determine path of given repository-info file!\n");
		return 1;
	}
	repositoryPath.Append("repo");

	// create repository
	BRepositoryInfo repositoryInfo(repositoryInfoEntry);
	status_t result = repositoryInfo.InitCheck();
	if (result != B_OK) {
		listener.PrintError(
			"Error: can't parse given repository-info file : %s\n",
			strerror(result));
		return 1;
	}
	BRepositoryWriter repositoryWriter(&listener, &repositoryInfo);
	if ((result = repositoryWriter.Init(repositoryPath.Path())) != B_OK) {
		listener.PrintError("Error: can't initialize repository-writer : %s\n",
			strerror(result));
		return 1;
	}

	// change directory, if requested
	if (changeToDirectory != NULL) {
		if (chdir(changeToDirectory) != 0) {
			listener.PrintError(
				"Error: Failed to change the current working directory to "
				"\"%s\": %s\n", changeToDirectory, strerror(errno));
			return 1;
		}
	}

	// add all given package files
	for (int i = 0; i < argc - optind; ++i) {
		if (verbose)
			printf("reading package '%s' ...\n", packageFileNames[i]);
		BEntry entry(packageFileNames[i]);
		result = repositoryWriter.AddPackage(entry);
		if (result != B_OK)
			return 1;
	}

	// write the repository
	result = repositoryWriter.Finish();
	if (result != B_OK)
		return 1;

	if (verbose) {
		printf("\nsuccessfully created repository '%s'\n",
			repositoryPath.Path());
	}

	return 0;
}
