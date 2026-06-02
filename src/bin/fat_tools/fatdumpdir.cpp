/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <fs_info.h>
#include <syscalls.h>

#include <AutoDeleter.h>

#define _KERNEL
extern "C"
{
#include "sys/namei.h"
#include "sys/param.h"

#include "fs/msdosfs/bootsect.h"
#include "fs/msdosfs/bpb.h"
#include "fs/msdosfs/direntry.h"
#include "fs/msdosfs/fat.h"
#include "fs/msdosfs/msdosfsmount.h"
}

#include "dosfs.h"


/*!	Prints a list of FAT directory entries to the syslog.
	@param argv[1] Path to a directory on a FAT volume.
*/
int
main(int argc, char** argv)
{
	if (argc < 2 || argc > 3 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s [path]\n", argv[0]);
		return 0;
	}

	const char* path = argv[1];

	struct stat st;
	status_t status = stat(path, &st);
	if (status != B_OK) {
		fprintf(stderr, "Can't stat %s.\n", path);
		return 1;
	}

	fs_info info;
	status = _kern_read_fs_info(st.st_dev, &info);
	if (strcmp(info.fsh_name, "fat") != 0) {
		fprintf(stderr, "%s is not on a FAT volume.\n", path);
		return 1;
	}

	// open the directory file
	FileDescriptorCloser fdc(open(path, O_RDONLY | O_DIRECTORY));
	if (!fdc.IsSet()) {
		fprintf(stderr, "Can't open %s (%s).\n", path, strerror(fdc.Get()));
		return 1;
	}

	size_t printed;
	status = ioctl(fdc.Get(), FAT_IOCTL_DUMP_DIR, &printed, sizeof(printed));
	if (status != B_OK) {
		fprintf(stderr, "ioctl failure (%s).\n", strerror(status));
		return 1;
	}

	printf("Printed %" B_PRIuSIZE " entries to the syslog.\n", printed);

	return B_OK;
}
