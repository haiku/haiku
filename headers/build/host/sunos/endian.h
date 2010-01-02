#ifndef _HAIKU_BUILD_COMPATIBILITY_SUNOS_ENDIAN
#define _HAIKU_BUILD_COMPATIBILITY_SUNOS_ENDIAN

// There's no <endian.h> in Solaris.

#include <sys/byteorder.h>

#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321
#define __PDP_ENDIAN	3412

#if defined(_LITTLE_ENDIAN)
#define	BYTE_ORDER		__LITTLE_ENDIAN
#elif defined(_BIG_ENDIAN)
#define	BYTE_ORDER		__BIG_ENDIAN
#else
#error Unable to determine byte order!
#endif

#define __BYTE_ORDER	BYTE_ORDER

#endif	// _HAIKU_BUILD_COMPATIBILITY_SUNOS_ENDIAN
