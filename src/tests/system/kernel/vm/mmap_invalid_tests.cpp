/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <OS.h>


static int gTestFD = -1;
static const size_t kFileSize = B_PAGE_SIZE * 2 + (B_PAGE_SIZE / 2);


int
map_negative_offset_test()
{
	// should fail (negative offset)
	void* ptr = mmap(NULL, B_PAGE_SIZE, PROT_READ, MAP_PRIVATE, gTestFD, -4096);
	if (ptr != MAP_FAILED) {
		printf("map-negative-offset unexpectedly succeeded!\n");
		return -1;
	}
	return 0;
}


int
resize_mapping_test()
{
	void* ptr = mmap(NULL, B_PAGE_SIZE, PROT_READ, MAP_SHARED, gTestFD, 0);
	if (ptr == MAP_FAILED)
		return -1;

	area_id area = area_for(ptr);
	if (area < B_OK)
		return -1;

	// should fail (not allowed to resize)
	if (resize_area(area, B_PAGE_SIZE * 2) == B_OK) {
		printf("resize area unexpectedly succeeded!\n");
		return -1;
	}

	if (delete_area(area) != B_OK)
		return -1;
	return 0;
}


int
map_past_end_test()
{
	void* ptr = mmap(NULL, B_PAGE_SIZE, PROT_READ, MAP_PRIVATE, gTestFD, B_PAGE_SIZE * 4);
	if (ptr == MAP_FAILED) {
		printf("map-past end 1 unexpectedly failed!\n");
		return -1;
	}

	ptr = mmap(NULL, B_PAGE_SIZE, PROT_READ, MAP_SHARED, gTestFD, B_PAGE_SIZE * 4);
	if (ptr != MAP_FAILED) {
		printf("map-past end 2 unexpectedly succeeded!\n");
		return -1;
	}
	return 0;
}


int
main()
{
	const char* fileName = "/tmp/mmap-invalid-test-file";

	// create file
	gTestFD = open(fileName, O_CREAT | O_RDWR | O_TRUNC, 0644);
	if (gTestFD < 0) {
		fprintf(stderr, "Failed to open \"%s\": %s\n", fileName,
			strerror(errno));
		return 1;
	}

	// write pages
	char buffer[2048];
	memset(buffer, 0xdd, sizeof(buffer));
	size_t written = 0;
	while (written < kFileSize) {
		if (write(gTestFD, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer)) {
			fprintf(stderr, "Failed to write to file!\n");
			return 1;
		}
		written += sizeof(buffer);
	}

	int status;

	if ((status = map_negative_offset_test()) != 0)
		return status;

	if ((status = resize_mapping_test()) != 0)
		return status;

	if ((status = map_past_end_test()) != 0)
		return status;

	unlink(fileName);
	return 0;
}
