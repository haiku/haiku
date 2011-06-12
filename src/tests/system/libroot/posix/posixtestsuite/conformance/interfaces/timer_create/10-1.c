/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 * adam li
 *
 * Test that CLOCK_PROCESS_CPUTIME_ID is supported by timer_create().
 *
 * Same test as 1-1.c.
 */

#define _XOPEN_SOURCE 600

#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "posixtest.h"

#define SIGTOTEST SIGALRM
#define TIMERSEC 2
#define SLEEPDELTA 3
#define ACCEPTABLEDELTA 1
#define CLOCK_TO_TEST CLOCK_PROCESS_CPUTIME_ID

static int handlerInvoked = 0;

static void sub_timespec(const struct timespec *a, const struct timespec *b,
	struct timespec *result)
{
	result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (result->tv_nsec < 0 && result->tv_sec > 0) {
		result->tv_sec--;
		result->tv_nsec += 1000000000;
	} else if (result->tv_nsec >= 1000000000) {
		result->tv_sec++;
		result->tv_nsec -= 1000000000;
	}
}

static int cmp_timespec(const struct timespec *a, const struct timespec *b)
{
	if (a->tv_sec < b->tv_sec)
		return -1;
	if (a->tv_sec > b->tv_sec)
		return 1;

	if (a->tv_nsec < b->tv_nsec)
		return -1;
	if (a->tv_nsec > b->tv_nsec)
		return 1;

	return 0;
}

static int spin(const struct timespec *time, struct timespec *timeLeft)
{
	struct timespec startTime;

	if (clock_gettime(CLOCK_TO_TEST, &startTime) != 0) {
		perror("clock_gettime() failed");
		exit(PTS_UNRESOLVED);
	}

	for (;;) {
		struct timespec tempTime;
		if (clock_gettime(CLOCK_TO_TEST, &tempTime) != 0) {
			perror("clock_gettime() failed");
			exit(PTS_UNRESOLVED);
		}


		sub_timespec(&tempTime, &startTime, &tempTime);
		if (cmp_timespec(time, &tempTime) <= 0)
			return 0;

		if (handlerInvoked) {
			if (timeLeft)
				sub_timespec(time, &tempTime, timeLeft);
			return -1;
		}
	}
}

void handler(int signo)
{
	printf("Caught signal\n");
	handlerInvoked = 1;
}

int main(int argc, char *argv[])
{
#if _POSIX_CPUTIME == -1
	printf("_POSIX_CPUTIME not defined\n");
	return PTS_UNSUPPORTED;
#else
	struct sigevent ev;
	struct sigaction act;
	timer_t tid;
	struct itimerspec its;
	struct timespec ts, tsleft;
	int rc;

	rc = sysconf(_SC_CPUTIME);
	printf("sysconf(_SC_CPUTIME) returns: %d\n", rc);
	if (rc <= 0) {
		return PTS_UNRESOLVED;
	}

	ev.sigev_notify = SIGEV_SIGNAL;
	ev.sigev_signo = SIGTOTEST;

	act.sa_handler=handler;
	act.sa_flags=0;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = TIMERSEC;
	its.it_value.tv_nsec = 0;

	ts.tv_sec=TIMERSEC+SLEEPDELTA;
	ts.tv_nsec=0;

	if (sigemptyset(&act.sa_mask) == -1) {
		perror("Error calling sigemptyset");
		return PTS_UNRESOLVED;
	}
	if (sigaction(SIGTOTEST, &act, 0) == -1) {
		perror("Error calling sigaction");
		return PTS_UNRESOLVED;
	}

	if (timer_create(CLOCK_TO_TEST, &ev, &tid) != 0) {
		perror("timer_create() did not return success");
		return PTS_UNRESOLVED;
	}

	if (timer_settime(tid, 0, &its, NULL) != 0) {
		perror("timer_settime() did not return success");
		return PTS_UNRESOLVED;
	}

	if (spin(&ts, &tsleft) != -1) {
		perror("spin() not interrupted");
		return PTS_FAIL;
	}

	if ( abs(tsleft.tv_sec-SLEEPDELTA) <= ACCEPTABLEDELTA) {
		printf("Test PASSED");
		return PTS_PASS;
	} else {
		printf("Timer did not last for correct amount of time\n");
		printf("timer: %d != correct %d\n",
				(int) ts.tv_sec- (int) tsleft.tv_sec,
				TIMERSEC);
		return PTS_FAIL;
	}

	return PTS_UNRESOLVED;
#endif
}
