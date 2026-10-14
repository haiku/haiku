/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


int
main()
{
	const char* fileName = "/tmp/mmap-read-self-test-file";

	// create file
	int fd = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (fd < 0)
		return -2;

	// write some data
	char buffer[512];
	memset(buffer, 0xdd, sizeof(buffer));
	if (write(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer))
		return -3;

	// map the page
	void* address = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (address == MAP_FAILED)
		return -4;

	// self-read
	if (pread(fd, address, sizeof(buffer), 0) != sizeof(buffer))
		return -5;

	// compare data
	if (memcmp(address, buffer, 512) != 0)
		return -6;

	if (munmap(address, 4096) != 0)
		return -7;

	return 0;
}
