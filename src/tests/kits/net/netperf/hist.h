/* hist.h

   Given a time difference in microseconds, increment one of 61
   different buckets: 
   
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
*/
#ifndef _HIST_INCLUDED
#define _HIST_INCLUDED

#ifdef IRIX
#include <sys/time.h>
#endif /* IRIX */
   
struct histogram_struct {
   int tenth_msec[10];
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

#endif
