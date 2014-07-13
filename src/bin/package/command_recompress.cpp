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

#include <File.h>

#include <package/hpkg/HPKGDefs.h>
#include <package/hpkg/PackageReader.h>
#include <package/hpkg/PackageWriter.h>

#include <DataPositionIOWrapper.h>
#include <FdIO.h>

#include "package.h"
#include "PackageWriterListener.h"


using BPackageKit::BHPKG::BPackageReader;
using BPackageKit::BHPKG::BPackageWriter;
using BPackageKit::BHPKG::BPackageWriterListener;
using BPackageKit::BHPKG::BPackageWriterParameters;


static BPositionIO*
create_stdio(bool isInput)
{
	BFdIO* dataIO = new BFdIO(isInput ? 0 : 1, false);
	return new BDataPositionIOWrapper(dataIO);
}


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

	// The remaining arguments are the input package file and the output
	// package file, i.e. two more arguments.
	if (argc - optind != 2)
		print_usage_and_exit(true);

	const char* inputPackageFileName = argv[optind++];
	const char* outputPackageFileName = argv[optind++];

	// open the input package
	status_t error = B_OK;
	BPositionIO* inputFile;
	if (strcmp(inputPackageFileName, "-") == 0) {
		inputFile = create_stdio(true);
	} else {
		BFile* inputFileFile = new BFile;
		error = inputFileFile->SetTo(inputPackageFileName, O_RDONLY);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open input file \"%s\": %s\n",
				inputPackageFileName, strerror(error));
			return 1;
		}
		inputFile = inputFileFile;
	}

	// write the output package
	BPackageWriterParameters writerParameters;
	writerParameters.SetCompressionLevel(compressionLevel);
	if (compressionLevel == 0) {
		writerParameters.SetCompression(
			BPackageKit::BHPKG::B_HPKG_COMPRESSION_NONE);
	}

	PackageWriterListener listener(verbose, quiet);
	BPackageWriter packageWriter(&listener);
	if (strcmp(outputPackageFileName, "-") == 0) {
		if (compressionLevel != 0) {
			fprintf(stderr, "Error: Writing to stdout is supported only with "
				"compression level 0.\n");
			return 1;
		}

		error = packageWriter.Init(create_stdio(false), true,
			&writerParameters);
	} else
		error = packageWriter.Init(outputPackageFileName, &writerParameters);
	if (error != B_OK)
		return 1;

	error = packageWriter.Recompress(inputFile);
	if (error != B_OK)
		return 1;

	if (verbose)
		printf("\nsuccessfully wrote package '%s'\n", outputPackageFileName);

	return 0;
}
