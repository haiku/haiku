/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include <TypeConstants.h>
#include <Mime.h>

#include <fs_attr.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


/** Used to present the characters in the raw data view */

static void
putCharOrDot(uchar c)
{
	putchar(isgraph(c) ? c : '.');
}


/** Dumps the contents of the attribute in the form of
 *	raw data. This view is used for the type B_RAW_DATA_TYPE,
 *	for custom types and for any type that is not directly
 *	supported by the utility "addattr"
 */

static void
dumpRawData(const char *buffer, size_t size)
{
	const uint32 kChunkSize = 16;
	uint32 dumpPosition = 0;

	while (dumpPosition < size) {
		// Position for this line
		printf("0x%06lx:  ", dumpPosition);

		// Print the bytes in form of hexadecimal numbers
		for (uint32 i = 0; i < kChunkSize; i++) {
			if (dumpPosition + i < size) {
				printf("%02x ", (uint8)buffer[dumpPosition + i]);
			} else
				printf("   ");
		}

		// Print the bytes in form of printable characters
		// (whenever possible)
		printf("  '");
		for (uint32 i = 0; i < kChunkSize; i++) {
			if (dumpPosition < size)
				putCharOrDot(buffer[dumpPosition]);
			else
				putchar(' ');

			dumpPosition++;
		}
		printf("'\n");
	}
} 


static status_t
catAttr(const char *attribute, const char *fileName)
{
	int fd = open(fileName, O_RDONLY);
	if (fd < 0)
		return errno;

	attr_info info;
	if (fs_stat_attr(fd, attribute, &info) < 0)
		return errno;

	// limit size of the attribute, only the first 64k will make it on screen
	off_t size = info.size;
	if (size > 64 * 1024)
		size = 64 * 1024;

	char buffer[size];
	ssize_t bytesRead = fs_read_attr(fd, attribute, info.type, 0, buffer, size);
	if (bytesRead < 0)
		return errno;

	if (bytesRead != size) {
		fprintf(stderr, "Could only read %ld bytes from attribute!\n", bytesRead);
		return B_ERROR;
	}

	switch (info.type) {
		case B_INT8_TYPE:
			printf("%s : int8 : %d\n", fileName, *((int8 *)buffer));
			break;
		case B_UINT8_TYPE:
			printf("%s : uint8 : %u\n", fileName, *((uint8 *)buffer));
			break;
		case B_INT16_TYPE:
			printf("%s : int16 : %d\n", fileName, *((int16 *)buffer));
			break;
		case B_UINT16_TYPE:
			printf("%s : uint16 : %u\n", fileName, *((uint16 *)buffer));
			break;
		case B_INT32_TYPE:
			printf("%s : int32 : %ld\n", fileName, *((int32 *)buffer));
			break;
		case B_UINT32_TYPE:
			printf("%s : uint32 : %lu\n", fileName, *((uint32 *)buffer));
			break;
		case B_INT64_TYPE:
			printf("%s : int64 : %Ld\n", fileName, *((int64 *)buffer));
			break;
		case B_UINT64_TYPE:
			printf("%s : uint64 : %Lu\n", fileName, *((uint64 *)buffer));
			break;
		case B_FLOAT_TYPE:
			printf("%s : float : %f\n", fileName, *((float *)buffer));
			break;
		case B_DOUBLE_TYPE:
			printf("%s : double : %f\n", fileName, *((double *)buffer));
			break;
		case B_BOOL_TYPE:
			printf("%s : bool : %d\n", fileName, *((unsigned char *)buffer));
			break;
		case B_STRING_TYPE:
		case B_MIME_STRING_TYPE:
			printf("%s : string : %s\n", fileName, buffer);
			break;

		default:
			// The rest of the attributes types are displayed as raw data
			printf("%s : raw_data : \n", fileName);
			dumpRawData(buffer, size);
			break;
	}

	return B_OK;
}


int
main(int argc, char *argv[])
{
	char *program = strrchr(argv[0], '/');
	if (program == NULL)
		program = argv[0];
	else
		program++;

	if (argc > 2) {
		const char *attr = argv[1];

		// Cat the attribute for every file given
		for (int32 i = 2; i < argc; i++) {
			status_t status = catAttr(attr, argv[i]);
			if (status != B_OK) {
				fprintf(stderr, "%s: file \"%s\", attribute \"%s\": %s\n",
					program, argv[i], attr, strerror(status));
			}
		}
	} else {
		// Issue usage message
		fprintf(stderr, "usage: %s attr_name file1 [file2...]\n", program);
		// Be's original version -only- returned 1 if the 
		// amount of parameters was wrong, not if the file
		// or attribute couldn't be found (!)
		// In all other cases it returned 0
		return 1;
	}

	return 0;
}


