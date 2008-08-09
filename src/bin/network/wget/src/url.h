/* Declarations for url.c.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

#ifndef URL_H
#define URL_H

/* Default port definitions */
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_FTP_PORT 21
#define DEFAULT_HTTPS_PORT 443

/* Specifies how, or whether, user auth information should be included
 * in URLs regenerated from URL parse structures. */
enum url_auth_mode {
  URL_AUTH_SHOW,
  URL_AUTH_HIDE_PASSWD,
  URL_AUTH_HIDE
};

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

char *url_escape (const char *);

struct url *url_parse (const char *, int *);
const char *url_error (int);
char *url_full_path (const struct url *);
void url_set_dir (struct url *, const char *);
void url_set_file (struct url *, const char *);
void url_free (struct url *);

enum url_scheme url_scheme (const char *);
bool url_has_scheme (const char *);
int scheme_default_port (enum url_scheme);
void scheme_disable (enum url_scheme);

char *url_string (const struct url *, enum url_auth_mode);
char *url_file_name (const struct url *);

char *uri_merge (const char *, const char *);

int mkalldirs (const char *);

char *rewrite_shorthand_url (const char *);
bool schemes_are_similar_p (enum url_scheme a, enum url_scheme b);

bool are_urls_equal (const char *u1, const char *u2);

#endif /* URL_H */
