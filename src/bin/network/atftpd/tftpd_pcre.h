/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_pcre.h
 *
 * $Id: tftpd_pcre.h,v 1.1 2003/02/21 05:06:06 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * The PCRE code is provided by Jeff Miller <jeff.miller@transact.com.au>
 *
 * Copyright (c) 2003 Jeff Miller <jeff.miller@transact.com.au>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */
#ifndef TFTPD_PCRE_H
#define TFTPD_PCRE_H
#include <pthread.h>
#include <pcre.h>

#include "tftpd.h"

/* Carry out filename substitution
 * example, pattern
 * (name)    sur($1)
 *
 * for the requested file "filename" would give "surname"
 *
 */

/*
 * for when we read files, what is the format of a line
 * pattern [whitespace] replacement_string
 */

#define TFTPD_PCRE_FILE_PATTERN "^(\\S+)\\s+(\\S+)$"

/*
 * Definition of struct to hold patterns
 */

struct tftpd_pcre_pattern
{
     unsigned int linenum;
     char *pattern;
     pcre *left_re;
     pcre_extra *left_pe;
     char *right_str;
     struct tftpd_pcre_pattern *next;
};

typedef struct tftpd_pcre_pattern tftpd_pcre_pattern_t;

struct tftpd_pcre_self
{
     pthread_mutex_t lock;
     char filename[MAXLEN];
     struct tftpd_pcre_pattern *list;
};

typedef struct tftpd_pcre_self tftpd_pcre_self_t;

/* function prototypes */
tftpd_pcre_self_t *tftpd_pcre_open(char *filename);
char *tftpd_pcre_getfilename(tftpd_pcre_self_t *self);
int tftpd_pcre_sub(tftpd_pcre_self_t *self, char *outstr, int outlen, char *str);
void tftpd_pcre_close(tftpd_pcre_self_t *self);

#endif
