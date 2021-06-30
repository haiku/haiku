/*
 * Copyright 2021 David Sebek, dasebek@gmail.com
 * Copyright 2013 Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <Drivers.h>

#include <AutoDeleter.h>


static struct option const kLongOptions[] = {
	{"help", no_argument, 0, 'h'},
	{"offset", required_argument, 0, 'o'},
	{"length", required_argument, 0, 'l'},
	{"discard-device", no_argument, 0, 'd'},
	{"force", no_argument, 0, 'f'},
	{"verbose", no_argument, 0, 'v'},
	{NULL}
};


extern const char* __progname;
static const char* kProgramName = __progname;


void
PrintUsage(void)
{
	fprintf(stderr, "Usage: %s [options] <path-to-mounted-file-system>\n",
		kProgramName);
	fprintf(stderr, "\n");
	fprintf(stderr, "%s reports unused blocks to a storage device.\n",
		kProgramName);
	fprintf(stderr, "\n");
	fprintf(stderr, "List of options:\n");
	fprintf(stderr, " -o, --offset <num>  Start of the trimmed region in bytes (default: 0)\n");
	fprintf(stderr, " -l, --length <num>  Length of the trimmed region in bytes. Trimming will stop\n");
	fprintf(stderr, "                     when a file system/device boundary is reached.\n");
	fprintf(stderr, "                     (default: trim until the end)\n");
	fprintf(stderr, " --discard-device    Trim a block or character device directly instead of\n");
	fprintf(stderr, "                     a file system. DANGEROUS: erases data on the device!\n");
	fprintf(stderr, "\n");
	fprintf(stderr, " -f, --force         Do not ask user for confirmation of dangerous operations\n");
	fprintf(stderr, " -v, --verbose       Enable verbose messages\n");
	fprintf(stderr, " -h, --help          Display this help\n");
}


bool
IsDirectory(const int fd)
{
	struct stat fdStat;
	if (fstat(fd, &fdStat) == -1) {
		fprintf(stderr, "%s: fstat failed: %s\n", kProgramName,
			strerror(errno));
		return false;
	}
	return S_ISDIR(fdStat.st_mode);
}


bool
IsBlockDevice(const int fd)
{
	struct stat fdStat;
	if (fstat(fd, &fdStat) == -1) {
		fprintf(stderr, "%s: fstat failed: %s\n", kProgramName,
			strerror(errno));
		return false;
	}
	return S_ISBLK(fdStat.st_mode);
}


bool
IsCharacterDevice(const int fd)
{
	struct stat fdStat;
	if (fstat(fd, &fdStat) == -1) {
		fprintf(stderr, "%s: fstat failed: %s\n", kProgramName,
			strerror(errno));
		return false;
	}
	return S_ISCHR(fdStat.st_mode);
}


int
YesNoPrompt(const char* message)
{
	char* buffer;
	size_t bufferSize;
	ssize_t inputLength;

	if (message != NULL)
		printf("%s\n", message);

	while (true) {
		printf("Answer [yes/NO]: ");

		buffer = NULL;
		bufferSize = 0;
		inputLength = getline(&buffer, &bufferSize, stdin);

		MemoryDeleter deleter(buffer);

		if (inputLength == -1) {
			fprintf(stderr, "%s: getline failed: %s\n", kProgramName,
				strerror(errno));
			return -1;
		}

		if (strncasecmp(buffer, "yes\n", bufferSize) == 0)
			return 1;

		if (strncasecmp(buffer, "no\n", bufferSize) == 0
			|| strncmp(buffer, "\n", bufferSize) == 0)
			return 0;
	}
}


bool
ParseUint64(const char* string, uint64* value)
{
	uint64 parsedValue;
	char dummy;

	if (string == NULL || value == NULL)
		return false;

	if (sscanf(string, "%" B_SCNu64 "%c", &parsedValue, &dummy) == 1) {
		*value = parsedValue;
		return true;
	}
	return false;
}


int
main(int argc, char** argv)
{
	bool discardDevice = false;
	bool force = false;
	bool verbose = false;
	uint64 offset = 0;
	uint64 length = UINT64_MAX;

	int c;
	while ((c = getopt_long(argc, argv, "ho:l:fv", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'o':
				if (!ParseUint64(optarg, &offset)) {
					fprintf(stderr, "%s: Invalid offset value\n", kProgramName);
					return EXIT_FAILURE;
				}
				break;
			case 'l':
				if (!ParseUint64(optarg, &length)) {
					fprintf(stderr, "%s: Invalid length value\n", kProgramName);
					return EXIT_FAILURE;
				}
				break;
			case 'd':
				discardDevice = true;
				break;
			case 'f':
				force = true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'h':
				PrintUsage();
				return EXIT_SUCCESS;
				break;
			default:
				PrintUsage();
				return EXIT_FAILURE;
				break;
		}
	}

	if (argc - optind < 1) {
		PrintUsage();
		return EXIT_FAILURE;
	}
	const char* path = argv[optind++];

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: Could not access path: %s\n", kProgramName,
			strerror(errno));
		return EXIT_FAILURE;
	}

	FileDescriptorCloser closer(fd);

	if (IsDirectory(fd)) {
		if (discardDevice) {
			fprintf(stderr, "%s: Block or character device requested but %s"
				" is a directory\n", kProgramName, path);
			return EXIT_FAILURE;
		}

		if (!force && YesNoPrompt("Trim support in Haiku is experimental and"
				" may result in data loss.\nContinue anyway?") != 1) {
			fprintf(stderr, "%s: Operation canceled by the user\n",
				kProgramName);
			return EXIT_SUCCESS;
		}
	} else if (IsBlockDevice(fd) || IsCharacterDevice(fd)) {
		if (!discardDevice) {
			fprintf(stderr, "%s: --discard-device must be specified to trim"
				" a block or character device\n", kProgramName);
			return EXIT_FAILURE;
		}

		if (!force && YesNoPrompt("Do you really want to PERMANENTLY ERASE"
				" data from the specified device?") != 1) {
			fprintf(stderr, "%s: Operation canceled by the user\n",
				kProgramName);
			return EXIT_SUCCESS;
		}
	} else {
		fprintf(stderr, "%s: %s is neither a directory nor a block or"
			" character device\n", kProgramName, path);
		return EXIT_FAILURE;
	}

	fs_trim_data trimData;
	trimData.range_count = 1;
	trimData.ranges[0].offset = offset;
	trimData.ranges[0].size = length;
	trimData.trimmed_size = 0;

	if (verbose) {
		printf("Range to trim (bytes): offset = %" B_PRIu64
			", length = %" B_PRIu64 "\n", offset, length);
	}

	int retval = EXIT_SUCCESS;

	if (ioctl(fd, B_TRIM_DEVICE, &trimData, sizeof(fs_trim_data)) != 0) {
		fprintf(stderr, "%s: Trimming failed: %s\n", kProgramName,
			strerror(errno));
		retval = EXIT_FAILURE;
	}

	printf("Trimmed %" B_PRIu64 " bytes from device%s.\n",
		trimData.trimmed_size,
		retval == EXIT_SUCCESS ? "" : " (number may be inaccurate)");

	return retval;
}
