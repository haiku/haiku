/*
 * Copyright 2002-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_SOCKIO_H
#define _SYS_SOCKIO_H


enum {
	SIOCADDRT = 8900,	/* add route */
	SIOCDELRT,			/* delete route */
	SIOCSIFADDR,		/* set interface address */
	SIOCGIFADDR,		/* get interface address */
	SIOCSIFDSTADDR,		/* set point-to-point address */
	SIOCGIFDSTADDR,		/* get point-to-point address */
	SIOCSIFFLAGS,		/* set interface flags */
	SIOCGIFFLAGS,		/* get interface flags */
	SIOCGIFBRDADDR,		/* get broadcast address */
	SIOCSIFBRDADDR,		/* set broadcast address */
	SIOCGIFCOUNT,		/* count interfaces */
	SIOCGIFCONF,		/* get interface list */
	SIOCGIFINDEX,		/* interface name -> index */
	SIOCGIFNAME,		/* interface index -> name */
	SIOCGIFNETMASK,		/* get net address mask */
	SIOCSIFNETMASK,		/* set net address mask */
	SIOCGIFMETRIC,		/* get interface metric */
	SIOCSIFMETRIC,		/* set interface metric */
	SIOCDIFADDR,		/* delete interface address */
	SIOCAIFADDR,		/* configure interface alias */
	SIOCADDMULTI,		/* add multicast address */
	SIOCDELMULTI,		/* delete multicast address */
	SIOCGIFMTU,			/* get interface MTU */
	SIOCSIFMTU,			/* set interface MTU */
	SIOCSIFMEDIA,		/* set net media */
	SIOCGIFMEDIA,		/* get net media */

	SIOCGRTSIZE,		/* get route table size */
	SIOCGRTTABLE,		/* get route table */
	SIOCGETRT,			/* get route information for destination */

	SIOCGIFSTATS,		/* get interface stats */
	SIOCGIFPARAM,		/* get interface parameter */
	SIOCGIFTYPE,		/* get interface type */

	SIOCSPACKETCAP,		/* Start capturing packets on an interface */
	SIOCCPACKETCAP,		/* Stop capturing packets on an interface */

	SIOCSHIWAT,			/* set high watermark */
	SIOCGHIWAT,			/* get high watermark */
	SIOCSLOWAT,			/* set low watermark */
	SIOCGLOWAT,			/* get low watermark */
	SIOCATMARK,			/* at out-of-band mark? */
	SIOCSPGRP,			/* set process group */
	SIOCGPGRP			/* get process group */
};

#endif	/* _SYS_SOCKIO_H */
