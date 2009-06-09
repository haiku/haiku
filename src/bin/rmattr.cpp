/*
 * Copyright 2001-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*! Remove an attribute from a file */

#include <fs_attr.h>

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern const char *__progname;
const char *kProgramName = __progname;

int gCurrentFile;


void
usage()
{
	printf("usage: %s [-P] [-p] attr filename1 [filename2...]\n"
		"\t'attr' is the name of an attribute of the file\n"
		"\t-P : Don't resolve links\n"
		"\tIf '-p' is specified, 'attr' is regarded as a pattern.\n", kProgramName);

	exit(1);	
}


void *
open_attr_dir(const char* /*path*/)
{
	return fs_fopen_attr_dir(gCurrentFile);
}


int
stat_attr(const char* /*attribute*/, struct stat* stat)
{
	memset(stat, 0, sizeof(struct stat));
	stat->st_mode = S_IFREG;
	return 0;
}


int
remove_attribute(int fd, const char* attribute, bool isPattern)
{
	if (!isPattern)
		return fs_remove_attr(fd, attribute);

	glob_t glob;
	memset(&glob, 0, sizeof(glob_t));

	glob.gl_closedir = (void (*)(void *))fs_close_attr_dir;
	glob.gl_readdir = (dirent* (*)(void*))fs_read_attr_dir;
	glob.gl_opendir = open_attr_dir;
	glob.gl_lstat = stat_attr;
	glob.gl_stat = stat_attr;

	// for open_attr_dir():
	gCurrentFile = fd;

	int result = ::glob(attribute, GLOB_ALTDIRFUNC, NULL, &glob);
	if (result < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}

	bool error = false;

	for (int i = 0; i < glob.gl_pathc; i++) {
		if (fs_remove_attr(fd, glob.gl_pathv[i]) != 0)
			error = true;
	}

	return error ? -1 : 0;
}


int
main(int argc, const char **argv)
{
	bool isPattern = false;
	bool resolveLinks = true;

	/* get all options */

	while (*++argv) {
		const char *arg = *argv;
		argc--;
		if (*arg != '-')
			break;
		if (!strcmp(arg, "-P"))
			resolveLinks = false;
		else if (!strcmp(arg, "-p"))
			isPattern = true;
	}

	// Make sure we have the minimum number of arguments.
	if (argc < 2)
		usage();

	for (int32 i = 1; i < argc; i++) {
		int fd = open(argv[i], O_RDONLY | (resolveLinks ? 0 : O_NOTRAVERSE));
		if (fd < 0) {
			fprintf(stderr, "%s: can\'t open file %s to remove attribute: %s\n", 
				kProgramName, argv[i], strerror(errno));
			return 1;
		}

		if (remove_attribute(fd, argv[0], isPattern) != B_OK) {
			fprintf(stderr, "%s: error removing attribute %s from %s: %s\n", 
				kProgramName, argv[0], argv[i], strerror(errno));
		}

		close(fd);
	}

	return 0;
}
