/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_ENDIAN_H_
#define _OBSD_COMPAT_SYS_ENDIAN_H_


#include_next <sys/endian.h>


/* original BSD names */
#define betoh16(x)	be16toh(x)
#define betoh32(x)	be32toh(x)
#define betoh64(x)	be64toh(x)
#define letoh16(x)	le16toh(x)
#define letoh32(x)	le32toh(x)
#define letoh64(x)	le64toh(x)

#define swap16(x)	bswap16(x)
#define swap32(x)	bswap32(x)
#define swap64(x)	bswap64(x)


#endif	/* _OBSD_COMPAT_SYS_ENDIAN_H_ */
