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

void
join_mcast(int fd, struct sockaddr_in *sin)
{
#ifdef	IP_ADD_MEMBERSHIP	/* only include if host supports mcasting */
	u_long			inaddr;
	struct ip_mreq	mreq;

	inaddr = sin->sin_addr.s_addr;
	if (IN_MULTICAST(inaddr) == 0)
		return;			/* not a multicast address */

	mreq.imr_multiaddr.s_addr = inaddr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);	/* need way to change */
	if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
												sizeof(mreq)) == -1 )
		err_sys("IP_ADD_MEMBERSHIP error");

	if (verbose)
			fprintf(stderr, "multicast group joined\n");
#endif	/* IP_ADD_MEMBERSHIP */
}
