/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <OS.h>


#define THREAD_COUNT	20


status_t
read_thread(void* _data)
{
	port_id port = (port_id)_data;

	printf("[%ld] read port...\n", find_thread(NULL));
	
	while (true) {
		ssize_t bytes = port_buffer_size(port);
		printf("[%ld] buffer size %ld waiting\n", find_thread(NULL), bytes);

		char buffer[256];
		int32 code;
		bytes = read_port(port, &code, buffer, sizeof(buffer));
		printf("[%ld] read port result (code %lx): %s\n", find_thread(NULL),
			code, strerror(bytes));
		if (bytes >= 0)
			break;
	}

	return B_OK;
}


int
main()
{
	port_id port = create_port(1, "test port");
	printf("created port %ld\n", port);

	thread_id threads[THREAD_COUNT];
	for (int32 i = 0; i < THREAD_COUNT; i++) {
		threads[i] = spawn_thread(read_thread, "read thread", B_NORMAL_PRIORITY,
			(void*)port);
		resume_thread(threads[i]);
	}

	printf("snooze for a bit, all threads should be waiting now.\n");
	snooze(100000);

	for (int32 i = 0; i < THREAD_COUNT; i++) {
		size_t bytes = 20 + i;
		char buffer[bytes];
		memset(buffer, 0x55, bytes);

		printf("send %ld bytes\n", bytes);
		write_port(port, 0x42, buffer, bytes);
		snooze(10000);
	}

	printf("waiting for threads to terminate\n");
	for (int32 i = 0; i < THREAD_COUNT; i++) {
		wait_for_thread(threads[i], NULL);
	}

	return 0;
}
