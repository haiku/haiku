/*
 * gcc -Wall -Werror -O3 -static -o ctx ctx.c
 */

#define errx(x,y...) { fprintf(stderr, y); fprintf(stderr, "\n"); exit(x); }

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#if defined (LWP)
#include <sys/ucontext.h>
#include <sys/lwp.h>
#endif

#define ITERATIONS	8192
#define PASSES		50

int fd0[2], fd1[2];
unsigned long elapsed_times[ITERATIONS];
unsigned long overhead;
pid_t childpid;

static void
usage(void)
{
	printf("ctx [-hl]\n");
	exit(1);
}

static void
child(void)
{
	int ch;

	ch = 0;
	if (write(fd1[1], &ch, 1) != 1)
		errx(1, "child write failed");
	while (1) {
		if (read(fd0[0], &ch, 1) != 1)
			errx(1, "child read failed");
		if (write(fd1[1], &ch, 1) != 1)
			errx(1, "child write failed");
	}
}

static void
dump_results(void)
{
	unsigned long min_time, max_time, sum;
	int i;

	min_time = elapsed_times[0];
	max_time = elapsed_times[0];
	sum = 0;
	for (i=1; i<ITERATIONS; i++) {
		if (elapsed_times[i] < min_time)
			min_time = elapsed_times[i];
		if (elapsed_times[i] > max_time)
			max_time = elapsed_times[i];
		sum += elapsed_times[i] - overhead;
	}
	min_time -= overhead;
	max_time -= overhead;

	printf("min latency: %f\n", (double)min_time / PASSES);
	printf("max latency: %f\n", (double)max_time / PASSES);
	printf("mean latency: %f\n", (double)sum / ITERATIONS / PASSES);
}

int
main(int argc, char *argv[])
{
	int i, ch, count;
	struct timeval before, after;
	unsigned long elapsed;
	int use_lwps = 0;

	memset(elapsed_times, 0, ITERATIONS);

	while ((ch = getopt(argc, argv, "hl")) != -1) {
		switch (ch) {
		case 'l':
#if defined(LWP)
			use_lwps = 1;
#else
			errx(1, "not supported");
#endif
			break;
		case 'h':
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	sleep(1);

	if (pipe(fd0) != 0)
		errx(1, "Unable to create pipe");
	if (pipe(fd1) != 0)
		errx(1, "Unable to create pipe");

	/*
	 * Determine overhead
	 */
	for (count=0; count<2; count++) {
		gettimeofday(&before, NULL);
		for (i=0; i<2*(PASSES/2); i++) {
			ch = 0;
			write(fd0[1], &ch, 1);
			read(fd0[0], &ch, 1);
		}
		gettimeofday(&after, NULL);
		overhead = 1000000 * (after.tv_sec - before.tv_sec);
		overhead += after.tv_usec - before.tv_usec;
	}

	if (use_lwps) {
#if defined(LWP)
		ucontext_t u;
		ucontext_t *contextp;
		int stacksize = 65536;
		void *stackbase;
		lwpid_t l;
		int error;

		getcontext(&u);
		contextp = (ucontext_t *)malloc(sizeof(ucontext_t));
		stackbase = malloc(stacksize);
		sigprocmask(SIG_SETMASK, NULL, &contextp->uc_sigmask);
		_lwp_makecontext(contextp, child, NULL, NULL,
			stackbase, stacksize);
		error = _lwp_create(contextp, 0, &l);
		if (error)
			errx(1, "error _lwp_create");
#endif
	} else {
		switch (childpid = fork()) {
		case 0:		/* child */
			child();
		case -1:	/* error */
			errx(1, "error forking");
			break;
		}
	}

	ch = 0;
	if (read(fd1[0], &ch, 1) != 1)
		errx(1, "parent read failed");
	for (count=0; count<ITERATIONS; count++) {
		gettimeofday(&before, NULL);
		for (i=0; i<PASSES/2; i++) {
			ch = 0;
			if (write(fd0[1], &ch, 1) != 1)
				errx(1, "parent write failed");
			if (read(fd1[0], &ch, 1) != 1)
				errx(1, "parent read failed");
		}
		gettimeofday(&after, NULL);
		elapsed = 1000000 * (after.tv_sec - before.tv_sec);
		elapsed += after.tv_usec - before.tv_usec;
		elapsed_times[count] = elapsed;
	}

	if (!use_lwps)
		kill(childpid, SIGTERM);
	dump_results();

	return (0);
}


/*
 * PMAX_SA:
 *
 * min latency: 93.100000
 * max latency: 150.700000
 * mean latency: 100.857581
 *
 * PMAX_CHOOSEPROC:
 *
 * min latency: 49.350000
 * max latency: 76.750000
 * mean latency: 54.141626
 *
 * PMAX_OLD:
 *
 * min latency: 54.750000
 * max latency: 76.050000
 * mean latency: 60.088654
 *
 * HP300_SA:
 *
 * min latency: 352.560000
 * max latency: 402.960000
 * mean latency: 367.836250
 *
 * HP300_CHOOSEPROC:
 *
 * min latency: 129.200000
 * max latency: 187.040000
 * mean latency: 142.528223
 *
 * HP300_OLD:
 *
 * min latency: 357.360000
 * max latency: 414.400000
 * mean latency: 372.436104
 *
 */
