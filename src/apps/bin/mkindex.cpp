//	Copyright (c) 2003, OpenBeOS
//
//	This software is part of the OpenBeOS distribution and is covered
//	by the OpenBeOS license.
//
//	File:        mkindex.cpp
//	Author:      Scott Dellinger (dellinsd@myrealbox.com)
//	Description: Adds an index to a volume.
//
//	Changelog:
//	  2003-03-08  initial version
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

int main (int32 argc, char **argv)
{
	int c;
	int verbose = 0;
	dev_t vol_device = 0;
	char *index_type_str = NULL;
	int index_type = B_STRING_TYPE;
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
			case 't':
			  index_type_str = optarg;
			  if (strncmp("int", optarg, 3) == 0)
			    index_type = B_INT32_TYPE;
			  else if (strncmp("llong", optarg, 5) == 0)
			    index_type = B_INT64_TYPE;
			  else if (strncmp("string", optarg, 6) == 0)
			    index_type = B_STRING_TYPE;
			  else if (strncmp("float", optarg, 5) == 0)
			    index_type = B_FLOAT_TYPE;
			  else if (strncmp("double", optarg, 6) == 0)
			    index_type = B_DOUBLE_TYPE;
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

	/* Create the index on the volume of the current
	   directory if no volume was specified. */
	if (vol_device == 0)
	{
		index_type_str = "string";
		vol_device	=	dev_for_path(".");
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

		fprintf(stdout, "Creating index \"%s\" of type %s on volume mounted at %s.\n", index_name, index_type_str, path.Path());
	}

	if ((retval = fs_create_index(vol_device, index_name, index_type, 0)) != 0)
	{
		switch (errno)
		{
			B_BAD_VALUE:
			  error_message = "The device does not exist, or name is reserved.";
			  break;
			B_NOT_ALLOWED:
			  error_message = "The device is read-only.";
			  break;
			B_NO_MEMORY:
			  error_message = "Insufficient memory to complete the operation.";
			  break;
			B_FILE_EXISTS:
			  error_message = "The index already exists.";
			  break;
			B_DEVICE_FULL:
			  error_message = "There's not enough room on the device to create the index.";
			  break;
			B_FILE_ERROR:
			  error_message = "Invalid directory reference.";
			  break;
			B_ERROR:
			  error_message = "The index type passed isn't supported.";
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
		"Usage: mkindex [OPTION]... INDEX_NAME\n"
		"\n"
                "Creates a new index named INDEX_NAME on a disk volume.  Once this has been\n"
		"done, adding an attribute named INDEX_NAME to a file causes the file to be\n"
		"added to the INDEX_NAME index, such that subsequent queries will be able\n"
		"to search for files that contain the INDEX_NAME attribute.\n"
		"\n"
		"  -d, --volume=PATH	a path on the volume to which the index will be added\n"
		"  -h, --help		display this help and exit\n"
		"  -t, --type=TYPE	the type of the attribute being indexed.  One of \"int\",\n"
		"                         \"llong\", \"string\", \"float\", or \"double\".\n"
		"  -v, --verbose         print information about the index being created\n"
		"\n"
		"INDEX_NAME is the name of a file attribute.\n"
		"\n"
		"If no volume is specified, the volume of the current directory is assumed.\n"
		"If no type is specified, \"string\" is assumed.\n");

	exit(status);
}

