/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "package.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "package.h"
#include "PackageWriter.h"


int
command_create(int argc, const char* const* argv)
{
	const char* changeToDirectory = NULL;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+C:h", sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'C':
				changeToDirectory = optarg;
				break;

			case 'h':
				print_usage_and_exit(false);
				break;

			default:
				print_usage_and_exit(true);
				break;
		}
	}

	// The remaining arguments are the package file and the list of files to
	// include, i.e. at least two more arguments.
	if (optind + 2 > argc)
		print_usage_and_exit(true);

	const char* packageFileName = argv[optind++];
	const char* const* fileNames = argv + optind;
	int fileNameCount = argc - optind;

	// create package
	PackageWriter packageWriter;
	status_t error = packageWriter.Init(packageFileName);
printf("Init(): %s\n", strerror(error));
	if (error != B_OK)
		return 1;

	// change directory, if requested
	if (changeToDirectory != NULL) {
		if (chdir(changeToDirectory) != 0) {
			fprintf(stderr, "Error: Failed to change the current working "
				"directory to \"%s\": %s\n", changeToDirectory,
				strerror(errno));
		}
	}

	// add files
	for (int i = 0; i < fileNameCount; i++) {
		error = packageWriter.AddEntry(fileNames[i]);
printf("AddEntry(\"%s\"): %s\n", fileNames[i], strerror(error));
		if (error != B_OK)
			return 1;
	}

	// write the package
	error = packageWriter.Finish();
printf("Finish(): %s\n", strerror(error));
	if (error != B_OK)
		return 1;

	return 0;
}
