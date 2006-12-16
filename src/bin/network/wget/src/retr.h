/* Declarations for retr.c.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

#ifndef RETR_H
#define RETR_H

/* Flags for fd_read_body. */
enum {
  rb_read_exactly  = 1,
  rb_skip_startpos = 2
};

int fd_read_body PARAMS ((int, FILE *, wgint, wgint, wgint *, wgint *, double *,
                          int));

typedef const char *(*hunk_terminator_t) PARAMS ((const char *, int, int));

char *fd_read_hunk PARAMS ((int, hunk_terminator_t, long, long));
char *fd_read_line PARAMS ((int));

uerr_t retrieve_url PARAMS ((const char *, char **, char **,
			     const char *, int *));
uerr_t retrieve_from_file PARAMS ((const char *, int, int *));

char *retr_rate PARAMS ((wgint, double, int));
double calc_rate PARAMS ((wgint, double, int *));
void printwhat PARAMS ((int, int));

void sleep_between_retrievals PARAMS ((int));

void rotate_backups PARAMS ((const char *));

/* Because there's no http.h. */

struct url;

uerr_t http_loop PARAMS ((struct url *, char **, char **, const char *,
			  int *, struct url *));
void save_cookies PARAMS ((void));


#endif /* RETR_H */
