/* tests TLS (thread locale storage) functionality */

/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <TLS.h>

#include <stdio.h>
#include <string.h>


sem_id gWait;
int32 gSlot;


#if __INTEL__
static void *
read_fs_register(void)
{
	void *fs;

	asm("movl %%fs, %0" : "=r" (fs));
	return fs;
}
#endif


static int32
thread1_func(void *data)
{
	printf("1. stack at %p, thread id = %ld\n", &data, find_thread(NULL));
	printf("1. TLS address = %p\n", tls_address(gSlot));
	tls_set(gSlot, (void *)"thread 1");
	printf("1. TLS get = \"%s\" (should be \"thread 1\")\n", (char *)tls_get(gSlot));

	acquire_sem(gWait);

	printf("1. TLS get = \"%s\" (should be \"thread 1\")\n", (char *)tls_get(gSlot));

	return 0;
}


static int32
thread2_func(void *data)
{
	printf("2. stack at %p, thread id = %ld\n", &data, find_thread(NULL));
	printf("2. TLS address = %p\n", tls_address(gSlot));
	printf("2. TLS get = \"%s\" (should be NULL)\n", (char *)tls_get(gSlot));

	acquire_sem(gWait);

	printf("2. TLS get = \"%s\" (should be NULL)\n", (char *)tls_get(gSlot));

	return 0;
}


int
main(int argc, char **argv)
{
	thread_id thread1, thread2;
	status_t returnCode;

	gWait = create_sem(0, "waiter");
	gSlot = tls_allocate();

	printf("got slot %ld, stack at %p, thread id = %ld\n", gSlot, &thread1, find_thread(NULL));

#if __INTEL__
	printf("FS register is %p\n", read_fs_register());
#endif
	printf("TLS address in main = %p\n", tls_address(gSlot));

	tls_set(gSlot, (void *)"main thread");

	thread1 = spawn_thread(thread1_func, "Thread 1" , B_NORMAL_PRIORITY, NULL);
	resume_thread(thread1);
	thread2 = spawn_thread(thread2_func, "Thread 2" , B_NORMAL_PRIORITY, NULL);
	resume_thread(thread2);

	printf("TLS in main = \"%s\" (should be \"main thread\")\n", (char *)tls_get(gSlot));

	// shut down
	snooze(100000);
	delete_sem(gWait);

	wait_for_thread(thread1, &returnCode);
	wait_for_thread(thread2, &returnCode);

	return 0;
}

