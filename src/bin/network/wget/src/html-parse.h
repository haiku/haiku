/* Declarations for html-parse.c.
   Copyright (C) 1998 Free Software Foundation, Inc.

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

#ifndef HTML_PARSE_H
#define HTML_PARSE_H

struct attr_pair {
  char *name;			/* attribute name */
  char *value;			/* attribute value */

  /* Needed for URL conversion; the places where the value begins and
     ends, including the quotes and everything. */
  const char *value_raw_beginning;
  int value_raw_size;

  /* Used internally by map_html_tags. */
  int name_pool_index, value_pool_index;
};

struct taginfo {
  char *name;			/* tag name */
  int end_tag_p;		/* whether this is an end-tag */
  int nattrs;			/* number of attributes */
  struct attr_pair *attrs;	/* attributes */

  const char *start_position;	/* start position of tag */
  const char *end_position;	/* end position of tag */
};

struct hash_table;		/* forward declaration */

/* Flags for map_html_tags: */
#define MHT_STRICT_COMMENTS  1  /* use strict comment interpretation */
#define MHT_TRIM_VALUES      2  /* trim attribute values, e.g. interpret
                                   <a href=" foo "> as "foo" */

void map_html_tags PARAMS ((const char *, int,
			    void (*) (struct taginfo *, void *), void *, int,
			    const struct hash_table *,
			    const struct hash_table *));

#endif /* HTML_PARSE_H */
