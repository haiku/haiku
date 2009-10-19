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

/* $Id: pc_unicode.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines for converting between Unicode values and Adobe glyph names
 *
 */

#include "pc_util.h"
#include "pc_chartabs.h"


/*
 * Returns the Unicode value of a glyph name. If the name is not
 * contained in the Adobe Glyph List (AGL) 0 will be returned.
 */

pdc_ushort
pdc_adobe2unicode(const char *name)
{
    int lo = 0;
    int hi = ((sizeof tab_agl2uni) / (sizeof (pdc_glyph_tab)));

    if (name)
    {
        while (lo < hi)
        {
            int i = (lo + hi) / 2;
            int cmp = strcmp(name, tab_agl2uni[i].glyphname);

            if (cmp == 0)
                return tab_agl2uni[i].code;

            if (cmp < 0)
                hi = i;
            else
                lo = i + 1;
        }
    }

    return 0;
}

/*
 * Returns the name in the Adobe Glyph List which corresponds to
 * the supplied Unicode value. If the value doesn't have a
 * corresponding Unicode name NULL will be returned.
 */

const char *
pdc_unicode2adobe(pdc_ushort uv)
{
    int lo = 0;
    int hi = ((sizeof tab_uni2agl) / (sizeof (pdc_glyph_tab)));

    if (uv)
    {
        while (lo < hi)
        {
            int i = (lo + hi) / 2;

            if (uv == tab_uni2agl[i].code)
                return tab_uni2agl[i].glyphname;

            if (uv < tab_uni2agl[i].code)
                hi = i;
            else
                lo = i + 1;
        }
    }

    return (char *) 0;
}



/*
 * Returns true if a character name is contained in pc_standard_latin_charset.
 * Otherwise false will be returned.
 */

pdc_bool
pdc_is_std_charname(const char *name)
{
    int lo = 0;
    int hi = ((sizeof pc_standard_latin_charset) / (sizeof (char *)));

    if (name)
    {
        while (lo < hi)
        {
            int i = (lo + hi) / 2;
            int cmp = strcmp(name, pc_standard_latin_charset[i]);

            if (cmp == 0)
                return pdc_true;

            if (cmp < 0)
                hi = i;
            else
                lo = i + 1;
        }
    }

    return pdc_false;
}

/*
 *  The following source is based on Unicode's original source
 *  code ConvertUTF.c. It has been adapted to PDFlib programming
 *  conventions.
 *
 *  The original file had the following notice:
 *
 *      Copyright 2001 Unicode, Inc.
 *
 *      Limitations on Rights to Redistribute This Code
 *
 *      Author: Mark E. Davis, 1994.
 *      Rev History: Rick McGowan, fixes & updates May 2001.
 *
 *
 *  Functions for conversions between UTF32, UTF-16, and UTF-8.
 *  These funtions forming a complete set of conversions between
 *  the three formats. UTF-7 is not included here.
 *
 *  Each of these routines takes pointers to input buffers and output
 *  buffers. The input buffers are const.
 *
 *  Each routine converts the text between *sourceStart and sourceEnd,
 *  putting the result into the buffer between *targetStart and
 *  targetEnd. Note: the end pointers are *after* the last item: e.g.
 *  *(sourceEnd - 1) is the last item.
 *
 *  The return result indicates whether the conversion was successful,
 *  and if not, whether the problem was in the source or target buffers.
 *  (Only the first encountered problem is indicated.)
 *
 *  After the conversion, *sourceStart and *targetStart are both
 *  updated to point to the end of last text successfully converted in
 *  the respective buffers.
 *
 *  Input parameters:
 *      sourceStart - pointer to a pointer to the source buffer.
 *              The contents of this are modified on return so that
 *              it points at the next thing to be converted.
 *      targetStart - similarly, pointer to pointer to the target buffer.
 *      sourceEnd, targetEnd - respectively pointers to the ends of the
 *              two buffers, for overflow checking only.
 *
 *  These conversion functions take a pdc_convers_flags argument. When this
 *  flag is set to strict, both irregular sequences and isolated surrogates
 *  will cause an error.  When the flag is set to lenient, both irregular
 *  sequences and isolated surrogates are converted.
 *
 *  Whether the flag is strict or lenient, all illegal sequences will cause
 *  an error return. This includes sequences such as: <F4 90 80 80>, <C0 80>,
 *  or <A0> in UTF-8, and values above 0x10FFFF in UTF-32. Conformant code
 *  must check for illegal sequences.
 *
 *  When the flag is set to lenient, characters over 0x10FFFF are converted
 *  to the replacement character; otherwise (when the flag is set to strict)
 *  they constitute an error.
 *
 *  Output parameters:
 *      The value "sourceIllegal" is returned from some routines if the input
 *      sequence is malformed.  When "sourceIllegal" is returned, the source
 *      value will point to the illegal value that caused the problem. E.g.,
 *      in UTF-8 when a sequence is malformed, it points to the start of the
 *      malformed sequence.
 *
 *  Author: Mark E. Davis, 1994.
 *  Rev History: Rick McGowan, fixes & updates May 2001.
 *
 */

/*
 * The following 4 definitions are compiler-specific.
 * The C standard does not guarantee that wchar_t has at least
 * 16 bits, so wchar_t is no less portable than unsigned short!
 * All should be unsigned values to avoid sign extension during
 * bit mask & shift operations.
 */

typedef unsigned long   UTF32;  /* at least 32 bits */
typedef unsigned short  UTF16;  /* at least 16 bits */
typedef unsigned char   UTF8;   /* typically 8 bits */

/* Some fundamental constants */
#define UNI_SUR_HIGH_START      (UTF32)0xD800
#define UNI_SUR_HIGH_END        (UTF32)0xDBFF
#define UNI_SUR_LOW_START       (UTF32)0xDC00
#define UNI_SUR_LOW_END         (UTF32)0xDFFF
#define UNI_REPLACEMENT_CHAR    (UTF32)0x0000FFFD
#define UNI_MAX_BMP             (UTF32)0x0000FFFF
#define UNI_MAX_UTF16           (UTF32)0x0010FFFF
#define UNI_MAX_UTF32           (UTF32)0x7FFFFFFF

static const int halfShift      = 10; /* used for shifting by 10 bits */

static const UTF32 halfBase     = 0x0010000UL;
static const UTF32 halfMask     = 0x3FFUL;


/* --------------------------------------------------------------------- */

#if 0
static pdc_convers_result
pdc_convertUTF32toUTF16 (
                UTF32** sourceStart, const UTF32* sourceEnd,
                UTF16** targetStart, const UTF16* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF32* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch;
        if (target >= targetEnd) {
            result = targetExhausted; break;
        }
        ch = *source++;
        if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
            if ((flags == strictConversion) &&
                (ch >= UNI_SUR_HIGH_START &&
                 ch <= UNI_SUR_LOW_END)) {
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            } else {
                *target++ = (UTF16) ch;     /* normal case */
            }
        } else if (ch > UNI_MAX_UTF16) {
            if (flags == strictConversion) {
                result = sourceIllegal;
            } else {
                *target++ = UNI_REPLACEMENT_CHAR;
            }
        } else {
            /* target is a character in range 0xFFFF - 0x10FFFF. */
            if (target + 1 >= targetEnd) {
                result = targetExhausted;
                break;
            }
            ch -= halfBase;
            *target++ = (UTF16) ((ch >> halfShift) + UNI_SUR_HIGH_START);
            *target++ = (UTF16) ((ch & halfMask) + UNI_SUR_LOW_START);
        }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

static pdc_convers_result
pdc_convertUTF16toUTF32 (
                UTF16** sourceStart, UTF16* sourceEnd,
                UTF32** targetStart, const UTF32* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF16* source = *sourceStart;
    UTF32* target = *targetStart;
    UTF32 ch, ch2;
    while (source < sourceEnd) {
        ch = *source++;
        if (ch >= UNI_SUR_HIGH_START &&
            ch <= UNI_SUR_HIGH_END &&
            source < sourceEnd) {
            ch2 = *source;
            if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) {
                ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
                      + (ch2 - UNI_SUR_LOW_START) + halfBase;
                ++source;
            } else if (flags == strictConversion) {
                /* it's an unpaired high surrogate */
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            }
        } else if ((flags == strictConversion) &&
                   (ch >= UNI_SUR_LOW_START &&
                    ch <= UNI_SUR_LOW_END)) {
            /* an unpaired low surrogate */
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
        }
        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        *target++ = ch;
    }
    *sourceStart = source;
    *targetStart = target;
#ifdef CVTUTF_DEBUG
if (result == sourceIllegal) {
    fprintf(stderr, "pdc_convertUTF16toUTF32 illegal seq 0x%04x,%04x\n",
            ch, ch2);
    fflush(stderr);
}
#endif
    return result;
}
#endif

/* --------------------------------------------------------------------- */

/*
 * Index into the table below with the first byte of a UTF-8 sequence to
 * get the number of trailing bytes that are supposed to follow it.
 */
static const char trailingBytesForUTF8[256] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

#if 0
static const char
pdc_get_trailingBytesForUTF8(int i) {
    return (trailingBytesForUTF8[i]);
}
#endif

/*
 * Magic values subtracted from a buffer value during UTF8 conversion.
 * This table contains as many values as there might be trailing bytes
 * in a UTF-8 sequence.
 */
static const UTF32 offsetsFromUTF8[6] = {
    0x00000000UL, 0x00003080UL, 0x000E2080UL,
    0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... six byte sequence.)
 */
static const UTF8 firstByteMark[7] = {
    0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC
};

/* --------------------------------------------------------------------- */

/* The interface converts a whole buffer to avoid function-call overhead.
 * Constants have been gathered. Loops & conditionals have been removed as
 * much as possible for efficiency, in favor of drop-through switches.
 * (See "Note A" at the bottom of the file for equivalent code.)
 * If your compiler supports it, the "pdc_islegalUTF8" call can be turned
 * into an inline function.
 */

/* --------------------------------------------------------------------- */

static pdc_convers_result
pdc_convertUTF16toUTF8 (
                UTF16** sourceStart, const UTF16* sourceEnd,
                UTF8** targetStart, const UTF8* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF16* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch;
        unsigned short bytesToWrite = 0;
        const UTF32 byteMask = 0xBF;
        const UTF32 byteMark = 0x80;
        ch = *source++;
        /* If we have a surrogate pair, convert to UTF32 first. */
        if (ch >= UNI_SUR_HIGH_START &&
            ch <= UNI_SUR_HIGH_END &&
            source < sourceEnd) {
            UTF32 ch2 = *source;
            if (ch2 >= UNI_SUR_LOW_START &&
                ch2 <= UNI_SUR_LOW_END) {
                ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
                        + (ch2 - UNI_SUR_LOW_START) + halfBase;
                ++source;
            } else if (flags == strictConversion) {
                /* it's an unpaired high surrogate */
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            }
        } else if ((flags == strictConversion) &&
                   (ch >= UNI_SUR_LOW_START &&
                    ch <= UNI_SUR_LOW_END)) {
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
        }
        /* Figure out how many bytes the result will require */
        if (ch < (UTF32)0x80) {                 bytesToWrite = 1;
        } else if (ch < (UTF32)0x800) {         bytesToWrite = 2;
        } else if (ch < (UTF32)0x10000) {       bytesToWrite = 3;
        } else if (ch < (UTF32)0x200000) {      bytesToWrite = 4;
        } else {                                bytesToWrite = 2;
                                                ch = UNI_REPLACEMENT_CHAR;
        }

        target += bytesToWrite;
        if (target > targetEnd) {
            target -= bytesToWrite; result = targetExhausted; break;
        }
        switch (bytesToWrite) { /* note: everything falls through. */
            case 4: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 3: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 2: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 1: *--target = (UTF8) (ch | firstByteMark[bytesToWrite]);
        }
        target += bytesToWrite;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * This must be called with the length pre-determined by the first byte.
 * If not calling this from pdc_convertUTF8to*, then the length can be set by:
 *      length = trailingBytesForUTF8[*source]+1;
 * and the sequence is illegal right away if there aren't that many bytes
 * available.
 * If presented with a length > 4, this returns pdc_false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */

static pdc_bool
pdc_islegalUTF8(UTF8 *source, int length) {
    UTF8 a;
    UTF8 *srcptr = source+length;
    switch (length) {
    default: return pdc_false;
        /* Everything else falls through when "pdc_true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return pdc_false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return pdc_false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return pdc_false;
        switch (*source) {
            /* no fall-through in this inner switch */
            case 0xE0: if (a < 0xA0) return pdc_false; break;
            case 0xF0: if (a < 0x90) return pdc_false; break;
            case 0xF4: if (a > 0x8F) return pdc_false; break;
            default:  if (a < 0x80) return pdc_false;
        }
    case 1: if (*source >= 0x80 && *source < 0xC2) return pdc_false;
            if (*source > 0xF4) return pdc_false;
    }
    return pdc_true;
}

/* --------------------------------------------------------------------- */

#if 0
/*
 * Exported function to return whether a UTF-8 sequence is legal or not.
 * This is not used here; it's just exported.
 */
static pdc_bool pdc_islegalUTF8sequence(UTF8 *source, UTF8 *sourceEnd) {
    int length = trailingBytesForUTF8[*source]+1;
    if (source+length > sourceEnd) {
        return pdc_false;
    }
    return pdc_islegalUTF8(source, length);
}
#endif

/* --------------------------------------------------------------------- */

static pdc_convers_result
pdc_convertUTF8toUTF16 (
                UTF8** sourceStart, UTF8* sourceEnd,
                UTF16** targetStart, const UTF16* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF8* source = *sourceStart;
    UTF16* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch = 0L;
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        if (source + extraBytesToRead >= sourceEnd) {
            result = sourceExhausted;
            break;
        }
        /* Do this check whether lenient or strict */
        if (! pdc_islegalUTF8(source, extraBytesToRead+1)) {
            result = sourceIllegal;
            break;
        }
        /*
         * The cases all fall through. See "Note A" below.
         */
        switch (extraBytesToRead) {
            case 3: ch += *source++; ch <<= 6;
            case 2: ch += *source++; ch <<= 6;
            case 1: ch += *source++; ch <<= 6;
            case 0: ch += *source++;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        if (ch <= UNI_MAX_BMP) { /* Target is a character <= 0xFFFF */
            if ((flags == strictConversion) &&
                (ch >= UNI_SUR_HIGH_START &&
                 ch <= UNI_SUR_LOW_END)) {
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            } else {
                *target++ = (UTF16) ch;     /* normal case */
            }
        } else if (ch > UNI_MAX_UTF16) {
            if (flags == strictConversion) {
                    result = sourceIllegal;
                    source -= extraBytesToRead; /* return to the start */
            } else {
                    *target++ = UNI_REPLACEMENT_CHAR;
            }
        } else {
            /* target is a character in range 0xFFFF - 0x10FFFF. */
            if (target + 1 >= targetEnd) {
                    result = targetExhausted;
                    break;
            }
            ch -= halfBase;
            *target++ = (UTF16) ((ch >> halfShift) + UNI_SUR_HIGH_START);
            *target++ = (UTF16) ((ch & halfMask) + UNI_SUR_LOW_START);
        }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

#if 0
static pdc_convers_result
pdc_convertUTF32toUTF8 (
                UTF32** sourceStart, const UTF32* sourceEnd,
                UTF8** targetStart, const UTF8* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF32* source = *sourceStart;
    UTF8* target = *targetStart;
    while (source < sourceEnd) {
        UTF32 ch;
        unsigned short bytesToWrite = 0;
        const UTF32 byteMask = 0x000000BF;
        const UTF32 byteMark = 0x00000080;
        ch = *source++;
        /* surrogates of any stripe are not legal UTF32 characters */
        if (flags == strictConversion ) {
            if ((ch >= UNI_SUR_HIGH_START) && (ch <= UNI_SUR_LOW_END)) {
                --source; /* return to the illegal value itself */
                result = sourceIllegal;
                break;
            }
        }
        /* Figure out how many bytes the result will require */
        if (ch < (UTF32)0x80) {                 bytesToWrite = 1;
        } else if (ch < (UTF32)0x800) {         bytesToWrite = 2;
        } else if (ch < (UTF32)0x10000) {       bytesToWrite = 3;
        } else if (ch < (UTF32)0x200000) {      bytesToWrite = 4;
        } else {                                bytesToWrite = 2;
                                                ch = UNI_REPLACEMENT_CHAR;
        }

        target += bytesToWrite;
        if (target > targetEnd) {
            target -= bytesToWrite; result = targetExhausted; break;
        }
        switch (bytesToWrite) { /* note: everything falls through. */
            case 4: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 3: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 2: *--target = (UTF8) ((ch | byteMark) & byteMask); ch >>= 6;
            case 1: *--target = (UTF8) (ch | firstByteMark[bytesToWrite]);
        }
        target += bytesToWrite;
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}

/* --------------------------------------------------------------------- */

static pdc_convers_result
pdc_convertUTF8toUTF32 (
                UTF8** sourceStart, UTF8* sourceEnd,
                UTF32** targetStart, const UTF32* targetEnd,
                const pdc_convers_flags flags) {
    pdc_convers_result result = conversionOK;
    UTF8* source = *sourceStart;
    UTF32* target = *targetStart;

    (void) flags;

    while (source < sourceEnd) {
        UTF32 ch = 0;
        unsigned short extraBytesToRead = trailingBytesForUTF8[*source];
        if (source + extraBytesToRead >= sourceEnd) {
            result = sourceExhausted; break;
        }
        /* Do this check whether lenient or strict */
        if (! pdc_islegalUTF8(source, extraBytesToRead+1)) {
            result = sourceIllegal;
            break;
        }
        /*
         * The cases all fall through. See "Note A" below.
         */
        switch (extraBytesToRead) {
            case 3: ch += *source++; ch <<= 6;
            case 2: ch += *source++; ch <<= 6;
            case 1: ch += *source++; ch <<= 6;
            case 0: ch += *source++;
        }
        ch -= offsetsFromUTF8[extraBytesToRead];

        if (target >= targetEnd) {
            result = targetExhausted;
            break;
        }
        if (ch <= UNI_MAX_UTF32) {
            *target++ = ch;
        } else if (ch > UNI_MAX_UTF32) {
            *target++ = UNI_REPLACEMENT_CHAR;
        } else {
            if (target + 1 >= targetEnd) {
                result = targetExhausted;
                break;
            }
            ch -= halfBase;
            *target++ = (ch >> halfShift) + UNI_SUR_HIGH_START;
            *target++ = (ch & halfMask) + UNI_SUR_LOW_START;
        }
    }
    *sourceStart = source;
    *targetStart = target;
    return result;
}
#endif

/* ---------------------------------------------------------------------

        Note A.
        The fall-through switches in UTF-8 reading code save a
        temp variable, some decrements & conditionals.  The switches
        are equivalent to the following loop:
                {
                        int tmpBytesToRead = extraBytesToRead+1;
                        do {
                                ch += *source++;
                                --tmpBytesToRead;
                                if (tmpBytesToRead) ch <<= 6;
                        } while (tmpBytesToRead > 0);
                }
        In UTF-8 writing code, the switches on "bytesToWrite" are
        similarly unrolled loops.

   --------------------------------------------------------------------- */

/*
 *  pdc_convert_string converts a arbitrary encoded string (maybe UTF) to
 *  another string.
 *
 *  The new converted string is allocated and terminated by required zeros.
 *  The caller is responsible for freeing the string buffer.
 *
 *
 *  LBP: low byte picking
 *
 *  Input-Parameter:
 *
 *  inutf:      input string format (see pc_unicode.h):
 *
 *              pdc_auto:     If a BOM is recognized:
 *                                pdc_utf8 or pdc_utf16xx resp.
 *                            Otherwise if input encoding <inev> is specified:
 *                                pdc_bytes
 *                            Otherwise:
 *                                pdc_utf16
 *
 *              pdc_auto2:    If input encoding is not specified:
 *                                pdc_utf16
 *                            Otherwise after successfull LBP:
 *                                pdc_auto
 *                            Otherwise
 *                                pdc_utf16
 *
 *              pdc_bytes:    8-bit string. Encoding is <inev> if specified.
 *
 *              pdc_bytes2:   After successfull LBP:
 *                                pdc_bytes
 *                            Otherwise
 *                                pdc_utf16
 *
 *              pdc_utf8:     UTF-8 formatted string.
 *
 *              pdc_utf16:    If a UTF16 BOM is recognized:
 *                                pdc_utf16be or pdc_utf16le
 *                            Otherwise UTF-16 machine byte ordered string.
 *
 *              pdc_utf16be   UTF-16 big endian formatted string.
 *
 *              pdc_utf16le   UTF-16 little endian formatted string.
 *
 *  inev:       Encoding vector for input pdc_bytes string.
 *
 *  instring:   Input string.
 *
 *  inlen:      Length of input string in byte.
 *
 *  oututf:     Target format for output string.
 *              pdc_auto, pdc_auto2 and pdc_bytes2 are not supported.
 *
 *  outev:      Encoding vector for output pdc_bytes string.
 *
 *  flags:      PDC_CONV_KEEPBYTES:
 *              Input pdc_bytes strings will be kept differing from oututf.
 *              *oututf: pdc_byte.
 *
 *              PDC_CONV_TRY7BYTES:
 *              UTF-8 output strings will have no BOM if every byte
 *              is smaller than x80.
 *              *oututf: pdc_byte.
 *
 *              PDC_CONV_TRYBYTES:
 *              UTF-UTF-16xx output strings will be converted by LBP
 *              if every character is smaller than x0100.
 *              *oututf: pdc_byte.
 *
 *              PDC_CONV_WITHBOM:
 *              UTF-8 or UTF-UTF-16xx output strings will be armed
 *              with an appropriate BOM.
 *
 *              PDC_CONV_NOBOM:
 *              In UTF-8 or UTF-UTF-16xx output strings any BOM sequence
 *              will be removed.
 *
 *  verbose:    Error messages are put out. Otherwise they are saved only.
 *
 *  Output-Parameter:
 *
 *  oututf:     Reached format for output string.
 *
 *  outstring:  Pointer of allocated output string
 *
 *  outlen:     Length of output string.
 *
 */

int
pdc_convert_string(pdc_core *pdc,
                   pdc_text_format inutf, pdc_encodingvector *inev,
                   pdc_byte *instring, int inlen,
                   pdc_text_format *oututf_p, pdc_encodingvector *outev,
                   pdc_byte **outstring, int *outlen, int flags,
                   pdc_bool verbose)
{
    static const char *fn = "pdc_convert_string";
    pdc_text_format oututf = *oututf_p;
    pdc_text_format oututf_s;
    pdc_ushort *usinstr = (pdc_ushort *) instring;
    pdc_ushort uv = 0;
    pdc_byte *instr = (pdc_byte *) instring;
    pdc_bool inalloc = pdc_false;
    pdc_bool hasbom = pdc_false;
    pdc_bool toswap = pdc_false;
    int errcode = 0;
    int i, j, len;

    /* analyzing 2 byte textformat */
    if (inutf == pdc_auto2 || inutf == pdc_bytes2)
    {
        if (inutf == pdc_auto2 && !inev)
        {
            inutf = pdc_utf16;
        }
        else
        {
            len = inlen / 2;
            if (2 * len != inlen)
            {
                errcode = PDC_E_CONV_ILLUTF16;
                goto PDC_CONV_ERROR;
            }
            for (i = 0; i < len; i++)
                if (usinstr[i] > 0x00FF)
                    break;

            /* low byte picking */
            if (i == len)
            {
                instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (len + 2), fn);
                for (i = 0; i < len; i++)
                    instr[i] = (pdc_byte) usinstr[i];
                instr[len] = 0;
                instr[len + 1] = 0;

                inalloc = pdc_true;
                instring = instr;
                inlen = len;

                if (inutf == pdc_bytes2)
                    inutf = pdc_bytes;
                else
                    inutf = pdc_auto;
            }
            else
            {
                inutf = pdc_utf16;
            }
        }
    }

    /* analyzing UTF-16 textformat */
    if (inutf == pdc_utf16)
    {
        if (pdc_is_utf16be_unicode(instring))
            inutf = pdc_utf16be;
        else if (pdc_is_utf16le_unicode(instring))
            inutf = pdc_utf16le;
    }

    /* analyzing auto textformat */
    else if (inutf == pdc_auto)
    {
        if (pdc_is_utf8_unicode(instring))
            inutf = pdc_utf8;
        else if (pdc_is_utf16be_unicode(instring))
            inutf = pdc_utf16be;
        else if (pdc_is_utf16le_unicode(instring))
            inutf = pdc_utf16le;
        else if (inev)
            inutf = pdc_bytes;
        else
            inutf = pdc_utf16;
    }

    /* conversion to UTF-16 by swapping */
    if ((inutf == pdc_utf16be  || inutf == pdc_utf16le) &&
        (inutf != oututf || flags & PDC_CONV_TRYBYTES))
    {
        if (inlen &&
            ((inutf == pdc_utf16be && !PDC_ISBIGENDIAN) ||
             (inutf == pdc_utf16le &&  PDC_ISBIGENDIAN)))
        {
            if (inalloc)
                pdc_swap_bytes((char *) instring, inlen, NULL);
            else
            {
                instr = (pdc_byte *) pdc_malloc(pdc, (size_t) inlen, fn);
                pdc_swap_bytes((char *) instring, inlen, (char *) instr);

                inalloc = pdc_true;
                instring = instr;
            }
        }
        inutf = pdc_utf16;
    }

    /* conversion to UTF-16 by inflation or encoding vector */
    if (inutf == pdc_bytes)
    {
        if ((oututf != pdc_bytes && !(flags & PDC_CONV_KEEPBYTES)) ||
            inev != NULL || outev != NULL)
        {
            len = 2 * inlen;
            instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (len + 2), fn);
            usinstr = (pdc_ushort *) instr;

            for (i = 0; i < inlen; i++)
            {
                uv = (pdc_ushort) instring[i];
                if (inev && uv)
                {
                    uv = inev->codes[uv];
                    if (!uv) uv = 0x0020;
                }
                usinstr[i] = uv;
            }

            if (inalloc)
                pdc_free(pdc, instring);

            inalloc = pdc_true;
            instring = instr;
            inlen = len;
            inutf = pdc_utf16;
        }
        else if (flags & PDC_CONV_KEEPBYTES)
        {
            oututf = pdc_bytes;
        }
    }

    /* illegal UTF-16 */
    if (inutf != pdc_bytes && inutf != pdc_utf8 && inlen % 2)
    {
        if (inalloc)
            pdc_free(pdc, instring);
        errcode = PDC_E_CONV_ILLUTF16;
        goto PDC_CONV_ERROR;
    }

    /* UTF conversion */
    oututf_s = oututf;
    if ((oututf_s == pdc_bytes && inutf == pdc_utf8) ||
         oututf_s == pdc_utf16be || oututf_s == pdc_utf16le)
        oututf_s = pdc_utf16;
    if (inutf != oututf_s && oututf_s != pdc_bytes)
    {
        len = 4 * inlen + 2;
        instr = (pdc_byte *) pdc_malloc(pdc, (size_t) len, fn);

        if (inlen)
        {
            pdc_convers_result result;
            pdc_byte *instringa, *instra, *instringe, *instre;

            instringa = instring;
            instringe = instring + inlen;
            instra = instr;
            instre = instr + len;

            if (inutf == pdc_utf8)
                result = pdc_convertUTF8toUTF16(
                             (UTF8 **) &instringa, (UTF8 *) instringe,
                             (UTF16 **) &instra, (UTF16 *) instre,
                             strictConversion);
            else
                result = pdc_convertUTF16toUTF8(
                             (UTF16 **) &instringa, (UTF16 *) instringe,
                             (UTF8 **) &instra, (UTF8 *) instre,
                             strictConversion);

            if (inalloc)
                pdc_free(pdc, instring);

            switch (result)
            {
                case targetExhausted:
                errcode = PDC_E_CONV_MEMOVERFLOW;
                break;

                case sourceExhausted:
                case sourceIllegal:
                errcode = PDC_E_CONV_ILLUTF;
                break;

                default:
                break;
            }

            if (errcode)
            {
                pdc_free(pdc, instr);
                goto PDC_CONV_ERROR;
            }

            inlen = instra - instr;
        }

        if (inlen + 2 != len)
            instr = pdc_realloc(pdc, instr, (size_t) (inlen + 2), fn);
        instr[inlen] = 0;
        instr[inlen + 1] = 0;

        inalloc = pdc_true;
        instring = instr;
        inutf = oututf_s;
    }

    if (inutf == pdc_bytes)
    {
        if (!inalloc)
        {
            instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (inlen + 2), fn);
            memcpy(instr, instring, (size_t) inlen);
            instr[inlen] = 0;
            instr[inlen + 1] = 0;

            instring = instr;
        }
    }

    /* trying to reduce UTF-16 string to bytes string */
    if (inutf == pdc_utf16 &&
        (flags & PDC_CONV_TRYBYTES || oututf == pdc_bytes))
    {
        len = inlen / 2;
        instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (len + 2), fn);
        usinstr = (pdc_ushort *) instring;

        for (i = 0; i < len; i++)
        {
            uv = usinstr[i];
            if (outev && uv)
                uv = (pdc_ushort) pdc_get_encoding_bytecode(pdc, outev, uv);
            if (uv > 0x00FF)
                break;

            instr[i] = (pdc_byte) uv;
        }

        if (i == len)
        {
            instr[len] = 0;
            instr[len + 1] = 0;

            if (inalloc)
                pdc_free(pdc, instring);

            inalloc = pdc_true;
            instring = instr;
            inlen = len;
            inutf = pdc_bytes;
        }
        else
            pdc_free(pdc, instr);
    }

    /* UTF-8 format */
    if (inutf == pdc_utf8)
    {
        hasbom = pdc_is_utf8_unicode(instring);

        if (flags & PDC_CONV_TRY7BYTES)
        {
            for (i = hasbom ? 3 : 0; i < inlen; i++)
                if (instring[i] > 0x7F)
                    break;
            if (i == inlen)
            {
                flags &= ~PDC_CONV_WITHBOM;
                flags |= PDC_CONV_NOBOM;
                inutf = pdc_bytes;
            }
        }

        if (!inalloc || flags & PDC_CONV_WITHBOM || flags & PDC_CONV_NOBOM)
        {
            i = (flags & PDC_CONV_WITHBOM && !hasbom) ? 3 : 0;
            j = (flags & PDC_CONV_NOBOM && hasbom) ? 3 : 0;

            len = inlen + i - j;
            instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (len + 1), fn);
            memcpy(&instr[i], &instring[j], (size_t) (inlen - j));
            instr[len] = 0;

            if (inalloc)
                pdc_free(pdc, instring);

            instring = instr;
            inlen = len;

            hasbom = (flags & PDC_CONV_WITHBOM);
        }

        if (hasbom)
        {
            instring[0] = PDF_BOM2;
            instring[1] = PDF_BOM3;
            instring[2] = PDF_BOM4;
        }
    }

    /* UTF-16 formats */
    if (inutf == pdc_utf16 || inutf == pdc_utf16be || inutf == pdc_utf16le)
    {
        hasbom = pdc_is_utf16be_unicode(instring) ||
                 pdc_is_utf16le_unicode(instring);

        if (!inalloc || oututf == pdc_utf16be || oututf == pdc_utf16le ||
            flags & PDC_CONV_WITHBOM || flags & PDC_CONV_NOBOM)
        {
            i = (flags & PDC_CONV_WITHBOM && !hasbom) ? 2 : 0;
            j = (flags & PDC_CONV_NOBOM && hasbom) ? 2 : 0;

            len = inlen + i - j;
            instr = (pdc_byte *) pdc_malloc(pdc, (size_t) (len + 2), fn);
            memcpy(&instr[i], &instring[j], (size_t) (inlen - j));
            instr[len] = 0;
            instr[len + 1] = 0;

            if (inalloc)
                pdc_free(pdc, instring);

            instring = instr;
            inlen = len;

            hasbom = (flags & PDC_CONV_WITHBOM);
        }

        i = hasbom ? 2 : 0;
        if (inutf == pdc_utf16)
        {
            if (oututf == pdc_utf16be)
            {
                inutf = pdc_utf16be;
                toswap = !PDC_ISBIGENDIAN;
            }
            if (oututf == pdc_utf16le)
            {
                inutf = pdc_utf16le;
                toswap = PDC_ISBIGENDIAN;
            }
            if (toswap)
                pdc_swap_bytes((char *) &instring[i], inlen - i, NULL);
        }

        if (hasbom)
        {
            if (inutf == pdc_utf16be ||
                (inutf == pdc_utf16 && PDC_ISBIGENDIAN))
            {
                instring[0] = PDF_BOM0;
                instring[1] = PDF_BOM1;
            }
            if (inutf == pdc_utf16le ||
                (inutf == pdc_utf16 && !PDC_ISBIGENDIAN))
            {
                instring[0] = PDF_BOM1;
                instring[1] = PDF_BOM0;
            }
        }
    }

    *oututf_p = inutf;
    *outlen = inlen;
    *outstring = instring;
    return 0;

    PDC_CONV_ERROR:
    *outlen = 0;
    *outstring = NULL;

    if (errcode == PDC_E_CONV_ILLUTF)
    {
        const char *stemp =
            pdc_errprintf(pdc, "%d", inutf == pdc_utf8 ? 8 : 16);
        pdc_set_errmsg(pdc, errcode, stemp, 0, 0, 0);
    }
    else
        pdc_set_errmsg(pdc, errcode, 0, 0, 0, 0);

    if (verbose)
        pdc_error(pdc, -1, 0, 0, 0, 0);

    return errcode;
}





































