/*
**	Copyright (c) 2001-2002, OpenBeOS
**
**	This software is part of the OpenBeOS distribution and is covered 
**	by the OpenBeOS license.
**
**	File:        mount.c
**	Author:      Axel Dörfler (axeld@users.sf.net)
**	Description: mounts a volume with the specified file system
*/


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>


static void
usage(const char *programName)
{
	
	printf("usage: %s [-ro] [-t fstype] device directory\n"
		"\t-ro\tmounts the volume read-only\n"
		"\t-t\tspecifies the file system to use (defaults to \"bfs\")\n",programName);
	exit(0);
}


int
main(int argc, char **argv)
{
	const char *programName = argv[0];
	const char *device, *mountPoint;
	const char *parameter = NULL;
	const char *fs = NULL;
	struct stat mountStat;
	int flags = 0;

	/* prettify the program name */

	if (strrchr(programName, '/'))
		programName = strrchr(programName, '/') + 1;

	/* get all options */

	while (*++argv) {
		char *arg = *argv;
		if (*arg != '-')
			break;

		if (!strcmp(++arg, "ro") && (flags & B_MOUNT_READ_ONLY) == 0)
			flags |= B_MOUNT_READ_ONLY;
		else if (!strcmp(arg, "t") && fs == NULL)
			fs = *++argv;
		else if (!strcmp(arg, "p") && parameter == NULL)
			parameter = *++argv;
		else
			usage(programName);
	}

	/* check the arguments */

	if (fs == NULL)
		fs = "bfs";

	device = argv[0];
	mountPoint = argv[1];

	if (device == NULL || mountPoint == NULL)
		usage(programName);

	if (stat(mountPoint, &mountStat) < 0) {
		fprintf(stderr, "%s: The mount point '%s' is not accessible\n", programName, mountPoint);
		return -1;
	}
	if (!S_ISDIR(mountStat.st_mode)) {
		fprintf(stderr, "%s: The mount point '%s' is not a directory\n", programName, mountPoint);
		return -1;
	}

	/* do the work */

	if (mount(fs, mountPoint, device, flags, (void *)parameter, parameter ? strlen(parameter) : 0) < 0) {
		fprintf(stderr, "%s: %s\n", programName, strerror(errno));
		return -1;
	}
	return 0;
}

