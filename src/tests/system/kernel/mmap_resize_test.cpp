/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


int
main()
{
	const char* fileName = "/tmp/mmap-resize-test-file";

	// create file
	printf("creating file...\n");
	int fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd < 0) {
		fprintf(stderr, "Failed to open \"%s\": %s\n", fileName,
			strerror(errno));
		exit(1);
	}

	// write half a page
	printf("writing data to file...\n");
	char buffer[2048];
	memset(buffer, 0xdd, sizeof(buffer));
	if (write(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer)) {
		fprintf(stderr, "Failed to write to file!\n");
		exit(1);
	}

	// map the page
	printf("mapping file...\n");
	void* address = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (address == MAP_FAILED) {
		fprintf(stderr, "Failed to map the file: %s\n", strerror(errno));
		exit(1);
	}

	// truncate the file
	printf("truncating file...\n");
	if (ftruncate(fd, 0) < 0) {
		fprintf(stderr, "Failed to truncate the file: %s\n", strerror(errno));
		exit(1);
	}

	// touch data
	printf("touching no longer mapped data (should fail with SIGBUS)...\n");
	*(int*)address = 42;

	printf("Shouldn't get here!!!\n");

	return 0;
}
