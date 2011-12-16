/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Entry.h>

#include <package/PackageInfo.h>
#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageWriter.h>

#include "package.h"
#include "PackageWriterListener.h"
#include "PackageWritingUtils.h"
#include "StandardErrorOutput.h"


using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriter;


int
command_create(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;
	const char* packageInfoFileName = NULL;
	const char* installPath = NULL;
	bool isBuildPackage = false;
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
		int c = getopt_long(argc, (char**)argv, "+bC:hi:I:qv", sLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'b':
				isBuildPackage = true;
				break;

			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'i':
				packageInfoFileName = optarg;
				break;

			case 'I':
				installPath = optarg;
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

	// The remaining arguments is the package file, i.e. one more argument.
	if (optind + 1 != argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// -I is only allowed when -b is given
	if (installPath != NULL && !isBuildPackage) {
		fprintf(stderr, "Error: \"-I\" is only allowed when \"-b\" is "
			"given.\n");
		return 1;
	}

	// create package
	PackageWriterListener listener(verbose, quiet);
	BPackageWriter packageWriter(&listener);
	status_t result = packageWriter.Init(packageFileName);
	if (result != B_OK)
		return 1;

	// If a package info file has been specified explicitly, open it.
	int packageInfoFD = -1;
	if (packageInfoFileName != NULL) {
		packageInfoFD = open(packageInfoFileName, O_RDONLY);
		if (packageInfoFD < 0) {
			fprintf(stderr, "Error: Failed to open package info file \"%s\": "
				"%s\n", packageInfoFileName, strerror(errno));
			return 1;
		}
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

	if (isBuildPackage)
		packageWriter.SetCheckLicenses(false);

	// set install path, if specified
	if (installPath != NULL) {
		result = packageWriter.SetInstallPath(installPath);
		if (result != B_OK) {
			fprintf(stderr, "Error: Failed to set the package install path: "
				"%s\n", strerror(result));
			return 1;
		}
	}

	// add all files of the current directory, save for the .PackageInfo
	if (!isBuildPackage) {
		if (add_current_directory_entries(packageWriter, listener, true)
				!= B_OK) {
			return 1;
		}
	}

	// add the .PackageInfo
	result = packageWriter.AddEntry(B_HPKG_PACKAGE_INFO_FILE_NAME,
		packageInfoFD);
	if (result != B_OK)
		return 1;

	// write the package
	result = packageWriter.Finish();
	if (result != B_OK)
		return 1;

	if (verbose)
		printf("\nsuccessfully created package '%s'\n", packageFileName);

	return 0;
}
