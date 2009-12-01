/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/heap.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>


extern "C" void* heap_malloc(size_t size);
extern "C" void* heap_realloc(void* oldBuffer, size_t size);
extern "C" void heap_free(void* buffer);
extern void dump_chunks(void);
extern uint32 heap_available(void);


const int32	kHeapSize = 32 * 1024;

int32 gVerbosity = 1;


void
platform_release_heap(struct stage2_args* args, void* base)
{
	free(base);
}


status_t
platform_init_heap(struct stage2_args* args, void** _base, void** _top)
{
	void* base = malloc(kHeapSize);
	if (base == NULL)
		return B_NO_MEMORY;

	*_base = base;
	*_top = (void*)((uint8*)base + kHeapSize);

	return B_OK;
}


void
panic(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	exit(-1);
}


void
dprintf(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}


// #pragma mark -


static void
dump_allocated_chunk(int32 index, void* buffer)
{
	if (buffer == NULL || gVerbosity < 3)
		return;

	size_t* size = (size_t*)((uint8*)buffer - sizeof(uint32));
	printf("\t%ld. allocation at %p, chunk at %p, size = %ld\n", index, buffer,
		size, *size);

	if (gVerbosity > 3)
		dump_chunks();
}


static void*
test_malloc(size_t bytes)
{
	return heap_malloc(bytes);
}


static void*
test_realloc(void* oldBuffer, size_t size)
{
	return heap_realloc(oldBuffer, size);
}


static void
test_free(void* buffer)
{
	if (gVerbosity > 4) {
		printf("\tfreeing buffer at %p\n", buffer);
		dump_allocated_chunk(-1, buffer);
	}

	heap_free(buffer);

	if (gVerbosity > 4) {
		puts("\t- after:");
		dump_chunks();
	}
}


static int32
random_allocations(void* array[], size_t maxSize)
{
	printf("* random allocations (up to %ld bytes)\n", maxSize);

	size_t total = 0;
	int32 count = 0;

	for (int32 i = 0; i < 100; i++) {
		size_t size = size_t(rand() * 1. * maxSize / RAND_MAX);
		array[i] = test_malloc(size);
		if (array[i] == NULL) {
			if ((size > heap_available() || size == 0) && gVerbosity < 2)
				continue;

			printf(	"%ld. allocating %ld bytes failed (%ld bytes total allocated, "
					"%ld free (%ld))\n",
					i, size, total, heap_available(), kHeapSize - total);
		} else {
			dump_allocated_chunk(i, array[i]);

			total += size;
			count++;
		}
	}

	printf("\t%ld bytes allocated\n", total);
	if (gVerbosity > 3)
		dump_chunks();

	return count;
}


int
main(int argc, char** argv)
{
	if (argc > 1)
		gVerbosity = atoi(argv[1]);

	stage2_args args;
	memset(&args, 0, sizeof(args));
	args.heap_size = kHeapSize;

	if (heap_init(&args) < B_OK) {
		fprintf(stderr, "Could not initialize heap.\n");
		return -1;
	}

	printf("heap size == %ld\n", kHeapSize);
	if (gVerbosity > 2)
		dump_chunks();

	puts("* simple allocation of 100 * 128 bytes");
	void* array[100];
	for (int32 i = 0; i < 100; i++) {
		array[i] = test_malloc(128);
		dump_allocated_chunk(i, array[i]);
	}

	if (gVerbosity > 2)
		dump_chunks();

	puts("* testing different deleting order");
	if (gVerbosity > 2)
		puts("- free 30 from the end (descending):");

	for (int32 i = 100; i-- > 70; ) {
		test_free(array[i]);
		array[i] = NULL;
	}

	if (gVerbosity > 2) {
		dump_chunks();
		puts("- free 40 from the middle (ascending):");
	}

	for (int32 i = 30; i < 70; i++) {
		test_free(array[i]);
		array[i] = NULL;
	}

	if (gVerbosity > 2) {
		dump_chunks();
		puts("- free 30 from the start (ascending):");
	}

	for (int32 i = 0; i < 30; i++) {
		test_free(array[i]);
		array[i] = NULL;
	}

	if (gVerbosity > 2)
		dump_chunks();

	puts("* allocate until it fails");
	int32 i = 0;
	for (i = 0; i < 100; i++) {
		array[i] = test_malloc(kHeapSize / 64);
		if (array[i] == NULL) {
			printf("\tallocation %ld failed - could allocate %ld bytes (64th should fail).\n", i + 1, (kHeapSize / 64) * (i + 1));

			if (gVerbosity > 2)
				dump_chunks();

			while (i-- > 0) {
				test_free(array[i]);
				array[i] = NULL;
			}

			break;
		} else
			dump_allocated_chunk(i, array[i]);
	}
	if (i == 100)
		fprintf(stderr, "could allocate more memory than in heap\n");

	random_allocations(array, 768);

	puts("* free memory again");
	for (i = 0; i < 100; i++) {
		test_free(array[i]);
		array[i] = NULL;
	}

	for (size_t amount = 32; amount < 1024; amount *= 2) {
		int32 count = random_allocations(array, amount);

		puts("* random freeing");
		while (count) {
			i = int32(rand() * 100. / RAND_MAX);
			if (array[i] == NULL)
				continue;

			test_free(array[i]);
			array[i] = NULL;
			count--;

			if (gVerbosity > 2) {
				puts("- freed one");
				dump_chunks();
			}
		}
	}

	puts("* realloc() test");

	uint8* buffer = (uint8*)test_malloc(1);
	buffer[0] = 'h';

	uint8* newBuffer = (uint8*)test_realloc(buffer, 2);
	if (newBuffer != buffer)
		panic("  could not reuse buffer");
	newBuffer[1] = 'a';
	newBuffer = (uint8*)test_realloc(buffer, 3);
	if (newBuffer != buffer)
		panic("  could not reuse buffer");
	newBuffer[2] = 'i';
	newBuffer = (uint8*)test_realloc(buffer, 4);
	if (newBuffer != buffer)
		panic("  could not reuse buffer");
	newBuffer[3] = 'k';
	newBuffer = (uint8*)test_realloc(buffer, 5);
	if (newBuffer == buffer)
		panic("  could reuse buffer!");
	newBuffer[4] = 'u';
	if (memcmp(newBuffer, "haiku", 5))
		panic("  contents differ!");

	heap_release(&args);
	return 0;
}

