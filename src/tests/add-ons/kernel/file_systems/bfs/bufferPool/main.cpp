#include "BufferPool.h"

#include <stdio.h>
#include <string.h>


#define BLOCK_SIZE 2048
#define NUM_THREADS 10
#define NUM_LOOPS 50


int32 gID;


int32
allocator(void *_pool)
{
	BufferPool &pool = *(BufferPool *)_pool;
	int32 id = atomic_add(&gID, 1);

	for (int32 i = 0; i < NUM_LOOPS; i++) {
		snooze(50000);

		printf("%ld. Get buffer...\n", id);
		status_t status;
		void *buffer;
		if ((status = pool.GetBuffer(&buffer)) != B_OK) {
			printf("\t%ld. Couldn't get buffer: %s\n", id, strerror(status));
			continue;
		}
		printf("\t%ld. got buffer\n", id);
		snooze(50000);
		pool.PutBuffer(buffer);
		printf("\t%ld. released buffer\n", id);
	}
	return 0;
}


int
main(int argc, char **argv)
{
	BufferPool pool;
	thread_id thread[NUM_THREADS];

	if (pool.RequestBuffers(BLOCK_SIZE) == B_OK) {
		for (int i = 0; i < NUM_THREADS; i++) {
			thread[i] = spawn_thread(allocator, "", B_NORMAL_PRIORITY, (void *)&pool);
			if (thread[i] < B_OK)
				fprintf(stderr, "Couldn't spawn thread %d\n", i);
			resume_thread(thread[i]);
		}

		for (int32 i = 0; i < NUM_THREADS; i++) {
			status_t status;
			wait_for_thread(thread[i], &status);
		}

		pool.ReleaseBuffers();
	}

	return 0;
}
