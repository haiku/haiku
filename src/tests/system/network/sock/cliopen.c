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

int cliopen(char *host, char *port)
{
	int			fd, i, on;
	char			*protocol;
	struct in_addr		inaddr;
	struct servent		*sp;
	struct hostent		*hp;

	protocol = udp ? "udp" : "tcp";
  
	/* initialize socket address structure */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
  
	/* see if "port" is a service name or number */
	if ( (i = atoi(port)) == 0) {
		if ( (sp = getservbyname(port, protocol)) == NULL)
			err_quit("getservbyname() error for: %s/%s", port, protocol);
      
		servaddr.sin_port = sp->s_port;
	} else
		servaddr.sin_port = htons(i);
  
	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */
  
	if (inet_aton(host, &inaddr) == 1)
		servaddr.sin_addr = inaddr;	/* it's dotted-decimal */
	else if ( (hp = gethostbyname(host)) != NULL)
		bcopy(hp->h_addr, &servaddr.sin_addr, hp->h_length);
	else
		err_quit("invalid hostname: %s", host);

	if ( (fd = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0)) < 0)
		err_sys("socket() error");

	if (reuseaddr) {
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0)
			err_sys("setsockopt of SO_REUSEADDR error");
	}

#ifdef	SO_REUSEPORT
	if (reuseport) {
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof (on)) < 0)
			err_sys("setsockopt of SO_REUSEPORT error");
	}
#endif

	/*
	 * User can specify port number for client to bind.  Only real use
	 * is to see a TCP connection initiated by both ends at the same time.
	 * Also, if UDP is being used, we specifically call bind() to assign
	 * an ephemeral port to the socket.
	 * Also, for experimentation, client can also set local IP address
	 * (and port) using -l option.  Allow localip[] to be set but bindport
	 * to be 0.
	 */
	
	if (bindport != 0 || localip[0] != 0 || udp) {
		bzero(&cliaddr, sizeof(cliaddr));
		cliaddr.sin_family      = AF_INET;
		cliaddr.sin_port        = htons(bindport);   /* can be 0 */
		if (localip[0] != 0) {
			if (inet_aton(localip, &cliaddr.sin_addr) == 0)
				err_quit("invalid IP address: %s", localip);
		} else
			cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);	/* wildcard */
		
		if (bind(fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0)
			err_sys("bind() error");
	}
	
	/* Need to allocate buffers before connect(), since they can affect
	 * TCP options (window scale, etc.).
	 */
	
	buffers(fd);
	sockopts(fd, 0);	/* may also want to set SO_DEBUG */
	
	/*
	 * Connect to the server.  Required for TCP, optional for UDP.
	 */
	
	if (udp == 0 || connectudp) {
		for ( ; ; ) {
			if (connect(fd, (struct sockaddr *) &servaddr, sizeof(servaddr))
			    == 0)
				break;		/* all OK */
			if (errno == EINTR)		/* can happen with SIGIO */
				continue;
			if (errno == EISCONN)	/* can happen with SIGIO */
				break;
			err_sys("connect() error");
		}
	}
  
	if (verbose) {
		/* Call getsockname() to find local address bound to socket:
		   TCP ephemeral port was assigned by connect() or bind();
		   UDP ephemeral port was assigned by bind(). */
		i = sizeof(cliaddr);
		if (getsockname(fd, (struct sockaddr *) &cliaddr, &i) < 0)
			err_sys("getsockname() error");
		
		/* Can't do one fprintf() since inet_ntoa() stores
		   the result in a static location. */
		fprintf(stderr, "connected on %s.%d ",
			INET_NTOA(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
		fprintf(stderr, "to %s.%d\n",
			INET_NTOA(servaddr.sin_addr), ntohs(servaddr.sin_port));
	}
	
	sockopts(fd, 1);	/* some options get set after connect() */
	
	return(fd);
}
