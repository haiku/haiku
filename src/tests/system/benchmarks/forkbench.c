/*
 * from ftp://ftp.netbsd.org/pub/NetBSD/misc/gmcgarry/bench/forkbench.tar.gz
 */

/* From 4.4 BSD sys/tests/benchmarks/forks.c. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/wait.h>

/*
 * Benchmark program to calculate fork+wait
 * overhead (approximately).  Process
 * forks and exits while parent waits.
 * The time to run this program is used
 * in calculating exec overhead.
 */

int
main(argc, argv)
        char *argv[];
{
        int nforks, i;
        char *cp;
        int pid, child, status, brksize;
        struct timeval before, after;
	unsigned elapsed;

        if (argc < 3) {
                printf("usage: %s number-of-forks sbrk-size\n", argv[0]);
                exit(1);
        }
        nforks = atoi(argv[1]);
        if (nforks < 0) {
                printf("%s: bad number of forks\n", argv[1]);
                exit(2);
        }
        brksize = atoi(argv[2]);
        if (brksize < 0) {
                printf("%s: bad size to sbrk\n", argv[2]);
                exit(3);
        }

        gettimeofday(&before, NULL);
        cp = (char *)sbrk(brksize);
        if (cp == (void *)-1) {
                perror("sbrk");
                exit(4);
        }
        for (i = 0; i < brksize; i += 1024)
                cp[i] = i;
	for (i=0; i<nforks; i++) {
                child = fork();
                if (child == -1) {
                        perror("fork");
                        exit(-1);
                }
                if (child == 0)
                        _exit(-1);
                while ((pid = wait(&status)) != -1 && pid != child)
                        ;
        }
        gettimeofday(&after, NULL);
	elapsed = 1000000 * (after.tv_sec - before.tv_sec);
	elapsed += (after.tv_usec - before.tv_usec);
        printf ("Time: %d microseconds.\n", elapsed/nforks);
        exit(0);
}
