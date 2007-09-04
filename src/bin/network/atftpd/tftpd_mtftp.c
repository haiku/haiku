/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_mtftp.c
 *    mtftp support for atftp
 *
 * $Id: tftpd_mtftp.c,v 1.12 2004/02/27 02:05:26 jp Exp $
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

#if HAVE_MTFTP

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#ifdef HAVE_WRAP
#include <tcpd.h>
#endif
#include "tftp_io.h"
#include "tftp_def.h"
#include "logger.h"
#include "tftpd.h"
#include "tftpd_mtftp.h"

#define S_BEGIN         0
#define S_SEND_DATA     4
#define S_WAIT_PACKET   5
#define S_REQ_RECEIVED  6
#define S_ABORT         10
#define S_END           11
#define S_EXIT          12

/* function used in this file only */
int tftpd_mtftp_thread_add(struct mtftp_data *data, struct mtftp_thread *thread);
struct mtftp_thread *tftpd_mtftp_find_server(struct mtftp_data *data, char *filename);
int tftpd_mtftp_unique(struct mtftp_data *data, char *filename, char *ip, char *port);
void *tftpd_mtftp_send_file(void *arg);

/* read only except for the main thread */
extern int tftpd_cancel;

/* 
 * This function parse the configuration file and create data structure
 * Format is:
 *   filename mcast_ip client_port
 * one entry per line
 */
struct mtftp_data *tftpd_mtftp_init(char *filename)
{
     FILE *fp;
     struct mtftp_data *data = NULL;
     struct mtftp_thread *thread = NULL;

     char string[MAXLEN];
     char *token;

     int line = 0;
     struct stat file_stat;
     struct hostent host;

     /* open file */
     if ((fp = fopen(filename, "r")) == NULL)
     {
          logger(LOG_ERR, "mtftp: failed to open configuration file, continuing");
          return NULL;
     }
     logger(LOG_DEBUG, "mtftp: opened configuration file %s", filename);

     /* allocate memory */
     if ((data = malloc(sizeof(struct mtftp_data))) == NULL)
     {
          logger(LOG_ERR, "%s: %d: Memory allocation failed",
                 __FILE__, __LINE__);
          fclose(fp);
          return NULL;
     }
     /* Allocate memory for tftp option structure. */
     if ((data->tftp_options = 
          malloc(sizeof(tftp_default_options))) == NULL)
     {
          logger(LOG_ERR, "%s: %d: Memory allocation failed",
                 __FILE__, __LINE__);
          tftpd_mtftp_clean(data);
          return NULL;
     }
     /* Allocate data buffer for tftp transfer. */
     if ((data->data_buffer = malloc((size_t)SEGSIZE + 4)) == NULL)
     {
          logger(LOG_ERR, "%s: %d: Memory allocation failed",
                 __FILE__, __LINE__);
          tftpd_mtftp_clean(data);
          return NULL;
     }
     data->data_buffer_size = SEGSIZE + 4;

     /* Copy default options. */
     memcpy(data->tftp_options, tftp_default_options,
            sizeof(tftp_default_options));
     data->tftp_options[OPT_TIMEOUT].enabled = 0;
     data->tftp_options[OPT_TSIZE].enabled = 0;
     data->tftp_options[OPT_BLKSIZE].enabled = 0;
     data->tftp_options[OPT_MULTICAST].enabled = 0;
     
     /* init data structure */
     data->thread_data = NULL;
     data->number_of_thread = 0;

     logger(LOG_DEBUG, "mtftp options: ");

     /* parse the file */
     while (fgets(string, MAXLEN, fp) != NULL)
     {
          line++;

          if (thread == NULL)
          {
               /* allocate memory */
               if ((thread = calloc(1, sizeof(struct mtftp_thread))) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    fclose(fp);
                    tftpd_mtftp_clean(data);
                    return NULL;
               }
               /* Allocate data buffer for tftp transfer. */
               if ((thread->data_buffer = malloc((size_t)SEGSIZE + 4)) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    tftpd_mtftp_clean(data);
                    return NULL;
               }
               thread->data_buffer_size = SEGSIZE + 4;
               
               /* Add this thread to the list */                    
               tftpd_mtftp_thread_add(data, thread);
          }

          /* Parse the line */
          token = strtok(string, " ");
          if (!token)
               continue;
          if (token[0] == '#') /* if first char is #, this is a comment */
               continue;
          Strncpy(thread->file_name, token, MAXLEN);
          
          token = strtok(NULL, " ");
          if (!token)
               continue;
          Strncpy(thread->mcast_ip, token, MAXLEN);

          token = strtok(NULL, " ");
          if (!token)
               continue;
          Strncpy(thread->client_port, token, MAXLEN);

          /* validate arguements */
          /* file name verification */
          if (tftpd_rules_check(thread->file_name) != OK)
          {
               logger(LOG_WARNING, "mtftp: file name rules violated %s (%s line %d)",
                      thread->file_name, filename, line);
               continue;
          }
          /* open file */
          if ((thread->fp = fopen(thread->file_name, "r")) == NULL)
          {
               logger(LOG_WARNING, "mtftp: can't open file %s (%s line %d)",
                      thread->file_name,
                      filename, line);
               continue;
          }
          /* verify file size */
          fstat(fileno(thread->fp), &file_stat);  
          if ((file_stat.st_size / SEGSIZE) > 65535)
          {
               logger(LOG_WARNING, "mtftp: file %s too big (%s line %d)", thread->file_name,
                      filename, line);
               fclose(thread->fp);
               continue;
          }
          /* verify port */
          thread->mcast_port = atoi(thread->client_port);
          if ((thread->mcast_port < 0) || (thread->mcast_port > 65535))
          {
               logger(LOG_WARNING, "mtftp: bad port number %d (%s line %d)",
                      thread->client_port, filename, line);
               fclose(thread->fp);
               continue;
          } 
          /* verify IP is valid */
          if (Gethostbyname(thread->mcast_ip, &host) == OK)
          {
               thread->sa_mcast.sin_family = host.h_addrtype;
               memcpy(&thread->sa_mcast.sin_addr.s_addr,
                      host.h_addr_list[0], host.h_length);
               thread->sa_mcast.sin_port = htons(thread->mcast_port);
               if (!IN_MULTICAST(ntohl(thread->sa_mcast.sin_addr.s_addr)))
               {
                    logger(LOG_WARNING, "mtftp: bad multicast address %s\n",
                           thread->mcast_ip);
                    fclose(thread->fp);
                    continue;
               }
          }
          /* verify IP/port is unique */
          if (tftpd_mtftp_unique(data, thread->file_name, thread->mcast_ip, thread->client_port))
          {
               logger(LOG_INFO, "mtftp: duplicate server (%s line %d)", filename, line);
               continue;
          }

          /* some useful info */
          logger(LOG_INFO, "mtftp: will serve %s on %s port %s", thread->file_name,
                 thread->mcast_ip, thread->client_port);
          /* next loop we must allocate a new structure */
          thread = NULL;
     }

     fclose(fp);
     return data;
}

int tftpd_mtftp_clean(struct mtftp_data *data)
{
     struct mtftp_thread *thread = data->thread_data;
     struct mtftp_thread *tmp = data->thread_data;

     if (data == NULL)
          return OK;

     /* clean all thread data structure */
     while (thread)
     {
          /* point to the next item before freeing memory */
          tmp = thread->next;
          /* free everything */
          if (thread->fp)
               fclose(thread->fp);
          if (thread->data_buffer)
               free(thread->data_buffer);
          free(thread);
          thread = tmp;
     }
     /* clean mtftp_data structure */
     if (data->data_buffer)
          free(data->data_buffer);
     if (data->tftp_options)
          free(data->tftp_options);
     free(data);

     return OK;
}

/* 
 * Add a mtftp_thread structure to the list. I guest this could use the code in
 * tftpd_list.c, but I prefer keeping it appart for now.
 */
int tftpd_mtftp_thread_add(struct mtftp_data *data, struct mtftp_thread *thread)
{
     struct mtftp_thread *tmp = data->thread_data;

     if (data->thread_data == NULL)
     {
          data->thread_data = thread;
          return OK;
     }
     while (tmp->next != NULL)
          tmp = tmp->next;
     tmp->next = thread;
     return OK;
}

void tftpd_mtftp_kill_threads(struct mtftp_data *data)
{
     struct mtftp_thread *current = data->thread_data; /* head of list */

     while (current != NULL)
     {
          /* kill running threads */
          if (current->running && current->tid)
               pthread_kill(current->tid, SIGTERM);
          current = current->next;
     }
}

struct mtftp_thread *tftpd_mtftp_find_server(struct mtftp_data *data, char *filename)
{
     struct mtftp_thread *tmp = data->thread_data;

     while (tmp)
     {
          if (strcmp(filename, tmp->file_name) == 0)
               return tmp;
          tmp = tmp->next;
     }

     return NULL;
}

int tftpd_mtftp_unique(struct mtftp_data *data, char *filename, char *ip, char *port)
{
     struct mtftp_thread *tmp = data->thread_data;

     while (tmp->next)
     {
          if (strcmp(filename, tmp->file_name) == 0)
               return 1;
          if (strcmp(ip, tmp->mcast_ip) == 0)
               if (strcmp(port, tmp->client_port) == 0)
                    return 1;
          tmp = tmp->next;
     }
     return 0;
}

/* 
 * This thread listen to the specified port for read request. If the requested
 * file as been specified in the mtftp.conf file and the server is not currently
 * serving this file, spawn the serving thread.
 */
void *tftpd_mtftp_server(void *arg)
{
     fd_set rfds;               /* for select */
     struct mtftp_data *data = (struct mtftp_data *)arg;
     struct mtftp_thread *thread;

     int sockfd;
     struct sockaddr_in sa;
     socklen_t len = sizeof(struct sockaddr);
#ifdef HAVE_WRAP
     char client_addr[16];
#endif
     int retval;                /* hold return value for testing */
     int data_size;             /* returned size by recvfrom */
     char filename[MAXLEN];
     char string[MAXLEN];       /* hold the string we pass to the logger */

     logger(LOG_NOTICE, "mtftp main server thread started");

     /* initialise sockaddr_in structure */
     memset(&sa, 0, sizeof(sa));
     sa.sin_family = AF_INET;
     sa.sin_addr.s_addr = htonl(INADDR_ANY);
     sa.sin_port = htons(data->server_port);

     /* open the socket */
     if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
     {
          logger(LOG_ERR, "mtftp: can't open socket");
          pthread_exit(NULL);
     }
     /* bind the socket to the tftp port  */
     if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
     {
          logger(LOG_ERR, "mtftp: can't bind port");
          pthread_exit(NULL);
     }

     while (!tftpd_cancel)
     {
          FD_ZERO(&rfds);
          FD_SET(sockfd, &rfds);

          select(sockfd + 1, &rfds, NULL, NULL, NULL);

          if (FD_ISSET(sockfd, &rfds) && (!tftpd_cancel))
          {
               /* read the data packet and verify it's a RRQ and a thread exist for
                  that file name */
               memset(&sa, 0, sizeof(sa)); /* this will hold the client info */
               data_size = data->data_buffer_size;
               retval = tftp_get_packet(sockfd, -1, NULL, &sa, NULL, NULL,
                                        data->timeout,
                                        &data_size, data->data_buffer);

#ifdef HAVE_WRAP
               /* Verify the client has access. We don't look for the name but
                  rely only on the IP address for that. */
               inet_ntop(AF_INET, &sa.sin_addr,
                         client_addr, sizeof(client_addr));
               if (hosts_ctl("in.tftpd", STRING_UNKNOWN, client_addr,
                             STRING_UNKNOWN) == 0)
               {
                    logger(LOG_ERR, "mtftp: connection refused from %s", client_addr);
                    continue;
               }
#endif
               /* read options from this request */
               opt_parse_request(data->data_buffer, data_size,
                                 data->tftp_options);
               opt_request_to_string(data->tftp_options, string, MAXLEN);
               Strncpy(filename, data->tftp_options[OPT_FILENAME].value,
                       MAXLEN);
               /* verify this is a RRQ */
               if (retval != GET_RRQ)
               {
                    logger(LOG_WARNING, "unsupported request <%d> from %s",
                           retval, inet_ntoa(sa.sin_addr));
                    tftp_send_error(sockfd, &sa, EBADOP, data->data_buffer, data->data_buffer_size);
                    if (data->trace)
                         logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EBADOP,
                                tftp_errmsg[EBADOP]);
                    continue;
               }
               else
               {
                    logger(LOG_NOTICE, "Serving %s to %s:%d", filename,
                           inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
                    if (data->trace)
                         logger(LOG_DEBUG, "received RRQ <%s>", string);
               }
               /* validity check, only octet mode supported */
               if (strcasecmp(data->tftp_options[OPT_MODE].value, "octet") != 0)
               {
                    logger(LOG_WARNING, "mtftp: support only octet mode");
                    continue;
               }
               /* file name verification */
               if (tftpd_rules_check(filename) != OK)
               {
                    logger(LOG_WARNING, "mtftp: file name rules violated %s", filename);
                    continue;
               }
               /* find server for this file*/
               if ((thread = tftpd_mtftp_find_server(data, filename)) == NULL)
               {
                    logger(LOG_WARNING, "mtftp: no server found for file %s", filename);
                    continue;
               }
               if (thread->running)
               {
                    logger(LOG_NOTICE, "mtftp: already serving this file");
                    continue;
               }
               /* copy client info for server */
               memcpy(&thread->sa_in, &sa, sizeof(struct sockaddr_in));
               /* open a socket for client communication */
               if ((thread->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
               {
                    logger(LOG_ERR, "mtftp: can't open socket");
                    pthread_exit(NULL);
               }
               getsockname(sockfd, (struct sockaddr *)&(sa), &len);
               //memset(&sa, 0, sizeof(sa));
               sa.sin_port = 0;
               /* bind the socket to the tftp port  */
               if (bind(thread->sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
               {
                    logger(LOG_ERR, "mtftp: can't bind port");
                    pthread_exit(NULL);
               }
               getsockname(thread->sockfd, (struct sockaddr *)&(sa), &len);

               /* configure multicast socket */
               thread->mcastaddr.imr_multiaddr.s_addr = thread->sa_mcast.sin_addr.s_addr;
               thread->mcastaddr.imr_interface.s_addr = htonl(INADDR_ANY);
               setsockopt(thread->sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
                          &data->mcast_ttl, sizeof(data->mcast_ttl));

               /* give server thread access to mtftp options */
               thread->mtftp_data = data;

               /* mask the thread as running */
               thread->running = 1;

               /* spawn the new thread */
               /* Start a new server thread. */
               if (pthread_create(&thread->tid, NULL, tftpd_mtftp_send_file,
                                  (void *)thread) != 0)
               {
                    logger(LOG_ERR, "mtftp: failed to start new thread");
                    thread->running = 0;
               }
          }
     }

     tftpd_mtftp_clean(data);

     logger(LOG_NOTICE, "mtftp main server thread exiting");

     pthread_exit(NULL);
}

void *tftpd_mtftp_send_file(void *arg)
{
     int state = S_BEGIN;
     int timeout_state = state;
     int result;
     int block_number = 0;
     int last_block = -1;
     int data_size;

     struct mtftp_thread *data = (struct mtftp_thread *)arg;
     struct sockaddr_in *sa = &data->sa_in;
     struct sockaddr_in from;
     int sockfd = data->sockfd;

     struct tftphdr *tftphdr = (struct tftphdr *)data->data_buffer;
     char string[MAXLEN];
     int number_of_timeout = 0;

     /* Detach ourself. That way the main thread does not have to
      * wait for us with pthread_join. */
     pthread_detach(pthread_self());

     /* sockets are opened and every as been initialised for us,
        just proceed */     
     while (1)
     {
          if (tftpd_cancel)
          {
               logger(LOG_DEBUG, "thread cancelled");
               tftp_send_error(sockfd, sa, EUNDEF, data->data_buffer, data->data_buffer_size);
               if (data->mtftp_data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EUNDEF,
                           tftp_errmsg[EUNDEF]);
               state = S_ABORT;
          }
          switch (state)
          {
          case S_BEGIN:
               /* The first data packet as to be sent to the unicast address
                  of the client */
               timeout_state = state;
               fseek(data->fp, block_number * (data->data_buffer_size - 4),
                     SEEK_SET);
               /* read data from file */
               data_size = fread(tftphdr->th_data, 1,
                                 data->data_buffer_size - 4, data->fp) + 4;
               /* record the last block number */
               if (feof(data->fp))
                    last_block = block_number;
               /* send data to unicast address */
               tftp_send_data(sockfd, sa, block_number + 1,
                              data_size, data->data_buffer);
               if (data->mtftp_data->trace)
                    logger(LOG_DEBUG, "sent DATA <block: %d, size %d>",
                           block_number + 1, data_size - 4);
               state = S_WAIT_PACKET;
               break;
          case S_SEND_DATA:
               timeout_state = state;
               fseek(data->fp, block_number * (data->data_buffer_size - 4),
                     SEEK_SET);
               /* read data from file */
               data_size = fread(tftphdr->th_data, 1,
                                 data->data_buffer_size - 4, data->fp) + 4;
               /* record the last block number */
               if (feof(data->fp))
                    last_block = block_number;
               /* send data to multicast address */
               tftp_send_data(sockfd, &data->sa_mcast,
                              block_number + 1, data_size,
                              data->data_buffer);
               if (data->mtftp_data->trace)
                    logger(LOG_DEBUG, "sent DATA <block: %d, size %d>",
                           block_number + 1, data_size - 4);
               state = S_WAIT_PACKET;
               break;
          case S_WAIT_PACKET:
               data_size = data->data_buffer_size;
               result = tftp_get_packet(sockfd, -1, NULL, sa, &from, NULL,
                                        data->mtftp_data->timeout,
                                        &data_size, data->data_buffer);

               switch (result)
               {
               case GET_TIMEOUT:
                    number_of_timeout++;
                    
                    if (number_of_timeout > NB_OF_RETRY)
                    {
                         logger(LOG_INFO, "client (%s) not responding",
                                inet_ntoa(data->sa_in.sin_addr));
                         state = S_END;
                         break;
                    }
                    logger(LOG_WARNING, "timeout: retrying...");
                    state = timeout_state;
                    break;
               case GET_ACK:
                    if (sa->sin_port != from.sin_port)
                    {
                         logger(LOG_WARNING, "packet discarded");
                         break;
                    }
                    /* handle case where packet come from un unexpected client */
                    if ((sa->sin_port == from.sin_port) &&
                        (sa->sin_addr.s_addr == from.sin_addr.s_addr))
                    {
                         /* The ACK is from the exected client */
                         number_of_timeout = 0;
                         block_number = ntohs(tftphdr->th_block);
                         if (data->mtftp_data->trace)
                              logger(LOG_DEBUG, "received ACK <block: %d>",
                                     block_number);
                         if ((last_block != -1) && (block_number > last_block))
                         {
                              state = S_END;
                              break;
                         }
                         state = S_SEND_DATA;
                    }
                    break;
               case GET_ERROR:
                    if (sa->sin_port != from.sin_port)
                    {
                         logger(LOG_WARNING, "packet discarded");
                         break;
                    }
                    /* handle case where packet come from un unexpected client */
                    if ((sa->sin_port == from.sin_port) &&
                        (sa->sin_addr.s_addr == from.sin_addr.s_addr))
                    {
                         /* Got an ERROR from the current master client */
                         Strncpy(string, tftphdr->th_msg,
                                 (((data_size - 4) > MAXLEN) ? MAXLEN :
                                  (data_size - 4)));
                         if (data->mtftp_data->trace)
                              logger(LOG_DEBUG, "received ERROR <code: %d, msg: %s>",
                                     ntohs(tftphdr->th_code), string);
                         state = S_ABORT;
                    }
                    break;
               case GET_DISCARD:
                    logger(LOG_WARNING, "packet discarded");
                    break;
               case ERR:
                    logger(LOG_ERR, "%s: %d: recvfrom: %s",
                           __FILE__, __LINE__, strerror(errno));
                    state = S_ABORT;
                    break;
               default:
                    logger(LOG_ERR, "%s: %d: abnormal return value %d",
                           __FILE__, __LINE__, result);
               }
               break;
          case S_END:
               logger(LOG_DEBUG, "End of transfer");
               state = S_EXIT;
               break;
          case S_ABORT:
               logger(LOG_DEBUG, "Aborting transfer");
               state = S_EXIT;
               break;
          case S_EXIT:
               data->running = 0;
               data->tid = 0;
               pthread_exit(NULL);
          default:
               logger(LOG_ERR, "%s: %d: abnormal condition",
                      __FILE__, __LINE__);
               state = S_EXIT;
          }
     }
}

#endif
