/*
 * Copyright 2020, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef _USB_ENDIAN_H_
#define	_USB_ENDIAN_H_

#include <stdint.h>

typedef uint8_t uByte;
typedef uint16_t uWord;
typedef uint32_t uDWord;
typedef uint64_t uQWord;

#define	UGETB(w)	(uint8_t)(w)
#define	UGETW(w)	(uint16_t)(w)
#define	UGETDW(w)	(uint32_t)(w)
#define	UGETQW(w)	(uint64_t)(w)

#define USETB(w,v)	{ (w) = (uint8_t)(v); }
#define USETW(w,v)	{ (w) = (uint16_t)(v); }
#define USETDW(w,v)	{ (w) = (uint32_t)(v); }
#define USETQW(w,v)	{ (w) = (uint64_t)(v); }

#endif					/* _USB_ENDIAN_H_ */
