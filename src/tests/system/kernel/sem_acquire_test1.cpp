#include <stdio.h>
#include <string.h>

#include <OS.h>


static sem_id sSemaphore = -1;


static status_t
thread_function1(void* data)
{
	const char* threadName = (const char*)data;
	printf("%s: going to acquire sem...\n", threadName);
	status_t error = acquire_sem_etc(sSemaphore, 2, 0, 0);
	printf("%s: acquire_sem_etc() returned: %s\n", threadName, strerror(error));
	return error;
}


static status_t
thread_function2(void*)
{
	printf("thread2: going to acquire sem...\n");
	status_t error = acquire_sem_etc(sSemaphore, 1, 0, 0);
	printf("thread2: acquire_sem_etc() returned: %s\n", strerror(error));
	return error;
}



int
main()
{
	sSemaphore = create_sem(1, "test sem");

	thread_id thread1 = spawn_thread(&thread_function1, "thread1",
		B_NORMAL_PRIORITY, (void*)"thread1");
	resume_thread(thread1);

	snooze(100000);

	thread_id thread2 = spawn_thread(&thread_function2, "thread2",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(thread2);

	snooze(100000);

	thread_id thread3 = spawn_thread(&thread_function1, "thread3",
		B_NORMAL_PRIORITY, (void*)"thread3");
	resume_thread(thread3);

	snooze(100000);

	int32 semCount = 42;
	get_sem_count(sSemaphore, &semCount);
	printf("sem count: %ld\n", semCount);

	printf("killing thread1...\n");
	kill_thread(thread1);

	snooze(2000000);

	return 0;
}
