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


static inline void baswap(bdaddr_t* dst, bdaddr_t* src)
{
  register uint8* d = (uint8*)dst;
  register uint8* s = (uint8*)src;
  register int i;

  for(i=0; i<6; i++) d[i] = s[5-i];

}


/* TODO: Bluetooth Errors */
static inline char*	btstrerror(int error_code)
{
	return "Unknown Bluetooth error";
}


#endif // _BLUETOOTH_UTIL_H
