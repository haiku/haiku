/*
 * Copyright 2003-2018 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _NET_IF_TUN_H
#define _NET_IF_TUN_H

#include <sys/sockio.h>

/*
 * public API for tun/tap device.
 * This API is compatible with the Linux tun/tap driver from
 * http://vtun.sourceforge.net/tun/index.html
 */


/* max of each type */
#define TUN_MAX_DEV     10
/*255*/

/* TX queue size */
#define TUN_TXQ_SIZE    10

/* Max frame size */
#define TUN_MAX_FRAME   4096

/* TUN device flags */
#define TUN_TUN_DEV     0x0001
#define TUN_TAP_DEV     0x0002
#define TUN_TYPE_MASK   0x000f

/* not yet*//*#define TUN_FASYNC      0x0010*/
#define TUN_NOCHECKSUM  0x0020
#define TUN_NO_PI       0x0040

#define TUN_IFF_SET     0x1000

/* Ioctl defines */
/* XXX: NOT OFFICIAL */
#define TUNSETNOCSUM (B_DEVICE_OP_CODES_END+0x90)
#define TUNSETDEBUG  (B_DEVICE_OP_CODES_END+0x91)
#define TUNSETIFF    (B_DEVICE_OP_CODES_END+0x92)

/* get/set MAC address */
//#define SIOCGIFHWADDR	(BONE_SOCKIO_IOCTL_BASE+0x95)
//#define SIOCSIFHWADDR	(BONE_SOCKIO_IOCTL_BASE+0x96)

/* TUNSETIFF ifr flags */
#define IFF_TUN         0x0001
#define IFF_TAP         0x0002
#define IFF_NO_PI       0x1000
/* XXX: fix the confusion about which *NO_PI go where ! */

struct tun_pi {
	unsigned short flags;
	unsigned short proto;
};
/* tun_pi::flags */
#define TUN_PKT_STRIP   0x0001

#define TUN_DEVICE "/dev/config/tun"

#endif /* __IF_TUN_H */
