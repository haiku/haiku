/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_mtftp.h
 *
 * $Id: tftpd_mtftp.h,v 1.5 2004/02/27 02:05:26 jp Exp $
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

#ifndef tftpd_mtftp_h
#define tftpd_mtftp_h

#include <pthread.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>
#include "tftp_io.h"
#include "options.h"

/*
 * This structure hold information on mtftp configuration
 * 
 */
struct mtftp_data {
     struct mtftp_thread *thread_data;
     int number_of_thread;

     /* for receiving of initial request */
     char *data_buffer;
     int data_buffer_size;
     struct tftp_opt *tftp_options;

     /* options scanned from command line */
     int server_port;
     int mcast_ttl;
     int timeout;
     int checkport;
     int trace;
};

struct mtftp_thread {
     pthread_t tid;

     /* Configuration data */
     int running;
     char file_name[MAXLEN];
     char mcast_ip[MAXLEN];     /* FIXME: could be less memory */
     char client_port[MAXLEN];

     /* Server thread variables */
     FILE *fp;

     int sockfd;
     struct sockaddr_in sa_in;
     struct sockaddr_in sa_client;

     int mcast_sockfd;
     int mcast_port;
     struct sockaddr_in sa_mcast;
     struct ip_mreq mcastaddr;

     char *data_buffer;
     int data_buffer_size;

     /* For options access */
     struct mtftp_data *mtftp_data;

     struct mtftp_thread *next;     
};

/*
 * Functions defined in tftpd_file.c
 */
struct mtftp_data *tftpd_mtftp_init(char *filename);
int tftpd_mtftp_clean(struct mtftp_data *data);
void *tftpd_mtftp_server(void *arg);
void tftpd_mtftp_kill_threads(struct mtftp_data *data);

#endif
