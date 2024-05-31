/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>


extern const char* __progname;
const char* kProgramName = __progname;

const int32_t kDefaultFiles = -1;
const off_t kDefaultFileSize = 4096;


static void
usage(int status)
{
	printf("usage: %s [--files <num-of-files>] [--size <file-size>]\n",
		kProgramName);
	printf("options:\n");
	printf("  -f  --files  Number of files to be created. Defaults to as "
		"many as fit.\n");
	printf("  -s  --size   Size of each file. Defaults to %lldKB.\n",
		kDefaultFileSize / 1024);

	exit(status);
}


static bool
create_file(int32_t i, const char* suffix, const char* buffer, size_t size)
{
	char name[64];
	snprintf(name, sizeof(name), "fragments/%06d%s", i, suffix);

	int fd = open(name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "%s: Could not create file %d: %s\n", kProgramName,
			i, strerror(errno));
		return false;
	}

	if (write(fd, buffer, size) < (ssize_t)size) {
		fprintf(stderr, "%s: Could not write file %d: %s\n", kProgramName,
			i, strerror(errno));
		return false;
	}

	close(fd);
	return true;
}


int
main(int argc, char** argv)
{
	int32_t numFiles = kDefaultFiles;
	off_t fileSize = kDefaultFileSize;

	int optionIndex = 0;
	int opt;
	static struct option longOptions[] = {
		{"help", no_argument, 0, 'h'},
		{"size", required_argument, 0, 's'},
		{"files", required_argument, 0, 'f'},
		{0, 0, 0, 0}
	};

	do {
		opt = getopt_long(argc, argv, "hs:f:", longOptions, &optionIndex);
		switch (opt) {
			case -1:
				// end of arguments, do nothing
				break;

			case 'f':
				numFiles = strtoul(optarg, NULL, 0);
				break;

			case 's':
				fileSize = strtoul(optarg, NULL, 0);
				break;

			case 'h':
			default:
				usage(0);
				break;
		}
	} while (opt != -1);

	// fill buffer

	char* buffer = (char*)malloc(fileSize);
	if (buffer == NULL) {
		fprintf(stderr, "%s: not enough memory.\n", kProgramName);
		exit(1);
	}

	for (uint32_t i = 0; i < fileSize; i++) {
		buffer[i] = (char)(i & 0xff);
	}

	// create files

	if (numFiles > 0)
		printf("Creating %d files...\n", numFiles);
	else
		printf("Creating as many files as fit...\n");

	mkdir("fragments", 0777);

	int32_t filesCreated = 0;
	bigtime_t lastTime = 0;

	for (int32_t i = 0; i < numFiles || numFiles < 0; i++) {
		if (!create_file(i, "", buffer, fileSize)
			|| !create_file(i, ".remove", buffer, fileSize))
			break;

		filesCreated++;

		if (lastTime + 250000LL < system_time()) {
			printf("%10d\33[1A\n", filesCreated);
			lastTime = system_time();
		}
	}

	free(buffer);

	// delete fragmentation files

	printf("Deleting %d temporary files...\n", filesCreated);

	for (int32_t i = 0; i < filesCreated; i++) {
		char name[64];
		snprintf(name, sizeof(name), "fragments/%06d.remove", i);

		if (remove(name) != 0) {
			fprintf(stderr, "%s: Could not remove file %d: %s\n",
				kProgramName, i, strerror(errno));
		}

		if (lastTime + 250000LL < system_time()) {
			printf("%10d\33[1A\n", i);
			lastTime = system_time();
		}
	}

	printf("          \33[1A\n");
		// delete progress count

	return 0;
}
