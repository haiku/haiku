/* Copyright (C) 1991-1993,1997,1998,2000-2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.
*/


#include <errno.h>
#include <stdio.h>
#include <string.h>


/**	Print a line on stderr consisting of the text in S, a colon,
 *	a space, a message describing the meaning of the contents of
 *	`errno' and a newline.
 *	If S is NULL or "", the colon and space are omitted.
 */

void
perror(const char *s)
{
	const char *colon;
	char buffer[1024];

	if (s == NULL || *s == '\0')
		s = colon = "";
	else
		colon = ": ";

	fprintf(stderr, "%s%s%s\n", s, colon, strerror_r(errno, buffer, sizeof(buffer)));
}

