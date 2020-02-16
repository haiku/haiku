/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OS.h>

#include <x86intrin.h>

extern const char* __progname;


static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout,
		"Usage: %s\n",
		__progname);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


int
main(int argc, const char* const* argv)
{

	if (argc > 2)
		print_usage_and_exit(true);

	if (argc > 1) {
		const char* arg = argv[1];
		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
			print_usage_and_exit(false);
	}

#ifdef __x86_64__
	uint32 cpuNum;
	__rdtscp(&cpuNum);
	printf("%" B_PRIu32 "\n", cpuNum);
	return 0;
#else
	return 1;
#endif
}
