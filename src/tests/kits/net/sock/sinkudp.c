/* -*- c-basic-offset: 8; -*-
 *
 * Copyright (c) 1993 W. Richard Stevens.  All rights reserved.
 * Permission to use or modify this software and its documentation only for
 * educational purposes and without fee is hereby granted, provided that
 * the above copyright notice appear in all copies.  The author makes no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 */

#include <stdio.h>
#include	"sock.h"

void
sink_udp(int sockfd)	/* TODO: use recvfrom ?? */
{
	int n, flags;

	if (pauseinit)
		sleep_us(pauseinit*1000);
	
	for ( ; ; ) {	/* read until peer closes connection; -n opt ignored */
			/* msgpeek = 0 or MSG_PEEK */
		flags = msgpeek;
	oncemore:
		if ( (n = recv(sockfd, rbuf, readlen, flags)) < 0) {
			err_sys("recv error");
			
		} else if (n == 0) {
			if (verbose)
				fprintf(stderr, "connection closed by peer\n");
			break;

#ifdef	notdef		/* following not possible with TCP */
		} else if (n != readlen)
			err_quit("read returned %d, expected %d", n, readlen);
#else
	}
#endif

	if (verbose) {
		fprintf(stderr, "received %d bytes%s\n", n,
			(flags == MSG_PEEK) ? " (MSG_PEEK)" : "");
		if (verbose > 1) {
			fprintf(stderr, "printing %d bytes\n", n);
			rbuf[n] = 0;	/* make certain it's null terminated */
			fprintf(stderr, "SDAP header: %lx\n", *((long *) rbuf));
			fprintf(stderr, "next long: %lx\n", *((long *) rbuf+4));
			fputs(&rbuf[8], stderr);
		}
	}

	if (pauserw)
		sleep_us(pauserw*1000);

	if (flags != 0) {
		flags = 0;		/* avoid infinite loop */
		goto oncemore;	/* read the message again */
	}
}

if (pauseclose) {
	if (verbose)
		fprintf(stderr, "pausing before close\n");
	sleep_us(pauseclose*1000);
 }

if (close(sockfd) < 0)
	err_sys("close error");
}
