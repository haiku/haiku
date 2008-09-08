/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <sys/stat.h>

#include <OS.h>


static void
time_lstat(const char* path)
{
	printf("%-60s ...", path);
	fflush(stdout);
	bigtime_t startTime = system_time();

	static const int32 iterations = 10000;
	for (int32 i = 0; i < iterations; i++) {
		struct stat st;
		lstat(path, &st);
	}

	bigtime_t totalTime = system_time() - startTime;
	printf(" %5.3f us/call\n", (double)totalTime / iterations);
}


int
main()
{
	const char* const paths[] = {
		"/",
		"/boot",
		"/boot/develop",
		"/boot/develop/headers",
		"/boot/develop/headers/posix",
		"/boot/develop/headers/posix/sys",
		"/boot/develop/headers/posix/sys/stat.h",
		NULL
	};

	for (int32 i = 0; paths[i] != NULL; i++)
		time_lstat(paths[i]);

	return 0;
}
