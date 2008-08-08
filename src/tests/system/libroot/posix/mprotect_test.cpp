/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/mman.h>


#ifndef PAGE_SIZE
#	define PAGE_SIZE 4096
#endif


static const size_t kMapChunkSize = 4 * PAGE_SIZE;
static const size_t kTestSize = 256 * PAGE_SIZE;

static int64_t sHandledSignals = 0;

static uint8_t* sMappedBase;
static size_t sMappedSize;
static uint8_t* sTouchedAddress;


static void
signal_handler(int signal)
{
	sHandledSignals++;

	//printf("SIGSEGV at %p\n", sTouchedAddress);

	// protect the last page of the current allocation writable
	if (mprotect(sMappedBase + sMappedSize - PAGE_SIZE, PAGE_SIZE,
			PROT_READ | PROT_WRITE) < 0) {
		fprintf(stderr, "SIGSEGV: mprotect() failed: %s\n", strerror(errno));
		exit(1);
	}

	// allocate the next chunk
	void* mappedAddress = mmap(sMappedBase + sMappedSize, kMapChunkSize,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
	if (mappedAddress == MAP_FAILED) {
		fprintf(stderr, "SIGSEGV: mmap() failed: %s\n", strerror(errno));
		exit(1);
	}

	printf("mapped %d bytes at %p\n", (int)kMapChunkSize, mappedAddress);

	sMappedSize += kMapChunkSize;

	// map the last page read-only
	if (mprotect(sMappedBase + sMappedSize - PAGE_SIZE, PAGE_SIZE, PROT_READ)
			< 0) {
		fprintf(stderr, "SIGSEGV: mprotect() failed: %s\n", strerror(errno));
		exit(1);
	}
}


int
main()
{
	// install signal handler
	if (signal(SIGSEGV, signal_handler) == SIG_ERR) {
		fprintf(stderr, "Error: Failed to install signal handler: %s\n",
			strerror(errno));
		exit(1);
	}

	// Map the complete test size plus one chunk and unmap all but the first
	// chunk again, so no other memory gets into the way, when we mmap() the
	// other chunks with MAP_FIXED.
	sMappedBase = (uint8_t*)mmap(NULL, kTestSize + kMapChunkSize,
		PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (sMappedBase == MAP_FAILED) {
		fprintf(stderr, "mmap() failed: %s\n", strerror(errno));
		return 1;
	}
	munmap(sMappedBase + kMapChunkSize, kTestSize);

	sMappedSize = kMapChunkSize;

	printf("mapped %d bytes at %p\n", (int)sMappedSize, sMappedBase);

	if (mprotect(sMappedBase + sMappedSize - PAGE_SIZE, PAGE_SIZE, PROT_READ)
			< 0) {
		fprintf(stderr, "mprotect() failed: %s\n", strerror(errno));
		return 1;
	}

	for (int i = 0; i < 256 * PAGE_SIZE; i++) {
		sTouchedAddress = sMappedBase + i;
		*sTouchedAddress = 1;
	}

	printf("test finished successfully!\n");

	return 0;
}
