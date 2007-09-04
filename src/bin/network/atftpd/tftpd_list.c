/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_list.c
 *    thread_data and client list related functions
 *
 * $Id: tftpd_list.c,v 1.9 2004/02/27 02:05:26 jp Exp $
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "tftpd.h"
#include "logger.h"

/*
 * thread_data is a double link list of server threads. Server threads
 * are started by the main thread when a new request arrives. The number
 * of threads running is held in number_of_thread. Any access to the
 * thread_data list: insertion and extraction of elements must be protected
 * by the mutex. This is done in this file by all functions playing with the thread list.
 * Note that individual threads do not need to lock the list when playing in their data.
 * See tftpd.h.
 *
 * In addition, it is needed to use mutex when working on the client list of
 * individual thread. In some case, the thread data mutex is needed also.
 * Again, the functions in this file take care of this.
 */

struct thread_data *thread_data = NULL; /* head of thread list */
static int number_of_thread = 0;

pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Add a new thread_data structure to the list. Thread list mutex is locked
 * before walking the list and doing the insertion.
 */
int tftpd_list_add(struct thread_data *new)
{
     struct thread_data *current = thread_data;
     int ret;

     pthread_mutex_lock(&thread_list_mutex);

     number_of_thread++;
     
     ret = number_of_thread;

     if (thread_data == NULL)
     {
          thread_data = new;
          new->prev = NULL;
          new->next = NULL;
     }
     else
     {
          while (current->next != NULL)
               current = current->next;
          current->next = new;
          new->prev = current;
          new->next = NULL;
     }
     pthread_mutex_unlock(&thread_list_mutex);
     return ret;
}

/*
 * Remove a thread_data structure from the list.
 */
int tftpd_list_remove(struct thread_data *old)
{
     struct thread_data *current = thread_data;
     int ret;

     pthread_mutex_lock(&thread_list_mutex);

     number_of_thread--;
     ret = number_of_thread;
    
     if (thread_data == old)
     {
          if (thread_data->next != NULL)
          {
               thread_data = thread_data->next;
               thread_data->prev = NULL;
          }
          else
               thread_data = NULL;
     }
     else
     {
          while (current != old)
               current = current->next;
          if (current->next && current->prev)
          {
               current->prev->next = current->next;
               current->next->prev = current->prev;
          }
          else
               current->prev->next = NULL;
     }
     pthread_mutex_unlock(&thread_list_mutex);
     return ret;
}

/*
 * Return the number of threads actually started.
 */
int tftpd_list_num_of_thread(void)
{
     int ret;

     pthread_mutex_lock(&thread_list_mutex);
     ret = number_of_thread;
     pthread_mutex_unlock(&thread_list_mutex);
     return ret;
}

/*
 * This function looks for a thread serving exactly the same
 * file and with the same options as another client. This implies a
 * multicast enabled client.
 */
int tftpd_list_find_multicast_server_and_add(struct thread_data **thread,
                                             struct thread_data *data,
                                             struct client_info *client)
{
     struct thread_data *current = thread_data; /* head of the list */
     struct tftp_opt *tftp_options = data->tftp_options;
     struct client_info *tmp;
     char options[MAXLEN];
     char string[MAXLEN];
     char *index;
     int len;

     *thread = NULL;

     opt_request_to_string(tftp_options, options, MAXLEN);
     index = strstr(options, "multicast");
     len = (int)index - (int)options;

     /* lock the whole list before walking it */
     pthread_mutex_lock(&thread_list_mutex);

     while (current)
     {
          if (current != data)
          {
               /* Lock the client list before reading client ready state */
               pthread_mutex_lock(&current->client_mutex);
               if (current->client_ready == 1)
               {
                    opt_request_to_string(current->tftp_options, string, MAXLEN);
                    /* must have exact same option string */
                    if (strncmp(string, options, len) == 0)
                    {
                         *thread = current;                         
                         /* insert the new client at the end. If the client is already
                            in the list, don't add it again. */
                         tmp = current->client_info;

                         while (1)
                         {
                              if ((tmp->client.sin_port == client->client.sin_port) &&
                                  (tmp->client.sin_addr.s_addr == client->client.sin_addr.s_addr) &&
                                  (tmp->done == 0))
                              {
                                   /* unlock mutex and exit */
                                   pthread_mutex_unlock(&current->client_mutex);
                                   pthread_mutex_unlock(&thread_list_mutex);
                                   return 2;
                              }
                              if (tmp->next == NULL)
                                   break;
                              tmp = tmp->next;
                         }
                         tmp->next = client;
                         /* unlock mutex and exit */
                         pthread_mutex_unlock(&current->client_mutex);                    
                         pthread_mutex_unlock(&thread_list_mutex);
                         return 1;
                    }
               }
               pthread_mutex_unlock(&current->client_mutex);                    
          }
          current = current->next;
     }
     pthread_mutex_unlock(&thread_list_mutex);
     
     return 0;
}

inline void tftpd_clientlist_ready(struct thread_data *thread)
{
     pthread_mutex_lock(&thread->client_mutex);
     thread->client_ready = 1;
     pthread_mutex_unlock(&thread->client_mutex);
}

/*
 * As of the the above comment, we never remove the head element.
 */
void tftpd_clientlist_remove(struct thread_data *thread,
                             struct client_info *client)
{
     struct client_info *tmp = thread->client_info;

     pthread_mutex_lock(&thread->client_mutex);
     while ((tmp->next != client) && (tmp->next != NULL))
          tmp = tmp->next;
     if (tmp->next == NULL)
          return;
     tmp->next = tmp->next->next;
     pthread_mutex_unlock(&thread->client_mutex);
}

/*
 * Free all allocated client structure.
 */
void tftpd_clientlist_free(struct thread_data *thread)
{
     struct client_info *tmp;
     struct client_info *head = thread->client_info;

     pthread_mutex_lock(&thread->client_mutex);
     while (head)
     {
          tmp = head;
          head = head->next;
          free(tmp);
     }
     pthread_mutex_unlock(&thread->client_mutex);
}

/*
 * Mark a client as done
 */
int tftpd_clientlist_done(struct thread_data *thread,
                          struct client_info *client,
                          struct sockaddr_in *sock)
{
     struct client_info *head = thread->client_info;

     pthread_mutex_lock(&thread->client_mutex);

     if (client)
     {
          client->done = 1;
          pthread_mutex_unlock(&thread->client_mutex);
          return 1;
     }
     if (sock)
     {
          /* walk the list to find this client */
          while (head)
          {
               if (memcmp(sock, &head->client, sizeof(struct sockaddr_in)) == 0)
               {
                    head->done = 1;
                    pthread_mutex_unlock(&thread->client_mutex);
                    return 1;
               }
               else
                    head = head->next;
          }
     }
     pthread_mutex_unlock(&thread->client_mutex);
     return 0;
}

/*
 * Return the next client in the list.
 * If a client is found, *client got the address and return 1.
 * If no more client are available, 0 is returned and client_ready is unset
 * If the new client is the same, -1 is returned.
 *
 * List is search from the current client address and wrap around.
 */
int tftpd_clientlist_next(struct thread_data *thread,
                          struct client_info **client)
{
     struct client_info *tmp;

     pthread_mutex_lock(&thread->client_mutex);

     /* If there's a next, start with it. Else search from the
        beginning */
     if ((*client)->next)
          tmp = (*client)->next;
     else
          tmp = thread->client_info;

     /* search from the current client */
     while (tmp)
     {
          if (!tmp->done)
          {
               /* If this is the same as *client */
               if (tmp == *client)
               {
                    pthread_mutex_unlock(&thread->client_mutex);
                    return -1;
               }
               *client = tmp;
               pthread_mutex_unlock(&thread->client_mutex);
               return 1;
          }
          tmp = tmp->next;
          /* if no more entry, start at begining */
          if (tmp == NULL)
               tmp = thread->client_info;
          /* if we fall back on the current client, there is no more
             client to serve */
          if (tmp == *client)
               tmp = NULL;
     }
     /* There is no more client to server */
     thread->client_ready = 0;
     *client = NULL;
     pthread_mutex_unlock(&thread->client_mutex);
     return 0;
}

void tftpd_list_kill_threads(void)
{
     struct thread_data *current = thread_data; /* head of list */

     pthread_mutex_lock(&thread_list_mutex);


     while (current != NULL)
     {
          pthread_kill(current->tid, SIGTERM);
          current = current->next;
     }

     pthread_mutex_unlock(&thread_list_mutex);
}
