/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ETHER_DRIVER_H
#define _ETHER_DRIVER_H

/*! Standard ethernet driver interface */


#include <Drivers.h>


/* ioctl() opcodes a driver should support */
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,
		/* get ethernet address (required) */
	ETHER_INIT,								/* (obsolete) */
	ETHER_NONBLOCK,							/* change non blocking mode (int *) */
	ETHER_ADDMULTI,							/* add multicast address */
	ETHER_REMMULTI,							/* remove multicast address */
	ETHER_SETPROMISC,						/* set promiscuous mode (int *) */
	ETHER_GETFRAMESIZE,						/* get frame size (required) (int *) */
	ETHER_GETLINKSTATE
		/* get line speed, quality, duplex mode, etc. (ether_link_state_t *) */
};


/* ETHER_GETADDR - MAC address */
typedef struct {
	uint8 ebyte[6];
} ether_address_t;

/* ETHER_GETLINKSTATE */
typedef struct ether_link_state {
	uint32	link_media;		/* as specified in net/if_media.h */
	uint32  link_quality;	/* in one tenth of a percent */
	uint64	link_speed;		/* in Kbit/s */
} ether_link_state_t;

#endif	/* _ETHER_DRIVER_H */
