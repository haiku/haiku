#ifndef AVUTIL_AVCONFIG_H
#define AVUTIL_AVCONFIG_H
#include <OS.h>
#ifdef __PPC__
#	define AV_HAVE_BIGENDIAN 1
#else
#	define AV_HAVE_BIGENDIAN 0
#endif
#endif /* AVUTIL_AVCONFIG_H */
