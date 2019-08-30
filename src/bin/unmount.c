/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/**	Unmounts a volume */


#include <fs_volume.h>

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static void
usage(const char *programName)
{
	
	printf("usage: %s [-f] <path to volume>\n"
		"\t-f\tforces unmounting in case of open files left\n", programName);
	exit(1);
}


int
main(int argc, char **argv)
{
	const char *programName = argv[0];
	struct stat pathStat;
	const char *path;
	status_t status;
	uint32 flags = 0;

	/* prettify the program name */

	if (strrchr(programName, '/'))
		programName = strrchr(programName, '/') + 1;

	/* get all options */

	while (*++argv) {
		char *arg = *argv;
		if (*arg != '-')
			break;

		if (!strcmp(++arg, "f"))
			flags |= B_FORCE_UNMOUNT;
		else
			usage(programName);
	}

	path = argv[0];

	/* check the arguments */

	if (path == NULL)
		usage(programName);

	if (stat(path, &pathStat) < 0) {
		fprintf(stderr, "%s: The path \"%s\" is not accessible\n", programName, path);
		return 1;
	}

	/* do the work */

	status = fs_unmount_volume(path, flags);
	if (status != B_OK) {
		fprintf(stderr, "%s: unmounting failed: %s\n", programName, strerror(status));
		return 1;
	}

	return 0;
}

