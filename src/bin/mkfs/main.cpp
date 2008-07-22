/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marco Minutoli, mminutoli@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <DiskSystem.h>

#include "FsCreator.h"


extern "C" const char* __progname;
static const char* kProgramName = __progname;


/*!	Print program help on the right stream */
static void
print_help(bool out)
{
	fprintf(out ? stdout : stderr,
		"Usage: %s <options> <device> <volume name>\n"
		"\n"
		"Options:\n"
		"  -t, --type       <fs>   - set type of file system to create\n\n"
		"  -l, --list-types        - list file systems that support initializing\n"
		"  -h, --help              - print this help text\n"
		"  -o, --options    <opt>  - set fs specific options\n"
		"  -q, --dont-ask          - do not ask before initializing\n"
		"  -v, --verbose           - set verbose output\n",
		kProgramName);
}


/*!	Print program help and exit */
static void
print_help_exit(bool out)
{
	print_help(out);
	exit(out ? 0 : 1);
}


static void
list_types()
{
	const char* kFormat = "%-10s  %-25s  %s\n";
	BDiskDeviceRoster roster;
	BDiskSystem diskSystem;

	printf("Installed file systems that support initializing:\n\n");
	printf(kFormat, "Name", "Pretty Name", "Module");
	printf(kFormat, "--", "--", "--");

	while (roster.GetNextDiskSystem(&diskSystem) == B_OK) {
		if (!diskSystem.SupportsInitializing()
			|| !diskSystem.IsFileSystem())
			continue;

		printf(kFormat, diskSystem.ShortName(), diskSystem.PrettyName(),
			diskSystem.Name());
	}
}


int
main(int argc, char* const* argv)
{
	const struct option kLongOptions[] = {
		{ "help", 0, NULL, 'h' },
		{ "options", 0, NULL, 'o' },
		{ "type", 1, NULL, 't' },
		{ "list-types", 0, NULL, 'l' },
		{ "verbose", 0, NULL, 'v' },
		{ "dont-ask", 0, NULL, 'q' },
		{ NULL, 0, NULL, 0 }
	};
	const char* kShortOptions = "t:o:lhvq";

	// parse argument list
	const char* fsType = "bfs";
	const char* fsOptions = NULL;
	bool verbose = false;
	bool quick = false;

	while (true) {
		int nextOption = getopt_long(argc, argv, kShortOptions, kLongOptions,
			NULL);
		if (nextOption == -1)
			break;

		switch (nextOption) {
			case 't':	// -t or --type
				fsType = optarg;
				break;
			case 'h':	// -h or --help
				print_help_exit(true);
				break;
			case 'v':	// -v or --verbose
				verbose = true;
				break;
			case 'o':	// -o or --options
				fsOptions = optarg;
				break;
			case 'q':	// -q or --quick
				quick = true;
				break;
			case 'l':	// list types
				list_types();
				return 0;
			case '?':	// invalid option
				break;
			case -1:	// done with options
				break;
			default:	// everything else
				print_help(false);
				abort();
		}
	}

	// the device name should be the first non-option element
	// right before the volume name
	if (optind > argc - 1)
		print_help_exit(false);

	const char* device = argv[optind];
	const char* volumeName = NULL;
	if (optind == argc - 2)
		volumeName = argv[argc - 1];
	else {
		if (!strncmp(device, "/dev", 4))
			volumeName = "Unnamed";
		else
			volumeName = "Unnamed Image";
	}

	FsCreator creator(device, fsType, volumeName, fsOptions, quick, verbose);
	return creator.Run() ? 0 : 1;
}
