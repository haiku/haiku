/*
	Copyright 1999-2001, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/


#include <ByteOrder.h>
#include <ctype.h>
#include <KernelExport.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>


#include "encodings.h"


// Pierre's Uber Macro
#define u_lendian_to_utf8(str, uni_str)\
{\
	if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\
		*str++ = B_LENDIAN_TO_HOST_INT16(*uni_str++);\
	else if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\
		str[0] = 0xc0|(B_LENDIAN_TO_HOST_INT16(uni_str[0])>>6);\
		str[1] = 0x80|(B_LENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 2;\
	} else if ((B_LENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(B_LENDIAN_TO_HOST_INT16(uni_str[0])>>12);\
		str[1] = 0x80|((B_LENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\
		str[2] = 0x80|(B_LENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((B_LENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_LENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}

// Pierre's Uber Macro
#define u_hostendian_to_utf8(str, uni_str)\
{\
	if ((uni_str[0]&0xff80) == 0)\
		*str++ = *uni_str++;\
	else if ((uni_str[0]&0xf800) == 0) {\
		str[0] = 0xc0|(uni_str[0]>>6);\
		str[1] = 0x80|(*uni_str++&0x3f);\
		str += 2;\
	} else if ((uni_str[0]&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(uni_str[0]>>12);\
		str[1] = 0x80|((uni_str[0]>>6)&0x3f);\
		str[2] = 0x80|(*uni_str++&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((uni_str[0]-0xd7c0)<<10) | (uni_str[1]&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}

// Another Uber Macro
#define utf8_to_u_hostendian(str, uni_str, err_flag) \
{\
	err_flag = 0;\
	if ((str[0]&0x80) == 0)\
		*uni_str++ = *str++;\
	else if ((str[1] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=1;\
	} else if ((str[0]&0x20) == 0) {\
		*uni_str++ = ((str[0]&31)<<6) | (str[1]&63);\
		str+=2;\
	} else if ((str[2] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=2;\
	} else if ((str[0]&0x10) == 0) {\
		*uni_str++ = ((str[0]&15)<<12) | ((str[1]&63)<<6) | (str[2]&63);\
		str+=3;\
	} else if ((str[3] & 0xC0) != 0x80) {\
		*uni_str++ = 0xfffd;\
		str+=3;\
	} else {\
		err_flag = 1;\
	}\
}

// Count the number of bytes of a UTF-8 character
#define utf8_char_len(c) ((((int32)0xE5000000 >> ((c >> 3) & 0x1E)) & 3) + 1)

// converts LENDIAN unicode to utf8
static status_t
_lendian_unicode_to_utf8(
	const char *src,
	int32 *srcLen,
	char *dst,
	uint32 *dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16  *UNICODE = (uint16 *)&src[srcCount];
		if (*UNICODE == 0)
			break;
		uchar	utf8[4];
		uchar	*UTF8 = utf8;
		int32 utf8Len;
		int32 j;

		u_lendian_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if ((dstCount + utf8Len) > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;
	dst[dstCount] = '\0';
	
	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}

// utf8 to LENDIAN unicode
static status_t
_utf8_to_lendian_unicode(
	const char	*src,
	int32		*srcLen,
	char		*dst,
	uint32		*dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen - 1;
	int32 srcCount = 0;
	int32 dstCount = 0;

	while ((srcCount < srcLimit) && (dstCount < dstLimit)) {
		uint16	unicode;
		uint16	*UNICODE = &unicode;
		uchar	*UTF8 = (uchar *)src + srcCount;
		int     err_flag;

		if ((srcCount + utf8_char_len(src[srcCount])) > srcLimit)
			break;

		utf8_to_u_hostendian(UTF8, UNICODE, err_flag);
		if(err_flag == 1)
			return EINVAL;

		unicode = B_HOST_TO_LENDIAN_INT16(unicode);
		dst[dstCount++] = unicode & 0xFF;
		dst[dstCount++] = unicode >> 8;

		srcCount += UTF8 - ((uchar *)(src + srcCount));
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return ((dstCount > 0) ? B_NO_ERROR : B_ERROR);
}


// takes a unicode name of unilen uchar's and converts to a utf8 name of at
// most utf8len uint8's
status_t unicode_to_utf8(const uchar *uni, uint32 unilen, uint8 *utf8,
	uint32 *utf8len)
{
	uint32 origlen = unilen;
	status_t result = _lendian_unicode_to_utf8((char *)uni,
			(int32 *)&unilen, (char *)utf8, utf8len);

	/*if (unilen < origlen) {
		panic("Name is too long (%lx < %lx)\n", unilen, origlen);
		return B_ERROR;
	}*/

	return result;
}


status_t utf8_to_unicode(const char *utf8, uchar *uni, uint32 *unilen)
{
	uint32 origlen = strlen(utf8) + 1;
	uint32 utf8len = origlen;

	status_t result = _utf8_to_lendian_unicode(utf8,
			(int32 *)&utf8len, (char *)uni, unilen);

	/*if (origlen < utf8len) {
		panic("Name is too long (%lx < %lx)\n", *unilen, origlen);
		return B_ERROR;
	}*/

	return result;
}

