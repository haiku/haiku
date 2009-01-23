/*
 * Copyright 2003-2006 Haiku Inc.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Scott Dellinger (dellinsd@myrealbox.com)
 *		Jérôme Duval
 */


#include <fs_info.h>
#include <fs_index.h>
#include <TypeConstants.h>

#include <errno.h>
#include <getopt.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern const char *__progname;
const char *kProgramName = __progname;

dev_t gCurrentDevice;

// The following enum and #define are copied from gnu/sys2.h, because it
// didn't want to compile when including that directly.  Since that file
// is marked as being temporary and getting migrated into system.h,
// assume these'll go away soon.

/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */
enum {
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

static struct option const longopts[] = {
	{"volume", required_argument, 0, 'd'},
	{"type", required_argument, 0, 't'},
	{"pattern", no_argument, 0, 'p'},
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, GETOPT_HELP_CHAR},
	{0, 0, 0, 0}
};


void
usage(int status)
{
	fprintf (stderr, 
		"Usage: %s [OPTION]... INDEX_NAME\n"
		"\n"
		"Removes the index named INDEX_NAME from a disk volume.  Once this has been\n"
		"done, it will no longer be possible to use the query system to search for\n"
		"files with the INDEX_NAME attribute.\n"
		"\n"
		"  -d, --volume=PATH	a path on the volume from which the index will be\n"
		"                         removed\n"
		"  -h, --help		display this help and exit\n"
		"  -p, --pattern		INDEX_NAME is a pattern\n"	
		"  -v, --verbose		print information about the index being removed\n"
		"\n"
		"INDEX_NAME is the name of a file attribute.\n"
		"\n"
		"If no volume is specified, the volume of the current directory is assumed.\n",
		kProgramName);

	exit(status);
}


const char* 
lookup_index_type(uint32 device_type)
{
	switch (device_type) {
		case B_DOUBLE_TYPE:
		  return "double";
		case B_FLOAT_TYPE:
		  return "float";
		case B_INT64_TYPE:
		  return "llong";
		case B_INT32_TYPE:
		  return "int";
		case B_STRING_TYPE:
		  return "string";

		default:
		  return "unknown";
	}
}


int
remove_index(dev_t device, const char* indexName, bool verbose)
{
	if (verbose) {
		// Get the index type
		index_info info;
		status_t status = fs_stat_index(device, indexName, &info);
		if (status != B_OK) {
			fprintf(stderr, "%s: Can't get type of index %s: %s\n",
				kProgramName, indexName, strerror(errno));
			return -1;
		}

		fprintf(stdout, "Removing index \"%s\" of type %s.\n",
			indexName, lookup_index_type(info.type));
	}

	if (fs_remove_index(device, indexName) != 0) {
		fprintf(stderr, "%s: Cannot remove index %s: %s\n", kProgramName, indexName, strerror(errno));
		return -1;
	}

	return 0;
}


void *
open_index_dir(const char* /*path*/)
{
	return fs_open_index_dir(gCurrentDevice);
}


int
stat_index(const char* /*index*/, struct stat* stat)
{
	memset(stat, 0, sizeof(struct stat));
	stat->st_mode = S_IFREG;
	return 0;
}


int
remove_indices(dev_t device, const char* indexPattern, bool verbose)
{
	glob_t glob;
	memset(&glob, 0, sizeof(glob_t));

	glob.gl_closedir = (void (*)(void *))fs_close_index_dir;
	glob.gl_readdir = (dirent *(*)(void *))fs_read_index_dir;
	glob.gl_opendir = open_index_dir;
	glob.gl_lstat = stat_index;
	glob.gl_stat = stat_index;

	// for open_attr_dir():
	gCurrentDevice = device;

	int result = ::glob(indexPattern, GLOB_ALTDIRFUNC, NULL, &glob);
	if (result < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}

	bool error = false;

	for (int i = 0; i < glob.gl_pathc; i++) {
		if (remove_index(device, glob.gl_pathv[i], verbose) != 0)
			error = true;
	}

	return error ? -1 : 0;
}


int
main(int argc, char **argv)
{
	bool isPattern = false;
	bool verbose = false;
	dev_t device = 0;
	char *indexName = NULL;
	char *path = NULL;
	
	int c;
	while ((c = getopt_long(argc, argv, "d:ht:pv", longopts, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'd':
				path = optarg;
			  	break;
			case GETOPT_HELP_CHAR:
				usage(0);
				break;
			case 'p':
				isPattern = true;
				break;
			case 'v':
				verbose = true;
				break;
			default:
	  			usage(1);
		  		break;
		}
	}

	// Remove the index from the volume of the current
	// directory if no volume was specified.
	if (path == NULL)
		path = ".";

	device = dev_for_path(path);
	if (device < 0) {
		fprintf(stderr, "%s: can't get information about volume %s\n", kProgramName, path);
		return 1;
	}

	if (argc - optind == 1) {
		// last argument
		indexName = argv[optind];
	} else
		usage(1);

	int result;
	if (isPattern)
		result = remove_indices(device, indexName, verbose);
	else
		result = remove_index(device, indexName, verbose);

	return result == 0 ? 0 : 1;
}
