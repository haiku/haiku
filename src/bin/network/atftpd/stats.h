/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * stats.h
 *
 * $Id: stats.h,v 1.3 2000/12/27 17:02:23 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#ifndef stats_h
#define stats_h

#include <pthread.h>
#include <sys/time.h>
#include <sys/times.h>

/* structure to collect some stats */
struct server_stats {
     /* updated by main thread */
     struct timeval start_time;
     struct timeval end_time;
     struct tms tms;
     
     /* connection statistics, updated by main thread */
     int max_simul_threads;     /* maximum number of simultaneous server */
     struct timeval min_time;   /* time between connection stats */
     struct timeval max_time;
     struct timeval curr_time;  /* temporary usage for calculation */
     struct timeval prev_time;
     struct timeval diff_time;

     /* updated by server thread */
     pthread_mutex_t mutex;
     struct tms tms_thread;
     int number_of_server;      /* number of server that return successfully */
     int number_of_abort;       /* when number max of client is reached */
     int number_of_err;         /* send or receive that return with error */
     int num_file_send;
     int num_file_recv;
     int byte_send;             /* total byte transfered to client (file) */
     int byte_recv;             /* total byte read from client (file) */
};

/* Functions defined in stats.c */
void stats_start(void);
void stats_end(void);
void stats_send_locked(void);
void stats_recv_locked(void);
void stats_err_locked(void);
void stats_abort_locked(void);
void stats_new_thread(int number_of_thread);
void stats_thread_usage_locked(void);
void stats_print(void);

#endif
