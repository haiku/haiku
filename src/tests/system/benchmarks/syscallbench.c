/*
 * from ftp://ftp.netbsd.org/pub/NetBSD/misc/gmcgarry/bench/syscallbench.tar.gz
 *
 * gcc -Wall -Werror -O3 -static -o ctx ctx.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <OS.h>

#define ITERATIONS	500000

static void
usage(void)
{
	printf("syscallbench [-h]\n");
	exit(1);
}

static int null_func(void)
{
	return 0;
}

static unsigned long test_func(int (*func)(void))
{
	struct timeval before, after;
	unsigned long elapsed;
	int i;

	gettimeofday(&before, NULL);
	for (i=0; i<ITERATIONS; i++) {
		func();
	}
	gettimeofday(&after, NULL);

	elapsed = 1000000 * (after.tv_sec - before.tv_sec);
	elapsed += after.tv_usec - before.tv_usec;
	return elapsed;
}

int
main(int argc, char *argv[])
{
	unsigned long overhead;
	unsigned long libcall;
	unsigned long syscall;
	
	if (argc > 1)
		usage();

	overhead = test_func(&null_func);
	libcall = test_func((void *)&getpid); // getpid is currently implemented as a library function returning the value of a global
	syscall = test_func((void *)&is_computer_on);

	printf("overhead time: %ld nanoseconds\n",
	    (1000*(overhead))/ITERATIONS);

	printf("libcall time: %ld nanoseconds\n",
	    (1000*(libcall-overhead))/ITERATIONS);

	printf("syscall time: %ld nanoseconds\n",
	    (1000*(syscall-overhead))/ITERATIONS);

	return (0);
}
