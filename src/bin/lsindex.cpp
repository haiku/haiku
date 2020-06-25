/*
 * Copyright 2002-2020, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		jonas.sundstrom@kirilla.com
 *		revol@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <TypeConstants.h>
#include <fs_info.h>
#include <fs_index.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>


static void
print_help(void)
{
	fprintf (stderr,
		"Usage: lsindex [--help | -v | --verbose | --mkindex | -l | --long] [volume path]\n"
		"   -l --long\t outputs long listing\n"
		"   -v --verbose\t gives index type, dates and owner\n"
		"      --mkindex\t outputs mkindex commands to recreate all the indices\n"
		"      --help\t prints out this text\n\n"
		"   If no volume is specified, the volume of the current directory is assumed.\n");
}


static const char *
print_index_type(const index_info &info, bool mkindexOutput)
{
	static char buffer[30];

	switch (info.type) {
		case B_INT32_TYPE:
			return mkindexOutput ? "int" : "Int-32";
		case B_INT64_TYPE:
			return mkindexOutput ? "llong" : "Int-64";
		case B_STRING_TYPE:
			return mkindexOutput ? "string" : "Text";
		case B_FLOAT_TYPE:
			return mkindexOutput ? "float" : "Float";
		case B_DOUBLE_TYPE:
			return mkindexOutput ? "double" : "Double";

		default:
			sprintf(buffer, mkindexOutput
				 ? "0x%08" B_PRIx32
				 : "Unknown type (0x%" B_PRIx32 ")",
					info.type);
			return buffer;
	}
}


static const char *
type_string(type_code type)
{
	// all types from <TypeConstants.h> listed for completeness,
	// even though they don't all apply to attribute indices

#define RETURN_TYPE(x) case x: return #x

	switch (type) {
		RETURN_TYPE(B_ANY_TYPE);
		RETURN_TYPE(B_BOOL_TYPE);
		RETURN_TYPE(B_CHAR_TYPE);
		RETURN_TYPE(B_COLOR_8_BIT_TYPE);
		RETURN_TYPE(B_DOUBLE_TYPE);
		RETURN_TYPE(B_FLOAT_TYPE);
		RETURN_TYPE(B_GRAYSCALE_8_BIT_TYPE);
		RETURN_TYPE(B_INT64_TYPE);
		RETURN_TYPE(B_INT32_TYPE);
		RETURN_TYPE(B_INT16_TYPE);
		RETURN_TYPE(B_INT8_TYPE);
		RETURN_TYPE(B_MESSAGE_TYPE);
		RETURN_TYPE(B_MESSENGER_TYPE);
		RETURN_TYPE(B_MIME_TYPE);
		RETURN_TYPE(B_MONOCHROME_1_BIT_TYPE);
		RETURN_TYPE(B_OBJECT_TYPE);
		RETURN_TYPE(B_OFF_T_TYPE);
		RETURN_TYPE(B_PATTERN_TYPE);
		RETURN_TYPE(B_POINTER_TYPE);
		RETURN_TYPE(B_POINT_TYPE);
		RETURN_TYPE(B_RAW_TYPE);
		RETURN_TYPE(B_RECT_TYPE);
		RETURN_TYPE(B_REF_TYPE);
		RETURN_TYPE(B_RGB_32_BIT_TYPE);
		RETURN_TYPE(B_RGB_COLOR_TYPE);
		RETURN_TYPE(B_SIZE_T_TYPE);
		RETURN_TYPE(B_SSIZE_T_TYPE);
		RETURN_TYPE(B_STRING_TYPE);
		RETURN_TYPE(B_TIME_TYPE);
		RETURN_TYPE(B_UINT64_TYPE);
		RETURN_TYPE(B_UINT32_TYPE);
		RETURN_TYPE(B_UINT16_TYPE);
		RETURN_TYPE(B_UINT8_TYPE);
		RETURN_TYPE(B_MEDIA_PARAMETER_TYPE);
		RETURN_TYPE(B_MEDIA_PARAMETER_WEB_TYPE);
		RETURN_TYPE(B_MEDIA_PARAMETER_GROUP_TYPE);
		RETURN_TYPE(B_ASCII_TYPE);

		default:
			return NULL;
	}
#undef RETURN_TYPE
}


static void
print_index_long_stat(const index_info &info, char *name)
{
	char modified[30];
	strftime(modified, 30, "%m/%d/%Y %I:%M %p",
		localtime(&info.modification_time));
	printf("%16s  %s  %8" B_PRIdOFF " %s\n",
		print_index_type(info, false), modified, info.size, name);
}


static void
print_index_verbose_stat(const index_info &info, char *name)
{
	printf("%-18s\t", name);

	// Type
	const char *typeString = type_string(info.type);
	if (typeString != NULL)
		printf("%-10s\t", typeString);
	else
		printf("%" B_PRIu32 "\t", info.type);

	// Size
	printf("%10" B_PRIdOFF "  ", info.size);

	// Created
	char string[30];
	strftime(string, sizeof(string), "%Y-%m-%d %H:%M",
		localtime(&info.creation_time));
	printf("%s  ", string);

	// Modified
	strftime(string, sizeof(string), "%Y-%m-%d %H:%M",
		localtime(&info.modification_time));
	printf("%s", string);

	// User
	printf("%5d", info.uid);

	// Group
	printf("%5d\n", info.gid);
}


int
main(int argc, char **argv)
{
	dev_t device = dev_for_path(".");
	DIR *indices = NULL;
	bool verbose = false;
	bool longListing = false;
	bool mkindexOutput = false; /* mkindex-ready output */

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp(argv[i], "--help")) {
				print_help();
				return 0;
			}
			if (!strcmp(argv[i], "--verbose") || !strcmp(argv[i], "-v"))
				verbose = true;
			else if (!strcmp(argv[i], "--long") || !strcmp(argv[i], "-l"))
				longListing = true;
			else if (!strcmp(argv[i], "--mkindex"))
				mkindexOutput = true;
			else {
				fprintf(stderr, "%s: option %s is not understood (use --help "
					"for help)\n", argv[0], argv[i]);
				return -1;
			}
		} else {
			device = dev_for_path(argv[i]);
			if (device < 0) {
				fprintf(stderr, "%s: can't get information about volume: %s\n",
					argv[0], argv[i]);
				return -1;
			}
		}
	}

	indices = fs_open_index_dir(device);
	if (indices == NULL) {
		fprintf(stderr, "%s: can't open index dir of device %" B_PRIdDEV "\n",
			argv[0], device);
		return -1;
	}

	if (verbose) {
		printf(" Name   Type   Size   Created   Modified   User   Group\n");
		printf("********************************************************\n");
	}

	while (1) {
		// We have to reset errno before calling fs_read_index_dir().
		errno = 0;
		dirent *index = fs_read_index_dir(indices);
		if (index == NULL) {
			if (errno != B_ENTRY_NOT_FOUND && errno != B_OK) {
				printf("%s: fs_read_index_dir: (%d) %s\n", argv[0], errno,
					strerror(errno));
				return errno;
			}
			break;
		}

		if (verbose || longListing || mkindexOutput) {
			index_info info;

			if (fs_stat_index(device, index->d_name, &info) != B_OK) {
				printf("%s: fs_stat_index(): (%d) %s\n", argv[0], errno,
					strerror(errno));
				return errno;
			}

			if (verbose)
				print_index_verbose_stat(info, index->d_name);
			else if (longListing)
				print_index_long_stat(info, index->d_name);
			else {
				// mkindex output
				printf("mkindex -t %s '%s'\n", print_index_type(info, true),
					index->d_name);
			}
		} else
			printf("%s\n", index->d_name);
	}

	fs_close_index_dir(indices);
	return 0;
}

