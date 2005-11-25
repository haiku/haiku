/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */

/*********************************************************************
MuTerminal Code Conversion class (MC3).

version 1.1 Internal coding system is UTF8.

Code conversion member functions are convert character code and calc
font width.

Available coding systems.

ISO-8859-1 ... 10
Shift JIS
EUC-jp
UTF8

************************************************************************/

#include <support/UTF8.h>
#include <string.h>

#include "CodeConv.h"

extern char utf8_width_table[]; /* define UTF8WidthTbl.c */


CodeConv::CodeConv()
{
}


CodeConv::~CodeConv()
{
}


//! get font width in coding.
int32
CodeConv::UTF8GetFontWidth(const char *string)
{
	uchar width, point;
	ushort unicode, offset;

	offset = unicode = UTF8toUnicode(string);
	width = utf8_width_table[unicode >> 3];
	offset = offset & 0x07;

	point = 0x80 >> offset;

	return (width & point) > 0 ? 2 : 1;
}


//!	Convert internal coding from src coding.
int32
CodeConv::ConvertFromInternal(const char *src, int32 srclen, char *dst, int coding)
{
	int32 dstlen = 0;
	long state = 0;
	int theCoding;

	if (srclen == -1)
		srclen = strlen(src);

	if (coding == M_UTF8) {
		memcpy(dst, src, srclen);
		dst[srclen] = '\0';
		return srclen;
	}

	theCoding = coding_translation_table[coding];


	dstlen = srclen * 256;

	convert_from_utf8(theCoding, (char *)src, &srclen,
		(char *)dst, &dstlen, &state, '?');

	if (coding == M_ISO_2022_JP && state != 0) {
		const char *end_of_jis = "(B";
		strncpy((char *)dst + dstlen, end_of_jis, 3);
		dstlen += 3;
	}

	dst[dstlen] = '\0';
	return dstlen;
}


//! Convert internal coding to src coding.
int32
CodeConv::ConvertToInternal(const char *src, int32 srclen, char *dst, int coding)
{
	int32 dstlen = 4;
	long state = 0;
	int theCoding;

	if (srclen == -1)
		srclen = strlen(src);

	if (coding == M_UTF8) {
		memcpy(dst, src, srclen);
		dst[srclen] = '\0';
		return srclen;
	}

#if 0
#ifndef __INTEL__
  if (coding = M_EUC_JP) {
    uchar buf[2];
    memcpy (buf, src, 2);
    euc_to_sjis (buf);
    srclen = 2;
    convert_to_utf8 (B_SJIS_CONVERSION, (char*) buf, &srclen,
		     (char *)dst, &dstlen,
		     &state, '?');
    dst[dstlen] = '\0';
    return;
  }
#endif  
#endif

	theCoding = coding_translation_table[coding];

	convert_to_utf8(theCoding, (char *)src, &srclen,
		(char *)dst, &dstlen, &state, '?');

	dst[dstlen] = '\0';
	return dstlen;
}


unsigned short
CodeConv::UTF8toUnicode(const char *utf8)
{
	uchar tmp;
	unsigned short unicode = 0x0000;

	tmp = *utf8 & 0xe0;

	if (tmp < 0x80) {
		unicode = (unsigned short)*utf8;
	} else if (tmp < 0xe0) {
		unicode = (unsigned short)*utf8++ & 0x1f;
		unicode = unicode << 6;
		unicode = unicode | ((unsigned short)*utf8 & 0x3f);
	} else if (tmp < 0xf0) {
		unicode = (unsigned short)*utf8++ & 0x0f;
		unicode = unicode << 6;
		unicode = unicode | ((unsigned short)*utf8++ & 0x3f);
		unicode = unicode << 6;
		unicode = unicode | ((unsigned short)*utf8 & 0x3f);
	}

	return unicode;
}


void
CodeConv::euc_to_sjis(uchar *buf)
{
	int gl, gr;
	int p;
	int c1, c2;

	gl = *buf & 0x7F;
	gr = *(buf + 1) & 0x7F;
	gl -= 0x21;
	gr -= 0x21;

	p = gl * 94 + gr;

	c1 = p / 188;
	c2 = p % 188;

	if (c1 >= 31)
		c1 = c1 - 31 + 0xE0;
	else
		c1 += 0x81;

	if (c2 >= 63)
		c2 = c2 - 63 + 0x80;
	else
		c2 += 0x40;

	*buf++ = c1;
	*buf = c2;
}
