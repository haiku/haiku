/*
 *  FILE: util.c
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: stubgen utility macros and funcs
 *
 *  DATE: Wed Sep 11 23:31:55 EDT 1996
 *   $Id: util.c 10 2002-07-09 12:24:59Z ejakowatz $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

static const char rcsid[] = "$Id: util.c 10 2002-07-09 12:24:59Z ejakowatz $";

#ifdef SGDEBUG
static FILE *logfile;

int log_open(const char *logfilename) 
{
  logfile = fopen(logfilename, "w");
  return (logfile != NULL);
}

void log_flush()
{
  fflush(logfile);
}

void log_close()
{
  fclose(logfile);
  logfile = NULL;
}

int log_printf(const char *format, ...)
{
  int retval;
  va_list ap;

  va_start(ap, format);
  retval = vfprintf(logfile, format, ap);
  va_end(ap);
 
  return retval;
}

int log_vprintf(const char *format, va_list ap)
{
  return vfprintf(logfile, format, ap);
}

#else
int log_printf(const char *format /*UNUSED*/, ...)
{
  return 0;
}
#endif /* SGDEBUG */


int inform_user(const char *format, ...)
{
  extern int opt_q;
  int retval = 0;
  va_list ap;

  if (!opt_q) {
    va_start(ap, format);
    retval = vfprintf(stderr, format, ap);
    va_end(ap);
  }
 
  return retval;
}

void fatal(int status, const char *format, ...)
{
  va_list ap;
 
  va_start(ap, format);
  log_vprintf(format, ap);
  log_flush();
  log_close();
  (void) vfprintf(stderr, format, ap);
  va_end(ap);
 
  fprintf(stderr, "\n");
  exit(status);
}

#if 0 /* removeDefaultValues() not needed */
/*
 * modifies s by putting a null char in place of the first trailing space
 */
static void removeTrailingSpaces(char *s)
{
    char *end = s + strlen(s) - 1;
    char *orig_end = end;
    
    while(*end == ' ') end--;
    if (end != orig_end) *++end = '\0';
}

/*
 * this both destructively modifies args and conses up a new string.
 * be sure to free it up.
 */
char * removeDefaultValues(char *args)
{
    char *token;
    char *new_arglist = (char *) malloc(strlen(args) + 1);
    int once = 0;
    
    strcpy(new_arglist, "");
    token = strtok(args, "=");
    
    while(token != NULL) {
	/* only append a comma if strtok got the comma off the arglist */
	if (once++) strcat(new_arglist, ",");
	removeTrailingSpaces(token);
	strcat(new_arglist, token);
	token = strtok(NULL, ",");   /* throw away the constant value */
	token = strtok(NULL, "=");   /* grab args up til the next = sign */
    }

    return new_arglist;
}
#endif /* removeDefaultValues() not needed */
