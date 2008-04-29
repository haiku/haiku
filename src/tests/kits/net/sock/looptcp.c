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

/* Copy everything from stdin to "sockfd",
 * and everything from "sockfd" to stdout. */

void loop_tcp(int sockfd)
{
	int		maxfdp1, nread, ntowrite, stdineof, flags;
	fd_set	rset;
  
	if (pauseinit)
		sleep_us(pauseinit*1000);	/* intended for server */
  
	flags = 0;
	stdineof = 0;
	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;	/* check descriptors [0..sockfd] */
  
	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(STDIN_FILENO, &rset);
		FD_SET(sockfd, &rset);
      
		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
			err_sys("select error");
      
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			/* data to read on stdin */
			if ( (nread = read(STDIN_FILENO, rbuf, readlen)) < 0)
				err_sys("read error from stdin");
			else if (nread == 0) {
				/* EOF on stdin */
				if (halfclose) {
					if (shutdown(sockfd, SHUT_WR) < 0)
						err_sys("shutdown() error");
		
					FD_CLR(STDIN_FILENO, &rset);
					stdineof = 1;	/* don't read stdin anymore */
					continue;		/* back to select() */
				}
				break;		/* default: stdin EOF -> done */
			}
	  
			if (crlf) {
				ntowrite = crlf_add(wbuf, writelen, rbuf, nread);
				if (dowrite(sockfd, wbuf, ntowrite) != ntowrite)
					err_sys("write error");
			} else {
				if (dowrite(sockfd, rbuf, nread) != nread)
					err_sys("write error");
			}
		}
      
		if (FD_ISSET(sockfd, &rset)) {
			/* data to read from socket */
			/* msgpeek = 0 or MSG_PEEK */
			flags = msgpeek;
		oncemore:
			if ( (nread = recv(sockfd, rbuf, readlen, flags)) < 0)
				err_sys("recv error");
			else if (nread == 0) {
				if (verbose)
					fprintf(stderr, "connection closed by peer\n");
				break;		/* EOF, terminate */
			}

			if (crlf) {
				ntowrite = crlf_strip(wbuf, writelen, rbuf, nread);
				if (writen(STDOUT_FILENO, wbuf, ntowrite) != ntowrite)
					err_sys("writen error to stdout");
			} else {
				if (writen(STDOUT_FILENO, rbuf, nread) != nread)
					err_sys("writen error to stdout");
			}
	  
			if (flags != 0) {
				flags = 0;		/* no infinite loop */
				goto oncemore;	/* read the message again */
			}
		}
	}
  
	if (pauseclose) {
		if (verbose)
			fprintf(stderr, "pausing before close\n");
		sleep_us(pauseclose*1000);
	}
  
	if (close(sockfd) < 0)
		err_sys("close error");		/* since SO_LINGER may be set */
}
