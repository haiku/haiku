/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd.h
 *
 * $Id: tftpd.h,v 1.22 2004/02/27 02:05:26 jp Exp $
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

#ifndef tftpd_h
#define tftpd_h

#include <pthread.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>
#include "tftp_io.h"

/*
 * Per thread data. There is a thread for each client or group
 * (multicast) of client.
 */
struct thread_data {
     pthread_t tid;

     /* private to thread */
     char *data_buffer;
     int data_buffer_size;

     int timeout;
     int checkport;             /* Disable TID check. Violate RFC */
     int mcast_switch_client;   /* When set, server will switch to next client
                                   on first timeout from the current client. */
     int trace;

     int sockfd;

     /* multicast stuff */
     short mc_port;             /* multicast port */
     char *mc_addr;             /* multicast address */
     struct sockaddr_in sa_mcast;
     struct ip_mreq mcastaddr;
     u_char mcast_ttl;
     
     /*
      * Self can read/write until client_ready is set. Then only allowed to read.
      * Other thread can read it only when client_ready is set. Remember that access
      * to client_ready bellow is protected by a mutex.
      */
     struct tftp_opt *tftp_options;
 
     /* 
      * Must lock to insert in the list or search, but not to read or write 
      * in the client_info structure, since only the propriotary thread do it.
      */
     pthread_mutex_t client_mutex;
     struct client_info *client_info;
     int client_ready;        /* one if other thread may add client */
 
     /* must be lock (list lock) to update */
     struct thread_data *prev;
     struct thread_data *next;
};

struct client_info {
     struct sockaddr_in client;
     int done;                  /* that client as receive it's file */
     struct client_info *next;
};

/*
 * Functions defined in tftpd_file.c
 */
int tftpd_rules_check(char *filename);
int tftpd_receive_file(struct thread_data *data);
int tftpd_send_file(struct thread_data *data);

/*
 * Defined in tftpd_list.c, operation on thread_data list.
 */
int tftpd_list_add(struct thread_data *new);
int tftpd_list_remove(struct thread_data *old);
int tftpd_list_num_of_thread(void);
int tftpd_list_find_multicast_server_and_add(struct thread_data **thread,
                                             struct thread_data *data,
                                             struct client_info *client);
/*
 * Defined in tftpd_list.c, operation on client structure list.
 */
inline void tftpd_clientlist_ready(struct thread_data *thread);
void tftpd_clientlist_remove(struct thread_data *thread,
                             struct client_info *client);
void tftpd_clientlist_free(struct thread_data *thread);
int tftpd_clientlist_done(struct thread_data *thread,
                          struct client_info *client,
                          struct sockaddr_in *sock);
int tftpd_clientlist_next(struct thread_data *thread,
                          struct client_info **client);
void tftpd_list_kill_threads(void);

/*
 * Defines in tftpd_mcast.c
 */
int tftpd_mcast_get_tid(char **addr, short *port);
int tftpd_mcast_free_tid(char *addr, short port);
int tftpd_mcast_parse_opt(char *addr, char *ports);

#endif
