/*
 * Copyright (C) 1996 Be, Inc.
 * All Rights Reserved
 * 
 * This source code was published by Be, Inc. in the file gnu_x86.tar.gz for R3,
 * a mirror is at http://linux.inf.elte.hu/ftp/beos/development/gnu/r3.0/
 * needs to link to termcap.
 * The GPL should apply here, since AFAIK termcap is GPLed.
 */

/*
 * Top -- a program for finding the top cpu-eating threads
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <OS.h>
#include <termcap.h>
#include <termios.h>

static const char IDLE_NAME[] = "idle thread ";
static bigtime_t lastMeasure = 0;

/*
 * Keeps track of a single thread's times 
 */
typedef struct {
	thread_id thid;
	bigtime_t user_time;
	bigtime_t kernel_time;
} thread_times_t;

/*
 * Keeps track of all the threads' times
 */
typedef struct {
	int nthreads;
	int maxthreads;
	thread_times_t *thread_times;
} thread_time_list_t;


#define FREELIST_SIZE 3
static thread_time_list_t freelist[FREELIST_SIZE];

static char *clear_string; /* output string for clearing the screen */
static int rows;	/* how many rows on the screen */
static int screen_size_changed = 0;	/* tells to refresh the screen size */
static int cpus;	/* how many cpus we are runing on */

/* SIGWINCH handler */
static void
winch_handler(int notused)
{
	(void) notused;
	screen_size_changed = 1;
}

/*
 * Grow the list to add just one more entry
 */
static void
grow(thread_time_list_t *times)
{
	int i;

	if (times->nthreads == times->maxthreads) {
		times->thread_times = realloc(times->thread_times,
									  (sizeof(times->thread_times[0]) *
									   (times->nthreads + 1)));
		times->maxthreads = times->nthreads + 1;
	}
	i = times->nthreads;
	times->thread_times[i].thid = -1;
	times->thread_times[i].user_time = 0;
	times->thread_times[i].kernel_time = 0;
	times->nthreads++;
}

static void
init_times(thread_time_list_t *times)
{
	int i;

	for (i = 0; i < FREELIST_SIZE; i++) {
		if (freelist[i].nthreads == 0) {
			*times = freelist[i];
			freelist[i].nthreads = 1;
			return;
		}
	}
	fprintf(stderr, "This can't happen\n");
}

static void
free_times(thread_time_list_t *times)
{
	int i;

	for (i = 0; i < FREELIST_SIZE; i++) {
		if (freelist[i].nthreads == 1) {
			freelist[i] = *times;
			freelist[i].nthreads = 0;
			return;
		}
	}
	fprintf(stderr, "This can't happen\n");
}

/*
 * Compare two thread snapshots (for qsort)
 */
static int
comparetime(const void *a, const void *b)
{
	thread_times_t *ta = (thread_times_t *)a;
	thread_times_t *tb = (thread_times_t *)b;

	return ((tb->user_time + tb->kernel_time) -
			(ta->user_time + ta->kernel_time));
}

/*
 * Calculate the cpu percentage used by a given thread
 * Remember: for multiple CPUs, multiply the interval by # cpus
 * when calculating
 */
static float
cpu_perc(bigtime_t spent, bigtime_t interval)
{
	return ((100.0 * spent) / (cpus * interval));
}

/*
 * Clear the screen
 */
static void
clear(void)
{
	if (clear_string) {
		printf(clear_string);
		fflush(stdout);
	}
}

/*
 * Compare an old snapshot with the new one
 */
static void
compare(
		thread_time_list_t *old,
		thread_time_list_t *new,
		bigtime_t old_busy,
		bigtime_t new_busy,
		bigtime_t uinterval,
		int refresh
		)
{
	int i;
	int j;
	int k;
	bigtime_t oldtime;
	bigtime_t newtime;
	thread_time_list_t times;
	thread_info t;
	team_info tm;
	bigtime_t total;
	bigtime_t utotal;
	bigtime_t ktotal;
	bigtime_t gtotal;
	bigtime_t idletime;
	//thread_times_t ttime;
	int newthread;
	int ignore;
	int linecount;
	//system_info info;
	char *p;

	/*
	 * First, merge both old and new lists, computing the differences in time
	 * Threads in only one list are dropped.
	 * Threads with no elapsed time are dropped too.
	 */
	init_times(&times);
	k = 0;
	gtotal = 0;
	utotal = 0;
	ktotal = 0;
	for (j = 0; j < new->nthreads; j++) {
		newthread = 1;
		ignore = 0;
		for (i = 0; i < old->nthreads; i++) {
			if (old->thread_times[i].thid != new->thread_times[j].thid) {
				continue;
			}
			newthread = 0;
			oldtime = (old->thread_times[i].user_time +
					   old->thread_times[i].kernel_time);
			newtime = (new->thread_times[j].user_time +
					   new->thread_times[j].kernel_time);
			if (oldtime == newtime) {
				ignore = 1;
				break;
			}
			grow(&times);
			times.thread_times[k].thid = new->thread_times[j].thid;
			times.thread_times[k].user_time = (new->thread_times[j].user_time -
											   old->thread_times[i].user_time);
			times.thread_times[k].kernel_time = (new->thread_times[j].kernel_time -
												 old->thread_times[i].kernel_time);
		}
		if (newthread) {
			grow(&times);
			times.thread_times[k].thid = new->thread_times[j].thid;
			times.thread_times[k].user_time = new->thread_times[j].user_time;
			times.thread_times[k].kernel_time = new->thread_times[j].kernel_time;
		}
		if (!ignore) {
			total = (times.thread_times[k].user_time +
					 times.thread_times[k].kernel_time);
			gtotal += total;
			utotal += times.thread_times[k].user_time;
			ktotal += times.thread_times[k].kernel_time;
			k++;
		}
	}

	/*
	 * Sort what we found and print
	 */
	qsort(times.thread_times, times.nthreads, 
		  sizeof(times.thread_times[0]), comparetime);

	if (refresh) {
		clear();
	}
	printf("%6s %7s %7s %7s %4s %16s %16s\n", "thid", "total", "user", "kernel",
		   "%cpu", "team name", "thread name");
	linecount = 1;
	idletime = new_busy - old_busy;
	gtotal = 0;
	ktotal = 0;
	utotal = 0;
	for (i = 0; i < times.nthreads; i++) {
		ignore = 0;
		if (get_thread_info(times.thread_times[i].thid, &t) < B_NO_ERROR) {
			strcpy(t.name, "(unknown)");
			strcpy(tm.args, "(unknown)");
		} else {
			if (strncmp(t.name, IDLE_NAME, strlen(IDLE_NAME)) == 0) {
				ignore++;
			}
			if (get_team_info(t.team, &tm) < B_NO_ERROR) {
				strcpy(tm.args, "(unknown)");
			} else {
				if ((p = strrchr(tm.args, '/'))) {
					strcpy(tm.args, p + 1);
				}
			}
		}

		tm.args[16] = 0;
		t.name[16] = 0;
		total = (times.thread_times[i].user_time +
				 times.thread_times[i].kernel_time);
		if (ignore) {
			idletime += total;
		} else {
			gtotal += total;
			ktotal += times.thread_times[i].kernel_time;
			utotal += times.thread_times[i].user_time;
		}
		if (!ignore && (!refresh || (linecount < (rows - 1)))) {

			printf("%6ld %7.2f %7.2f %7.2f %4.1f %16s %16s\n",
				   times.thread_times[i].thid,
				   total / 1000.0,
				   (double)(times.thread_times[i].user_time / 1000),
				   (double)(times.thread_times[i].kernel_time / 1000),
				   cpu_perc(total, uinterval),
				   tm.args,
				   t.name);
			linecount++;
		}
	}
	free_times(&times);
	printf("------ %7.2f %7.2f %7.2f %4.1f%% TOTAL (%4.1f%% idle time, %4.1f%% unknown)", 
		   (double) (gtotal / 1000),
		   (double) (utotal / 1000),
		   (double) (ktotal / 1000),
		   cpu_perc(gtotal, uinterval),
		   cpu_perc(idletime, uinterval),
		   cpu_perc(cpus * uinterval - (gtotal + idletime), uinterval));
	fflush(stdout);
	if (!refresh) {
		printf("\n\n");
	}
}

static int
setup_term(bool onlyRows)
{
	char *term;
	char *str;
	char buf[2048];
	char *entries;
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, &ws) < 0) {
		return (0);
	}
	if (ws.ws_row <= 0) {
		return (0);
	}
	rows = ws.ws_row;
	if (onlyRows)
		return 1;
	term = getenv("TERM");
	if (term == NULL) {
		return (0);
	}
	if (tgetent(buf, term) <= 0) {
		return (0);
	}
	entries = &buf[0];
	str = tgetstr("cl", &entries);
	if (str == NULL) {
		return (0);
	}
	clear_string = strdup(str);
	return (1);
}

/*
 * Gather up thread data for uinterval microseconds
 */
static thread_time_list_t
gather(
	   thread_time_list_t *old,
	   bigtime_t *busy_wait_time,
	   int refresh
	   )
{
	int32		tmcookie;
	int32		thcookie;
	thread_info	t;
	team_info	tm;
	thread_time_list_t times;
	bigtime_t old_busy;
	int i;
	system_info info;
	bigtime_t	oldLastMeasure;

	i = 0;
	init_times(&times);
	tmcookie = 0;
	oldLastMeasure = lastMeasure;
	lastMeasure = system_time();

	get_system_info(&info);
	old_busy = *busy_wait_time;
	*busy_wait_time = info._busy_wait_time;

	while (get_next_team_info(&tmcookie, &tm) == B_NO_ERROR) {
		thcookie = 0;
		while (get_next_thread_info(tm.team, &thcookie, &t) == B_NO_ERROR) {
			grow(&times);
			times.thread_times[i].thid = t.thread;
			times.thread_times[i].user_time = t.user_time;
			times.thread_times[i].kernel_time = t.kernel_time;
			i++;
		}
	}
	if (old != NULL) {
		if (screen_size_changed) {
			setup_term(true);
			screen_size_changed = 0;
		}
		compare(old, &times, old_busy, *busy_wait_time,
			system_time() - oldLastMeasure, refresh);
		free_times(old);
	}
	return (times);
}

/*
 * print usage message and exit
 */
static void
usage(const char *myname)
{
	fprintf(stderr, "usage: %s [-d] [-i interval] [-n ntimes]\n", myname);
	fprintf(stderr, 
			" -d,          do not clear the screen between displays\n");
	fprintf(stderr, 
			" -i interval, wait `interval' seconds before displaying\n");
	fprintf(stderr, 
			" -n ntimes,   display `ntimes' times before exiting\n");
	fprintf(stderr, "Default is clear screen, interval=5, ntimes=infinite\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	thread_time_list_t baseline;
	int i;
	int iters = -1;
	int interval = 5;
	int refresh = 1;
	system_info sysinfo;
	//bigtime_t now;
	bigtime_t then;
	bigtime_t uinterval;
	bigtime_t elapsed;
	bigtime_t busy;
	char *myname;

	get_system_info (&sysinfo);
	cpus = sysinfo.cpu_count;

	myname = argv[0];
	for (argc--, argv++; argc > 0 && **argv == '-'; argc--, argv++) {
		if (strcmp(argv[0], "-i") == 0) {
			argc--, argv++;
			if (argc == 0) {
				usage(myname);
			}
			interval = atoi(argv[0]);
		} else if (strcmp(argv[0], "-n") == 0) {
			argc--, argv++;
			if (argc == 0) {
				usage(myname);
			}
			iters = atoi(argv[0]);
		} else if (strcmp(argv[0], "-d") == 0) {
			refresh = 0;
		} else {
			usage(myname);
		}
	}
	if (argc) {
		usage(myname);
	}
	if (refresh) {
		if (!setup_term(false)) {
			refresh = 0;
		}
	}
	if (iters >= 0) { 
		printf("Starting: %d interval%s of %d second%s each\n", iters, 
			   (iters == 1) ? "" : "s", interval,
			   (interval == 1) ? "" : "s");
	}
	
	signal(SIGWINCH, winch_handler);
	
	lastMeasure = system_time();
	if (iters < 0) {
		// You will only have to wait half a second for the first iteration.
		uinterval = 1 * 1000000 / 2;
		baseline = gather(NULL, &busy, refresh);
		elapsed = system_time() - lastMeasure;
		if (elapsed < uinterval)
			snooze(uinterval - elapsed);
		then = system_time();
		baseline = gather(&baseline, &busy, refresh);

	} else
		baseline = gather(NULL, &busy, refresh);

	uinterval = interval * 1000000;
	for (i = 0; iters < 0 || i < iters; i++) {
		elapsed = system_time() - lastMeasure;
		if (elapsed < uinterval)
			snooze(uinterval - elapsed);
		baseline = gather(&baseline, &busy, refresh);
	}
	exit(0);
}
