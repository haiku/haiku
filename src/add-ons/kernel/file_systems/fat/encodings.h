/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _DOSFS_ENCODINGS_H_
#define _DOSFS_ENCODINGS_H_


#ifndef FS_SHELL
#include <ByteOrder.h>
#else // FS_SHELL
typedef unsigned char uchar;

#include "fssh_api_wrapper.h"
#endif // FS_SHELL


#ifdef __cplusplus
extern "C" {
#endif


static inline bool is_unicode_japanese(uint16 c)
{
	if (((c >= 0x3000) && (c <= 0x30ff)) ||
			((c >= 0x3200) && (c <= 0x3400)) ||
			((c >= 0x4e00) && (c <= 0x9fff)) ||
			((c >= 0xf900) && (c <= 0xfaff)) ||
			((c >= 0xfe30) && (c <= 0xfe6f)) ||
			((c >= 0xff00) && (c <= 0xffef)))
		return true;

	return false;
}


status_t unicode_to_utf8(const uchar *uni, uint32 unilen, uint8 *utf8,
	uint32 utf8len);

bool requires_munged_short_name(const uchar *utf8name,
               const uchar nshort[11], int encoding);

bool requires_long_name(const char *utf8, const uchar *unicode);
status_t utf8_to_unicode(const char *utf8, uchar *uni, uint32 unilen);
status_t munge_short_name2(uchar nshort[11], int encoding);
status_t munge_short_name1(uchar nshort[11], int iteration, int encoding);
status_t generate_short_name(const uchar *name, const uchar *uni,
		uint32 unilen, uchar nshort[11], int *encoding);
status_t generate_short_name_sjis(const uchar *utf8, const uint16 *uni,
		uint32 unilen, uchar nshort[11]);

status_t msdos_to_utf8(uchar *msdos, uchar *utf8, uint32 utf8len,
		uint8 toLower);

#ifdef __cplusplus
}
#endif

#endif
