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

#ifndef _CODING__H_
#define _CODING__H_

#include <UTF8.h>

enum {
  M_UTF8,	  /* UTF-8 */
  M_ISO_8859_1,  /* ISO-8859 */
  M_ISO_8859_2,
  M_ISO_8859_3,
  M_ISO_8859_4,
  M_ISO_8859_5,
  M_ISO_8859_6,
  M_ISO_8859_7,
  M_ISO_8859_8,
  M_ISO_8859_9,
  M_ISO_8859_10,

  M_MAC_ROMAN,  /* Macintosh Roman */

  M_ISO_2022_JP,
  M_SJIS,	/* Japanese */
  M_EUC_JP,
  M_EUC_KR
  
  //  M_EUC_TW,	/* Chinese */
  //  M_BIG5,
  //  M_ISO_2022_CN,

  //  M_EUC_KR,	/* Koeran */
  //  M_ISO_2022_KR,

};

const uint32 coding_translation_table[] = {
  0,
  B_ISO1_CONVERSION,				/* ISO 8859-1 */
  B_ISO2_CONVERSION,				/* ISO 8859-2 */
  B_ISO3_CONVERSION,				/* ISO 8859-3 */
  B_ISO4_CONVERSION,				/* ISO 8859-4 */
  B_ISO5_CONVERSION,				/* ISO 8859-5 */
  B_ISO6_CONVERSION,				/* ISO 8859-6 */
  B_ISO7_CONVERSION,				/* ISO 8859-7 */
  B_ISO8_CONVERSION,				/* ISO 8859-8 */
  B_ISO9_CONVERSION,				/* ISO 8859-9 */
  B_ISO10_CONVERSION,				/* ISO 8859-10 */
  B_MAC_ROMAN_CONVERSION,			/* Macintosh Roman */
  B_JIS_CONVERSION,				/* JIS X 0208-1990 */
  B_SJIS_CONVERSION,				/* Shift-JIS */
  B_EUC_CONVERSION,				/* EUC Packed Japanese */
  B_EUC_KR_CONVERSION				/* EUC Korean */
};

struct etable
{
  const char *name;      // long name for menu item.
  const char *shortname; // short name (use for command-line etc.)
  const char shortcut;	 // short cut key code
  const uint32 op;       // encodeing opcode 
};

/*
 * encoding_table ... use encoding menu, message, and preference keys.
 */
const etable encoding_table[]=
{
  {"UTF-8", "UTF8", 'U', M_UTF8},
  {"ISO-8859-1", "8859-1", '1', M_ISO_8859_1},
  {"ISO-8859-2", "8859-2", '2', M_ISO_8859_2},
  {"ISO-8859-3", "8859-3", '3', M_ISO_8859_3},
  {"ISO-8859-4", "8859-4", '4', M_ISO_8859_4},
  {"ISO-8859-5", "8859-5", '5', M_ISO_8859_5},
  {"ISO-8859-6", "8859-6", '6', M_ISO_8859_6},
  {"ISO-8859-7", "8859-7", '7', M_ISO_8859_7},
  {"ISO-8859-8", "8859-8", '8', M_ISO_8859_8},
  {"ISO-8859-9", "8859-9", '9', M_ISO_8859_9},
  {"ISO-8859-10", "8859-10", '0', M_ISO_8859_10},
  {"MacRoman", "MacRoman",'M',  M_MAC_ROMAN},
  {"JIS", "JIS", 'J', M_ISO_2022_JP},
  {"Shift-JIS", "SJIS", 'S', M_SJIS},
  {"EUC-jp", "EUCJ", 'E', M_EUC_JP},
  {"EUC-kr", "EUCK", 'K', M_EUC_KR},

  /* Not Implement.
  {"EUC-tw", "EUCT", "T", M_EUC_TW},
  {"Big5", "Big5", 'B', M_BIG5},
  {"ISO-2022-cn", "ISOC", 'C', M_ISO_2022_CN},
  {"ISO-2022-kr", "ISOK", 'R', M_ISO_2022_KR},
  */  

  {NULL, NULL, 0, 0},
};


#endif /* _CODING_H_ */
