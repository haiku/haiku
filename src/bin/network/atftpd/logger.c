/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * logger.c
 *    functions for logging messages.
 *
 * $Id: logger.c,v 1.12 2004/02/27 02:05:26 jp Exp $
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
#include <sys/types.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include "logger.h"

#define MAXLEN 128

static int log_syslog_is_open = 0;
static int log_priority = 0;
static char *log_filename = NULL;
static int log_fd;
static FILE *log_fp = NULL;
static char *log_ident;

/*
 * Open a file for logging. If filename is NULL, then use
 * stderr for the client or the syslog for the server. Log
 * only message less or equal to priority.
 */
void open_logger(char *ident, char *filename, int priority)
{
     close_logger(); /* make sure we initialise variables and close
                        previously opened log. */

     log_priority = priority;

     if (ident)
          log_ident = strdup(ident);
     else
          log_ident = "unset";

     if (filename)
          log_filename = strdup(filename);
     else
     {
          openlog(log_ident, LOG_PID, LOG_DAEMON);
          log_syslog_is_open = 1;
     }

     if (log_filename)
     {
          if ((log_fd = open(log_filename, O_WRONLY | O_APPEND)) < 0)
          {
               openlog(log_ident, LOG_PID, LOG_DAEMON);
               log_syslog_is_open = 1;
               logger(LOG_CRIT, "Unable to open %s for logging, "
                           "reverting to syslog", log_filename);
          }
          else
               log_fp = fdopen(log_fd, "a");
     }
}

/*
 * Same as syslog but allow to format a string, like printf, when logging to
 * file. This fonction will either call syslog or fprintf depending of the
 * previous call to open_logger().
 */
void logger(int severity, const char *fmt, ...)
{
     char message[MAXLEN];
     char time_buf[MAXLEN];
     char hostname[MAXLEN];
     time_t t;
     struct tm *tm;


     va_list args;
     va_start(args, fmt);

     time(&t);
     tm = localtime(&t);
     strftime(time_buf, MAXLEN, "%b %d %H:%M:%S", tm);
     gethostname(hostname, MAXLEN);

     if (severity <= log_priority)
     {
          vsnprintf(message, sizeof(message), fmt, args);
          
          if (log_fp)
          {
               fprintf(log_fp, "%s %s %s[%d.%d]: %s\n", time_buf, hostname,
                       log_ident, getpid(), pthread_self(), message);
               fflush(log_fp);
          }
          else if (log_syslog_is_open)
               syslog(severity, "%s", message);
          else
               fprintf(stderr, "%s %s %s[%d.%d]: %s\n", time_buf, hostname,
                       log_ident, getpid(), pthread_self(), message);
     }
     va_end(args);
}

/*
 * Close the file or syslog. Initialise variables.
 */
void close_logger(void)
{
     log_priority = 0;
     if (log_syslog_is_open)
          closelog();
     log_syslog_is_open = 0;
     if (log_fp)
          fclose(log_fp);
     log_fp = NULL;
     if (log_filename)
          free(log_filename);
     log_filename = NULL;
     if (log_ident)
          free(log_ident);
}
