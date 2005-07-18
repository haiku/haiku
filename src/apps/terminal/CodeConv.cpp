/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/************************************************************************
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

//////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////
CodeConv::CodeConv (void)
{

}
//////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////
CodeConv::~CodeConv (void)
{

}
//////////////////////////////////////////////////////////////////////
// UTF8GetFontWidth (const uchar *string)
// get font width in coding.
//////////////////////////////////////////////////////////////////////
int
CodeConv::UTF8GetFontWidth (const uchar *string)
{
  uchar  width, point;
  ushort unicode, offset;
  
  offset = unicode = UTF8toUnicode (string);
  width = utf8_width_table [unicode >> 3];
  offset = offset & 0x07;

  point = 0x80 >> offset;
  
  return ((width & point) > 0 ) ? 2 : 1;
}

//////////////////////////////////////////////////////////////////////
// ConvertFromInternal (const uchar *src, uchar *dst, int coding)
//	 Convert internal coding from src coding.
//////////////////////////////////////////////////////////////////////
int
CodeConv::ConvertFromInternal (const uchar *src, uchar *dst, int coding)
{
  const uchar *src_p = src;
  long srclen = 0, dstlen = 0;
  long state = 0;
  int theCoding;

  if (coding == M_UTF8) return 0;

  theCoding = coding_translation_table[coding];
  
  while (*src_p++) srclen++;

  dstlen = srclen * 256;

  convert_from_utf8 (theCoding,
		     (char *)src, &srclen,
		     (char *)dst, &dstlen,
		     &state, '?');

  if (coding == M_ISO_2022_JP && state != 0) {
    const char *end_of_jis = "(B";
    strncpy ((char *)dst+dstlen, end_of_jis, 3);
    dstlen += 3;
  }
  
  dst[dstlen] = '\0';
  
  return dstlen;

}
//////////////////////////////////////////////////////////////////////
// ConvertToInternal (const uchar *src, uchar *dst, int coding)
//	Convert internal coding to src coding.
//////////////////////////////////////////////////////////////////////
void
CodeConv::ConvertToInternal (const uchar *src, uchar *dst, int coding)
{
  const uchar *src_p = src;
  long srclen = 0, dstlen = 4;
  long state = 0;
  int theCoding;
  

  if (coding == M_UTF8) return;

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

  while (*src_p++) srclen++;
  
  convert_to_utf8 (theCoding,
		   (char *)src, &srclen,
		   (char *)dst, &dstlen,
		   &state, '?');

  dst[dstlen] = '\0';

  return;
}


/*
 * Private member functions.
 */
//////////////////////////////////////////////////////////////////////
// UTF8toUnicode (uchar *utf8)
// 
//////////////////////////////////////////////////////////////////////
unsigned short
CodeConv::UTF8toUnicode (const uchar *utf8)
{
  uchar tmp;
  unsigned short unicode = 0x0000;
  
  tmp = *utf8 & 0xe0;
  
  if (tmp < 0x80) {
    unicode = (unsigned short) *utf8;
  }
  else if (tmp < 0xe0) {
    unicode = (unsigned short) *utf8++ & 0x1f;
    unicode = unicode << 6;
    unicode = unicode | ((unsigned short) *utf8 & 0x3f);
  }
  else if (tmp < 0xf0) {
    unicode = (unsigned short) *utf8++ & 0x0f;
    unicode = unicode << 6;
    unicode = unicode | ((unsigned short) *utf8++ & 0x3f);
    unicode = unicode << 6;
    unicode = unicode | ((unsigned short) *utf8 & 0x3f);
  }

  return unicode;
}

//////////////////////////////////////////////////////////////////////
// euc_to_sjis
// 
//////////////////////////////////////////////////////////////////////
void
CodeConv::euc_to_sjis (uchar *buf)
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
    
  if (c1 >= 31) {
    c1 = c1 - 31 + 0xE0;
  }
  else {
    c1 += 0x81;
  }
    
  if (c2 >= 63) {
    c2 = c2 - 63 + 0x80;
  }
  else {
    c2 += 0x40;
  }
  
  *buf++ = c1;
  *buf = c2;

  return;
}
