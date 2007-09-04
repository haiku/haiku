/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd.c
 *    main server file
 *
 * $Id: tftpd.c,v 1.61 2004/02/27 02:05:26 jp Exp $
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <getopt.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h> 
#include <pthread.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#ifdef HAVE_WRAP
#include <tcpd.h>
#endif
#include "tftpd.h"
#include "tftp_io.h"
#include "tftp_def.h"
#include "logger.h"
#include "options.h"
#include "stats.h"
#ifdef HAVE_PCRE
#include "tftpd_pcre.h"
#endif
#ifdef HAVE_MTFTP
#include "tftpd_mtftp.h"
#endif

#undef RATE_CONTROL

/*
 * Global variables set by main when starting. Read-only for threads
 */
int tftpd_max_thread = 100;     /* number of concurent thread allowed */
int tftpd_timeout = 300;        /* number of second of inactivity
                                   before exiting */
char directory[MAXLEN] = "/tftpboot/";
int retry_timeout = S_TIMEOUT;

int tftpd_daemon = 0;           /* By default we are started by inetd */
int tftpd_daemon_no_fork = 0;   /* For who want a false daemon mode */
short tftpd_port = 0;           /* Port atftpd listen to */
char tftpd_addr[MAXLEN] = "";   /* IP address atftpd binds to */

int tftpd_cancel = 0;           /* When true, thread must exit. pthread
                                   cancellation point are not used because
                                   thread id are not tracked. */
char *pidfile = NULL;           /* File to write own's pid */

pthread_t main_thread_id;

/*
 * Multicast related. These reflect option from command line
 * argument --mcast-ttl, --mcast-port and  --mcast-addr
 */
int mcast_ttl = 1;
char mcast_addr[MAXLEN] = "239.255.0.0-255";
char mcast_port[MAXLEN] = "1758";

/*
 * "logging level" is the maximum error level that will get logged.
 *  This can be increased with the -v switch.
 */
int logging_level = LOG_NOTICE;
char *log_file = NULL;

/* logging level as requested by libwrap */
#ifdef HAVE_WRAP
int allow_severity = LOG_NOTICE;
int deny_severity = LOG_NOTICE;
#endif

/* user ID and group ID when running as a daemon */
char user_name[MAXLEN] = "nobody";
char group_name[MAXLEN] = "nogroup";

/* For special uses, disable source port checking */
int source_port_checking = 1;

/* Special feature to make the server switch to new client as soon as
   the current client timeout in multicast mode */
int mcast_switch_client = 0;

int trace = 0;

#ifdef HAVE_PCRE
/* Use for PCRE file name substitution */
tftpd_pcre_self_t *pcre_top = NULL;
char *pcre_file;
#endif

#ifdef HAVE_MTFTP
/* mtftp options */
struct mtftp_data *mtftp_data = NULL;
char mtftp_file[MAXLEN] = "";
int mtftp_sport = 75;
#endif

#ifdef RATE_CONTROL
int rate = 0;
#endif

/*
 * We need a lock on stdin from the time we notice fresh data coming from
 * stdin to the time the freshly created server thread as read it.
 */
pthread_mutex_t stdin_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Function defined in this file
 */
void *tftpd_receive_request(void *);
void signal_handler(int signal);
int tftpd_cmd_line_options(int argc, char **argv);
void tftpd_log_options(void);
int tftpd_pid_file(char *file, int action);
void tftpd_usage(void);

/*
 * Main thread. Do required initialisation and then go through a loop
 * listening for client requests. When a request arrives, we allocate
 * memory for a thread data structure and start a thread to serve the
 * new client. If theres no activity for more than 'tftpd_timeout'
 * seconds, we exit and tftpd must be respawned by inetd.
 */
int main(int argc, char **argv)
{
     fd_set rfds;               /* for select */
     struct timeval tv;         /* for select */
     int run = 1;               /* while (run) loop */
     struct thread_data *new;   /* for allocation of new thread_data */
     int sockfd;                /* used in daemon mode */
     struct sockaddr_in sa;     /* used in daemon mode */
     struct servent *serv;
     struct passwd *user;
     struct group *group;

#ifdef HAVE_MTFTP
     pthread_t mtftp_thread;
#endif

#ifdef RATE_CONTROL
     struct timeval current_time = {0, 0}; /* For rate control */
     long long current = 0L;
     struct timeval previous_time = {0, 0};
     long long previous = 0L;
     int rcvbuf;                /* size of input socket buffer */
#endif
     int one = 1;               /* for setsockopt() */

     /*
      * Parse command line options. We parse before verifying
      * if we are running on a tty or not to make it possible to
      * verify the command line arguments
      */
     if (tftpd_cmd_line_options(argc, argv) == ERR)
          exit(1);

     /*
      * Can't be started from the prompt without explicitely specifying 
      * the --daemon option.
      */
     if (isatty(0) && !(tftpd_daemon))
     {
          tftpd_usage();
          exit(1);
     }

     /* Using syslog facilties through a wrapper. This call divert logs
      * to a file as specified or to syslog if no file specified. Specifying
      * /dev/stderr or /dev/stdout will work if the server is started in
      * daemon mode.
      */
     open_logger("atftpd", log_file, logging_level);
     logger(LOG_NOTICE, "Advanced Trivial FTP server started (%s)", VERSION);

#ifdef HAVE_PCRE
     /*  */
     if (pcre_file)
     {
          if ((pcre_top = tftpd_pcre_open(pcre_file)) == NULL)
               logger(LOG_WARNING, "Failed to initialise PCRE, continuing anyway.");
     }
#endif

#ifdef RATE_CONTROL
     /* Some tuning */
     rcvbuf = 128;
     if (setsockopt(0, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf)) == 0)
     {
          logger(LOG_WARNING, "Failed to set socket option: %s", strerror(errno));
     }
     logger(LOG_DEBUG, "Receive socket buffer set to %d bytes", rcvbuf);
#endif

     /*
      * If tftpd is run in daemon mode ...
      */
     if (tftpd_daemon)
     {
          /* daemonize here */
          if (!tftpd_daemon_no_fork)
          {
               if (daemon(0, 0) == -1)
                    exit(2);
          }

          /* find the port */
          if (tftpd_port == 0)
          {
               if ((serv = getservbyname("tftp", "udp")) == NULL)
               {
                    logger(LOG_ERR, "atftpd: udp/tftp, unknown service");
                    exit(1);
               }
               tftpd_port = ntohs(serv->s_port);
          }
          /* initialise sockaddr_in structure */
          memset(&sa, 0, sizeof(sa));
          sa.sin_family = AF_INET;
          sa.sin_port = htons(tftpd_port);
          if (strlen(tftpd_addr) > 0)
          {
               if (inet_aton(tftpd_addr, &(sa.sin_addr)) == 0)
               {
                    logger(LOG_ERR, "atftpd: invalid IP address %s", tftpd_addr);
                    exit(1);
               }
          }
          else
               sa.sin_addr.s_addr = htonl(INADDR_ANY);
          /* open the socket */
          if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
          {
               logger(LOG_ERR, "atftpd: can't open socket");
               exit(1);
          }
          /* bind the socket to the desired address and port  */
          if (bind(sockfd, (struct sockaddr*)&sa, sizeof(sa)) < 0)
          {
               logger(LOG_ERR, "atftpd: can't bind port %s:%d/udp",
                      tftpd_addr, tftpd_port);
               exit(1);
          }
          /*
           * dup sockfd on 0 only, not 1,2 like inetd do to allow
           * logging to stderr in pseudo daemon mode (--daemon --no-fork)
           */
          dup2(sockfd, 0);
          close(sockfd);

          /* release priviliedge */
          user = getpwnam(user_name);
          group = getgrnam(group_name);
          if (!user || !group)
          {
               logger(LOG_ERR,
                      "atftpd: can't change identity to %s.%s, exiting.",
                      user_name, group_name);
               exit(1);
          }

          /* write our pid in the specified file before changing user*/
          if (pidfile)
          {
               if (tftpd_pid_file(pidfile, 1) != OK)
                    exit(1);
               /* to be able to remove it later */
               chown(pidfile, user->pw_uid, group->gr_gid);
          }

          setgid(group->gr_gid);
          setuid(user->pw_uid);

          /* Reopen log file now that we changed user, and that we've
           * open and dup2 the socket. */
          open_logger("atftpd", log_file, logging_level);
     }

     /* We need to retrieve some information from incomming packets */
#ifndef __HAIKU__
     if (setsockopt(0, SOL_IP, IP_PKTINFO, &one, sizeof(one)) != 0)
#else
     if (setsockopt(0, IPPROTO_IP, IP_RECVDSTADDR, &one, sizeof(one)) != 0)
#endif
     {
          logger(LOG_WARNING, "Failed to set socket option: %s", strerror(errno));
     }

     /* save main thread ID for proper signal handling */
     main_thread_id = pthread_self();

     /* Register signal handler. */
     signal(SIGINT, signal_handler);
     signal(SIGTERM, signal_handler);

     /* print summary of options */
     tftpd_log_options();

     /* start collecting stats */
     stats_start();

#ifdef HAVE_MTFTP
     /* start mtftp server thread */
     if (strlen(mtftp_file) > 0)
     {
          if (!tftpd_daemon)
               logger(LOG_ERR, "Can't start mtftp server, use --deamon");
          else
          {
               if ((mtftp_data = tftpd_mtftp_init(mtftp_file)) == NULL)
                    logger(LOG_ERR, "Failled initialise mtftp");
               else
               {
                    mtftp_data->server_port = mtftp_sport;
                    mtftp_data->mcast_ttl = mcast_ttl;
                    mtftp_data->timeout = retry_timeout;
                    mtftp_data->checkport = source_port_checking;
                    mtftp_data->trace = trace;
                    /* Start server thread. */
                    if (pthread_create(&mtftp_thread, NULL, tftpd_mtftp_server,
                                       (void *)mtftp_data) != 0)
                    {
                         logger(LOG_ERR, "Failed to start mtftp server thread");
                         tftpd_mtftp_clean(mtftp_data);
                         mtftp_data = NULL;
                    } 
              }
          }
     }
#endif

     /* Wait for read or write request and exit if timeout. */
     while (run)
     {
          /*
           * inetd dups the socket file descriptor to 0, 1 and 2 so we can
           * use any of those as the socket fd. We use 0. stdout and stderr
           * may not be used to print messages.
           */
          FD_ZERO(&rfds);
          FD_SET(0, &rfds);
          tv.tv_sec = tftpd_timeout;
          tv.tv_usec = 0;

          /* We need to lock stdin, and release it when the thread
             is done reading the request. */
          pthread_mutex_lock(&stdin_mutex);

#ifdef RATE_CONTROL          
          /* if we want to implement rate control, we sleep some time here
             so we cannot exceed the allowed thread/sec. */
          if (rate > 0)
          {
               gettimeofday(&current_time, NULL);
               current = (long long)current_time.tv_sec*1000000L + current_time.tv_usec;
               logger(LOG_DEBUG, "time from previous request: %Ld", current - previous);
               if ((current - previous) < (60L*1000000L / (long long)rate))
               {
                    logger(LOG_DEBUG, "will sleep for rate control (%Ld)",
                           (60L*1000000L / (long long)rate) - (current - previous));
                    usleep((60L*1000000L / (long long)rate) - (current - previous));
               }
          }
#endif

          /* A timeout of 0 is interpreted as infinity. Wait for incomming
             packets */
          if (!tftpd_cancel)
          {
               if ((tftpd_timeout == 0) || (tftpd_daemon))
                    select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
               else
                    select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
          }

#ifdef RATE_CONTROL
          /* get the time we receive this packet */
          if (rate > 0)
          {
               gettimeofday(&previous_time, NULL);
               previous = (long long)previous_time.tv_sec*1000000L +
                    previous_time.tv_usec;
          }
#endif

          if (FD_ISSET(0, &rfds) && (!tftpd_cancel))
          {
               /* Allocate memory for thread_data structure. */
               if ((new = calloc(1, sizeof(struct thread_data))) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    exit(1);
               }

               /*
                * Initialisation of thread_data structure.
                */
               pthread_mutex_init(&new->client_mutex, NULL);
               new->sockfd = 0;

               /* Allocate data buffer for tftp transfer. */
               if ((new->data_buffer = malloc((size_t)SEGSIZE + 4)) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    exit(1);
               }
               new->data_buffer_size = SEGSIZE + 4;

               /* Allocate memory for tftp option structure. */
               if ((new->tftp_options = 
                    malloc(sizeof(tftp_default_options))) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    exit(1);
               }

               /* Copy default options. */
               memcpy(new->tftp_options, tftp_default_options,
                      sizeof(tftp_default_options));

               /* default timeout */
               new->timeout = retry_timeout;

               /* wheter we check source port or not */
               new->checkport = source_port_checking;

               /* other options */
               new->mcast_switch_client = mcast_switch_client;
               new->trace = trace;

               /* default ttl for multicast */
               new->mcast_ttl = mcast_ttl;

               /* Allocate memory for a client structure. */
               if ((new->client_info =
                    calloc(1, sizeof(struct client_info))) == NULL)
               {
                    logger(LOG_ERR, "%s: %d: Memory allocation failed",
                           __FILE__, __LINE__);
                    exit(1);
               }
               new->client_info->done = 0;
               new->client_info->next = NULL;
               
               /* Start a new server thread. */
               if (pthread_create(&new->tid, NULL, tftpd_receive_request,
                                  (void *)new) != 0)
               {
                    logger(LOG_ERR, "Failed to start new thread");
                    free(new);
                    pthread_mutex_unlock(&stdin_mutex);
               }
          }
          else
          {
               pthread_mutex_unlock(&stdin_mutex);
               
               /* Either select return after timeout of we've been killed. In the first case
                  we wait for server thread to finish, in the other we kill them */
               if (tftpd_cancel)
                    tftpd_list_kill_threads();

               while (tftpd_list_num_of_thread() != 0)
                    sleep(1);

               run = 0;
               if (tftpd_daemon || (tftpd_timeout == 0))
                    logger(LOG_NOTICE, "atftpd terminating");
               else
                    logger(LOG_NOTICE,
                           "atftpd terminating after %d seconds",
                           tftpd_timeout);
          }
     }

     /* close all open file descriptors */
     close(0);
     close(1);
     close(2);

#ifdef HAVE_MTFTP
     /*  stop the mtftp threads */
     if (mtftp_data)
     {
          /* kill mtftp serving thread */
          tftpd_mtftp_kill_threads(mtftp_data);
          /* kill mtftp main thread */
          pthread_kill(mtftp_thread, SIGTERM);
          pthread_join(mtftp_thread, NULL);
     }
#endif

     /* stop collecting stats and print them*/
     stats_end();
     stats_print();
     
#ifdef HAVE_PCRE
     /* remove allocated memory for tftpd_pcre */
     if (pcre_top)
          tftpd_pcre_close(pcre_top);
#endif

     /* some cleaning */
     if (log_file)
          free(log_file);
#ifdef HAVE_PCRE
     if (pcre_file)
          free(pcre_file);
#endif

     /* remove PID file */
     if (pidfile)
          tftpd_pid_file(pidfile, 0);

     logger(LOG_NOTICE, "Main thread exiting");
     close_logger();
     exit(0);
}

/* 
 * This function handles the initial connection with a client. It reads
 * the request from stdin and then release the stdin_mutex, so the main
 * thread may listen for new clients. After that, we process options and
 * call the sending or receiving function.
 *
 * arg is a thread_data structure pointer for that thread.
 */
void *tftpd_receive_request(void *arg)
{
     struct thread_data *data = (struct thread_data *)arg;
     
     int retval;                /* hold return value for testing */
     int data_size;             /* returned size by recvfrom */
     char string[MAXLEN];       /* hold the string we pass to the logger */
     int num_of_threads;
     int abort = 0;             /* 1 if we need to abort because the maximum
                                   number of threads have been reached*/ 
     struct sockaddr_in to;     /* destination of client's packet */
     socklen_t len = sizeof(struct sockaddr);

#ifdef HAVE_WRAP
     char client_addr[16];
#endif

     /* Detach ourself. That way the main thread does not have to
      * wait for us with pthread_join. */
     pthread_detach(pthread_self());

     /* Read the first packet from stdin. */
     data_size = data->data_buffer_size;     
     retval = tftp_get_packet(0, -1, NULL, &data->client_info->client, NULL,
                              &to, data->timeout, &data_size,
                              data->data_buffer);

     /* now unlock stdin */
     pthread_mutex_unlock(&stdin_mutex);

     /* Verify the number of threads */
     if ((num_of_threads = tftpd_list_num_of_thread()) >= tftpd_max_thread)
     {
          logger(LOG_INFO, "Maximum number of threads reached: %d",
                 num_of_threads);
          abort = 1;
     }

#ifdef HAVE_WRAP
     if (!abort)
     {
          /* Verify the client has access. We don't look for the name but
             rely only on the IP address for that. */
          inet_ntop(AF_INET, &data->client_info->client.sin_addr,
                    client_addr, sizeof(client_addr));
          if (hosts_ctl("in.tftpd", STRING_UNKNOWN, client_addr,
                        STRING_UNKNOWN) == 0)
          {
               logger(LOG_ERR, "Connection refused from %s", client_addr);
               abort = 1;
          }
     }
#endif

     /* Add this new thread structure to the list. */
     if (!abort)
          stats_new_thread(tftpd_list_add(data));

     /* if the maximum number of thread is reached, too bad we abort. */
     if (abort)
          stats_abort_locked();
     else
     {
          /* open a socket for client communication */
          data->sockfd = socket(PF_INET, SOCK_DGRAM, 0);
          to.sin_family = AF_INET;
          to.sin_port = 0;
          if (data->sockfd > 0)
          {
               /* bind the socket to the interface */
               if (bind(data->sockfd, (struct sockaddr *)&to, len) == -1)
               {
                    logger(LOG_ERR, "bind: %s", strerror(errno));
                    retval = ABORT;
               }
               /* read back assigned port */
               len = sizeof(struct sockaddr);
               if (getsockname(data->sockfd, (struct sockaddr *)&to, &len) == -1)
               {
                    logger(LOG_ERR, "getsockname: %s", strerror(errno));
                    retval = ABORT;
               }
               /* connect the socket, faster for kernel operation */
               if (connect(data->sockfd,
                           (struct sockaddr *)&data->client_info->client,
                           sizeof(data->client_info->client)) == -1)
               {
                    logger(LOG_ERR, "connect: %s", strerror(errno));
                    retval = ABORT;
               }
               logger(LOG_DEBUG, "Creating new socket: %s:%d",
                      inet_ntoa(to.sin_addr), ntohs(to.sin_port));
               
               /* read options from request */
               opt_parse_request(data->data_buffer, data_size,
                                 data->tftp_options);
               opt_request_to_string(data->tftp_options, string, MAXLEN);
          }
          else
          {
               retval = ABORT;
          }

          /* Analyse the request. */
          switch (retval)
          {
          case GET_RRQ:
               logger(LOG_NOTICE, "Serving %s to %s:%d",
                      data->tftp_options[OPT_FILENAME].value,
                      inet_ntoa(data->client_info->client.sin_addr),
                      ntohs(data->client_info->client.sin_port));
               if (data->trace)
                    logger(LOG_DEBUG, "received RRQ <%s>", string);
               if (tftpd_send_file(data) == OK)
                    stats_send_locked();
               else
                    stats_err_locked();
               break;
          case GET_WRQ:
               logger(LOG_NOTICE, "Fetching from %s to %s",
                      inet_ntoa(data->client_info->client.sin_addr),
                      data->tftp_options[OPT_FILENAME].value);
               if (data->trace)
                    logger(LOG_DEBUG, "received WRQ <%s>", string);
               if (tftpd_receive_file(data) == OK)
                    stats_recv_locked();
               else
                    stats_err_locked();
               break;
          case ERR:
               logger(LOG_ERR, "Error from tftp_get_packet");
               tftp_send_error(data->sockfd, &data->client_info->client,
                               EUNDEF, data->data_buffer, data->data_buffer_size);
               if (data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EUNDEF,
                           tftp_errmsg[EUNDEF]);
               stats_err_locked();
               break;
          case ABORT:
               if (data->trace)
                    logger(LOG_ERR, "thread aborting");
               stats_err_locked();
               break;
          default:
               logger(LOG_NOTICE, "Invalid request <%d> from %s",
                      retval, inet_ntoa(data->client_info->client.sin_addr));
               tftp_send_error(data->sockfd, &data->client_info->client,
                               EBADOP, data->data_buffer, data->data_buffer_size);
               if (data->trace)
                    logger(LOG_DEBUG, "sent ERROR <code: %d, msg: %s>", EBADOP,
                           tftp_errmsg[EBADOP]);
               stats_err_locked();
          }
     }  

     /* make sure all data is sent to the network */
     if (data->sockfd)
     {
          fsync(data->sockfd);
          close(data->sockfd);
     }

     /* update stats */
     stats_thread_usage_locked();

     /* Remove the thread_data structure from the list, if it as been
        added. */
     if (!abort)
          tftpd_list_remove(data);

     /* Free memory. */
     if (data->data_buffer)
          free(data->data_buffer);

     free(data->tftp_options);

     /* if the thread had reserverd a multicast IP/Port, deallocate it */
     if (data->mc_port != 0)
          tftpd_mcast_free_tid(data->mc_addr, data->mc_port);

     /* this function take care of freeing allocated memory by other threads */
     tftpd_clientlist_free(data);

     /* free the thread structure */
     free(data);
     
     logger(LOG_INFO, "Server thread exiting");
     pthread_exit(NULL);
}

/*
 * If we receive signals, we must exit in a clean way. This means
 * sending an ERROR packet to all clients to terminate the connection.
 */
void signal_handler(int signal)
{
     /* Any thread may receive the signal, always make sure the main thread receive it
        and cancel other threads itself. However, if tftpd_cancel already set, just
        ignore the signal since the master thread as already got the signal. */
     if (pthread_self() != main_thread_id)
     {
          if (tftpd_cancel == 0)
          {
               logger(LOG_ERR, "Forwarding signal to main thread");
               pthread_kill(main_thread_id, signal);
          }
          return;
     }

     /* Only signals for the main thread get there */
     switch (signal)
     {
     case SIGINT:
          logger(LOG_ERR, "SIGINT received, stopping threads and exiting.");
          tftpd_cancel = 1;
          break;
     case SIGTERM:
          logger(LOG_ERR, "SIGTERM received, stopping threads and exiting.");
          tftpd_cancel = 1;
          break;
     default:
          logger(LOG_WARNING, "Signal %d received, ignoring.", signal);
          break;
     }
}

#define OPT_RATE       'R'
#define OPT_MCAST_TTL  '0'
#define OPT_MCAST_ADDR '1'
#define OPT_MCAST_PORT '2'
#define OPT_PCRE       '3'
#define OPT_PCRE_TEST  '4'
#define OPT_PORT_CHECK '5'
#define OPT_MCAST_SWITCH '6'
#define OPT_MTFTP      '7'
#define OPT_MTFTP_PORT '8'
#define OPT_TRACE      '9'

/*
 * Parse the command line using the standard getopt function.
 */
int tftpd_cmd_line_options(int argc, char **argv)
{
     int c;
     char *tmp;
#ifdef HAVE_PCRE
     char string[MAXLEN], out[MAXLEN];
#endif
     static struct option options[] = {
          { "tftpd-timeout", 1, NULL, 't' },
          { "retry-timeout", 1, NULL, 'r' },
          { "maxthread", 1, NULL, 'm' },
#ifdef RATE_CONTROL
          { "rate", 1, NULL, OPT_RATE },
#endif
          { "verbose", 2, NULL, 'v' },
          { "trace", 0, NULL, OPT_TRACE },
          { "no-timeout", 0, NULL, 'T' },
          { "no-tsize", 0, NULL, 'S' },
          { "no-blksize", 0, NULL, 'B' },
          { "no-multicast", 0, NULL, 'M' },
          { "logfile", 1, NULL, 'L' },
          { "pidfile", 1, NULL, 'I'},
          { "daemon", 0, NULL, 'D' },
          { "no-fork", 0, NULL, 'N'},
          { "user", 1, NULL, 'U'},
          { "group", 1, NULL, 'G'},
          { "port", 1, NULL, 'P' },
          { "bind-address", 1, NULL, 'A'},
          { "mcast-ttl", 1, NULL, OPT_MCAST_TTL },
          { "mcast_ttl", 1, NULL, OPT_MCAST_TTL },
          { "mcast-addr", 1, NULL, OPT_MCAST_ADDR },
          { "mcast_addr", 1, NULL, OPT_MCAST_ADDR },
          { "mcast-port", 1, NULL, OPT_MCAST_PORT },
          { "mcast_port", 1, NULL, OPT_MCAST_PORT },
#ifdef HAVE_PCRE
          { "pcre", 1, NULL, OPT_PCRE },
          { "pcre-test", 1, NULL, OPT_PCRE_TEST },
#endif
#ifdef HAVE_MTFTP
          { "mtftp", 1, NULL, OPT_MTFTP },
          { "mtftp-port", 1, NULL, OPT_MTFTP_PORT },
#endif
          { "no-source-port-checking", 0, NULL, OPT_PORT_CHECK },
          { "mcast-switch-client", 0, NULL, OPT_MCAST_SWITCH },
          { "version", 0, NULL, 'V' },
          { "help", 0, NULL, 'h' },
          { 0, 0, 0, 0 }
     };
     
     while ((c = getopt_long(argc, argv, "t:r:m:v::Vh",
                             options, NULL)) != EOF)
     {
          switch (c)
          {
          case 't':
               tftpd_timeout = atoi(optarg);
               break;
          case 'r':
               retry_timeout = atoi(optarg);
               break;
          case 'm':
               tftpd_max_thread = atoi(optarg);
               break;
#ifdef RATE_CONTROL
          case OPT_RATE:
               rate = atoi(optarg);
               break;
#endif
          case 'v':
               if (optarg)
                    logging_level = atoi(optarg);
               else
                    logging_level++;
#ifdef HAVE_WRAP
               allow_severity = logging_level;
               deny_severity = logging_level;
#endif
               break;
          case OPT_TRACE:
               trace = 1;
               break;
          case 'T':
               tftp_default_options[OPT_TIMEOUT].enabled = 0;
               break;
          case 'S':
               tftp_default_options[OPT_TSIZE].enabled = 0;
               break;
          case 'B':
               tftp_default_options[OPT_BLKSIZE].enabled = 0;
               break;
          case 'M':
               tftp_default_options[OPT_MULTICAST].enabled = 0;
               break;
          case 'L':
               log_file = strdup(optarg);
               break;
          case 'I':
               pidfile = strdup(optarg);
               break;
          case 'D':
               tftpd_daemon = 1;
               break;
          case 'N':
               tftpd_daemon_no_fork = 1;
               break;
          case 'U':
               tmp = strtok(optarg, ".");
               if (tmp != NULL)
                    Strncpy(user_name, tmp, MAXLEN);
               tmp = strtok(NULL, "");
               if (tmp != NULL)
                    Strncpy(group_name, optarg, MAXLEN);
               break;
          case 'G':
               Strncpy(group_name, optarg, MAXLEN);
               break;
          case 'P':
               tftpd_port = (short)atoi(optarg);
               break;
          case 'A': 
               Strncpy(tftpd_addr, optarg, MAXLEN);
               break;
          case OPT_MCAST_TTL:
               mcast_ttl = atoi(optarg);
               break;
          case OPT_MCAST_ADDR:
               Strncpy(mcast_addr, optarg, MAXLEN);
               break;
          case '2':
               Strncpy(mcast_port, optarg, MAXLEN);
               break;
#ifdef HAVE_PCRE
          case OPT_PCRE:
               pcre_file = strdup(optarg);
               break;
          case OPT_PCRE_TEST:
               /* test the pattern file */
               if ((pcre_top = tftpd_pcre_open(optarg)) == NULL)
               {
                    fprintf(stderr, "Failed to initialise PCRE with file %s", optarg);
                    exit(1);
               }
               /* run test */
               while (1)
               {
                    if (isatty(0))
                         printf("> ");
                    if (fgets(string, MAXLEN, stdin) == NULL)
                    {
                         tftpd_pcre_close(pcre_top);
                         exit(0);
                    }
                    /* exit on empty line */
                    if (string[0] == '\n')
                    {
                         tftpd_pcre_close(pcre_top);
                         exit(0);
                    }
                    /* remove \n from input */
                    string[strlen(string) - 1] = '\0';
                    /* do the substitution */
                    if (tftpd_pcre_sub(pcre_top, out, MAXLEN, string) < 0)
                         printf("Substitution: \"%s\" -> \"\"\n", string);
                    else
                         printf("Substitution: \"%s\" -> \"%s\"\n", string, out);
               }
#endif
          case OPT_PORT_CHECK:
               source_port_checking = 0;
               break;
          case OPT_MCAST_SWITCH:
               mcast_switch_client = 1;
               break;
#ifdef HAVE_MTFTP
          case OPT_MTFTP:
               Strncpy(mtftp_file, optarg, MAXLEN);         
               break;
          case OPT_MTFTP_PORT:
               mtftp_sport = atoi(optarg);
               break;
#endif
          case 'V':
               printf("atftp-%s (server)\n", VERSION);
               exit(0);
          case 'h':
               tftpd_usage();
               exit(0);
          case '?':
               exit(1);
               break;
          }
     }
          
     /* verify that only one arguement is left */
     if (optind < argc)
          Strncpy(directory, argv[optind], MAXLEN);
     /* make sure the last caracter is a / */
     if (directory[strlen(directory)] != '/')
          strcat(directory, "/");
     /* build multicast address/port range */
     if (tftpd_mcast_parse_opt(mcast_addr, mcast_port) != OK)
          exit(1);
     return OK;
}

/*
 * Output option to the syslog.
 */
void tftpd_log_options(void)
{
     if (tftpd_daemon == 1)
     {
          logger(LOG_INFO, "  running in daemon mode on port %d", tftpd_port);
          if (strlen(tftpd_addr) > 0)
               logger(LOG_INFO, "  bound to IP address %s only", tftpd_addr);
     }
     else
          logger(LOG_INFO, "  started by inetd");
     logger(LOG_INFO, "  logging level: %d", logging_level);
     if (trace)
          logger(LOG_INFO, "     trace enabled");
     logger(LOG_INFO, "  directory: %s", directory);
     logger(LOG_INFO, "  user: %s.%s", user_name, group_name);
     logger(LOG_INFO, "  log file: %s", (log_file==NULL) ? "syslog":log_file);
     if (pidfile)
          logger(LOG_INFO, "  pid file: %s", pidfile);
     if (tftpd_daemon == 1)
          logger(LOG_INFO, "  server timeout: Not used");
     else
          logger(LOG_INFO, "  server timeout: %d", tftpd_timeout);
     logger(LOG_INFO, "  tftp retry timeout: %d", retry_timeout);
     logger(LOG_INFO, "  maximum number of thread: %d", tftpd_max_thread);
#ifdef RATE_CONTROL
     if (rate > 0)
          logger(LOG_INFO, "  request per minute limit: %d", rate);
     else
          logger(LOG_INFO, "  request per minute limit: ---");
#endif
     logger(LOG_INFO, "  option timeout:   %s",
            tftp_default_options[OPT_TIMEOUT].enabled ? "enabled":"disabled");
     logger(LOG_INFO, "  option tzise:     %s",
            tftp_default_options[OPT_TSIZE].enabled ? "enabled":"disabled");
     logger(LOG_INFO, "  option blksize:   %s",
            tftp_default_options[OPT_BLKSIZE].enabled ? "enabled":"disabled");
     logger(LOG_INFO, "  option multicast: %s",
            tftp_default_options[OPT_MULTICAST].enabled ? "enabled":"disabled");
     logger(LOG_INFO, "     address range: %s", mcast_addr);
     logger(LOG_INFO, "     port range:    %s", mcast_port);
#ifdef HAVE_PCRE
     if (pcre_top)
          logger(LOG_INFO, "  PCRE: using file: %s", pcre_file);
#endif
#ifdef HAVE_MTFTP
     if (strcmp(mtftp_file, "") != 0)
     {
          logger(LOG_INFO, "  mtftp: using file: %s", mtftp_file);
          logger(LOG_INFO, "  mtftp: listenning on port %d", mtftp_sport);
     }
#endif
     if (mcast_switch_client)
          logger(LOG_INFO, "  --mcast-switch-client turned on");
     if (!source_port_checking)
          logger(LOG_INFO, "  --no-source-port-checking turned on");
}

/* 
 *
 */
int tftpd_pid_file(char *file, int action)
{
     FILE *fp;
     pid_t pid;

     if (action)
     {
          /* check if file exist */
          if ((fp = fopen(file, "r")) != NULL)
          {
               logger(LOG_NOTICE, "pid file already exist, overwriting");
               fclose(fp);
          }
          /* open file for writing */
          if ((fp = fopen(file, "w")) == NULL)
          {
               logger(LOG_ERR, "error writing PID to file %s\n", file);
               return ERR;
          }
          /* write it */
          pid = getpid();     
          fprintf(fp, "%d\n", pid);
          fclose(fp);
          return OK;
     }
     else
     {
          /* unlink the pid file */
          if (unlink(file) == -1)
               logger(LOG_ERR, "unlink: %s", strerror(errno));
          return OK;
     }
}

/*
 * Show a nice usage...
 */
void tftpd_usage(void)
{
     printf("Usage: tftpd [options] [directory]\n"
            " [options] may be:\n"
            "  -t, --tftpd-timeout <value>: number of second of inactivity"
            " before exiting\n"
            "  -r, --retry-timeout <value>: time to wait a reply before"
            " retransmition\n"
            "  -m, --maxthread <value>    : number of concurrent thread"
            " allowed\n"
#ifdef RATE_CONTROL
            "  --rate <value>             : number of request per minute limit\n"
#endif
            "  -v, --verbose [value]      : increase or set the level of"
            " output messages\n"
            "  --trace                    : log all sent and received packets\n"
            "  --no-timeout               : disable 'timeout' from RFC2349\n"
            "  --no-tisize                : disable 'tsize' from RFC2349\n"
            "  --no-blksize               : disable 'blksize' from RFC2348\n"
            "  --no-multicast             : disable 'multicast' from RFC2090\n"
            "  --logfile <file>           : logfile to log logs to ;-)\n"
            "  --pidfile <file>           : write PID to this file\n"
            "  --daemon                   : run atftpd standalone (no inetd)\n"
            "  --no-fork                  : run as a daemon, don't fork\n"
            "  --user <user[.group]>      : default is nobody\n"
            "  --group <group>            : default is nogroup\n"
            "  --port <port>              : port on which atftp listen\n"
            "  --bind-address <IP>        : local address atftpd listen to\n"
            "  --mcast-ttl                : ttl to used for multicast\n"
            "  --mcast-addr <address list>: list/range of IP address to use\n"
            "  --mcast-port <port range>  : ports to use for multicast"
            " transfer\n"
#ifdef HAVE_PCRE
            "  --pcre <file>              : use this file for pattern replacement\n"
            "  --pcre-test <file>         : just test pattern file, not starting server\n"
#endif
#ifdef HAVE_MTFTP
            "  --mtftp <file>             : mtftp configuration file\n"
            "  --mtftp-port <port>        : port mtftp will listen\n"
#endif
            "  --no-source-port-checking  : violate RFC, see man page\n"
            "  --mcast-switch-client      : switch client on first timeout, see man page\n"
            "  -V, --version              : print version information\n"
            "  -h, --help                 : print this help\n"
            "\n"
            " [directory] must be a world readable/writable directories.\n"
            " By default /tftpboot is assumed."
            "\n");
}
