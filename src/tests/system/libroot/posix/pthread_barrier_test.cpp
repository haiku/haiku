#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define THREAD_COUNT 10
#define CYCLES 2

pthread_barrier_t mybarrier;

void* threadFn(void* id_ptr)
{
	int thread_id = *(int*)id_ptr;

	for (int i = 0; i < CYCLES; ++i) {
		int wait_sec = 1 + rand() % 10;
		printf("thread %d: Wait %d seconds.\n", thread_id, wait_sec);
		sleep(wait_sec);
		printf("thread %d: Waiting on barrier...\n", thread_id);

		int status = pthread_barrier_wait(&mybarrier);
		if (status == PTHREAD_BARRIER_SERIAL_THREAD)
			printf("thread %d: serial thread.\n", thread_id);
		printf("thread %d: Finished!\n", thread_id);
	}

	return NULL;
}


int main()
{
	pthread_t ids[THREAD_COUNT];
	int short_ids[THREAD_COUNT];

	srand(time(NULL));
	pthread_barrier_init(&mybarrier, NULL, THREAD_COUNT);

	for (int i = 0; i < THREAD_COUNT; i++) {
		short_ids[i] = i;
		pthread_create(&ids[i], NULL, threadFn, &short_ids[i]);
	}

	for (int i = 0; i < THREAD_COUNT; i++)
		pthread_join(ids[i], NULL);

	pthread_barrier_destroy(&mybarrier);

	return 0;
}

