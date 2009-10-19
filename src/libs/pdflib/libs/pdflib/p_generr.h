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

/* $Id: p_generr.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib error messages
 *
 */

#if	pdf_genNames
#define gen(n, num, nam, msg)	PDF_E_##nam = num,
#elif	pdf_genInfo
#define gen(n, num, nam, msg)	{ n, num, msg, (const char *) 0 },

#else
#error	invalid inclusion of generator file
#endif


/* -------------------------------------------------------------------- */
/* Configuration 					(20xx)		*/
/* -------------------------------------------------------------------- */

gen(0, 2000, UNSUPP_CRYPT, "Encryption not supported in this configuration")

gen(0, 2002, UNSUPP_KERNING, "Kerning not supported in this configuration")

gen(0, 2004, UNSUPP_SUBSET, "Subsetting not supported in this configuration")

gen(0, 2006, UNSUPP_PDFX, "PDF/X not supported in this configuration")

gen(1, 2008, UNSUPP_IMAGE, "$1 images not supported in this configuration")

gen(0, 2010, UNSUPP_ICC, "ICC profiles not supported in this configuration")

gen(0, 2012, UNSUPP_UNICODE,
    "Unicode and glyph id addressing not supported in this configuration")

gen(0, 2014, UNSUPP_SPOTCOLOR,
    "Spot colors not supported in this configuration")

gen(0, 2016, UNSUPP_PDI, "PDF import (PDI) not supported in this configuration")

gen(0, 2018, UNSUPP_BLOCK,
    "Personalization with blocks not supported in this configuration")


/* -------------------------------------------------------------------- */
/* Document, page, scoping and resource                 (21xx)          */
/* -------------------------------------------------------------------- */

gen(1, 2100, DOC_SCOPE,	"Function must not be called in '$1' scope")

gen(1, 2102, DOC_FUNCUNSUPP, "Function not supported for '$1'")

gen(2, 2104, DOC_PDFVERSION, "$1 is not supported in PDF $2")

gen(0, 2106, DOC_EMPTY,	"Generated document doesn't contain any pages")

gen(0, 2110, PAGE_SIZE_ACRO4, "Page size incompatible with Acrobat 4")

gen(2, 2112, PAGE_BADBOX, "Illegal $1 [$2]")

gen(0, 2114, PAGE_ILLCHGSIZE,
    "Page size cannot be changed in top-down coordinate system")

gen(2, 2120, RES_BADRES, "Bad resource specification '$1' for category '$2'")

gen(2, 2122, DOC_SCOPE_GET, "Can't get parameter '$1' in '$2' scope")

gen(2, 2124, DOC_SCOPE_SET, "Can't set parameter '$1' in '$2' scope")


/* -------------------------------------------------------------------- */
/* Graphics and Text					(22xx)		*/
/* -------------------------------------------------------------------- */

gen(0, 2200, GSTATE_UNMATCHEDSAVE, "Unmatched save level")

gen(0, 2202, GSTATE_RESTORE, "Invalid restore (no matching save level")

gen(1, 2204, GSTATE_SAVELEVEL, "Too many save levels (max. $1)")

/* Currently unused */
gen(0, 2210, PATTERN_SELF, "Can't use a pattern within its own definition")

gen(0, 2212, SHADING13, "Smooth shadings are not supported in PDF 1.3")

gen(1, 2220, TEMPLATE_SELF,
    "Can't place template handle $1 within its own definition")

gen(1, 2230, TEXT_UNICODENOTSHOW,
    "Can't show text character with Unicode value $1")

gen(1, 2232, TEXT_GLYPHIDNOTSHOW, "Can't show text character with glyph ID $1")

gen(1, 2234, TEXT_TOOLONG, "Text too long (max. $1)")

gen(0, 2236, TEXT_TOOMANYCODES, "Too many different unicode values (> 256)")

gen(0, 2238, TEXT_NOFONT, "No font set for text")

gen(1, 2240, TEXT_NOFONT_PAR, "No font set for parameter '$1'")



/* -------------------------------------------------------------------- */
/* Color						(23xx)		*/
/* -------------------------------------------------------------------- */

gen(0, 2300, COLOR_SPOT,
"Spot color can not be based on a Pattern, Indexed, or Separation color space")

gen(2, 2302, COLOR_BADSPOT, "Color name '$1' not found in $2 table")

gen(0, 2304, COLOR_SPOTBW, "Alternate color for spot color is black or white")

gen(1, 2306, COLOR_UNLIC_SPOTCOLOR, "$1 spot colors not licensed")



/* -------------------------------------------------------------------- */
/* Image						(24xx)		*/
/* -------------------------------------------------------------------- */

gen(2, 2400, IMAGE_CORRUPT, "Corrupt $1 image file '$2'")

gen(3, 2402, IMAGE_NOPAGE, "Requested page $1 in $2 image '$3' not found")

gen(2, 2404, IMAGE_BADDEPTH,
    "Bad number of bits per pixel ($1) in image file '$2'")

gen(1, 2406, IMAGE_BADMASK,
    "Image '$1' not suitable as mask (has more than one color component)")

gen(1, 2408, IMAGE_MASK1BIT13,
    "Image '$1' with more than 1 bit is not supported as mask in PDF 1.3")

gen(1, 2410, IMAGE_COLORMAP, "Couldn't read colormap in image '$1'")

gen(2, 2412, IMAGE_BADCOMP,
    "Bad number of color components ($1) in image '$2'")

gen(1, 2414, IMAGE_COLORIZE,
    "Can't colorize image '$1' with more than 1 component")

gen(1, 2416, IMAGE_ICC, "Couldn't handle embedded ICC profile in image '$1'")

gen(1, 2418, IMAGE_ICC2,
    "ICC profile for image file '$1' doesn't match image data")

gen(0, 2420, IMAGE_THUMB, "More than one thumbnail for this page")

gen(1, 2422, IMAGE_THUMB_MULTISTRIP,
    "Can't use multi-strip image $1 as thumbnail")

gen(1, 2424, IMAGE_THUMB_CS,
    "Unsupported color space in thumbnail image handle $1")

gen(2, 2426, IMAGE_THUMB_SIZE, "Thumbnail image $1 larger than $2 pixels")

gen(2, 2428, IMAGE_OPTUNSUPP,
    "Option '$1' for $2 images will be ignored (not supported)")

gen(2, 2430, IMAGE_OPTUNREAS, "Option '$1' for $2 images will be ignored")

gen(2, 2432, IMAGE_OPTBADMASK, "Option '$1' has bad image mask $2")

gen(1, 2434, IMAGE_UNKNOWN, "Unknown image type in file '$1'")

gen(0, 2436, IMAGE_NOADJUST,
    "Option 'adjustpage' must not be used in top-down system")

gen(2, 2440, RAW_ILLSIZE,
    "Size ($1 bytes) of raw image file '$2' doesn't match specified options")

gen(1, 2444, BMP_VERSUNSUPP,
    "Version of BMP image file '$1' not supported")

gen(1, 2446, BMP_COMPUNSUPP,
    "Compression in BMP image file '$1' not supported")

gen(2, 2450, JPEG_COMPRESSION,
    "JPEG compression scheme '$1' in file '$2' not supported in PDF")

gen(1, 2452, JPEG_MULTISCAN,
    "JPEG file '$1' contains multiple scans, which is not supported in PDF")

gen(1, 2460, GIF_LZWOVERFLOW, "LZW code size overflow in GIF file '$1'")

gen(1, 2462, GIF_LZWSIZE,
    "Color palette in GIF file '$1' with fewer than 128 colors not supported")

gen(1, 2464, GIF_INTERLACED, "Interlaced GIF image '$1' not supported")

gen(2, 2470, TIFF_UNSUPP_CS,
    "Couldn't open TIFF image '$1' (unsupported color space; photometric $2)")

gen(2, 2472, TIFF_UNSUPP_PREDICT,
    "Couldn't open TIFF image '$1' (unsupported predictor tag $2)")

gen(1, 2474, TIFF_UNSUPP_LZW_TILED,
    "Couldn't open TIFF image '$1' (tiled image with LZW compression)")

gen(1, 2476, TIFF_UNSUPP_LZW_PLANES,
    "Couldn't open TIFF image '$1' (separate planes with LZW compression)")

gen(1, 2478, TIFF_UNSUPP_LZW_ALPHA,
    "Couldn't open TIFF image '$1' (alpha channel with LZW compression)")

gen(2, 2480, TIFF_UNSUPP_JPEG,
    "Couldn't open TIFF image '$1' (JPEG compression scheme $2)")

gen(1, 2482, TIFF_UNSUPP_JPEG_TILED,
    "Couldn't open TIFF image '$1' (tiled image with JPEG compression)")

gen(1, 2484, TIFF_UNSUPP_JPEG_ALPHA,
    "Couldn't open TIFF image '$1' (alpha channel with JPEG compression)")

gen(2, 2486, TIFF_UNSUPP_SEP_NONCMYK,
    "Couldn't open TIFF image '$1' (unsupported inkset tag $2)")

gen(1, 2488, TIFF_MASK_MULTISTRIP, "Can't mask multistrip TIFF image '$1'")

gen(1, 2490, TIFF_MULTISTRIP_MASK,
    "Can't use multistrip TIFF image '$1' as mask")


/* -------------------------------------------------------------------- */
/* Font							(25xx)		*/
/* -------------------------------------------------------------------- */

gen(2, 2500, FONT_CORRUPT, "Corrupt $1 font file '$2'")

gen(2, 2502, FONT_BADENC, "Font '$1' doesn't support '$2' encoding")

gen(3, 2504, FONT_FORCEENC, "Using '$1' encoding instead of '$2' for font '$3'")

gen(2, 2505, FONT_NEEDUCS2,
    "Font '$2' requires UCS2-compatible CMap instead of '$1'")

gen(2, 2506, FONT_FORCEEMBED, "Encoding '$1' for font '$2' requires embedding")

gen(1, 2508, FONT_BADTEXTFORM,
    "Current text format not allowed for builtin encoding")

gen(1, 2510, FONT_HOSTNOTFOUND, "Host font '$1' not found")

gen(1, 2512, FONT_TTHOSTNOTFOUND, "TrueType host font '$1' not found")

gen(1, 2514, FONT_EMBEDMM, "Multiple Master font '$1' cannot be embedded")

gen(1, 2516, FONT_NOMETRICS, "Metrics data for font '$1' not found")

gen(1, 2518, FONT_NOOUTLINE,
    "No file specified with outline data for font '$1'")

gen(1, 2520, FONT_NOGLYPHID, "Font '$1' does not contain glyph IDs")

gen(1, 2530, CJK_NOSTANDARD, "Unknown standard CJK font '$1'")

gen(1, 2540, T3_BADBBOX,
    "Bounding box values must be 0 for colorized Type 3 font '$1'")

gen(2, 2542, T3_GLYPH, "Glyph '$1' already defined in Type 3 font '$2'")

gen(1, 2544, T3_FONTEXISTS, "Font '$1' already exists")

gen(3, 2550, T1_BADCHARSET,
    "Encoding (dfCharSet $1) in font/PFM file '$2' not supported")

/* Unused
gen(3, 2552, T1_PFMFONTNAME,
    "Font name mismatch in PFM file '$1' ('$2' vs. '$3')")
*/

gen(2, 2554, T1_AFMBADKEY,
    "Unknown key '$1' in AFM file '$2'")

/* Unused
gen(3, 2556, T1_AFMFONTNAME,
    "Font name mismatch in AFM file '$1' ('$2' vs. '$3')")
*/

gen(1, 2558, T1_NOFONT, "'$1' is not a PostScript Type 1 font")

gen(1, 2560, TT_BITMAP, "TrueType bitmap Font '$1' not supported")

gen(1, 2562, TT_NOFONT, "Font '$1' is not a TrueType or OpenType font")

gen(1, 2564, TT_BADCMAP, "Font '$1' contains unknown encodings (cmaps) only")

gen(1, 2566, TT_SYMBOLOS2, "Symbol font '$1' does not contain OS/2 table")

gen(1, 2568, TT_EMBED,
"Couldn't embed font '$1' due to licensing restrictions in the font file")

gen(0, 2570, TT_ASSERT1, "TrueType parser error")

gen(1, 2572, TT_ASSERT2, "TrueType parser error in font '$1'")

gen(2, 2574, TTC_NOTFOUND,
    "Font '$1' not found in TrueType Collection file '$2'")

gen(1, 2576, TT_NOGLYFDESC,
    "TrueType font '$1' does not contain any character outlines")

gen(1, 2578, TT_NONAME, "TrueType font '$1' contains only unsupported "
                        "font name records in naming table")

gen(1, 2580, OT_CHARSET, "OpenType font '$1' contains no charset data")

/* Unused
gen(3, 2582, OT_GLYPH,
    "OpenType font '$1' does not contain glyph name '$2' from encoding '$3'")
*/

gen(2, 2584, TT_GLYPHIDNOTFOUND,
    "Couldn't find glyph ID for Unicode value $1 in TrueType font '$2'")


/* -------------------------------------------------------------------- */
/* Encoding						(26xx)		*/
/* -------------------------------------------------------------------- */

gen(1, 2600, ENC_NOTFOUND, "Couldn't find encoding '$1'")

gen(1, 2602, ENC_UNSUPP, "Code page '$1' not supported")

/* Unused
gen(2, 2604, ENC_ADAPT, "Encoding '$1' was adapted to font '$2'")
*/

gen(1, 2606, ENC_CANTQUERY, "Can't query encoding '$1'")

gen(1, 2608, ENC_CANTCHANGE, "Can't change encoding '$1'")

gen(1, 2610, ENC_INUSE,
    "Encoding '$1' can't be changed since it has already been used")

gen(2, 2612, ENC_TOOLONG, "Encoding name '$1' too long (max. $2)")

gen(2, 2614, ENC_BADLINE, "Syntax error in encoding file '$1' (line '$2')")

gen(0, 2616, ENC_GLYPHORCODE, "Glyph name or Unicode value required")

gen(3, 2618, ENC_BADGLYPH,
    "Glyph name '$1' for Unicode value $2 differs from AGL name '$3'")

gen(3, 2620, ENC_BADUNICODE,
    "Unicode value $1 for glyph name '$2' differs from AGL value $3")

gen(2, 2622, ENC_BADFONT,
    "Current font $1 wasn't specified with encoding '$2'")




/* -------------------------------------------------------------------- */
/* Hypertext 						(28xx)		*/
/* -------------------------------------------------------------------- */

gen(2, 2802, HYP_OPTIGNORE_FORTYPE,
    "Option '$1' for destination type '$2' will be ignored")

gen(1, 2804, HYP_OPTIGNORE_FORELEM,
    "Option '$1' for hypertext function will be ignored")


/* -------------------------------------------------------------------- */
/* Internal 						(29xx)		*/
/* -------------------------------------------------------------------- */

gen(1, 2900, INT_BADSCOPE, "Bad scope '$1'")

gen(1, 2902, INT_BADANNOT, "Bad annotation type '$1'")

gen(1, 2904, INT_BADCS, "Unknown color space $1")

gen(1, 2906, INT_BADALTERNATE, "Bad alternate color space $1")

gen(1, 2908, INT_BADPROFILE, "Unknown number of profile components ($1)")

gen(1, 2910, INT_SSTACK_OVER, "State stack overflow in function '$1'")

gen(1, 2912, INT_SSTACK_UNDER, "State stack underflow in function '$1'")

gen(3, 2914, INT_WRAPPER, "Error in PDFlib $1 wrapper function $2 ($3)")


#undef	gen
#undef	pdf_genNames
#undef	pdf_genInfo
