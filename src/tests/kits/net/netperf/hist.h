#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/* hist.h

   Given a time difference in microseconds, increment one of 61
   different buckets: 
   
   0 - 9 in increments of 1 usec
   0 - 9 in increments of 10 usecs
   0 - 9 in increments of 100 usecs
   0 - 9 in increments of 1 msec
   0 - 9 in increments of 10 msecs
   0 - 9 in increments of 100 msecs
   0 - 9 in increments of 1 sec
   0 - 9 in increments of 10 sec
   > 100 secs
   
   This will allow any time to be recorded to within an accuracy of
   10%, and provides a compact representation for capturing the
   distribution of a large number of time differences (e.g.
   request-response latencies).
   
   Colin Low  10/6/93
   Rick Jones 2004-06-15 - extend to 1 and 10 usec
*/
#ifndef _HIST_INCLUDED
#define _HIST_INCLUDED

#ifdef IRIX
#include <sys/time.h>
#endif /* IRIX */

#if defined(HAVE_GET_HRT)
#include "hrt.h"
#endif
   
struct histogram_struct {
  int unit_usec[10];
  int ten_usec[10];
  int hundred_usec[10];
  int unit_msec[10];
  int ten_msec[10];
  int hundred_msec[10];
  int unit_sec[10];
  int ten_sec[10];
  int ridiculous;
  int total;
};

typedef struct histogram_struct *HIST;

/* 
   HIST_new - return a new, cleared histogram data type
*/

HIST HIST_new(void); 

/* 
   HIST_clear - reset a histogram by clearing all totals to zero
*/

void HIST_clear(HIST h);

/*
   HIST_add - add a time difference to a histogram. Time should be in
   microseconds. 
*/

void HIST_add(register HIST h, int time_delta);

/* 
  HIST_report - create an ASCII report on the contents of a histogram.
  Currently printsto standard out 
*/

void HIST_report(HIST h);

/*
  HIST_timestamp - take a timestamp suitable for use in a histogram.
*/

#ifdef HAVE_GETHRTIME
void HIST_timestamp(hrtime_t *timestamp);
#elif defined(HAVE_GET_HRT)
void HIST_timestamp(hrt_t *timestamp);
#elif defined(WIN32)
void HIST_timestamp(LARGE_INTEGER *timestamp);
#else
void HIST_timestamp(struct timeval *timestamp);
#endif

/*
  delta_micro - calculate the difference in microseconds between two
  timestamps
*/
#ifdef HAVE_GETHRTIME
int delta_micro(hrtime_t *begin, hrtime_t *end);
#elif defined(HAVE_GET_HRT)
int delta_micro(hrt_t *begin, hrt_t *end);
#elif defined(WIN32)
int delta_micro(LARGE_INTEGER *begin, LARGE_INTEGER *end);
#else
int delta_micro(struct timeval *begin, struct timeval *end);
#endif

#endif

