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
#include "sock.h"
#include <netinet/ip.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * There is a fundamental limit of 9 IP addresses in a source route.
 * But we allocate sroute_opt[44] with room for 10 IP addresses (and
 * the 3-byte source route type/len/offset) because with the BSD
 * IP_OPTIONS socket option we can specify up to 9 addresses, followed
 * by the destination address.  The in_pcbopts() function in the kernel
 * then takes the final address in the list (the destination) and moves
 * it to the front, as shown in Figure 9.33 of "TCP/IP Illustrated,
 * Volume 2".  Also note that this destination address that we pass as
 * the final IP address in our array overrides the destination address
 * of the sendto() (Figure 9.28 of Volume 2).
 */

u_char	sroute_opt[44];		/* some implementations require this to be
							   on a 4-byte boundary */
u_char	*optr;				/* pointer into options being formed */

/*
 * Process either the -g (loose) or -G (strict) command-line option,
 * specifying one hop in a source route.
 * Either option can be specified up to 9 times.
 * With IPv4 the entire source route is either loose or strict, but we
 * set the source route type field based on the first option encountered,
 * either -g or -G.
 */

void
sroute_doopt(int strict, char *argptr)
{
	struct in_addr	inaddr;
	struct hostent	*hp;

	if (sroute_cnt >= 9)
		err_quit("too many source routes with: %s", argptr);

	if (sroute_cnt == 0) {	/* first one */
		bzero(sroute_opt, sizeof(sroute_opt));
		optr = sroute_opt;
		*optr++ = strict ? IPOPT_SSRR : IPOPT_LSRR;
		optr++;			/* we fill in the total length later */
		*optr++ = 4;	/* ptr to first source-route address */
	}

	if (inet_aton(argptr, &inaddr) == 1) {
		bcopy(&inaddr, optr, sizeof(u_long));	/* dotted decimal */
		if (verbose)
			fprintf(stderr, "source route to %s\n", inet_ntoa(inaddr));
	} else if ( (hp = gethostbyname(argptr)) != NULL) {
		bcopy(hp->h_addr, optr, sizeof(u_long));/* hostname */
		if (verbose)
			fprintf(stderr, "source route to %s\n",
							inet_ntoa(*((struct in_addr *) hp->h_addr)));
	} else
		err_quit("unknown host: %s\n", argptr);

	optr += sizeof(u_long);		/* for next IP addr in list */
	sroute_cnt++;
}

/*
 * Set the actual source route with the IP_OPTIONS socket option.
 * This function is called if srouce_cnt is nonzero.
 * The final destination goes at the end of the list of IP addresses.
 */

void
sroute_set(int sockfd)
{
	sroute_cnt++;			      /* account for destination */
	sroute_opt[1] = 3 + (sroute_cnt * 4); /* total length, incl. destination */

	/* destination must be stored as final entry */
	bcopy(&servaddr.sin_addr, optr, sizeof(u_long));
	optr += sizeof(u_long);
	if (verbose) {
		fprintf(stderr, "source route to %s\n", inet_ntoa(servaddr.sin_addr));
		fprintf(stderr, "source route size %d bytes\n", sroute_opt[1]);
	}

	/*
	 * The number of bytes that we pass to setsockopt() must be a multiple
	 * of 4.  Since the buffer was initialized to 0, this leaves an EOL
	 * following the final IP address.
	 * For optimization we could put a NOP before the 3-byte type/len/offset
	 * field, which would then align all the IP addresses on 4-byte boundaries,
	 * but the source routing code is not exactly in the fast path of most
	 * routers.
	 */
	while ((optr - sroute_opt) & 3)
		optr++;

	if (setsockopt(sockfd, IPPROTO_IP, IP_OPTIONS,
		       sroute_opt, optr - sroute_opt) < 0)
		err_sys("setsockopt error for IP_OPTIONS");

	sroute_cnt = 0;		/* don't call this function again */
}
