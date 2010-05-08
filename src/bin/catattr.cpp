/*
 * Copyright 2010, Alexander Shagarov, alexander.shagarov@gmail.com.
 * Copyright 2005, Stephan Aßmus, superstippi@yellowbites.com.
 * Copyright 2004-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Sebastian Nozzi.
 *
 * Distributed under the terms of the MIT license.
 */


#include <Mime.h>
#include <TypeConstants.h>

#include <fs_attr.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/*!	Used to present the characters in the raw data view */
static void
putCharOrDot(uchar c)
{
	putchar(isgraph(c) ? c : '.');
}


/*!	Dumps the contents of the attribute in the form of
	raw data. This view is used for the type B_RAW_DATA_TYPE,
	for custom types and for any type that is not directly
	supported by the utility "addattr"
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


static const char*
type_to_string(uint32 type)
{
	if (type == B_RAW_TYPE)
		return "raw_data";

	static char buffer[32];

	int32 missed = 0, shift = 24;
	uint8 value[4];
	for (int32 i = 0; i < 4; i++, shift -= 8) {
		value[i] = uint8(type >> shift);
		if (value[i] < ' ' || value[i] > 127) {
			value[i] = '.';
			missed++;
		}
	}

	if (missed < 2) {
		sprintf(buffer, "'%c%c%c%c'", value[0], value[1], value[2],
			value[3]);
	} else
		sprintf(buffer, "0x%08lx", type);

	return buffer;
}


static status_t
catAttr(const char *attribute, const char *fileName, bool keepRaw = false,
	bool resolveLinks = true)
{
	int fd = open(fileName, O_RDONLY | (resolveLinks ? 0 : O_NOTRAVERSE));
	if (fd < 0)
		return errno;

	attr_info info;
	if (fs_stat_attr(fd, attribute, &info) < 0)
		return errno;

	// limit size of the attribute, only the first 64k will make it on screen
	off_t size = info.size;
	bool cut = false;
	if (size > 64 * 1024) {
		size = 64 * 1024;
		cut = true;
	}

	char* buffer = (char*)malloc(size);
	if (!buffer) {
		fprintf(stderr, "Could not allocate read buffer!\n");
		return B_NO_MEMORY;
	}

	ssize_t bytesRead = fs_read_attr(fd, attribute, info.type, 0, buffer, size);
	if (bytesRead < 0) {
		free(buffer);
		return errno;
	}

	if (bytesRead != size) {
		fprintf(stderr, "Could only read %ld bytes from attribute!\n",
			bytesRead);
		free(buffer);
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
				if (written >= 0) {
					fprintf(stderr, "Could only write %ld bytes to stream!\n",
						written);
					written = B_ERROR;
				} else {
					fprintf(stderr, "Failed to write to stream: %s\n",
						strerror(written));
				}
				break;
			}
			// read next chunk of data at pos
			pos += bytesRead;
			bytesRead = fs_read_attr(fd, attribute, info.type, pos, buffer,
				size);
			// check for read error
			if (bytesRead < size && pos + bytesRead < info.size) {
				if (bytesRead >= 0) {
					fprintf(stderr, "Could only read %ld bytes from "
						"attribute!\n", bytesRead);
				} else {
					fprintf(stderr, "Failed to read from attribute: %s\n",
						strerror(bytesRead));
				}
				written = B_ERROR;
				break;
			}
		}
		free(buffer);
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
			printf("%s : string : %s\n", fileName, buffer);
			break;

		case B_MIME_STRING_TYPE:
		case 'MSIG':
		case 'MSDC':
		case 'MPTH':
			printf("%s : %s : %s\n", fileName, type_to_string(info.type),
				buffer);
			break;

		case B_MESSAGE_TYPE:
		{
			BMessage message;
			if (!cut && message.Unflatten(buffer) == B_OK) {
				printf("%s : message :\n", fileName);
				message.PrintToStream();
				break;
			}
			// supposed to fall through
		}

		default:
			// The rest of the attributes types are displayed as raw data
			printf("%s : %s : \n", fileName, type_to_string(info.type));
			dumpRawData(buffer, size);
			break;
	}

	free(buffer);
	return B_OK;
}


static int
usage(const char* program)
{
	// Issue usage message
	fprintf(stderr, "usage: %s [-P] [--raw|-r] <attribute-name> <file1> "
		"[<file2>...]\n"
		"\t-P : Don't resolve links\n"
		"\t--raw|-r : Get the raw data of attributes\n", program);
	// Be's original version -only- returned 1 if the
	// amount of parameters was wrong, not if the file
	// or attribute couldn't be found (!)
	// In all other cases it returned 0
	return 1;
}


int
main(int argc, char *argv[])
{
	char *program = strrchr(argv[0], '/');
	if (program == NULL)
		program = argv[0];
	else
		program++;

	bool keepRaw = false;
	bool resolveLinks = true;
	const struct option longOptions[] = {
		{"raw", no_argument, NULL, 'r'},
		{NULL, 0, NULL, 0}
	};

	int rez;
	while ((rez = getopt_long(argc, argv, "rP", longOptions, NULL)) != -1) {
		switch (rez) {
			case 'r':
				keepRaw = true;
				break;
			case 'P':
				resolveLinks = false;
				break;
			default:
				return usage(program);
		}
	}

	if (optind + 2 > argc)
		return usage(program);

	const char* attrName = argv[optind++];
	while (optind < argc) {
		const char* fileName = argv[optind++];
		status_t status = catAttr(attrName, fileName, keepRaw,
				resolveLinks);
		if (status != B_OK) {
			fprintf(stderr, "%s: \"%s\", attribute \"%s\": %s\n",
					program, fileName, attrName, strerror(status));
		}
	}

	return 0;
}
