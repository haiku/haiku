/*
 * ether_driver.h
 *
 * Ethernet driver: handles NE2000 and 3C503 cards
 */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _ETHER_DRIVER_H
#define _ETHER_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <Drivers.h>

/*
 * ioctls: belongs in a public header file
 * somewhere, so that the net_server and other ethernet drivers can use.
 */
enum {
	ETHER_GETADDR = B_DEVICE_OP_CODES_END,	/* get ethernet address */
	ETHER_INIT,								/* set irq and port */
	ETHER_NONBLOCK,							/* set/unset nonblocking mode */
	ETHER_ADDMULTI,							/* add multicast addr */
	ETHER_REMMULTI,							/* rem multicast addr */
	ETHER_SETPROMISC,						/* set promiscuous */
	ETHER_GETFRAMESIZE,						/* get frame size */
	ETHER_ADDTIMESTAMP,						/* (try to) add timestamps to packets (BONE ext) */
	ETHER_HASIOVECS,						/* does the driver implement writev ? (BONE ext) (bool *) */
	ETHER_GETIFTYPE,						/* get the IFT_ type of the interface (int *) */
	ETHER_GETLINKSTATE						/* get line speed, quality, duplex mode, etc. */
};


/*
 * 48-bit ethernet address, passed back from ETHER_GETADDR
 */
typedef struct {
	unsigned char ebyte[6];
} ether_address_t;


/*
 *	info passed to ETHER_INIT
 */

typedef struct ether_init_params {
	short port;
	short irq;
	unsigned long mem;
} ether_init_params_t;

/*
 *  info returned from ETHER_GETLINKSTATE
 */

typedef struct ether_link_state {
	float  link_speed; 	 /* In Mbits per second */
	float  link_quality; /* Set to zero if not connected */
	char   duplex_mode;  /* Set to 1 for full duplex, 0 for half */
} ether_link_state_t;

#ifdef __cplusplus
}
#endif


#endif /* _ETHER_DRIVER_H */
