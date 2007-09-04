/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * options.h
 *
 * $Id: options.h,v 1.7 2001/07/06 23:35:18 jp Exp $
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

#ifndef options_h
#define options_h

#include "tftp_def.h"

/* Structure definition for tftp options. */
struct tftp_opt {
     char option[OPT_SIZE];
     char value[VAL_SIZE];
     int specified;             /* specified by the client (for tftp server) */
     int enabled;               /* enabled for use by server or client */
};

extern struct tftp_opt tftp_default_options[OPT_NUMBER];

int opt_parse_request(char *data, int data_size, struct tftp_opt *options);
int opt_parse_options(char *data, int data_size, struct tftp_opt *options);
int opt_set_options(struct tftp_opt *options, char *name, char *value);
int opt_get_options(struct tftp_opt *options, char *name, char *value);
int opt_disable_options(struct tftp_opt *options, char *name);
int opt_support_options(struct tftp_opt *options);
int opt_get_tsize(struct tftp_opt *options);
int opt_get_timeout(struct tftp_opt *options);
int opt_get_blksize(struct tftp_opt *options);
int opt_get_multicast(struct tftp_opt *options, char *addr, int *port, int *mc);
void opt_set_tsize(int tsize, struct tftp_opt *options);
void opt_set_timeout(int timeout, struct tftp_opt *options);
void opt_set_blksize(int blksize, struct tftp_opt *options);
void opt_set_multicast(struct tftp_opt *options, char *addr, int port, int mc);
void opt_request_to_string(struct tftp_opt *options, char *string, int len);
void opt_options_to_string(struct tftp_opt *options, char *string, int len);

#endif

