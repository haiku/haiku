/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <syscalls.h>


status_t
looper(void *)
{
	while (true) {
		_kern_thread_yield();
	}

	return B_OK;
}


int
main()
{
	thread_id thread = spawn_thread(looper, "Real-Time Looper", B_REAL_TIME_PRIORITY, NULL);
	if (thread < B_OK)
		return -1;

	resume_thread(thread);
	wait_for_thread(thread, NULL);

	return 0;
}
