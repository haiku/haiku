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

/* $Id: pc_font.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Header file for font handling
 *
 */

#ifndef PC_FONT_H
#define PC_FONT_H

#include "pc_core.h"
#include "pc_geom.h"
#include "pc_encoding.h"

/* Predefined character collections */
typedef enum {
    cc_simplified_chinese,
    cc_traditional_chinese,
    cc_japanese,
    cc_korean,
    cc_identity,
    cc_none
} pdc_charcoll;

/* Font types */
typedef enum {
    pdc_Type1,          /* Type1 fonts */
    pdc_MMType1,        /* Multiple master fonts */
    pdc_TrueType,       /* TrueType fonts for 1-byte encoding */
    pdc_CIDFontType2,   /* TrueType fonts for 2-byte encoding */
    pdc_Type1C,         /* OpenType fonts with CFF_ for 1-byte encoding */
    pdc_CIDFontType0,   /* OpenType fonts with CFF_ for 2-byte encoding */
    pdc_Type3           /* Type3 fonts */
} pdc_fonttype;

/* Font styles */
typedef enum {
    pdc_Normal,
    pdc_Bold,
    pdc_Italic,
    pdc_BoldItalic
} pdc_fontstyle;

typedef struct pdc_interwidth_s pdc_interwidth;
typedef struct pdc_core_metric_s pdc_core_metric;
typedef struct pdc_glyphwidth_s pdc_glyphwidth;
typedef struct pdc_kerningpair_s pdc_kerningpair;
typedef struct pdc_widthdata_s pdc_widthdata;
typedef struct pdc_t3glyph_s pdc_t3glyph;
typedef struct pdc_t3font_s pdc_t3font;
typedef struct pdc_font_s pdc_font;

/* Glyph width data structure */
struct pdc_glyphwidth_s
{
    pdc_ushort unicode;     /* unicode of glyph */
    short code;             /* builtin 8-bit code */
    pdc_ushort width;       /* glyph width */
};

/* Kerning pair data */
struct pdc_kerningpair_s
{
    pdc_ushort glyph1;      /* either 8-bit code or unicode of glyph 1 */
    pdc_ushort glyph2;      /* either 8-bit code or unicode of glyph 2 */
    short xamt;             /* x amount of kerning */
    short dominant;         /* = 1: kerning pair domimant */
};

struct pdc_widthdata_s
{
    pdc_ushort  startcode;  /* start unicode value of interval */
    int         width;      /* width of characters in the code */
                            /* interval  */
};

struct pdc_t3glyph_s
{
    char        *name;
    pdc_id      charproc_id;
    float       width;
};

struct pdc_t3font_s
{
    pdc_t3glyph *glyphs;        /* dynamically growing glyph table */
    int         capacity;       /* current number of slots */
    int         next_glyph;     /* next available slot */

    char        *fontname;      /* fontname */
    pdc_id      charprocs_id;   /* id of /CharProcs dict */
    pdc_id      res_id;         /* id of /Resources dict */
    pdc_bool    colorized;      /* glyphs colorized */
    pdc_matrix  matrix;         /* font matrix */
    pdc_rectangle bbox;		/* font bounding box */
};

/* The core PDFlib font structure */
struct pdc_font_s {
    pdc_bool    verbose;                /* put out warning/error messages */
    pdc_bool    verbose_open;           /* after opening font file */
    char        *name;                  /* fontname */
    char        *apiname;               /* fontname specified in API call */
    char        *ttname;                /* names[6] in the TT name table */
    char        *fontfilename;          /* name of external font file */

    pdc_bool    vertical;               /* vertical writing mode */
    pdc_bool    embedding;              /* font embedding */
    pdc_bool    kerning;                /* font kerning */
    pdc_bool    autocidfont;            /* automatic convert to CID font */
    pdc_bool    unicodemap;             /* automatic creation of Unicode CMap */
    pdc_bool    subsetting;             /* font subsetting */
    pdc_bool    autosubsetting;         /* automatic font subsetting */
    float       subsetminsize;          /* minimal size for font subsetting */
    float       subsetlimit;            /* maximal percent for font subsetting*/

    int         used_on_current_page;   /* this font is in use on current p. */
    pdc_id      obj_id;                 /* object id of this font */

    unsigned long flags;                /* font flags for font descriptor */
    pdc_fonttype  type;                 /* type of font */
    pdc_fontstyle style;                /* TT: style of font */
    pdc_bool      isstdlatin;           /* is standard latin font */
    pdc_bool      hasnomac;             /* TT: has no macroman cmap (0,1) */

    pdc_encoding  encoding;             /* PDFlib font encoding shortcut */
    pdc_charcoll  charcoll;             /* CID character collection supported */
    char          *cmapname;            /* CID CMap name */
    char          *encapiname;          /* Encoding name specified in API call*/

    float       italicAngle;            /* AFM key: ItalicAngle */
    int         isFixedPitch;           /* AFM key: IsFixedPitch */
    float       llx;                    /* AFM key: FontBBox */
    float       lly;                    /* AFM key: FontBBox */
    float       urx;                    /* AFM key: FontBBox */
    float       ury;                    /* AFM key: FontBBox */
    int         underlinePosition;      /* AFM key: UnderlinePosition */
    int         underlineThickness;     /* AFM key: UnderlineThickness */
    int         capHeight;              /* AFM key: CapHeight */
    int         xHeight;                /* AFM key: XHeight */
    int         ascender;               /* AFM key: Ascender */
    int         descender;              /* AFM key: Descender */
    int         StdVW;                  /* AFM key: StdVW */
    int         StdHW;                  /* AFM key: StdHW */
    int         monospace;              /* monospace amount */

    int                 numOfGlyphs;    /* # of Glyph ID's */
    pdc_glyphwidth      *glw;           /* ptr to glyph metrics array */
    int                 numOfPairs;     /* # of entries in pair kerning array */
    pdc_kerningpair     *pkd;           /* ptr to pair kerning array */

    int                 codeSize;       /* code size = 0 (unknown), -2,-1,1,2 */
    int                 numOfCodes;     /* # of codes defined by encoding */
    int                 lastCode;       /* last byte code */
    pdc_bool            names_tbf;      /* glyph names to be freed */
    char                **GID2Name;     /* mapping Glyph ID -> Glyph name */
    pdc_ushort          *GID2code;      /* mapping Glyph ID -> code */
                                        /* glyphid: code = unicode! */
    pdc_ushort          *code2GID;      /* mapping code -> Glyph ID */
                                        /* glyphid: code = glyphid! */
    pdc_ushort          *usedGIDs;      /* used Glyph IDs */
    int                 defWidth;       /* default width */
    int                 numOfWidths;    /* # of character width intervals */
    pdc_widthdata       *widthsTab;     /* ptr to character width intervals */
                                        /* or NULL - then consecutive */
    int                 *widths;        /* characters widths [numOfCodes] */
    char                *usedChars;     /* bit field for used characters  */
                                        /* in a document */

    char                *imgname;       /* name of virtual file contains *img */
    size_t              filelen;        /* length of uncompressed font data */
    pdc_byte            *img;           /* font (or CFF table) data in memory */
    long                cff_offset;     /* start of CFF table in font */
    size_t              cff_length;     /* length of CFF table in font */
    pdc_t3font          *t3font;        /* type 3 font data */
};

/* these defaults are used when the stem value must be derived from the name */
#define PDF_STEMV_MIN           50      /* minimum StemV value */
#define PDF_STEMV_LIGHT         71      /* light StemV value */
#define PDF_STEMV_NORMAL        109     /* normal StemV value */
#define PDF_STEMV_MEDIUM        125     /* mediumbold StemV value */
#define PDF_STEMV_SEMIBOLD      135     /* semibold StemV value */
#define PDF_STEMV_BOLD          165     /* bold StemV value */
#define PDF_STEMV_EXTRABOLD     201     /* extrabold StemV value */
#define PDF_STEMV_BLACK         241     /* black StemV value */

/* Bit positions for the font descriptor flag */
#define FIXEDWIDTH      (long) (1L<<0)
#define SERIF           (long) (1L<<1)
#define SYMBOL          (long) (1L<<2)
#define SCRIPT          (long) (1L<<3)
#define ADOBESTANDARD   (long) (1L<<5)
#define ITALIC          (long) (1L<<6)
#define SMALLCAPS       (long) (1L<<17)
#define FORCEBOLD       (long) (1L<<18)

#define PDC_DEF_ITALICANGLE    -12     /* default italic angle */

/* pc_font.c */
void pdc_init_font_struct(pdc_core *pdc, pdc_font *font);
void pdc_cleanup_font_struct(pdc_core *pdc, pdc_font *font);
void pdc_cleanup_t3font_struct(pdc_core *pdc, pdc_t3font *t3font);

/* pc_corefont.c */
const char *pdc_get_base14_name(int slot);
const pdc_core_metric *pdc_get_core_metric(const char *fontname);
void pdc_init_core_metric(pdc_core *pdc, pdc_core_metric *metric);
void pdc_cleanup_core_metric(pdc_core *pdc, pdc_core_metric *metric);
void pdc_fill_core_metric(pdc_core *pdc,
                          pdc_font *font, pdc_core_metric *metric);
void pdc_fill_font_metric(pdc_core *pdc,
                          pdc_font *font, const pdc_core_metric *metric);

#endif  /* PC_FONT_H */
