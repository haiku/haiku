/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#include <ByteOrder.h>

/* Bluetooth version */
#define BLUETOOTH_1_1B		0
#define BLUETOOTH_1_1		1
#define BLUETOOTH_1_2		2
#define BLUETOOTH_2_0		3

#define BLUETOOTH_VERSION	BLUETOOTH_2_0


/* Bluetooth common types */

/* BD Address */
typedef struct {
	uint8 b[6];
} __attribute__((packed)) bdaddr_t;


#define BDADDR_NULL			((bdaddr_t) {{0, 0, 0, 0, 0, 0}})
#define BDADDR_LOCAL		((bdaddr_t) {{0, 0, 0, 0xff, 0xff, 0xff}})
#define BDADDR_BROADCAST	((bdaddr_t) {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}})
#define BDADDR_ANY			BDADDR_BROADCAST


/* Link key */
typedef struct {
	uint8 l[16];
} __attribute__((packed)) linkkey_t;


/* 128 integer type needed for SDP */
struct int128 {
	int8	b[16];
};
typedef struct int128	int128;
typedef struct int128	uint128;

/* Protocol definitions - add to as required... */
#define BLUETOOTH_PROTO_HCI		134	/* HCI protocol number */
#define BLUETOOTH_PROTO_L2CAP	135	/* L2CAP protocol number */
#define BLUETOOTH_PROTO_RFCOMM	136	/* RFCOMM protocol number */

#define BLUETOOTH_PROTO_MAX		256


#endif // _BLUETOOTH_H
