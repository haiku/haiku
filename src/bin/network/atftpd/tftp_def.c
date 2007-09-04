/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftp_def.c
 *
 * $Id: tftp_def.c,v 1.15 2004/02/13 03:16:09 jp Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "tftp_def.h"
#include "options.h"
#include "logger.h"

/*
 * This is the default option structure, that must be used
 * for initialisation.
 */

// FIXME: is there a way to use TIMEOUT and SEGSIZE here?
struct tftp_opt tftp_default_options[OPT_NUMBER] = {
     { "filename", "", 0, 1},   /* file to transfer */
     { "mode", "octet", 0, 1},  /* mode for transfer */
     { "tsize", "0", 0, 1 },    /* RFC1350 options. See RFC2347, */
     { "timeout", "5", 0, 1 },  /* 2348, 2349, 2090.  */
     { "blksize", "512", 0, 1 }, /* This is the default option */
     { "multicast", "", 0, 1 }, /* structure */
     { "", "", 0, 0}
};

/* Error message defined in RFC1350. */
char *tftp_errmsg[9] = {
     "Undefined error code",
     "File not found",
     "Access violation",
     "Disk full or allocation exceeded",
     "Illegal TFTP operation",
     "Unknown transfer ID",
     "File already exists",
     "No such user",
     "Failure to negotiate RFC1782 options",
};


/*
 * Compute the difference of two timeval structs handling wrap around.
 * The result is retruned in *res.
 * Return value are:
 *     1 if t1 > t0
 *     0 if t1 = t0
 *    -1 if t1 < t0
 */ 
int timeval_diff(struct timeval *res, struct timeval *t1, struct timeval *t0)
{
     res->tv_sec = t1->tv_sec - t0->tv_sec;
     res->tv_usec = t1->tv_usec - t0->tv_usec;
     
     if (res->tv_sec > 0)
     {
          if (res->tv_usec >= 0)
          {
               return 1;
          }
          else
          {
               res->tv_sec -= 1;
               res->tv_usec += 1000000;
               return 1;
          }
     }
     else if (res->tv_sec < 0)
     {
          if (res->tv_usec > 0)
          {
               res->tv_sec += 1;
               res->tv_usec -= 1000000;
               return -1;
          }
          else if (res->tv_usec <= 0);
          {
               return -1;
          }
     }
     else
     {
          if (res->tv_usec > 0)
               return 1;
          else if (res->tv_usec < 0)
               return -1;
          else
               return 0;
     }
}

/*
 * Print a string in engineering notation.
 *
 * IN:
 *  value: value to print
 *  string: if NULL, the function print to stdout, else if print
 *          to the string.
 *  format: format string for printf.
 */
int print_eng(double value, char *string, int size, char *format)
{
     char suffix[] = {'f', 'p', 'n', 'u', 'm', 0, 'k', 'M', 'G', 'T', 'P'};
     double tmp;
     double div = 1e-15;
     int i;


     for (i = 0; i < 11; i++)
     {
          tmp = value / div;
          if ((tmp > 1.0) && (tmp < 1000.0))
               break;
          div *= 1000.0;
     }
     if (string)
          snprintf(string, size, format, tmp, suffix[i]);
     else
          printf(format, tmp, suffix[i]);
     return OK;
}

/*
 * This is a strncpy function that take care of string NULL termination
 */
inline char *Strncpy(char *to, const char *from, size_t size)
{
     to[size-1] = '\000';
     return strncpy(to, from, size - 1);
}


/* 
 * gethostbyname replacement that is reentrant. This function is copyied
 * from the libc manual.
 */
int Gethostbyname(char *addr, struct hostent *host)
{
     struct hostent *hp;
     char *tmpbuf;
     size_t tmpbuflen;
     int herr;
     
     tmpbuflen = 1024;

     if ((tmpbuf = (char *)malloc(tmpbuflen)) == NULL)
          return ERR;

     hp = gethostbyname_r(addr, host, tmpbuf, tmpbuflen, &herr);

     free(tmpbuf);

     /*  Check for errors. */
     if (hp == NULL)
     {
          logger(LOG_ERR, "%s: %d: gethostbyname_r: %s",
                 __FILE__, __LINE__, strerror(herr));
          return ERR;
     }
     if (hp != host)
     {
          logger(LOG_ERR, "%s: %d: abnormal return value",
                 __FILE__, __LINE__);
          return ERR;
     }

     return OK;
}
