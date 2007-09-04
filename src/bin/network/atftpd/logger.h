/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * logger.h
 *
 * $Id: logger.h,v 1.6 2000/12/27 00:57:16 remi Exp $
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

#ifndef logger_h
#define logger_h

#include <syslog.h>

void open_logger(char *ident, char *filename, int priority);
void logger(int severity, const char *fmt, ...);
void close_logger(void);

#endif
