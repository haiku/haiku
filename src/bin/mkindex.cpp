/*
 * Copyright (c) 2003-2005, Haiku
 *
 * This software is part of the Haiku distribution and is covered
 * by the MIT license.
 *
 * Authors:
 *		Scott Dellinger (dellinsd@myrealbox.com)
 *		Axel Dörfler, axeld@pinc-software.de.
 */

/**	Description: Adds an index to a volume. */


#include <fs_info.h>
#include <fs_index.h>
#include <TypeConstants.h>
#include <Directory.h>
#include <Path.h>
#include <Volume.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


static struct option const kLongOptions[] = {
	{"volume", required_argument, 0, 'd'},
	{"type", required_argument, 0, 't'},
	{"copy-from", required_argument, 0, 'f'},
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *kProgramName = __progname;


static void
copy_indexes(dev_t from, dev_t to, bool verbose)
{
	DIR *indexes = fs_open_index_dir(from);
	
	if (verbose)
		puts("Copying indexes:");

	while (dirent *dirent = fs_read_index_dir(indexes)) {
		if (!strcmp(dirent->d_name, "name")
			|| !strcmp(dirent->d_name, "size")
			|| !strcmp(dirent->d_name, "last_modified"))
			continue;

		index_info info;
		if (fs_stat_index(from, dirent->d_name, &info) != 0) {
			fprintf(stderr, "%s: Skipped index \"%s\": %s\n",
				kProgramName, dirent->d_name, strerror(errno));
			continue;
		}

		if (fs_create_index(to, dirent->d_name, info.type, 0) != 0) {
			if (errno == B_BAD_VALUE || errno == B_FILE_EXISTS) {
				// B_BAD_VALUE is what BeOS returns here...
				continue;
			}
			fprintf(stderr, "%s: Could not create index \"%s\": %s\n",
				kProgramName, dirent->d_name, strerror(errno));
		} else if (verbose)
			printf("\t%s\n", dirent->d_name);
	}

	fs_close_index_dir(indexes);
}


static void
usage(int status)
{
	fprintf(stderr,
		"Usage: %s [options] <attribute>\n"
		"Creates a new index for the specified attribute.\n"
		"\n"
		"  -d, --volume=PATH\ta path on the volume to which the index will be added,\n"
		"\t\t\tdefaults to current volume.\n"
		"  -t, --type=TYPE	the type of the attribute being indexed.  One of \"int\",\n"
		"\t\t\t\"llong\", \"string\", \"float\", or \"double\".\n"
		"\t\t\tDefaults to \"string\".\n"
		"      --copy-from\tpath to volume to copy the indexes from.\n"
		"  -v, --verbose\t\tprint information about the index being created\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char **argv)
{
	const char *indexTypeName = "string";
	int indexType = B_STRING_TYPE;
	char *indexName = NULL;
	bool verbose = false;
	dev_t device = -1, copyFromDevice = -1;

	int c;
	while ((c = getopt_long(argc, argv, "d:ht:v", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'd':
				device = dev_for_path(optarg);
				if (device < 0) {
					fprintf(stderr, "%s: can't get information about volume: %s\n",
						kProgramName, optarg);
					return -1;
				}
				break;
			case 'f':
				copyFromDevice = dev_for_path(optarg);
				if (copyFromDevice < 0) {
					fprintf(stderr, "%s: can't get information about volume: %s\n",
						kProgramName, optarg);
					return -1;
				}
				break;
			case 'h':
				usage(0);
				break;
			case 't':
				indexTypeName = optarg;
				if (strncmp("int", optarg, 3) == 0)
					indexType = B_INT32_TYPE;
				else if (strncmp("llong", optarg, 5) == 0)
					indexType = B_INT64_TYPE;
				else if (strncmp("string", optarg, 6) == 0)
					indexType = B_STRING_TYPE;
				else if (strncmp("float", optarg, 5) == 0)
					indexType = B_FLOAT_TYPE;
				else if (strncmp("double", optarg, 6) == 0)
					indexType = B_DOUBLE_TYPE;
				else
					usage(1);
				break;
			case 'v':
				verbose = 1;
				break;
			default:
				usage(1);
				break;
		}
	}

	if (device == -1) {
		// Create the index on the volume of the current
		// directory if no volume was specified.

		device = dev_for_path(".");
		if (device < 0) {
			fprintf(stderr, "%s: can't get information about current volume\n", kProgramName);
			return -1;
		}
	}

	if (copyFromDevice != -1) {
		copy_indexes(copyFromDevice, device, verbose);
		return 0;
	}

	if (argc - optind == 1) {
		// last argument
		indexName = argv[optind];
	} else
		usage(1);

	if (verbose) {
		/* Get the mount point of the specified volume. */
		BVolume volume(device);
		BDirectory dir;
		volume.GetRootDirectory(&dir);
		BPath path(&dir, NULL);

		printf("Creating index \"%s\" of type %s on volume mounted at %s.\n",
			indexName, indexTypeName, path.Path());
	}

	if (fs_create_index(device, indexName, indexType, 0) != 0)
		fprintf(stderr, "%s: Could not create index: %s\n", kProgramName, strerror(errno));

	return 0;
}
