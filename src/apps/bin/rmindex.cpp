//	Copyright (c) 2003, OpenBeOS
//
//	This software is part of the OpenBeOS distribution and is covered
//	by the OpenBeOS license.
//
//	File:        rmindex.cpp
//	Author:      Scott Dellinger (dellinsd@myrealbox.com)
//	Description: Removes an index from a volume.
//
//	Changelog:
//	  2003-03-09  initial version
//

// std C includes
#include <stdio.h>
#include <string.h>
#include <errno.h>

// BeOS C includes
#include <fs_info.h>
#include <fs_index.h>
#include <TypeConstants.h>
#include <Directory.h>
#include <Path.h>
#include <Volume.h>

#include <getopt.h>

// The following enum and #define are copied from gnu/sys2.h, because it
// didn't want to compile when including that directly.  Since that file
// is marked as being temporary and getting migrated into system.h,
// assume these'll go away soon.

/* These enum values cannot possibly conflict with the option values
   ordinarily used by commands, including CHAR_MAX + 1, etc.  Avoid
   CHAR_MIN - 1, as it may equal -1, the getopt end-of-options value.  */
enum
{
  GETOPT_HELP_CHAR = (CHAR_MIN - 2),
  GETOPT_VERSION_CHAR = (CHAR_MIN - 3)
};

#define GETOPT_HELP_OPTION_DECL \
  "help", no_argument, 0, GETOPT_HELP_CHAR

static struct option const longopts[] =
{
	{"volume", required_argument, 0, 'd'},
	{"type", required_argument, 0, 't'},
	{"verbose", no_argument, 0, 'v'},
	{GETOPT_HELP_OPTION_DECL},
	{0, 0, 0, 0}
};

void usage(int);
const char* lookup_index_type(uint32);

int main (int32 argc, char **argv)
{
	int c;
	int verbose = 0;
	dev_t vol_device = 0;
	char *index_name = NULL;
	int retval;
	char *error_message;
	
	while ((c = getopt_long(argc, argv, "d:ht:v", longopts, NULL)) != -1)
	{
		switch (c)
		{
			case 0:
			  break;
			case 'd':
			  vol_device = dev_for_path(optarg);
			  if (vol_device < 0)
			  {
			      fprintf(stderr, "%s: can't get information about volume: %s\n", argv[0], optarg);
			      return (-1);
			  }
			  break;
			case GETOPT_HELP_CHAR:
			  usage(0);
			  break;
			case 'v':
			  verbose = 1;
			  break;
			default:
			  usage(1);
			  break;
		}
	}

	/* Remove the index from the volume of the current
	   directory if no volume was specified. */
	if (vol_device == 0)
	{
		vol_device = dev_for_path(".");
		if (vol_device < 0)
		{
			fprintf(stderr, "%s: can't get information about current volume\n", argv[0]);
			return (-1);
		}
	}

	if (argc - optind == 1)  // last argument
	{
		index_name = argv[optind];
	} else
	{
		usage(1);
	}

	if (verbose)
	{
		/* Get the mount point of the specified volume. */
		BVolume volume(vol_device);
		BDirectory dir;
		volume.GetRootDirectory(&dir);
		BPath path(&dir, NULL);

		/* Get the index type. */
		index_info i_info;
		status_t status = fs_stat_index(vol_device, index_name, &i_info);
		if (status != B_OK)
		{
			printf("%s: fs_stat_index(): (%d) %s\n", argv[0], errno, strerror(errno));
			return (errno);
		}

		/* Look up the string name equivalent of the index type. */
		const char* index_type_str = lookup_index_type(i_info.type);

		fprintf(stdout, "Removing index \"%s\" of type %s from volume mounted at %s.\n", index_name, index_type_str, path.Path());
	}

	if ((retval = fs_remove_index(vol_device, index_name)) != 0)
	{
		switch (errno)
		{
			B_FILE_ERROR:
			  error_message = "A filesystem error prevented the operation.";
			  break;
			B_BAD_VALUE:
			  error_message = "Invalid device specified.";
			  break;
			B_NOT_ALLOWED:
			  error_message = "Can't remove a system-reserved index, or the device is read-only.";
			  break;
			B_NO_MEMORY:
			  error_message = "Insufficient memory to complete the operation.";
			  break;
			B_ENTRY_NOT_FOUND:
			  error_message = "The specified index does not exist.";
			  break;
			default:
			  error_message = "An unknown error has occured.";
			  break;
		}

		fprintf(stderr, "%s\n", error_message);
	}

	return (0);
}

void usage (int status)
{
	fprintf (stderr, 
		"Usage: rmindex [OPTION]... INDEX_NAME\n"
		"\n"
		"Removes the index named INDEX_NAME from a disk volume.  Once this has been\n"
		"done, it will no longer be possible to use the query system to search for\n"
		"files with the INDEX_NAME attribute.\n"
		"\n"
		"  -d, --volume=PATH	a path on the volume from which the index will be\n"
		"                         removed\n"
		"  -h, --help		display this help and exit\n"
		"  -v, --verbose         print information about the index being removed\n"
		"\n"
		"INDEX_NAME is the name of a file attribute.\n"
		"\n"
		"If no volume is specified, the volume of the current directory is assumed.\n");

	exit(status);
}

const char* lookup_index_type(uint32 device_type)
{
	switch (device_type)
	{
		case B_DOUBLE_TYPE:
		  return "double";
		  break;
		case B_FLOAT_TYPE:
		  return "float";
		  break;
		case B_INT64_TYPE:
		  return "llong";
		  break;
		case B_INT32_TYPE:
		  return "int";
		  break;
		case B_STRING_TYPE:
		  return "string";
		  break;
		default:
		  return "unknown";
		  break;
	}
}
