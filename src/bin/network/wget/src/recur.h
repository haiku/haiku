/* Declarations for recur.c.
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

#ifndef RECUR_H
#define RECUR_H

/* For most options, 0 means no limits, but with -p in the picture,
   that causes a problem on the maximum recursion depth variable.  To
   retain backwards compatibility we allow users to consider "0" to be
   synonymous with "inf" for -l, but internally infinite recursion is
   specified by -1 and 0 means to only retrieve the requisites of a
   single document. */
#define INFINITE_RECURSION -1

struct urlpos;

void recursive_cleanup PARAMS ((void));
uerr_t retrieve_tree PARAMS ((const char *));

/* These are really in html-url.c. */
struct urlpos *get_urls_file PARAMS ((const char *));
struct urlpos *get_urls_html PARAMS ((const char *, const char *, int *));
void free_urlpos PARAMS ((struct urlpos *));

#endif /* RECUR_H */
