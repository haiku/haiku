/* Declarations for url.c.
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

#ifndef URL_H
#define URL_H

/* Default port definitions */
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_FTP_PORT 21
#define DEFAULT_HTTPS_PORT 443

/* Note: the ordering here is related to the order of elements in
   `supported_schemes' in url.c.  */

enum url_scheme {
  SCHEME_HTTP,
#ifdef HAVE_SSL
  SCHEME_HTTPS,
#endif
  SCHEME_FTP,
  SCHEME_INVALID
};

/* Structure containing info on a URL.  */
struct url
{
  char *url;			/* Original URL */
  enum url_scheme scheme;	/* URL scheme */

  char *host;			/* Extracted hostname */
  int port;			/* Port number */

  /* URL components (URL-quoted). */
  char *path;
  char *params;
  char *query;
  char *fragment;

  /* Extracted path info (unquoted). */
  char *dir;
  char *file;

  /* Username and password (unquoted). */
  char *user;
  char *passwd;
};

/* Function declarations */

char *url_escape PARAMS ((const char *));

struct url *url_parse PARAMS ((const char *, int *));
const char *url_error PARAMS ((int));
char *url_full_path PARAMS ((const struct url *));
void url_set_dir PARAMS ((struct url *, const char *));
void url_set_file PARAMS ((struct url *, const char *));
void url_free PARAMS ((struct url *));

enum url_scheme url_scheme PARAMS ((const char *));
int url_has_scheme PARAMS ((const char *));
int scheme_default_port PARAMS ((enum url_scheme));
void scheme_disable PARAMS ((enum url_scheme));

char *url_string PARAMS ((const struct url *, int));
char *url_file_name PARAMS ((const struct url *));

char *uri_merge PARAMS ((const char *, const char *));

int mkalldirs PARAMS ((const char *));

char *rewrite_shorthand_url PARAMS ((const char *));
int schemes_are_similar_p PARAMS ((enum url_scheme a, enum url_scheme b));

#endif /* URL_H */
