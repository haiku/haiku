/*
 * Copyright 2005, Stephan Aßmus, superstippi@yellowbites.com.
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
#include <unistd.h>
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
catAttr(const char *attribute, const char *fileName, bool keepRaw = false)
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

	char* buffer = new char[size];
	ssize_t bytesRead = fs_read_attr(fd, attribute, info.type, 0, buffer, size);
	if (bytesRead < 0) {
		delete[] buffer;
		return errno;
	}

	if (bytesRead != size) {
		fprintf(stderr, "Could only read %ld bytes from attribute!\n", bytesRead);
		delete[] buffer;
		return B_ERROR;
	}

	if (keepRaw) {
		off_t pos = 0;
		ssize_t written = 0;
		while (pos < info.size) {
			// write what we have read so far
			written = write(STDOUT_FILENO, buffer, bytesRead);
			// check for write error
			if (written < bytesRead) {
				fprintf(stderr, "Could only write %ld bytes to stream!\n", written);
				if (written > 0)
					written = B_ERROR;
				break;
			}
			// read next chunk of data at pos
			pos += bytesRead;
			bytesRead = fs_read_attr(fd, attribute, info.type, pos, buffer, size);
			// check for read error
			if (bytesRead < size && pos + bytesRead < info.size) {
				fprintf(stderr, "Could only read %ld bytes from attribute!\n", bytesRead);
				written = B_ERROR;
				break;
			}
		}
		delete[] buffer;
		if (written > 0)
			written = B_OK;
		return written;
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

	delete[] buffer;
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
		int32 attrNameIndex = 1;
		bool keepRaw = false;

		// see if user wants to get to the raw data of the attribute
		if (strcmp(argv[attrNameIndex], "--raw") == 0 ||
			strcmp(argv[attrNameIndex], "-r") == 0) {
			attrNameIndex++;
			keepRaw = true;
		}

		// Cat the attribute for every file given
		for (int32 i = attrNameIndex + 1; i < argc; i++) {
			status_t status = catAttr(argv[attrNameIndex], argv[i], keepRaw);
			if (status != B_OK) {
				fprintf(stderr, "%s: file \"%s\", attribute \"%s\": %s\n",
					program, argv[i], argv[attrNameIndex], strerror(status));
			}
		}
	} else {
		// Issue usage message
		fprintf(stderr, "usage: %s [--raw|-r] attr_name file1 [file2...]\n", program);
		// Be's original version -only- returned 1 if the 
		// amount of parameters was wrong, not if the file
		// or attribute couldn't be found (!)
		// In all other cases it returned 0
		return 1;
	}

	return 0;
}


