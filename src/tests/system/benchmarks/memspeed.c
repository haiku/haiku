/*
 * from ftp://ftp.netbsd.org/pub/NetBSD/misc/gmcgarry/bench/memspeed.c
 *
 *  Compilation:
 *
 *    gcc -O3 -fomit-frame-pointer memspeed.c -o memspeed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/times.h>
#include <time.h>
#include <sys/types.h>

#define KB		(1024)
#define MB		(KB*KB)

#define TESTSIZE	(8*MB)
#define LOOPSIZE	(256*MB)

int main(int argc, char *argv[])
{
    u_long mb = TESTSIZE;
    u_long size, passes, d, i, j;
    volatile u_long *mem;
    struct tms tms;
    time_t start, stop;

    switch (argc) {
	case 2:
	    mb = atol(argv[1])*MB;
	case 1:
	    break;

	default:
	    fprintf(stderr, "Usage: %s megabytes\n", argv[0]);
	    exit(1);
	    break;
    }

    mem = malloc(mb);

    fprintf(stderr, "*** MEMORY WRITE PERFORMANCE (%d MB LOOP) ***\n",
	    LOOPSIZE/MB);
    for (size = 64; size <= mb; size <<= 1) {
	passes = LOOPSIZE/size;
	fprintf(stderr, "size = %9ld bytes: ", size);
	times(&tms);
	start = tms.tms_utime;
	for (i = 0; i < passes; i++)
	    for (j = 0; j < size/sizeof(u_long); j += 16) {
		mem[j] = 0; mem[j+1] = 0; mem[j+2] = 0; mem[j+3] = 0;
		mem[j+4] = 0; mem[j+5] = 0; mem[j+6] = 0; mem[j+7] = 0;
		mem[j+8] = 0; mem[j+9] = 0; mem[j+10] = 0; mem[j+11] = 0;
		mem[j+12] = 0; mem[j+13] = 0; mem[j+14] = 0; mem[j+15] = 0;
	    }
	times(&tms);
	stop = tms.tms_utime;
	fprintf(stderr, "%5.3f MB/s\n",
		(double)(LOOPSIZE/MB)/(double)(stop-start)*(double)CLK_TCK);
    }
    fprintf(stderr, "*** MEMORY READ PERFORMANCE (%d MB LOOP) ***\n",
	    LOOPSIZE/MB);
    for (size = 64; size <= mb; size <<= 1) {
	passes = LOOPSIZE/size;
	fprintf(stderr, "size = %9ld bytes: ", size);
	times(&tms);
	start = tms.tms_utime;
	for (i = 0; i < passes; i++)
	    for (j = 0; j < size/sizeof(u_long); j += 16) {
		d = mem[j]; d = mem[j+1]; d = mem[j+2]; d = mem[j+3];
		d = mem[j+4]; d = mem[j+5]; d = mem[j+6]; d = mem[j+7];
		d = mem[j+8]; d = mem[j+9]; d = mem[j+10]; d = mem[j+11];
		d = mem[j+12]; d = mem[j+13]; d = mem[j+14]; d = mem[j+15];
	    }
	times(&tms);
	stop = tms.tms_utime;
	fprintf(stderr, "%5.3f MB/s\n",
		(double)(LOOPSIZE/MB)/(double)(stop-start)*(double)CLK_TCK);
    }
    exit(0);
}
