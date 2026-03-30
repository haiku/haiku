/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <fs_attr.h>
#include <TypeConstants.h>
#include <pthread.h>


/*! This test tries to reproduce races between file_cache_read and
	file_cache_set_size. If the file_cache can't discard busy pages, or fails
	to recheck the cache size, then this can result in various deadlocks or
	assertion failures. */


const char* tmpfile = "__file";


static void*
writer(void* _data)
{
	rename_thread(find_thread(NULL), "writer");

	int fd = *(int*)_data;
	char buffer[2048];

	while (true) {
		ftruncate(fd, 0);
		pwrite(fd, buffer, sizeof(buffer), 0);

		// re-open the file with NOCACHE to evict all pages from the cache
		int nocached = open(tmpfile, O_RDWR | O_NOCACHE);
		close(nocached);
	}

	return NULL;
}


static void*
reader(void* _data)
{
	rename_thread(find_thread(NULL), "reader");

	int fd = *(int*)_data;

	char buffer[1];
	while (true)
		pread(fd, &buffer, sizeof(buffer), 1);

	return NULL;
}


int
main()
{
	int file = open(tmpfile, O_CREAT | O_TRUNC | O_RDWR);
	if (file < 0)
		return -1;

	pthread_t threads[2];
	pthread_create(&threads[0], NULL, writer, &file);
	pthread_create(&threads[1], NULL, reader, &file);

	pthread_join(threads[0], NULL);

	return 0;
}
