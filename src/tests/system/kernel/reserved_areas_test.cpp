/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


//!	Test application that reproduces bug #4778.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <syscalls.h>


status_t
memory_eater(void*)
{
	size_t total = 0;

	while (true) {
		size_t size = rand() % 16384 + 8;
		if (malloc(size) == NULL) {
			printf("out of memory after having allocated %ld bytes\n", total);
			break;
		} else
			total += size;

		putchar('.');
	}

	return B_OK;
}


status_t
area_creator(void*)
{
	while (true) {
		status_t status = B_ERROR;
		uint32 addressSpec = B_ANY_ADDRESS;
		void* base;
		bool readOnly = false;//(rand() % 256) > 127;
		if (!readOnly) {
			// reserve 128 MB of space for the area
			base = (void*)0x60000000;
			status = _kern_reserve_address_range((addr_t*)&base, B_BASE_ADDRESS,
				128 * 1024 * 1024);
			addressSpec = status == B_OK ? B_EXACT_ADDRESS : B_BASE_ADDRESS;
			printf("\naddress spec = %lx, base %p (status %s)\n", addressSpec,
				base, strerror(status));
		}

		area_id area = create_area(readOnly ? "read-only memory" : "r/w memory",
			&base, addressSpec, B_PAGE_SIZE * 4, B_NO_LOCK,
			B_READ_AREA | (readOnly ? 0 : B_WRITE_AREA));
		if (area >= 0) {
			printf("new %s area %ld at %p\n", readOnly ? "read-only" : "r/w",
				area, base);
		} else
			break;

		snooze(10000);
	}

	return B_OK;
}


int
main(int argc, char** argv)
{
	thread_id eater = spawn_thread(&memory_eater, "memory eater",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(eater);

	thread_id creator = spawn_thread(&area_creator, "area creator",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(creator);

	object_wait_info waitInfos[2] = {
		{ eater, B_OBJECT_TYPE_THREAD, 0 },
		{ creator, B_OBJECT_TYPE_THREAD, 0 }
	};
	ssize_t which = wait_for_objects(waitInfos, 2);
	printf("wait for objects: %ld\n", which);

	debugger("Welcome to the land of tomorrow");
	return 0;
}
