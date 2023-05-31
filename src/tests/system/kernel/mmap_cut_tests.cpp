/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <OS.h>


int gTestFd = -1;


int
map_negative_offset_test()
{
	// should fail (negative offset)
	void* ptr = mmap(NULL, B_PAGE_SIZE, PROT_READ, MAP_PRIVATE, gTestFd, -4096);
	if (ptr != MAP_FAILED) {
		printf("map-negative-offset unexpectedly succeeded!\n");
		return -1;
	}
	return 0;
}


int
map_cut_compare_test()
{
	uint8* ptr1 = (uint8*)mmap(NULL, 16 * B_PAGE_SIZE, PROT_READ, MAP_PRIVATE, gTestFd, 0);
	uint8 chunk[128];
	memcpy(chunk, &ptr1[3 * B_PAGE_SIZE], sizeof(chunk));

	// now cut the area
	uint8* ptr2 = (uint8*)mmap(&ptr1[B_PAGE_SIZE], B_PAGE_SIZE,
		PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);

	// validate that the area after the cut still has the expected data
	int status = memcmp(&ptr1[3 * B_PAGE_SIZE], chunk, sizeof(chunk));
	if (status != 0) {
		printf("map-cut-compare test failed!\n");
		return status;
	}
	return 0;
}


int
map_protect_cut_test()
{
	uint8* ptr = (uint8*)mmap(NULL, B_PAGE_SIZE * 4, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	// make the tail accessible
	mprotect(ptr + B_PAGE_SIZE * 3, B_PAGE_SIZE, PROT_READ | PROT_WRITE);

	// store any value
	ptr[B_PAGE_SIZE * 3] = 'a';

	// cut the area in the middle, before the accessible tail
	mmap(ptr + B_PAGE_SIZE, B_PAGE_SIZE, PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

	// validate that this does not crash
	if (ptr[B_PAGE_SIZE * 3] != 'a') {
		printf("map-protect-cut test failed!\n");
		return -1;
	}
	return 0;
}


int
main()
{
	gTestFd = open("/boot/system/lib/libroot.so", O_CLOEXEC | O_RDONLY);
	if (gTestFd < 0)
		return -1;

	int status;

	if ((status = map_negative_offset_test()) != 0)
		return status;

	if ((status = map_cut_compare_test()) != 0)
		return status;

	if ((status = map_protect_cut_test()) != 0)
		return status;

	return 0;
}
