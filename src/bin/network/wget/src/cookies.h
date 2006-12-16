/* Support for cookies.
   Copyright (C) 2001, 2002 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

GNU Wget is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

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

#ifndef COOKIES_H
#define COOKIES_H

struct cookie_jar;

struct cookie_jar *cookie_jar_new PARAMS ((void));
void cookie_jar_delete PARAMS ((struct cookie_jar *));

void cookie_handle_set_cookie PARAMS ((struct cookie_jar *, const char *,
				       int, const char *, const char *));
char *cookie_header PARAMS ((struct cookie_jar *,
			     const char *, int, const char *, int));

void cookie_jar_load PARAMS ((struct cookie_jar *, const char *));
void cookie_jar_save PARAMS ((struct cookie_jar *, const char *));

#endif /* COOKIES_H */
