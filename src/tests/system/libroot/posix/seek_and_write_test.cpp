/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>


void
test_for_content(int fd, off_t start, const char* contents, size_t length)
{
	char buffer[4096];
	while (length > 0) {
		size_t toRead = std::min(length, sizeof(buffer));

		if (pread(fd, buffer, toRead, start) != (ssize_t)toRead) {
			perror("reading failed");
			exit(1);
		}
		
		if (memcmp(contents, buffer, toRead)) {
			fprintf(stderr, "Contents at %lld differ!\n", start);
			exit(1);
		}

		contents += toRead;
		start += toRead;
		length -= toRead;
	}
}


void
test_for_zero(int fd, off_t start, off_t end)
{
	while (start < end) {
		char buffer[4096];
		size_t length = std::min((size_t)(end - start), sizeof(buffer));
		
		if (pread(fd, buffer, length, start) != (ssize_t)length) {
			perror("reading failed");
			exit(1);
		}

		for (size_t i = 0; i < length; i++) {
			if (buffer[i] != 0) {
				fprintf(stderr, "Buffer at %lld is not empty (%#x)!\n",
					start + i, buffer[i]);
				exit(1);
			}
		}

		start += length;
	}
}


int
main(int argc, char** argv)
{
	const char* name = "/tmp/seek_and_write";
	bool prefill = true;

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][1] == 'n')
				prefill = false;
			else {
				fprintf(stderr, "Unknown option\n");
				return 1;
			}

			continue;
		}

		name = argv[i];
	}

	int fd = open(name, O_RDWR | O_TRUNC | O_CREAT, 0644);
	if (fd < 0) {
		perror("failed to open file");
		return 1;
	}

	char buffer[256];
	for (size_t i = 0; i < 256; i++)
		buffer[i] = i;

	if (prefill) {
		// Write test data to file to make sure it's not empty

		for (size_t i = 0; i < 100; i++) {
			if (write(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer)) {
				perror("writing failed");
				return 1;
			}
		}
	}

	// Truncate it again in order to remove its contents

	ftruncate(fd, 0);

	// Seek past its end, and write something

	pwrite(fd, "---", 3, 100 * 1024);
	pwrite(fd, "+++", 3, 200 * 1024);

	// Test contents
	
	test_for_zero(fd, 0, 100 * 1024);
	test_for_content(fd, 100 * 1024, "---", 3);
	test_for_zero(fd, 100 * 1024 + 256, 200 * 1024);
	test_for_content(fd, 200 * 1024, "+++", 3);

	return 0;
}
