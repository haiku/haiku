/*
 *  FILE: util.h
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: stubgen utility macros and funcs
 *
 *  DATE: Wed Sep 11 23:31:55 EDT 1996
 *   $Id: util.h 10 2002-07-09 12:24:59Z ejakowatz $
 *
 *  Copyright (c) 1996-1998  Michael John Radwin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UTIL_HEADER
#define UTIL_HEADER

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdarg.h>

extern int yylex();

/* prints a message to the logfile if debugging is enabled */
int log_printf(const char *format, ...);

#ifdef SGDEBUG
int log_vprintf(const char *format, va_list ap);
int log_open(const char *filename); 
void log_flush();
void log_close();
#else
#define log_vprintf(a,b)
#define log_open(arg)
#define log_flush()
#define log_close()
#endif /* SGDEBUG */

/* prints a message to stderr if the -q option is not set */
int inform_user(const char *format, ...);

/* prints a message to stderr and exits with return value 1 */
void fatal(int status, const char *format, ...);

#ifdef	__cplusplus
}
#endif

#endif /* UTIL_HEADER */
