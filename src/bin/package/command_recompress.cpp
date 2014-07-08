/*
 * Copyright 2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/PackageWriter.h>

#include "package.h"
#include "PackageWriterListener.h"


using BPackageKit::BHPKG::BPackageReader;
using BPackageKit::BHPKG::BPackageWriter;
using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriterParameters;


int
command_recompress(int argc, const char* const* argv)
{
	bool quiet = false;
	bool verbose = false;
	int32 compressionLevel = BPackageKit::BHPKG::B_HPKG_COMPRESSION_LEVEL_BEST;

	while (true) {
		static struct option sLongOptions[] = {
			{ "help", no_argument, 0, 'h' },
			{ "quiet", no_argument, 0, 'q' },
			{ "verbose", no_argument, 0, 'v' },
			{ 0, 0, 0, 0 }
		};

		opterr = 0; // don't print errors
		int c = getopt_long(argc, (char**)argv, "+0123456789:hqv",
			sLongOptions, NULL);
		if (c == -1)
			break;

		switch (c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				compressionLevel = c - '0';
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

	// The remaining arguments are the input package file and optionally the
	// output package file, i.e. one or two more arguments.
	if (argc - optind < 1 || argc - optind > 2)
		print_usage_and_exit(true);

	const char* inputPackageFileName = argv[optind++];
	const char* outputPackageFileName = optind < argc ? argv[optind++] : NULL;

	// If compression is used, an output file is required.
	if (compressionLevel != 0 && outputPackageFileName == NULL) {
		fprintf(stderr, "Error: Writing to stdout is supported only with "
			"compression level 0.\n");
		return 1;
	}

	// TODO: Implement streaming support!
	if (outputPackageFileName == NULL) {
		fprintf(stderr, "Error: Writing to stdout is not supported yet.\n");
		return 1;
	}

	// open the input package
	PackageWriterListener listener(verbose, quiet);

	BPackageReader packageReader(&listener);
	status_t error = packageReader.Init(inputPackageFileName);
	if (error != B_OK)
		return 1;

	// write the output package
	BPackageWriterParameters writerParameters;
	writerParameters.SetCompressionLevel(compressionLevel);

	BPackageWriter packageWriter(&listener);
	error = packageWriter.Init(outputPackageFileName, &writerParameters);
	if (error != B_OK)
		return 1;

	error = packageWriter.Recompress(&packageReader);
	if (error != B_OK)
		return 1;

	if (verbose)
		printf("\nsuccessfully wrote package '%s'\n", outputPackageFileName);

	return 0;
}
