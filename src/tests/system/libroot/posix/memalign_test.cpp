#include <SupportDefs.h>
#include <OS.h>

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef MALLOC_DEBUG
void dump_heap_list(int argc, char **argv);
void dump_allocations(bool statsOnly, thread_id thread);
#endif


inline uint8
sum(addr_t address)
{
	return (address >> 24) | (address >> 16) | (address >> 8) | address;
}


inline void
write_test_pattern(void *address, size_t size)
{
	for (size_t i = 0; i < size; i++)
		*((uint8 *)address + i) = sum((addr_t)address + i);
}


inline void
verify_test_pattern(void *address, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		if (*((uint8 *)address + i) != sum((addr_t)address + i)) {
			printf("test patern invalid at %p: %u vs. %u\n",
				(uint8 *)address + i, *((uint8 *)address + i),
				sum((addr_t)address + i));
			exit(1);
		}
	}
}


void
allocate_random_no_alignment(int32 count, size_t maxSize)
{
	void **allocations = new void *[count];
	size_t *sizes = new size_t[count];
	for (int32 i = 0; i < count; i++) {
		sizes[i] = rand() % maxSize;
		allocations[i] = malloc(sizes[i]);
		if (allocations[i] == NULL) {
			printf("allocation of %lu bytes failed\n", sizes[i]);
			exit(1);
		}

		write_test_pattern(allocations[i], sizes[i]);
	}

	for (int32 i = count - 1; i >= 0; i--) {
		verify_test_pattern(allocations[i], sizes[i]);
		free(allocations[i]);
	}

	delete[] allocations;
	delete[] sizes;
}


void
allocate_random_fixed_alignment(int32 count, size_t maxSize, size_t alignment)
{
	void **allocations = new void *[count];
	size_t *sizes = new size_t[count];
	for (int32 i = 0; i < count; i++) {
		sizes[i] = rand() % maxSize;
		allocations[i] = memalign(alignment, sizes[i]);
		if (allocations[i] == NULL) {
			printf("allocation of %lu bytes failed\n", sizes[i]);
			exit(1);
		}

		if ((addr_t)allocations[i] % alignment != 0) {
			printf("allocation of %lu bytes misaligned: %p -> 0x%08lx "
				" with alignment %lu (0x%08lx)\n", sizes[i],
				allocations[i], (addr_t)allocations[i] % alignment, alignment,
				alignment);
			exit(1);
		}

		write_test_pattern(allocations[i], sizes[i]);
	}

	for (int32 i = count - 1; i >= 0; i--) {
		verify_test_pattern(allocations[i], sizes[i]);
		free(allocations[i]);
	}

	delete[] allocations;
	delete[] sizes;
}


void
allocate_random_random_alignment(int32 count, size_t maxSize)
{
	for (int32 i = 0; i < count / 128; i++)
		allocate_random_fixed_alignment(128, maxSize, 1 << (rand() % 18));
}


int
main(int argc, char *argv[])
{
	allocate_random_no_alignment(1024, B_PAGE_SIZE * 128);
	allocate_random_random_alignment(1024, B_PAGE_SIZE * 128);

#ifdef MALLOC_DEBUG
	dump_heap_list(0, NULL);
	dump_allocations(false, -1);
#endif

	printf("tests succeeded\n");
	return 0;
}
