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

/* $Id: pc_unicode.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Unicode glyph name conversion routines
 *
 */

#ifndef PC_UNICODE_H
#define PC_UNICODE_H

#define PDC_MAX_UNICODE         (int) 0x0000FFFF

#define PDC_REPL_CHAR           (int) 0x0000FFFD

#define PDC_EURO_SIGN           (pdc_ushort) 0x20AC

/* The Unicode byte order mark (BOM) byte parts */
#define PDF_BOM0		0376		/* '\xFE' */
#define PDF_BOM1                0377            /* '\xFF' */
#define PDF_BOM2                0357            /* '\xEF' */
#define PDF_BOM3                0273            /* '\xBB' */
#define PDF_BOM4                0277            /* '\xBF' */

/*
 * check whether the string is plain C or UTF16 unicode
 * by looking for the BOM in big-endian or little-endian format resp.
 * s must not be NULL.
 */
#define pdc_is_utf16be_unicode(s) \
        (((pdc_byte *)(s))[0] == PDF_BOM0 && \
         ((pdc_byte *)(s))[1] == PDF_BOM1)

#define pdc_is_utf16le_unicode(s) \
        (((pdc_byte *)(s))[0] == PDF_BOM1 && \
         ((pdc_byte *)(s))[1] == PDF_BOM0)

#define pdc_is_unicode(s) pdc_is_utf16be_unicode(s)

/*
 * check whether the string is plain C or UTF8 unicode
 * by looking for the BOM
 * s must not be NULL.
 */
#define pdc_is_utf8_unicode(s) \
        (((pdc_byte *)(s))[0] == PDF_BOM2 && \
         ((pdc_byte *)(s))[1] == PDF_BOM3 && \
         ((pdc_byte *)(s))[2] == PDF_BOM4)

typedef struct
{
    pdc_ushort code;
    const char *glyphname;
} pdc_glyph_tab;

typedef enum
{
    conversionOK,           /* conversion successful */
    sourceExhausted,        /* partial character in source, but hit end */
    targetExhausted,        /* insuff. room in target for conversion */
    sourceIllegal           /* source sequence is illegal/malformed */
} pdc_convers_result;

typedef enum
{
    strictConversion = 0,
    lenientConversion
} pdc_convers_flags;

#define PDC_CONV_KEEPBYTES  (1<<0)
#define PDC_CONV_TRY7BYTES  (1<<1)
#define PDC_CONV_TRYBYTES   (1<<2)
#define PDC_CONV_WITHBOM    (1<<3)
#define PDC_CONV_NOBOM      (1<<4)

typedef enum
{
    pdc_auto,
    pdc_auto2,
    pdc_bytes,
    pdc_bytes2,
    pdc_utf8,
    pdc_utf16,
    pdc_utf16be,
    pdc_utf16le
} pdc_text_format;

pdc_ushort pdc_adobe2unicode(const char *name);

const char *pdc_unicode2adobe(pdc_ushort uv);

unsigned int pdc_name2sid(const char *name);

const char *pdc_sid2name(unsigned int sid);

pdc_bool pdc_is_std_charname(const char *name);

pdc_ushort pdc_get_equi_unicode(pdc_ushort uv);

int pdc_convert_string(pdc_core *pdc,
    pdc_text_format inutf, pdc_encodingvector *inev,
    pdc_byte *instring, int inlen, pdc_text_format *oututf_p,
    pdc_encodingvector *outev, pdc_byte **outstring, int *outlen, int flags,
    pdc_bool verbose);

#endif /* PC_UNICODE_H */
