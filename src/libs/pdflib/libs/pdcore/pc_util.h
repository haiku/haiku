/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: pc_util.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Various utility routines
 *
 */

#ifndef PC_UTIL_H
#define PC_UTIL_H

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "pc_config.h"
#include "pc_core.h"
#include "pc_ebcdic.h"
#include "pc_encoding.h"
#include "pc_output.h"
#include "pc_unicode.h"

#define PDC_GET_SHORT  pdc_get_le_short
#define PDC_GET_USHORT pdc_get_le_ushort
#define PDC_GET_WORD   pdc_get_le_ushort
#define PDC_GET_DWORD  pdc_get_le_ulong
#define PDC_GET_DWORD3 pdc_get_le_ulong3
#define PDC_GET_LONG   pdc_get_le_long
#define PDC_GET_ULONG  pdc_get_le_ulong

#define PDC_TIME_SBUF_SIZE	50
void	pdc_get_timestr(char *str);

void     pdc_setbit(char *bitarr, int bit);
pdc_bool pdc_getbit(char *bitarr, int bit);
void     pdc_setbit_text(char *bitarr, const unsigned char *text,
                         int len, int nbits, int size);

pdc_short  pdc_get_le_short(pdc_byte *data);
pdc_ushort pdc_get_le_ushort(pdc_byte *data);
pdc_long   pdc_get_le_long(pdc_byte *data);
pdc_ulong  pdc_get_le_ulong3(pdc_byte *data);
pdc_ulong  pdc_get_le_ulong(pdc_byte *data);
pdc_short  pdc_get_be_short(pdc_byte *data);
pdc_ushort pdc_get_be_ushort(pdc_byte *data);
pdc_long   pdc_get_be_long(pdc_byte *data);
pdc_ulong  pdc_get_be_ulong3(pdc_byte *data);
pdc_ulong  pdc_get_be_ulong(pdc_byte *data);

size_t  pdc_strlen(const char *text);
char	*pdc_getenv(const char *name);
char	*pdc_strdup(pdc_core *pdc, const char *text);
const char *pdc_strprint(pdc_core *pdc, const char *str, int len);
int	pdc_split_stringlist(pdc_core *pdc, const char *text,
	                     const char *i_separstr, char ***stringlist);
void    pdc_cleanup_stringlist(pdc_core *pdc, char **stringlist);
int	pdc_stricmp(const char *s1, const char *s2);
int	pdc_strincmp(const char *s1, const char *s2, int n);
char    *pdc_strtrim(char *m_str);
char    *pdc_str2trim(char *m_str);
void    pdc_swap_bytes(char *instring, int inlen, char *outstring);
pdc_bool pdc_str2double(const char *string, double *o_dz);
pdc_bool pdc_str2integer(const char *string, int *o_iz);


float	pdc_min(float a, float b);
float	pdc_max(float a, float b);

#endif	/* PC_UTIL_H */
