/*
 * from ftp://ftp.netbsd.org/pub/NetBSD/misc/gmcgarry/bench/syscallbench.tar.gz
 *
 * gcc -Wall -Werror -O3 -static -o ctx ctx.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define ITERATIONS	1000000

static void
usage(void)
{
	printf("syscallbench [-h]\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct timeval before, after;
	unsigned long overhead, elapsed;
	int i;
	pid_t pid;
	
	if (argc > 1)
		usage();

	gettimeofday(&before, NULL);
	for (i=0; i<ITERATIONS; i++) {
	}
	gettimeofday(&after, NULL);
	overhead = 1000000 * (after.tv_sec - before.tv_sec);
	overhead += after.tv_usec - before.tv_usec;
	
	gettimeofday(&before, NULL);
	for (i=0; i<ITERATIONS; i++) {
		pid = getpid();
	}
	gettimeofday(&after, NULL);
	elapsed = 1000000 * (after.tv_sec - before.tv_sec);
	elapsed += after.tv_usec - before.tv_usec;

	printf("syscall time: %ld microseconds\n",
	    (1000*(elapsed-overhead))/ITERATIONS);

	return (0);
}
