/*
 * from ftp://ftp.netbsd.org/pub/NetBSD/misc/gmcgarry/bench/execbench.tar.gz
 */
#define errx(x,y...) { fprintf(stderr, y); fprintf(stderr, "\n"); exit(x); }

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	struct timeval before, after;
	unsigned long time, elapsed;
	char timestr[12], iterstr[12];
	char *timeptr, *countptr;
        int iter, count;

	if (argc < 2)
		errx(1, "Usage: %s iterations", argv[0]);

	iter = atoi(argv[1]);
	if (iter > 0) {  
		gettimeofday(&before, NULL);
		time = 1000000 * before.tv_sec + before.tv_usec;
		sprintf(timestr,"%lu", time);
		timeptr = timestr;
		countptr = argv[1];
	} else {
		iter = atoi(argv[2]);
		timeptr = argv[3];
		countptr = argv[4];
	} 

	if (iter != 0) {
		iter--;
		sprintf(iterstr, "%d", iter);
		execl(argv[0], argv[0], "0", iterstr, timeptr, countptr, NULL);
		errx(1, "execl failed");
	}

	gettimeofday(&after, NULL);
	sscanf(argv[3],"%lu", &time);
	count = atoi(argv[4]);
	elapsed = 1000000 * after.tv_sec + after.tv_usec;
	elapsed -= time;

	printf("time: %lu microseconds\n", elapsed / count);

	return (1);
}
