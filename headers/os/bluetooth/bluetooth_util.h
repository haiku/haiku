/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _BLUETOOTH_UTIL_H
#define _BLUETOOTH_UTIL_H

#include <bluetooth/bluetooth.h>
#include <string.h>

/* BD Address management */
static inline int bacmp(bdaddr_t* ba1, bdaddr_t* ba2)
{
	return memcmp(ba1, ba2, sizeof(bdaddr_t));
}


static inline void bacpy(bdaddr_t* dst, bdaddr_t* src)
{
	memcpy(dst, src, sizeof(bdaddr_t));
}


static inline void baswap(bdaddr_t* dst, bdaddr_t* src) {

}


static inline char*	batostr(bdaddr_t *ba)
{
	return "00:00:00:00:00:00";

}


static inline void strtoba(const char *str, bdaddr_t *ba)
{

}


/* Link key Management */
static inline char* lktostr( uint8 link_key[16] )
{
	return "00:00:00:00:00:00";
}


/* TODO: Bluetooth Errors */
static inline char*	btstrerror(int error_code)
{
	return "Unknown Bluetooth error";
}


#endif // _BLUETOOTH_UTIL_H
