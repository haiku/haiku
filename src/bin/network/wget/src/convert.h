/* Declarations for convert.c
   Copyright (C) 2003 Free Software Foundation, Inc.

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

#ifndef CONVERT_H
#define CONVERT_H

struct hash_table;		/* forward decl */
extern struct hash_table *downloaded_html_set;

enum convert_options {
  CO_NOCONVERT = 0,		/* don't convert this URL */
  CO_CONVERT_TO_RELATIVE,	/* convert to relative, e.g. to
                                   "../../otherdir/foo.gif" */
  CO_CONVERT_TO_COMPLETE,	/* convert to absolute, e.g. to
				   "http://orighost/somedir/bar.jpg". */
  CO_NULLIFY_BASE		/* change to empty string. */
};

struct url;

/* A structure that defines the whereabouts of a URL, i.e. its
   position in an HTML document, etc.  */

struct urlpos {
  struct url *url;		/* the URL of the link, after it has
				   been merged with the base */
  char *local_name;		/* local file to which it was saved
				   (used by convert_links) */

  /* reserved for special links such as <base href="..."> which are
     used when converting links, but ignored when downloading.  */
  unsigned int ignore_when_downloading	:1;

  /* Information about the original link: */

  unsigned int link_relative_p	:1; /* the link was relative */
  unsigned int link_complete_p	:1; /* the link was complete (had host name) */
  unsigned int link_base_p	:1; /* the url came from <base href=...> */
  unsigned int link_inline_p	:1; /* needed to render the page */
  unsigned int link_expect_html	:1; /* expected to contain HTML */

  unsigned int link_refresh_p	:1; /* link was received from
				       <meta http-equiv=refresh content=...> */
  int refresh_timeout;		/* for reconstructing the refresh. */

  /* Conversion requirements: */
  enum convert_options convert;	/* is conversion required? */

  /* URL's position in the buffer. */
  int pos, size;

  struct urlpos *next;		/* next list element */
};

/* downloaded_file() takes a parameter of this type and returns this type. */
typedef enum
{
  /* Return enumerators: */
  FILE_NOT_ALREADY_DOWNLOADED = 0,

  /* Return / parameter enumerators: */
  FILE_DOWNLOADED_NORMALLY,
  FILE_DOWNLOADED_AND_HTML_EXTENSION_ADDED,

  /* Parameter enumerators: */
  CHECK_FOR_FILE
} downloaded_file_t;

downloaded_file_t downloaded_file PARAMS ((downloaded_file_t, const char *));

void register_download PARAMS ((const char *, const char *));
void register_redirection PARAMS ((const char *, const char *));
void register_html PARAMS ((const char *, const char *));
void register_delete_file PARAMS ((const char *));
void convert_all_links PARAMS ((void));
void convert_cleanup PARAMS ((void));

char *html_quote_string PARAMS ((const char *));

#endif /* CONVERT_H */
