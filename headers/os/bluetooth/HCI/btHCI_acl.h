/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTHCI_ACL_H_
#define _BTHCI_ACL_H_

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI.h>

#define HCI_ACL_HDR_SIZE	4

struct hci_acl_header {
	uint16 	handle;		/* Handle & Flags(PB, BC) */
	uint16 	alen;
} __attribute__ ((packed)) ;

/* ACL handle and flags pack/unpack */
#define pack_acl_handle_flags(h, pb, bc)	(((h) & 0x0fff) | (((pb) & 3) << 12) | (((bc) & 3) << 14))
#define get_acl_handle(h)					((h) & 0x0fff)
#define get_acl_pb_flag(h)					(((h) & 0x3000) >> 12)
#define get_acl_bc_flag(h)					(((h) & 0xc000) >> 14)

/* PB flag values */
/* 00 - reserved for future use */
#define	HCI_ACL_PACKET_FRAGMENT		0x1 
#define	HCI_ACL_PACKET_START		0x2
/* 11 - reserved for future use */

/* BC flag values */
#define HCI_ACL_POINT2POINT			0x0 /* only Host controller to Host */
#define HCI_ACL_BROADCAST_ACTIVE	0x1 /* both directions */
#define HCI_ACL_BROADCAST_PICONET	0x2 /* both directions */
										/* 11 - reserved for future use */

#endif // _BTHCI_ACL_H_
