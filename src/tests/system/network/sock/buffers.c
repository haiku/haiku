/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include <sock.h>

void buffers(int sockfd)
{
	int		n;
	socklen_t	optlen;
  
	/* Allocate the read and write buffers. */
  
	if (rbuf == NULL) {
		if ( (rbuf = malloc(readlen)) == NULL)
			err_sys("malloc error for read buffer");
	}
  
	if (wbuf == NULL) {
		if ( (wbuf = malloc(writelen)) == NULL)
			err_sys("malloc error for write buffer");
	}
  
	/* Set the socket send and receive buffer sizes (if specified).
	   The receive buffer size is tied to TCP's advertised window. */
  
	if (rcvbuflen) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcvbuflen,
			       sizeof(rcvbuflen)) < 0)
			err_sys("SO_RCVBUF setsockopt error");
      
		optlen = sizeof(n);
		if (getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, &optlen) < 0)
			err_sys("SO_RCVBUF getsockopt error");
		if (n != rcvbuflen)
			err_quit("error: requested rcvbuflen = %d, resulting SO_RCVBUF = %d", rcvbuflen, n);
		if (verbose)
			fprintf(stderr, "SO_RCVBUF = %d\n", n);
	}

	if (sndbuflen) {
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuflen,
			       sizeof(sndbuflen)) < 0)
			err_sys("SO_SNDBUF setsockopt error");
      
		optlen = sizeof(n);
		if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &n, &optlen) < 0)
			err_sys("SO_SNDBUF getsockopt error");
		if (n != sndbuflen)
			err_quit("error: requested sndbuflen = %d, resulting SO_SNDBUF = %d", sndbuflen, n);
		if (verbose)
			fprintf(stderr, "SO_SNDBUF = %d\n", n);
	}
}
