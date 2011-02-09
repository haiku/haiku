/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/
#ifndef _ENCODINGS_H_
#define _ENCODINGS_H_

#ifdef __cplusplus
extern "C" {
#endif

status_t unicode_to_utf8(const uchar *uni, uint32 unilen, uint8 *utf8,
	uint32 *utf8len);
status_t utf8_to_unicode(const char *utf8, uchar *uni, uint32 *unilen);

#ifdef __cplusplus
}
#endif

#endif // _ENCODINGS_H_
