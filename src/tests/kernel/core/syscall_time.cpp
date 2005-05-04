/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <stdio.h>

#ifdef __HAIKU__
#	include <syscalls.h>
#else
extern "C" void _kclose_(int fd);
#endif


int
main(int argc, char **argv)
{
	const int32 loops = 100000;
	bigtime_t startTime = system_time();

	for (int32 i = 0; i < loops; i++) {
#ifdef __HAIKU__
//		_kern_null();
		_kern_close(-1);
#else
		_kclose_(-1);
#endif
	}

	bigtime_t runTime = system_time() - startTime;

	// empty loop time

	startTime = system_time();

	for (int32 i = 0; i < loops; i++)
		;

	runTime -= system_time() - startTime;

	printf("%f usecs/syscall\n", 1.0 * runTime / loops);
	return 0;
}
