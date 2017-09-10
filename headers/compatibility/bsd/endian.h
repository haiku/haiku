/*
 * Copyright 2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _BSD_ENDIAN_H_
#define _BSD_ENDIAN_H_


#include_next <endian.h>


#ifdef _BSD_SOURCE

#include <config/HaikuConfig.h>
#include <support/ByteOrder.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif


#endif


#endif	/* _BSD_ENDIAN_H_ */
