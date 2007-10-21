/*
 * Copyright 2007 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 */

#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#include <ByteOrder.h>

/* Bluetooth version */
#define BLUETOOTH_1_1B 0
#define BLUETOOTH_1_1  1
#define BLUETOOTH_1_2  2
#define BLUETOOTH_2_0  3

#define BLUETOOTH_VERSION BLUETOOTH_2_0


/* Bluetooth common types */

/* BD Address */
typedef struct {
	uint8 b[6];
} __attribute__((packed)) bdaddr_t;

/* 128 integer type needed for SDP */
struct int128 {
	int8	b[16];
};
typedef struct int128	int128;
typedef struct int128	uint128;


#endif
