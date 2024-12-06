/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int
main()
{
	char tmpfile[] = "/tmp/mmap_fixed_test_XXXXXX";
	int fd = mkstemp(tmpfile);

	if (fd < 0) {
		printf("cannot create temporary file.\n");
		return -1;
	}

	unlink(tmpfile);
	ftruncate(fd, 4096);

	// Should not crash the kernel (#18422)
	void* addr = mmap(NULL, 4096, PROT_NONE, MAP_SHARED, fd, 0);
	void* addr1 = mmap(addr, 4096, PROT_NONE, MAP_SHARED | MAP_FIXED, fd, 0);

	if (addr == MAP_FAILED || addr1 == MAP_FAILED) {
		printf("mmap failed.\n");
		return -1;
	}

	if (addr != addr1) {
		printf("MAP_FIXED did not return same address.\n");
		return -1;
	}

	return 0;
}
