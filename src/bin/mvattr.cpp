/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fs_attr.h>

#include <AutoDeleter.h>


static struct option const kLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{"force", no_argument, 0, 'f'},
	{"cross-file", no_argument, 0, 'x'},
	{"verbose", no_argument, 0, 'v'},
	{NULL}
};

// command flags
#define FLAG_VERBOSE				1
#define FLAG_FORCE					2
#define FLAG_DO_NOT_FOLLOW_LINKS	4


extern const char *__progname;
static const char *kProgramName = __progname;

static const size_t kBufferSize = 1024 * 1024;
static uint8 kBuffer[kBufferSize];


status_t
moveAttribute(const char* fromFile, const char* fromAttr, const char* toFile,
	const char* toAttr, uint32 flags)
{
	int fromFileFD = open(fromFile, O_RDONLY);
	if (fromFileFD < 0)
		return errno;

	FileDescriptorCloser fromFileFDCloser(fromFileFD);
	FileDescriptorCloser toFileFDCloser;

	int toFileFD = fromFileFD;
	if (strcmp(fromFile, toFile) != 0) {
		toFileFD = open(toFile, O_RDONLY);
		if (toFileFD < 0)
			return errno;

		toFileFDCloser.SetTo(toFileFD);
	}

	attr_info fromInfo;
	if (fs_stat_attr(fromFileFD, fromAttr, &fromInfo) != 0) {
		// TODO: optionally fail
		if ((flags & FLAG_VERBOSE) != 0)
			fprintf(stderr, "Warning: file \"%s\" does not have attribute "
				"\"%s\"\n", fromFile, fromAttr);
		return B_OK;
	}

	attr_info toInfo;
	if ((flags & FLAG_FORCE) == 0
		&& fs_stat_attr(toFileFD, toAttr, &toInfo) == 0)
		return B_FILE_EXISTS;

	int fromAttrFD = fs_fopen_attr(fromFileFD, fromAttr, fromInfo.type,
		O_RDONLY | O_EXCL);
	if (fromAttrFD < 0)
		return errno;

	FileDescriptorCloser fromAttrFDCloser(fromAttrFD);

	int toAttrFD = fs_fopen_attr(toFileFD, toAttr, fromInfo.type,
		O_CREAT | O_WRONLY | O_TRUNC);
	if (fromAttrFD < 0)
		return errno;

	FileDescriptorCloser toAttrFDCloser(toAttrFD);

	size_t bytesLeft = fromInfo.size;
	off_t offset = 0;
	while (bytesLeft > 0) {
		size_t size = min_c(kBufferSize, bytesLeft);
		ssize_t bytesRead = read_pos(fromAttrFD, offset, kBuffer, size);
		if (bytesRead < 0) {
			fs_remove_attr(toFileFD, toAttr);
			return errno;
		}

		ssize_t bytesWritten = write_pos(toAttrFD, offset, kBuffer, bytesRead);
		if (bytesWritten != bytesRead) {
			fs_remove_attr(toFileFD, toAttr);
			if (bytesWritten >= 0)
				return B_IO_ERROR;
			return errno;
		}

		bytesLeft -= bytesRead;
		offset += bytesRead;
	}

	if (fs_remove_attr(fromFileFD, fromAttr) < 0)
		return errno;

	return B_OK;
}


status_t
moveAttributes(const char* fromFile, const char* toFile,
	const char* attrPattern, uint32 flags)
{
	// TODO: implement me (with pattern support)
	return EXIT_FAILURE;
}


status_t
renameAttribute(const char* fileName, const char* fromAttr, const char* toAttr,
	uint32 flags)
{
	if ((flags & FLAG_VERBOSE) != 0)
		printf("Rename attribute: %s\n", fileName);

	status_t status = moveAttribute(fileName, fromAttr, fileName, toAttr,
		flags);
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not rename attribute for file \"%s\": %s\n",
			kProgramName, fileName, strerror(status));
	}
	return status;
}


void
usage(int returnValue)
{
	fprintf(stderr, "usage: %s [-fPv] attr-from attr-to file1 [file2...]\n"
		"   or: %s -x[fPv] <attr-pattern> from-file to-file\n\n"
		"\t-f|--force       Overwrite existing attributes\n"
		"\t-P               Don't resolve links\n"
		"\t-x|--cross-file  Moves the attributes to another file\n"
		"\t-v|--verbose     Verbose output\n",
		kProgramName, kProgramName);

	exit(returnValue);
}


int
main(int argc, char** argv)
{
	uint32 flags = 0;
	bool crossFile = false;

	int c;
	while ((c = getopt_long(argc, argv, "hfxPv", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'f':
				flags |= FLAG_FORCE;
				break;
			case 'x':
				crossFile = true;
				break;
			case 'P':
				flags |= FLAG_DO_NOT_FOLLOW_LINKS;
				break;
			case 'v':
				flags |= FLAG_VERBOSE;
				break;
			case 'h':
				usage(EXIT_SUCCESS);
				break;
			default:
				usage(EXIT_FAILURE);
				break;
		}
	}

	if (argc - optind < 3)
		usage(EXIT_FAILURE);

	if (crossFile) {
		const char* attrPattern = argv[optind++];
		const char* fromFile = argv[optind++];
		const char* toFile = argv[optind++];

		return moveAttributes(fromFile, toFile, attrPattern, flags);
	}

	const char* fromAttr = argv[optind++];
	const char* toAttr = argv[optind++];
	int returnCode = EXIT_SUCCESS;

	for (int i = optind; i < argc; i++) {
		if (renameAttribute(argv[i], fromAttr, toAttr, flags) != B_OK)
			returnCode = EXIT_FAILURE;
	}

	return returnCode;
}
