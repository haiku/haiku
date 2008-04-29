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
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "sock.h"
#include <fcntl.h>
#include <sys/ioctl.h>

#ifdef	FIOASYNC
static void sigio_func(int);

static void
sigio_func(int signo)
{
	fprintf(stderr, "SIGIO\n");
	/* shouldn't printf from a signal handler ... */
}
#endif

void
sockopts(int sockfd, int doall)
{
	int 		option;
	unsigned	optlen;
	struct linger	ling;
	struct timeval	timer;

	/* "doall" is 0 for a server's listening socket (i.e., before
	   accept() has returned.)  Some socket options such as SO_KEEPALIVE
	   don't make sense at this point, while others like SO_DEBUG do. */

	if (debug) {
		option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_DEBUG,
			       &option, sizeof(option)) < 0)
			err_sys("SO_DEBUG setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, SOL_SOCKET, SO_DEBUG,
			       &option, &optlen) < 0)
			err_sys("SO_DEBUG getsockopt error");
		if (option == 0)
			err_quit("SO_DEBUG not set (%d)", option);

		if (verbose)
			fprintf(stderr, "SO_DEBUG set\n");
	}

	if (dontroute) {
		option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE,
			       &option, sizeof(option)) < 0)
			err_sys("SO_DONTROUTE setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE,
			       &option, &optlen) < 0)
			err_sys("SO_DONTROUTE getsockopt error");
		if (option == 0)
			err_quit("SO_DONTROUTE not set (%d)", option);

		if (verbose)
			fprintf(stderr, "SO_DONTROUTE set\n");
	}

#ifdef	IP_TOS
	if (iptos != -1 && doall == 0) {
		if (setsockopt(sockfd, IPPROTO_IP, IP_TOS,
			       &iptos, sizeof(iptos)) < 0)
			err_sys("IP_TOS setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_IP, IP_TOS,
			       &option, &optlen) < 0)
			err_sys("IP_TOS getsockopt error");
		if (option != iptos)
			err_quit("IP_TOS not set (%d)", option);

		if (verbose)
			fprintf(stderr, "IP_TOS set to %d\n", iptos);
	}
#endif

#ifdef	IP_TTL
	if (ipttl != -1 && doall == 0) {
		if (setsockopt(sockfd, IPPROTO_IP, IP_TTL,
			       &ipttl, sizeof(ipttl)) < 0)
			err_sys("IP_TTL setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_IP, IP_TTL,
			       &option, &optlen) < 0)
			err_sys("IP_TTL getsockopt error");
		if (option != ipttl)
			err_quit("IP_TTL not set (%d)", option);

		if (verbose)
			fprintf(stderr, "IP_TTL set to %d\n", ipttl);
	}
#endif

	if (maxseg && udp == 0) {
		/* Need to set MSS for server before connection established */
		/* Beware: some kernels do not let the process set this socket
		   option; others only let it be decreased. */
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG,
			       &maxseg, sizeof(maxseg)) < 0)
			err_sys("TCP_MAXSEG setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG,
			       &option, &optlen) < 0)
		    err_sys("TCP_MAXSEG getsockopt error");

		if (verbose)
			fprintf(stderr, "TCP_MAXSEG = %d\n", option);
	}

	if (sroute_cnt > 0)
		sroute_set(sockfd);

	if (broadcast) {
		option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
			       &option, sizeof(option)) < 0)
			err_sys("SO_BROADCAST setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
			       &option, &optlen) < 0)
			err_sys("SO_BROADCAST getsockopt error");
		if (option == 0)
			err_quit("SO_BROADCAST not set (%d)", option);

		if (verbose)
			fprintf(stderr, "SO_BROADCAST set\n");

#ifdef	IP_ONESBCAST
		if (onesbcast) {
			option = 1;
			if (setsockopt(sockfd, IPPROTO_IP, IP_ONESBCAST,
				       &option, sizeof(option)) < 0)
				err_sys("IP_ONESBCAST setsockopt error");

			option = 0;
			optlen = sizeof(option);
			if (getsockopt(sockfd, IPPROTO_IP, IP_ONESBCAST,
				       &option, &optlen) < 0)
				err_sys("IP_ONESBCAST getsockopt error");
			if (option == 0)
				err_quit("IP_ONESBCAST not set (%d)", option);

			if (verbose)
				fprintf(stderr, "IP_ONESBCAST set\n");
		}
#endif
	}

#ifdef	IP_ADD_MEMBERSHIP
	if (joinip[0]) {
		struct ip_mreq	join;

		if (inet_aton(joinip, &join.imr_multiaddr) == 0)
			err_quit("invalid multicast address: %s", joinip);
		join.imr_interface.s_addr = htonl(INADDR_ANY);
		if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			       &join, sizeof(join)) < 0)
			err_sys("IP_ADD_MEMBERSHIP setsockopt error");

		if (verbose)
			fprintf(stderr, "IP_ADD_MEMBERSHIP set\n");
	}
#endif

#ifdef	IP_MULTICAST_TTL
	if (mcastttl) {
		u_char	ttl = mcastttl;

		if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
			       &ttl, sizeof(ttl)) < 0)
			err_sys("IP_MULTICAST_TTL setsockopt error");

		optlen = sizeof(ttl);
		if (getsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL,
			       &ttl, &optlen) < 0)
			err_sys("IP_MULTICAST_TTL getsockopt error");
		if (ttl != mcastttl)
			err_quit("IP_MULTICAST_TTL not set (%d)", ttl);

		if (verbose)
			fprintf(stderr, "IP_MULTICAST_TTL set to %d\n", ttl);
	}
#endif

	if (keepalive && doall && udp == 0) {
		option = 1;
		if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
			       &option, sizeof(option)) < 0)
			err_sys("SO_KEEPALIVE setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE,
			       &option, &optlen) < 0)
			err_sys("SO_KEEPALIVE getsockopt error");
		if (option == 0)
			err_quit("SO_KEEPALIVE not set (%d)", option);

		if (verbose)
			fprintf(stderr, "SO_KEEPALIVE set\n");
	}

	if (nodelay && doall && udp == 0) {
		option = 1;
		if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
			       &option, sizeof(option)) < 0)
			err_sys("TCP_NODELAY setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY,
			       &option, &optlen) < 0)
			err_sys("TCP_NODELAY getsockopt error");
		if (option == 0)
			err_quit("TCP_NODELAY not set (%d)", option);

		if (verbose)
			fprintf(stderr, "TCP_NODELAY set\n");
	}

	if (doall && verbose && udp == 0) {	/* just print MSS if verbose */
		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG,
			       &option, &optlen) < 0)
			err_sys("TCP_MAXSEG getsockopt error");

		fprintf(stderr, "TCP_MAXSEG = %d\n", option);
	}

	if (linger >= 0 && doall && udp == 0) {
		ling.l_onoff = 1;
		ling.l_linger = linger;		/* 0 for abortive disconnect */
		if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER,
			       &ling, sizeof(ling)) < 0)
			err_sys("SO_LINGER setsockopt error");

		ling.l_onoff = 0;
		ling.l_linger = -1;
		optlen = sizeof(struct linger);
		if (getsockopt(sockfd, SOL_SOCKET, SO_LINGER,
			       &ling, &optlen) < 0)
			err_sys("SO_LINGER getsockopt error");
		if (ling.l_onoff == 0 || ling.l_linger != linger)
			err_quit("SO_LINGER not set (%d, %d)", ling.l_onoff, ling.l_linger);

		if (verbose)
			fprintf(stderr, "linger %s, time = %d\n",
				ling.l_onoff ? "on" : "off", ling.l_linger);
	}

	if (doall && rcvtimeo) {
#ifdef	SO_RCVTIMEO
	    /* User specifies millisec, must convert to sec/usec */
		timer.tv_sec  =  rcvtimeo / 1000;
		timer.tv_usec = (rcvtimeo % 1000) * 1000;
		if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			       &timer, sizeof(timer)) < 0)
			err_sys("SO_RCVTIMEO setsockopt error");

		timer.tv_sec = timer.tv_usec = 0;
		optlen = sizeof(timer);
		if (getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,
			       &timer, &optlen) < 0)
			err_sys("SO_RCVTIMEO getsockopt error");

		if (verbose)
			fprintf(stderr, "SO_RCVTIMEO: %ld.%06ld\n",
				timer.tv_sec, timer.tv_usec);
#else
		fprintf(stderr, "warning: SO_RCVTIMEO not supported by host\n");
#endif
	}

	if (doall && sndtimeo) {
#ifdef	SO_SNDTIMEO
		/* User specifies millisec, must convert to sec/usec */
		timer.tv_sec  =  sndtimeo / 1000;
		timer.tv_usec = (sndtimeo % 1000) * 1000;
		if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
			       &timer, sizeof(timer)) < 0)
			err_sys("SO_SNDTIMEO setsockopt error");

		timer.tv_sec = timer.tv_usec = 0;
		optlen = sizeof(timer);
		if (getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO,
			       &timer, &optlen) < 0)
			err_sys("SO_SNDTIMEO getsockopt error");

		if (verbose)
			fprintf(stderr, "SO_SNDTIMEO: %ld.%06ld\n",
				timer.tv_sec, timer.tv_usec);
#else
		fprintf(stderr, "warning: SO_SNDTIMEO not supported by host\n");
#endif
	}

	if (recvdstaddr && udp) {
#ifdef	IP_RECVDSTADDR
		option = 1;
		if (setsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR,
			       &option, sizeof(option)) < 0)
			err_sys("IP_RECVDSTADDR setsockopt error");

		option = 0;
		optlen = sizeof(option);
		if (getsockopt(sockfd, IPPROTO_IP, IP_RECVDSTADDR,
			       &option, &optlen) < 0)
			err_sys("IP_RECVDSTADDR getsockopt error");
		if (option == 0)
			err_quit("IP_RECVDSTADDR not set (%d)", option);

		if (verbose)
			fprintf(stderr, "IP_RECVDSTADDR set\n");
#else
		fprintf(stderr, "warning: IP_RECVDSTADDR not supported by host\n");
#endif
	}

	if (sigio) {
#ifdef	FIOASYNC
		/*
		 * Should be able to set this with fcntl(O_ASYNC) or fcntl(FASYNC),
		 * but some systems (AIX?) only do it with ioctl().
		 *
		 * Need to set this for listening socket and for connected socket.
		 */
		signal(SIGIO, sigio_func);

		if (fcntl(sockfd, F_SETOWN, getpid()) < 0)
			err_sys("fcntl F_SETOWN error");

		option = 1;
		if (ioctl(sockfd, FIOASYNC, (char *) &option) < 0)
			err_sys("ioctl FIOASYNC error");

		if (verbose)
			fprintf(stderr, "FIOASYNC set\n");
#else
		fprintf(stderr, "warning: FIOASYNC not supported by host\n");
#endif
	}
}
