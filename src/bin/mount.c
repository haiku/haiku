/*
 * Copyright 2001-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/**	Mounts a volume with the specified file system */


#include <fs_volume.h>

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void
usage(const char *programName)
{
	
	printf("usage: %s [-ro] [-t fstype] [-p parameter] [device] directory\n"
		"\t-ro\tmounts the volume read-only\n"
		"\t-t\tspecifies the file system to use (defaults to automatic recognition)\n"
		"\t-p\tspecifies parameters to pass to the file system (-o also accepted)\n"
		"\tif device is not specified, NULL is passed (for in-memory filesystems)\n",programName);
	exit(1);
}


int
main(int argc, char **argv)
{
	const char *programName = argv[0];
	const char *device = NULL;
	const char *mountPoint;
	const char *parameter = NULL;
	const char *fs = NULL;
	struct stat mountStat;
	dev_t volume;
	uint32 flags = 0;

	/* prettify the program name */

	if (strrchr(programName, '/'))
		programName = strrchr(programName, '/') + 1;

	/* get all options */

	while (*++argv) {
		char *arg = *argv;
		argc--;
		if (*arg != '-')
			break;

		if (!strcmp(++arg, "ro") && (flags & B_MOUNT_READ_ONLY) == 0)
			flags |= B_MOUNT_READ_ONLY;
		else if (!strcmp(arg, "t") && fs == NULL) {
			if (argc <= 1)
				break;
			fs = *++argv;
			argc--;
		} else if ((!strcmp(arg, "p") || !strcmp(arg, "o")) && parameter == NULL) {
			if (argc <= 1)
				break;
			parameter = *++argv;
			argc--;
		} else
			usage(programName);
	}

	/* check the arguments */

	if (argc > 1) {
		device = argv[0];
		argv++;
		argc--;
	}
	mountPoint = argv[0];

	if (mountPoint == NULL)
		usage(programName);

	if (stat(mountPoint, &mountStat) < 0) {
		fprintf(stderr, "%s: The mount point '%s' is not accessible\n", programName, mountPoint);
		return 1;
	}
	if (!S_ISDIR(mountStat.st_mode)) {
		fprintf(stderr, "%s: The mount point '%s' is not a directory\n", programName, mountPoint);
		return 1;
	}

	/* do the work */

	volume = fs_mount_volume(mountPoint, device, fs, flags, parameter);
	if (volume < B_OK) {
		fprintf(stderr, "%s: %s\n", programName, strerror(volume));
		return 1;
	}
	return 0;
}

