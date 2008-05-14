/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marco Minutoli, mminutoli@gmail.com
 */

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <String.h>
#include <getopt.h>
#include "FsCreator.h"

extern "C" const char* __progname;
static const char* kProgramName = __progname;

static const char* kUsage = 
"Usage: %s -t <fs> <options> <device> <volume name>\n"
"\n"
"Options:\n"
"  -t, --type       <fs>   - set type of filesystem to create\n\n"
"  -h, --help              - print this help text\n"
"  -o, --options    <opt>  - set fs specific options\n"
"  -v, --verbose           - set verbose output\n"
;


/*
 * Print program help on the right stream
 */
inline void
print_help(bool out)
{
	fprintf(out ? stdout : stderr, kUsage, kProgramName, kProgramName);
}


/*
 * Print program help and exit
 */
inline void
print_help_exit(bool out)
{
	print_help(out);
	exit(out ? 0 : 1);
}


int
main(int argc, char* const* argv)
{
	const struct option longOptions[] = {
		{ "help", 0, NULL, 'h' },
		{ "options", 0, NULL, 'o' },
		{ "type", 1, NULL, 't' },
		{ "verbose", 0, NULL, 'v' },
		{ NULL, 0, NULL, 0 }
	};
	const char *shortOptions = "t:o:hv";

	// parse argument list
	int32 nextOption;
	BString fsType;
	BString fsOptions;
	bool verbose = false;

	nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);
	if (nextOption == 't')
		fsType = optarg;
	else
		print_help_exit(nextOption == 'h' ? true : false);

	do {
		nextOption = getopt_long(argc, argv, shortOptions, longOptions, NULL);

		switch (nextOption) {
			case 't':	// -t or --type again?
				print_help_exit(false);
				break;
			case 'h':	// -h or --help
				print_help_exit(true);
				break;
			case 'v':	// -v or --verbose
				verbose = true;
				break;
			case 'o':	// -o or --options
				fsOptions << optarg;
				break;
			case '?':	// invalid option
				break;
			case -1:	// done with options
				break;
			default:	// everything else
				print_help(false);
				abort();
		}
	} while (nextOption != -1);

	// the device name should be the first non-option element
	// right before the VolumeName
	if (optind != argc - 2)
		print_help_exit(false);
	const char* devPath = argv[optind];
	BString volName = argv[argc-1];

	FsCreator* creator = new FsCreator(devPath, fsType, volName,
		fsOptions.String(), verbose);
	if (creator == NULL) {
		std::cerr << "Error: FsCreator can't be allocated\n";
		abort();
	}

	return creator->Run();
}
