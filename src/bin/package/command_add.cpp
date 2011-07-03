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


using namespace BPackageKit::BHPKG;


int
command_add(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;
	const char* packageInfoFileName = NULL;
	bool quiet = false;
	bool verbose = false;
	bool force = false;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:fhi:qv", sLongOptions,
			NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			case 'f':
				force = true;
				break;

			case 'i':
				packageInfoFileName = optarg;
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

	// The remaining arguments are the package file and the entries to add.
	if (optind >= argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];

	// entries must be specified, if a .PackageInfo hasn't been specified via
	// an option
	if (optind >= argc && packageInfoFileName == NULL)
		print_usage_and_exit(true);

	const char* const* entriesToAdd = argv + optind;
	int entriesToAddCount = argc - optind;

	// create package
	PackageWriterListener listener(verbose, quiet);
	BPackageWriter packageWriter(&listener);
	status_t result = packageWriter.Init(packageFileName,
		B_HPKG_WRITER_UPDATE_PACKAGE | (force ? B_HPKG_WRITER_FORCE_ADD : 0));
	if (result != B_OK)
		return 1;

	// If a package info file has been specified explicitly, open it.
	int packageInfoFD = -1;
	if (packageInfoFileName != NULL) {
		packageInfoFD = open(packageInfoFileName, O_RDONLY);
		if (packageInfoFD < 0) {
			fprintf(stderr, "Error: Failed to open package info file \"%s\": "
				"%s\n", packageInfoFileName, strerror(errno));
		}
	}

	// change directory, if requested
	if (changeToDirectory != NULL) {
		if (chdir(changeToDirectory) != 0) {
			listener.PrintError(
				"Error: Failed to change the current working directory to "
				"\"%s\": %s\n", changeToDirectory, strerror(errno));
		}
	}

	// add the entries
	for (int i = 0; i < entriesToAddCount; i++) {
		const char* entry = entriesToAdd[i];

		if (strcmp(entry, ".") == 0) {
			// add all entries in the current directory; skip .PackageInfo,
			// if a different file was specified
			if (add_current_directory_entries(packageWriter,
					listener, packageInfoFileName != NULL) != B_OK)
				return 1;
		} else {
			// skip .PackageInfo, if a different file was specified
			if (packageInfoFileName != NULL
				&& strcmp(entry, B_HPKG_PACKAGE_INFO_FILE_NAME) == 0) {
				continue;
			}

			if (packageWriter.AddEntry(entry) != B_OK)
				return 1;
		}
	}

	// add the .PackageInfo, if explicitly specified
	if (packageInfoFileName != NULL) {
		result = packageWriter.AddEntry(B_HPKG_PACKAGE_INFO_FILE_NAME,
			packageInfoFD);
		if (result != B_OK)
			return 1;
	}

	// write the package
	result = packageWriter.Finish();
	if (result != B_OK)
		return 1;

	if (verbose)
		printf("\nsuccessfully created package '%s'\n", packageFileName);

	return 0;
}
