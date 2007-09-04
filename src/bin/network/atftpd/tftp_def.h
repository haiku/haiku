/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftp_def.h
 *
 * $Id: tftp_def.h,v 1.17 2004/02/13 03:16:09 jp Exp $
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

#ifndef tftp_def_h
#define tftp_def_h

#include <sys/time.h>
#include <sys/times.h>
#include <netdb.h>

/* standard return value */
#define OK            0
#define ERR          -1
#define ABORT        -2
#define QUIT         -10

#define MAXLEN      256         /* For file names and such */
#define IPADDRLEN    24         /* For IPv4 and IPv6 address string */
#define TIMEOUT       5         /* Client timeout */
#define S_TIMEOUT     5         /* Server timout. */
#define NB_OF_RETRY   5

/* definition to use tftp_options structure */
#define OPT_FILENAME  0
#define OPT_MODE      1
#define OPT_TSIZE     2
#define OPT_TIMEOUT   3
#define OPT_BLKSIZE   4
#define OPT_MULTICAST 5
#define OPT_NUMBER    7

#define OPT_SIZE     12
#define VAL_SIZE     MAXLEN

extern char *tftp_errmsg[9];

int timeval_diff(struct timeval *res, struct timeval *t1, struct timeval *t0);
int print_eng(double value, char *string, int size, char *format);
inline char *Strncpy(char *to, const char *from, size_t size);
int Gethostbyname(char *addr, struct hostent *host);

#endif
