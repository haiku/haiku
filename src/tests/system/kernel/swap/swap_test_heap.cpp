/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#ifndef PAGE_SIZE
#	define PAGE_SIZE 4096
#endif

#define PAGE_ELEMENT_COUNT	(PAGE_SIZE / 4)


int
main(int argc, const char* const* argv)
{
	size_t allocationSize = 256;

	if (argc > 1) {
		allocationSize = atoi(argv[1]);
		if (allocationSize == 0) {
			fprintf(stderr, "Usage: %s [ <size in MB> ]\n", argv[0]);
			return 1;
		}
	}

	allocationSize *= 1024 * 1024;
	size_t elementCount = allocationSize / 4;
	size_t pageCount = elementCount / PAGE_ELEMENT_COUNT;

	// allocate memory
	uint32_t* allocation = (uint32_t*)malloc(allocationSize);
	if (allocation == NULL) {
		fprintf(stderr, "Allocation failed!\n");
		return 1;
	}

	printf("Allocated %lu MB at %p. Filling the allocation...\n",
		(unsigned long)allocationSize / 1024 / 1024, allocation);

	// fill the pages
	for (size_t i = 0; i < elementCount; i++) {
		allocation[i] = i;
		if ((i + 1) % (PAGE_ELEMENT_COUNT * 32) == 0) {
			printf("\rfilled %9lu/%9lu pages",
				(unsigned long)(i + 1) / PAGE_ELEMENT_COUNT,
				(unsigned long)pageCount);
			fflush(stdout);
		}
	}

	printf("\rDone filling the allocation. Starting test iterations...\n");

	for (int testIteration = 0; testIteration < 5; testIteration++) {
		sleep(1);

		printf("Test iteration %d...\n", testIteration);

		// test the pages
		for (size_t i = 0; i < elementCount; i++) {
			if (allocation[i] != i) {
				printf("  incorrect value in page %lu\n",
					(unsigned long)i / PAGE_ELEMENT_COUNT);

				// skip the rest of the page
				i = i / PAGE_ELEMENT_COUNT * PAGE_ELEMENT_COUNT
					+ PAGE_ELEMENT_COUNT - 1;
			} else if ((i + 1) % PAGE_ELEMENT_COUNT == 0) {
//				printf("  page %lu ok\n",
//					(unsigned long)i / PAGE_ELEMENT_COUNT);
			}
		}
	}

	return 0;
}
