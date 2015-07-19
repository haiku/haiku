/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT license.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Drivers.h>

#include <AutoDeleter.h>


static struct option const kLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{NULL}
};


extern const char *__progname;
static const char *kProgramName = __progname;


void
usage(int returnValue)
{
	fprintf(stderr, "Usage: %s <path-to-mounted-file-system>\n", kProgramName);
	exit(returnValue);
}


int
main(int argc, char** argv)
{
	int c;
	while ((c = getopt_long(argc, argv, "h", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (argc - optind < 1)
		usage(1);
	const char* path = argv[optind++];

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: Could not access path: %s\n", kProgramName,
			strerror(errno));
		return EXIT_FAILURE;
	}

	FileDescriptorCloser closer(fd);

	fs_trim_data trimData;
	trimData.range_count = 1;
	trimData.ranges[0].offset = 0;
	trimData.ranges[0].size = UINT64_MAX;
	trimData.trimmed_size = 0;

	if (ioctl(fd, B_TRIM_DEVICE, &trimData, sizeof(fs_trim_data)) != 0) {
		fprintf(stderr, "%s: Trimming failed: %s\n", kProgramName,
			strerror(errno));
		return EXIT_FAILURE;
	}

	printf("Trimmed %" B_PRIu64 " bytes from device.\n", trimData.trimmed_size);
	return EXIT_SUCCESS;
}
