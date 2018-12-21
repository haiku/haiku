/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */
#include <sys/types.h>
#include <sys/socket.h>


#include <stdio.h>
#include "sock.h"

void
source_udp(int sockfd)	/* TODO: use sendto ?? */
{
	int		i, n, option;
	socklen_t	optlen;

	pattern(wbuf, writelen);	/* fill send buffer with a pattern */

	if (pauseinit)
		sleep_us(pauseinit*1000);

	for (i = 1; i <= nbuf; i++) {
		if (connectudp) {
			if ( (n = write(sockfd, wbuf, writelen)) != writelen) {
				if (ignorewerr) {
					err_ret("write returned %d, expected %d", n, writelen);
						/* also call getsockopt() to clear so_error */
					optlen = sizeof(option);
       				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
												&option, &optlen) < 0)
           				err_sys("SO_ERROR getsockopt error");
				} else
					err_sys("write returned %d, expected %d", n, writelen);
			}
		} else {
		  if ( (n = sendto(sockfd, wbuf, writelen, 0,
				   (struct sockaddr *) &servaddr,
				   sizeof(struct sockaddr))) != writelen) {
		    if (ignorewerr) {
					err_ret("sendto returned %d, expected %d", n, writelen);
						/* also call getsockopt() to clear so_error */
					optlen = sizeof(option);
       				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
												&option, &optlen) < 0)
           				err_sys("SO_ERROR getsockopt error");
				} else
					err_sys("sendto returned %d, expected %d", n, writelen);
			}
		}

		if (verbose)
			fprintf(stderr, "wrote %d bytes\n", n);

		if (pauserw)
			sleep_us(pauserw*1000);
	}

	if (pauseclose) {
		if (verbose)
			fprintf(stderr, "pausing before close\n");
		sleep_us(pauseclose*1000);
	}

	if (close(sockfd) < 0)
		err_sys("close error");		/* since SO_LINGER may be set */
}
