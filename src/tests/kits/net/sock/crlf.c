/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include	"sock.h"

/* Convert newline to return/newline. */

int
crlf_add(char *dst, int dstsize, const char *src, int lenin)
{
	int 	lenout;
	char	c;

	if ( (lenout = lenin) > dstsize)
			err_quit("crlf_add: destination not big enough");

	for ( ; lenin > 0; lenin--) {
		if ( (c = *src++) == '\n') {
			if (++lenout >= dstsize)
				err_quit("crlf_add: destination not big enough");
			*dst++ = '\r';
		}
		*dst++ = c;
	}

	return(lenout);
}

int
crlf_strip(char *dst, int dstsize, const char *src, int lenin)
{
	int		lenout;
	char	c;

	for (lenout = 0; lenin > 0; lenin--) { 
		if ( (c = *src++) != '\r') {
			if (++lenout >= dstsize)
				err_quit("crlf_strip: destination not big enough");
			*dst++ = c;
		}
	}

	return(lenout);
}
