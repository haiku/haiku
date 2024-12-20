/*
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include <OS.h>

void*
threadFn(void* ptr)
{
	snooze(1000000);
	return NULL;
}


int
main()
{
	pthread_t t;
	pthread_create(&t, NULL, threadFn, NULL);

	clockid_t c;
	if (pthread_getcpuclockid(t, &c) != 0)
		return 1;
	timespec ts;
	if (clock_gettime(c, &ts) != 0)
		return 1;

	if (clock_getcpuclockid(getpid(), &c) != 0)
		return 1;
	if (clock_gettime(c, &ts) != 0)
		return 1;

	if (pthread_join(t, NULL) != 0)
		return 1;
	return 0;
}
