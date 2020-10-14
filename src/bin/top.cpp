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
#include <termios.h>

#include <list>

#include "termcap.h"

static const char IDLE_NAME[] = "idle thread ";
static bigtime_t lastMeasure = 0;

/*
 * Keeps track of a single thread's times
 */
struct ThreadTime {
	thread_id thid;
	bigtime_t user_time;
	bigtime_t kernel_time;

	bigtime_t total_time() const {
		return user_time + kernel_time;
	}

	bool operator< (const ThreadTime& other) const {
		return total_time() > other.total_time();
	}
};

/*
 * Keeps track of all the threads' times
 */
typedef std::list<ThreadTime> ThreadTimeList;


static char *clear_string; /*output string for clearing the screen */
static char *enter_ca_mode; /* output string for switching screen buffer */
static char *exit_ca_mode; /* output string for releasing screen buffer */
static char *cursor_home;	/* Places cursor back to (1,1) */
static char *restore_cursor;
static char *save_cursor;
static int columns;	/* Columns on screen */
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


/* SIGINT handler */
static void
sigint_handler(int)
{
	tputs(exit_ca_mode, 1, putchar);
	tputs(restore_cursor, 1, putchar);
	exit(1);
}


static void
init_term()
{
	static char buf[2048];
	char *entries = &buf[0];

	tgetent(buf, getenv("TERM"));
	exit_ca_mode = tgetstr("te", &entries);
	enter_ca_mode = tgetstr("ti", &entries);
	cursor_home = tgetstr("ho", &entries);
	clear_string = tgetstr("cl", &entries);
	restore_cursor = tgetstr("rc", &entries);
	save_cursor = tgetstr("sc", &entries);

	tputs(save_cursor, 1, putchar);
	tputs(enter_ca_mode, 1, putchar);
	tputs(clear_string, 1, putchar);
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
 * Compare an old snapshot with the new one
 */
static void
compare(
		ThreadTimeList *old,
		ThreadTimeList *newList,
		bigtime_t uinterval,
		int refresh
		)
{
	bigtime_t oldtime;
	bigtime_t newtime;
	ThreadTimeList times;
	thread_info t;
	team_info tm;
	bigtime_t total;
	bigtime_t utotal;
	bigtime_t ktotal;
	bigtime_t gtotal;
	bigtime_t idletime;
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
	gtotal = 0;
	utotal = 0;
	ktotal = 0;
	ThreadTimeList::iterator it;
	ThreadTimeList::iterator itOld;
	ThreadTime entry;

	for (it = newList->begin(); it != newList->end(); it++) {
		newthread = 1;
		ignore = 0;
		for (itOld = old->begin(); itOld != old->end(); itOld++) {
			if (itOld->thid != it->thid) {
				continue;
			}
			newthread = 0;
			oldtime = itOld->total_time();
			newtime = it->total_time();
			if (oldtime == newtime) {
				ignore = 1;
				break;
			}
			entry.thid = it->thid;
			entry.user_time = (it->user_time - itOld->user_time);
			entry.kernel_time = (it->kernel_time - itOld->kernel_time);
		}
		if (newthread) {
			entry.thid = it->thid;
			entry.user_time = it->user_time;
			entry.kernel_time = it->kernel_time;
		}
		if (!ignore) {
			times.push_back(entry);

			total = entry.total_time();
			gtotal += total;
			utotal += entry.user_time;
			ktotal += entry.kernel_time;
		}
	}

	/*
	 * Sort what we found and print
	 */
	times.sort();

	printf("%6s %7s %7s %7s %4s %16s %-16s \n", "THID", "TOTAL", "USER",
		"KERNEL", "%CPU", "TEAM NAME", "THREAD NAME");
	linecount = 1;
	idletime = 0;
	gtotal = 0;
	ktotal = 0;
	utotal = 0;
	for (it = times.begin(); it != times.end(); it++) {
		ignore = 0;
		if (get_thread_info(it->thid, &t) < B_NO_ERROR) {
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
					strlcpy(tm.args, p + 1, sizeof(tm.args));
				}
			}
		}

		tm.args[16] = 0;

		if (columns <= 80)
			t.name[16] = 0;
		else if (columns - 64 < sizeof(t.name))
			t.name[columns - 64] = 0;

		total = it->total_time();
		if (ignore) {
			idletime += total;
		} else {
			gtotal += total;
			ktotal += it->kernel_time;
			utotal += it->user_time;
		}
		if (!ignore && (!refresh || (linecount < (rows - 1)))) {

			printf("%6" B_PRId32 " %7.2f %7.2f %7.2f %4.1f %16s %s \n",
				it->thid,
				total / 1000.0,
				(double)(it->user_time / 1000),
				(double)(it->kernel_time / 1000),
				cpu_perc(total, uinterval),
				tm.args,
				t.name);
			linecount++;
		}
	}

	printf("------ %7.2f %7.2f %7.2f %4.1f%% "
		"TOTAL (%4.1f%% idle time, %4.1f%% unknown)",
		(double) (gtotal / 1000),
		(double) (utotal / 1000),
		(double) (ktotal / 1000),
		cpu_perc(gtotal, uinterval),
		cpu_perc(idletime, uinterval),
		cpu_perc(cpus * uinterval - (gtotal + idletime), uinterval));
	fflush(stdout);
	tputs(clear_string, 1, putchar);
	tputs(cursor_home, 1, putchar);
}


static int
adjust_term(bool onlyRows)
{
	struct winsize ws;

	if (ioctl(1, TIOCGWINSZ, &ws) < 0) {
		return (0);
	}
	if (ws.ws_row <= 0) {
		return (0);
	}
	columns = ws.ws_col;
	rows = ws.ws_row;
	if (onlyRows)
		return 1;

	return (1);
}


/*
 * Gather up thread data since previous run
 */
static ThreadTimeList
gather(ThreadTimeList *old, int refresh)
{
	int32		tmcookie;
	int32		thcookie;
	thread_info	t;
	team_info	tm;
	ThreadTimeList times;
	bigtime_t	oldLastMeasure;

	tmcookie = 0;
	oldLastMeasure = lastMeasure;
	lastMeasure = system_time();

	while (get_next_team_info(&tmcookie, &tm) == B_NO_ERROR) {
		thcookie = 0;
		while (get_next_thread_info(tm.team, &thcookie, &t) == B_NO_ERROR) {
			ThreadTime entry;
			entry.thid = t.thread;
			entry.user_time = t.user_time;
			entry.kernel_time = t.kernel_time;
			times.push_back(entry);
		}
	}
	if (old != NULL) {
		if (screen_size_changed) {
			adjust_term(true);
			screen_size_changed = 0;
		}
		compare(old, &times, system_time() - oldLastMeasure, refresh);
		old->clear();
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
	ThreadTimeList baseline;
	int i;
	int iters = -1;
	int interval = 5;
	int refresh = 1;
	system_info sysinfo;
	bigtime_t uinterval;
	bigtime_t elapsed;
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

	init_term();

	if (refresh) {
		if (!adjust_term(false)) {
			refresh = 0;
		}
	}
	if (iters >= 0) {
		printf("Starting: %d interval%s of %d second%s each\n", iters,
			(iters == 1) ? "" : "s", interval,
			(interval == 1) ? "" : "s");
	}

	signal(SIGWINCH, winch_handler);
	signal(SIGINT, sigint_handler);

	lastMeasure = system_time();
	if (iters < 0) {
		// You will only have to wait half a second for the first iteration.
		uinterval = 1 * 1000000 / 2;
		baseline = gather(NULL, refresh);
		elapsed = system_time() - lastMeasure;
		if (elapsed < uinterval)
			snooze(uinterval - elapsed);
		// then = system_time();
		baseline = gather(&baseline, refresh);

	} else
		baseline = gather(NULL, refresh);

	uinterval = interval * 1000000;
	for (i = 0; iters < 0 || i < iters; i++) {
		elapsed = system_time() - lastMeasure;
		if (elapsed < uinterval)
			snooze(uinterval - elapsed);
		baseline = gather(&baseline, refresh);
	}

	tputs(exit_ca_mode, 1, putchar);
	tputs(restore_cursor, 1, putchar);

	exit(0);
}
