/*
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BTHCI_SCO_H_
#define _BTHCI_SCO_H_

#include <bluetooth/HCI/btHCI.h>

#define HCI_SCO_HDR_SIZE	3

struct hci_sco_header {
	uint16	handle;
	uint8	slen;
} __attribute__((packed));

#endif // _BTHCI_SCO_H_
