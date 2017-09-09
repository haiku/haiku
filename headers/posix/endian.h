/*
 * Copyright 2003-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ENDIAN_H_
#define _ENDIAN_H_


#include <config/HaikuConfig.h>
#ifdef _BSD_SOURCE
#include <support/ByteOrder.h>
#endif

/* Defines architecture dependent endian constants.
 * The constant reflects the byte order, "4" is the most
 * significant byte, "1" the least significant one.
 */

#if defined(__HAIKU_LITTLE_ENDIAN)
#	define LITTLE_ENDIAN	1234
#	define BIG_ENDIAN		0
#	define BYTE_ORDER		LITTLE_ENDIAN
#elif defined(__HAIKU_BIG_ENDIAN)
#	define BIG_ENDIAN		4321
#	define LITTLE_ENDIAN	0
#	define BYTE_ORDER		BIG_ENDIAN
#endif

#define __BIG_ENDIAN		BIG_ENDIAN
#define __LITTLE_ENDIAN		LITTLE_ENDIAN
#define __BYTE_ORDER		BYTE_ORDER

#ifdef _BSD_SOURCE

/*
 * General byte order swapping functions.
 */
#define	bswap16(x)	__swap_int16(x)
#define	bswap32(x)	__swap_int32(x)
#define	bswap64(x)	__swap_int64(x)

/*
 * Host to big endian, host to little endian, big endian to host, and little
 * endian to host byte order functions as detailed in byteorder(9).
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#define	htobe16(x)	bswap16((x))
#define	htobe32(x)	bswap32((x))
#define	htobe64(x)	bswap64((x))
#define	htole16(x)	((uint16_t)(x))
#define	htole32(x)	((uint32_t)(x))
#define	htole64(x)	((uint64_t)(x))

#define	be16toh(x)	bswap16((x))
#define	be32toh(x)	bswap32((x))
#define	be64toh(x)	bswap64((x))
#define	le16toh(x)	((uint16_t)(x))
#define	le32toh(x)	((uint32_t)(x))
#define	le64toh(x)	((uint64_t)(x))
#else /* BYTE_ORDER != LITTLE_ENDIAN */
#define	htobe16(x)	((uint16_t)(x))
#define	htobe32(x)	((uint32_t)(x))
#define	htobe64(x)	((uint64_t)(x))
#define	htole16(x)	bswap16((x))
#define	htole32(x)	bswap32((x))
#define	htole64(x)	bswap64((x))

#define	be16toh(x)	((uint16_t)(x))
#define	be32toh(x)	((uint32_t)(x))
#define	be64toh(x)	((uint64_t)(x))
#define	le16toh(x)	bswap16((x))
#define	le32toh(x)	bswap32((x))
#define	le64toh(x)	bswap64((x))
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

#endif /* _BSD_SOURCE */

#endif	/* _ENDIAN_H_ */
