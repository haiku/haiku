/* -*- c-basic-offset: 8; -*- */
#include	<sys/types.h>
#include	<sys/time.h>
#include	<errno.h>
#include	<stddef.h>
#include	"ourhdr.h"

void
sleep_us(unsigned int nusecs)
{
	struct timeval	tval;
	
	for ( ; ; ) {
		tval.tv_sec = nusecs / 1000000;
		tval.tv_usec = nusecs % 1000000;
		if (select(0, NULL, NULL, NULL, &tval) == 0)
			break;		/* all OK */
		/*
		 * Note than on an interrupted system call (i.e, SIGIO) there's not
		 * much we can do, since the timeval{} isn't updated with the time
		 * remaining.  We could obtain the clock time before the call, and
		 * then obtain the clock time here, subtracting them to determine
		 * how long select() blocked before it was interrupted, but that
		 * seems like too much work :-)
		 */
		if (errno == EINTR)
			continue;
		err_sys("sleep_us: select error");
	}
}
