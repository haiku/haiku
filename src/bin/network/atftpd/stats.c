/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * stats.c
 *    some functions to collect statistics
 *
 * $Id: stats.c,v 1.6 2002/03/27 03:02:12 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "config.h"

#include <limits.h>
#include <string.h>
#include "tftp_def.h"
#include "stats.h"
#include "logger.h"

/*
 * That structure allows global statistic to be collected. See stats.h.
 */
struct server_stats s_stats;

/*
 * Must be called to initilise stats structure and record the
 * start time.
 */
void stats_start(void)
{
     /* stats struct initialisation */
     memset(&s_stats, 0, sizeof(s_stats));
     s_stats.min_time.tv_sec = LONG_MAX;
     pthread_mutex_init(&s_stats.mutex, NULL);

     gettimeofday(&s_stats.start_time, NULL);
}

/*
 * Called when execution is finnished, before calling stats_print.
 */
void stats_end(void)
{
     gettimeofday(&s_stats.end_time, NULL);
}

/*
 * Called by server threads each time a file is sent succesfully.
 * Be aware that a mutex is locked in there.
 */
void stats_send_locked(void)
{
     pthread_mutex_lock(&s_stats.mutex);
     s_stats.number_of_server++;
     s_stats.num_file_send++;
     pthread_mutex_unlock(&s_stats.mutex);
}

/*
 * Called by verver threads each time a file is received.
 */
void stats_recv_locked(void)
{
     pthread_mutex_lock(&s_stats.mutex);
     s_stats.number_of_server++;
     s_stats.num_file_recv++;
     pthread_mutex_unlock(&s_stats.mutex);
}

/*
 * Called by server threads each time tftpd_send_file or tftpd_recv_file
 * return with error.
 */
void stats_err_locked(void)
{
     pthread_mutex_lock(&s_stats.mutex);
     s_stats.number_of_err++;
     pthread_mutex_unlock(&s_stats.mutex);
}

/*
 * Called by server threads when the maximum number of threads is reached.
 */
void stats_abort_locked(void)
{
     pthread_mutex_lock(&s_stats.mutex);
     s_stats.number_of_abort++;
     pthread_mutex_unlock(&s_stats.mutex);
}

/*
 * Called by main thread only (in fact in tftpd_receive_request(), but
 * before stdin_mutex is released) every time a new thread is created.
 * We record the number of thread, the number of simultaeous thread, the
 * between threads.
 */
void stats_new_thread(int number_of_thread)
{
     struct timeval tmp;

     if (number_of_thread > s_stats.max_simul_threads)
          s_stats.max_simul_threads = number_of_thread;

     /* calculate the arrival time of this thread */
     if (s_stats.prev_time.tv_sec != 0)
     {
          gettimeofday(&s_stats.curr_time, NULL);
          timeval_diff(&s_stats.diff_time, &s_stats.curr_time,
                       &s_stats.prev_time);
          if (timeval_diff(&tmp, &s_stats.diff_time,
                           &s_stats.min_time) < 0)
               memcpy(&s_stats.min_time, &s_stats.diff_time,
                      sizeof(struct timeval));
          if (timeval_diff(&tmp, &s_stats.diff_time,
                           &s_stats.max_time) > 0)
               memcpy(&s_stats.max_time, &s_stats.diff_time,
                      sizeof(struct timeval));
          memcpy(&s_stats.prev_time, &s_stats.curr_time,
                 sizeof(struct timeval));
     }
     else
          gettimeofday(&s_stats.prev_time, NULL);
}

/*
 * Called by server threads when the finnished to add CPU ressources
 * information.
 */
void stats_thread_usage_locked(void)
{
     struct tms tms_tmp;

     times(&tms_tmp);
     pthread_mutex_lock(&s_stats.mutex);
     s_stats.tms_thread.tms_utime += tms_tmp.tms_utime;
     s_stats.tms_thread.tms_stime += tms_tmp.tms_stime;
     pthread_mutex_unlock(&s_stats.mutex);
}

/*
 * Called at the end of the main thread, when no other threads are
 * running, to print the final statistics.
 */
void stats_print(void)
{
     struct timeval tmp;

     timeval_diff(&tmp, &s_stats.end_time, &s_stats.start_time);
     times(&s_stats.tms);
     s_stats.tms.tms_utime += s_stats.tms_thread.tms_utime;
     s_stats.tms.tms_stime += s_stats.tms_thread.tms_stime;

     logger(LOG_INFO, "  Load measurements:");
     logger(LOG_INFO, "   User: %8.3fs  Sys:%8.3fs",
            (double)(s_stats.tms.tms_utime) / CLK_TCK,
            (double)(s_stats.tms.tms_stime) / CLK_TCK);
     logger(LOG_INFO, "   Total:%8.3fs  CPU:%8.3f%%", 
            (double)(tmp.tv_sec + tmp.tv_usec * 1e-6),
            (double)(s_stats.tms.tms_utime + s_stats.tms.tms_stime) /
            (double)(tmp.tv_sec + tmp.tv_usec * 1e-6));
     logger(LOG_INFO, "  Time between connections:");
     if (s_stats.min_time.tv_sec == LONG_MAX)
          logger(LOG_INFO, "   Min: -----   Max: -----");
     else
          logger(LOG_INFO, "   Min: %.3fs Max: %.3fs",
                 (double)(s_stats.min_time.tv_sec + s_stats.min_time.tv_usec * 1e-6),
                 (double)(s_stats.max_time.tv_sec + s_stats.max_time.tv_usec * 1e-6));
     logger(LOG_INFO, "  Thread stats:");
     logger(LOG_INFO, "   simultaneous threads:     %d", s_stats.max_simul_threads);
     logger(LOG_INFO, "   number of servers:        %d", s_stats.number_of_server);
     logger(LOG_INFO, "   number of aborts:         %d", s_stats.number_of_abort);
     logger(LOG_INFO, "   number of errors:         %d", s_stats.number_of_err);
     logger(LOG_INFO, "   number of files sent:     %d", s_stats.num_file_send);
     logger(LOG_INFO, "   number of files received: %d", s_stats.num_file_recv);
}
