/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include "sock.h"

#ifndef	UIO_MAXIOV
#define	UIO_MAXIOV	16	/* we assume this; may not be true? */
#endif

ssize_t
dowrite(int fd, const void *vptr, size_t nbytes)
{
	struct iovec	iov[UIO_MAXIOV];
	const char     *ptr;
	int		chunksize, i, n, nleft, nwritten, ntotal;

	if (chunkwrite == 0 && usewritev == 0)
		return(write(fd, vptr, nbytes));		/* common case */

	/*
	 * Figure out what sized chunks to write.
	 * Try to use UIO_MAXIOV chunks.
	 */

	chunksize = nbytes / UIO_MAXIOV;
	if (chunksize <= 0)
		chunksize = 1;
	else if ((nbytes % UIO_MAXIOV) != 0)
		chunksize++;

	ptr = vptr;
	nleft = nbytes;
	for (i = 0; i < UIO_MAXIOV; i++)
	  {
	    iov[i].iov_base = (void *) ptr;
	    n = (nleft >= chunksize) ? chunksize : nleft;
	    iov[i].iov_len = n;
	    if (verbose)
	      fprintf(stderr, "iov[%2d].iov_base = %x, iov[%2d].iov_len = %d\n",
		      i, (u_int32_t) iov[i].iov_base, i, (int) iov[i].iov_len);
		ptr += n;
		if ((nleft -= n) == 0)
			break;
	}
	if (i == UIO_MAXIOV)
		err_quit("i == UIO_MAXIOV");

	if (usewritev)
		return(writev(fd, iov, i+1));
	else {
		ntotal = 0;
		for (n = 0; n <= i; n++) {
			nwritten = write(fd, iov[n].iov_base, iov[n].iov_len);
			if (nwritten != (int) iov[n].iov_len)
				return(-1);
			ntotal += nwritten;
		}
		return(ntotal);
	}
}
