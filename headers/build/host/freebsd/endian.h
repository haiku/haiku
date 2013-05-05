#ifndef _HAIKU_BUILD_COMPATIBILITY_FREEBSD_ENDIAN
#define _HAIKU_BUILD_COMPATIBILITY_FREEBSD_ENDIAN


// There's no <endian.h> in FreeBSD, but <sys/endian.h>. And the macro naming
// differs.

#include <sys/endian.h>


#define	__LITTLE_ENDIAN	_LITTLE_ENDIAN
#define	__BIG_ENDIAN	_BIG_ENDIAN
#define	__PDP_ENDIAN	_PDP_ENDIAN
#define	BYTE_ORDER		_BYTE_ORDER

#endif	// _HAIKU_BUILD_COMPATIBILITY_FREEBSD_ENDIAN
