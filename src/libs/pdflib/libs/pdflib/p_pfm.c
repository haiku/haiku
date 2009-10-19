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

/* $Id: p_pfm.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines for fast reading of PFM font metrics files
 *
 */

#include "p_intern.h"
#include "p_font.h"

/* read data types from the PFM */
#define PFM_BYTE(offset)  pfm[offset]
#define PFM_WORD(offset)  PDC_GET_WORD(&pfm[offset])
#define PFM_SHORT(offset) PDC_GET_SHORT(&pfm[offset])
#define PFM_DWORD(offset) PDC_GET_DWORD3(&pfm[offset])

/* Offsets in the buffer containing the various PFM structures */
#define header_base		0
#define header_dfVersion	(PFM_WORD(header_base + 0))
#define header_dfSize		(PFM_DWORD(header_base + 2))
#define header_dfAscent		(PFM_WORD(header_base + 74))
#define header_dfItalic		(PFM_BYTE(header_base + 80))
#define header_dfWeight		(PFM_WORD(header_base + 83))
#define header_dfCharSet	(PFM_BYTE(header_base + 85))
#define header_dfPitchAndFamily	(PFM_BYTE(header_base + 90))
#define header_dfMaxWidth	(PFM_WORD(header_base + 93))
#define header_dfFirstChar	(PFM_BYTE(header_base + 95))
#define header_dfLastChar	(PFM_BYTE(header_base + 96))
#define header_dfDefaultChar	(PFM_BYTE(header_base + 97))

#define ext_base		117
#define ext_dfExtentTable	(PFM_DWORD(ext_base + 6))
#define ext_dfKernPairs		(PFM_DWORD(ext_base + 14))
#define ext_dfKernTrack		(PFM_DWORD(ext_base + 18))
#define ext_dfDriverInfo	(PFM_DWORD(ext_base + 22))

#define etm_base		147
#define etmCapHeight		(PFM_SHORT(etm_base + 14))
#define etmXHeight		(PFM_SHORT(etm_base + 16))
#define etmLowerCaseAscent	(PFM_SHORT(etm_base + 18))
#define etmLowerCaseDescent	(PFM_SHORT(etm_base + 20))
#define etmSlant		(PFM_SHORT(etm_base + 22))
#define etmUnderlineOffset	(PFM_SHORT(etm_base + 32))
#define etmUnderlineWidth	(PFM_SHORT(etm_base + 34))

#define dfDevice		199

/* Windows font descriptor flags */
#define PDF_FIXED_PITCH         0x01  /* Fixed width font; rarely used flag */

#define PDF_DONTCARE            0x00  /* Don't care or don't know. */
#define PDF_ROMAN               0x10  /* Variable stroke width, serifed */
#define PDF_SWISS               0x20  /* Variable stroke width, sans-serifed */
#define PDF_MODERN              0x30  /* fixed pitch */
#define PDF_SCRIPT              0x40  /* Cursive, etc. */
#define PDF_DECORATIVE          0x50  /* Old English, etc. */

/* Windows character set flags */
#define PFM_ANSI_CHARSET        0       /* seems to imply codepage 1250/1252 */
#define PFM_SYMBOL_CHARSET      2
#define PFM_SHIFTJIS_CHARSET    128
#define PFM_OEM_CHARSET         255

#define PDF_STRING_PostScript	\
	((const char*) "\120\157\163\164\123\143\162\151\160\164")

/*
 *  Kerning pairs
 */
typedef struct kern_ {
    pdc_byte   first;           /* First character */
    pdc_byte   second;          /* Second character */
    pdc_byte   kern[2];         /* Kern distance */
} KERN;

pdc_bool
pdf_check_pfm_encoding(PDF *p, pdc_font *font, const char *fontname,
                       pdc_encoding enc)
{
    const char *encname;
    pdc_encoding enc_old = enc;
    int islatin1 = 0;

    /* Font encoding */
    switch (font->encoding)
    {
        case PFM_ANSI_CHARSET:
            font->encoding = pdc_winansi;
            font->isstdlatin = pdc_true;
            break;

        case PFM_SYMBOL_CHARSET:
            font->encoding = pdc_builtin;
            font->isstdlatin = pdc_false;
            break;

        default:
            pdc_set_errmsg(p->pdc, PDF_E_T1_BADCHARSET,
                pdc_errprintf(p->pdc, "%d", font->encoding), fontname, 0, 0);

            if (font->verbose == pdc_true)
                pdc_error(p->pdc, -1, 0, 0, 0, 0);

            return pdc_false;
    }

    /* iso8859-1 */
    if (enc > 0)
    {
        islatin1 = !strcmp(pdf_get_encoding_name(p, enc), "iso8859-1");
        if (font->encoding == pdc_winansi)
            font->encoding = enc;
    }

    /* Unicode encoding */
    if (enc == pdc_unicode)
    {
        enc = pdf_find_encoding(p, "iso8859-1");
        font->encoding = enc;
        islatin1 = 1;
    }

    /* Unallowed encoding */
    encname = pdc_errprintf(p->pdc, "%s", pdf_get_encoding_name(p, enc_old));
    if (enc < pdc_builtin)
    {
        pdc_set_errmsg(p->pdc, PDF_E_FONT_BADENC, fontname, encname, 0, 0);

        if (font->verbose == pdc_true)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return pdc_false;
    }

    if (font->verbose == pdc_true)
    {
        if (enc != pdc_winansi && !islatin1 &&
            font->encoding != pdc_builtin)
        {
            pdc_warning(p->pdc, PDF_E_FONT_FORCEENC,
                pdf_get_encoding_name(p, pdc_winansi), encname, fontname, 0);
        }
        if (enc != pdc_builtin && font->encoding == pdc_builtin)
        {
            pdc_warning(p->pdc, PDF_E_FONT_FORCEENC,
                pdf_get_encoding_name(p, pdc_builtin), encname, fontname, 0);
        }
    }

    return pdc_true;
}


/*
 * Currently we do not populate the following fields correctly:
 * - serif flag
 */

static pdc_bool
pdf_parse_pfm(PDF *p, pdc_file *fp, pdc_font *font)
{
    static const char fn[] = "pdf_parse_pfm";
    size_t length;
    pdc_byte *pfm;
    pdc_bool ismem;
    int i, dfFirstChar, dfLastChar, default_width;
    float w;
    unsigned long dfExtentTable;

    /* read whole file and close it */
    pfm = (pdc_byte *) pdc_freadall(fp, &length, &ismem);
    pdc_fclose(fp);

    /* check whether this is really a valid PostScript PFM file */
    if (pfm == NULL ||
	(header_dfVersion != 0x100 && header_dfVersion != 0x200) ||
	dfDevice > length ||
	strncmp((const char *) pfm + dfDevice, PDF_STRING_PostScript, 10) ||
	ext_dfDriverInfo > length) {

	if (!ismem)
	    pdc_free(p->pdc, pfm);
        return pdc_false;
    }

    /* fetch relevant data from the PFM */
    font->type  = pdc_Type1;
    w		= header_dfWeight / (float) 65;
    font->StdVW = (int) (PDF_STEMV_MIN + w * w);

    font->name = pdc_strdup(p->pdc, (const char *)pfm + ext_dfDriverInfo);

    switch (header_dfPitchAndFamily & 0xF0) {
	case PDF_ROMAN:
	    font->flags |= SERIF;
	    break;
	case PDF_MODERN:
	    /* Has to be ignored, contrary to MS's specs */
	    break;
	case PDF_SCRIPT:
	    font->flags |= SCRIPT;
	    break;
	case PDF_DECORATIVE:
	    /* the dfCharSet flag lies in this case... */
	    header_dfCharSet = PFM_SYMBOL_CHARSET;
	    break;
	case PDF_SWISS:
	case PDF_DONTCARE:
	default:
	    break;
    }

    /* temporarily */
    font->encoding = (pdc_encoding) header_dfCharSet;


    dfFirstChar = header_dfFirstChar;
    dfLastChar  = header_dfLastChar;
    dfExtentTable = ext_dfExtentTable;

    /*
     * Some rare PFMs do not contain any ExtentTable if the fixed pitch flag
     * is set. Use the dfMaxWidth entry for all glyphs in this case.
     * If the user forced the font to be monospaced we use this value instead.
     */
    if ((!(header_dfPitchAndFamily & PDF_FIXED_PITCH) && dfExtentTable == 0) ||
	font->monospace)
    {
	font->isFixedPitch = pdc_true;
	default_width = font->monospace ? font->monospace : header_dfMaxWidth;
    } else {

	/* default values -- don't take the width of the default character */
	default_width = PDF_DEFAULT_WIDTH;
    }

    font->widths = (int *) pdc_calloc(p->pdc,
                                 font->numOfCodes * sizeof(int), fn);
    for (i = 0; i < font->numOfCodes; i++)
	font->widths[i] = default_width;

    if (!font->isFixedPitch)
    {
	if (ext_dfExtentTable == 0 ||
	    ext_dfExtentTable + 2 * (header_dfLastChar-header_dfFirstChar) + 1 >
		length){
	    if (!ismem)
		pdc_free(p->pdc, pfm);
	    return pdc_false;
	}

	for (i = dfFirstChar; i <= dfLastChar; i++)
	    font->widths[i] =
                          (int) PFM_WORD(dfExtentTable + 2 * (i - dfFirstChar));
	/*
	 * Check whether the font is actually monospaced
	 * (the fixed pitch flag is not necessarily set)
	 */
	default_width = font->widths[dfFirstChar];

	for (i = dfFirstChar+1; i <= dfLastChar; i++)
	    if (default_width != font->widths[i])
		break;

	if (i == dfLastChar + 1)
	    font->isFixedPitch = pdc_true;
    }

    font->italicAngle		=
    	(header_dfItalic ? etmSlant/((float) 10.0) : (float) 0.0);
    font->capHeight		= etmCapHeight;
    font->xHeight		= etmXHeight;
    font->descender		= -etmLowerCaseDescent;
    if (font->descender > 0)
	font->descender = 0;
    font->ascender		= (int) header_dfAscent;

    font->underlinePosition	= -etmUnderlineOffset;
    font->underlineThickness	= etmUnderlineWidth;

    font->llx			= (float) -200;
    font->lly			= (float) font->descender;
    font->urx			= (float) header_dfMaxWidth;
    font->ury			= (float) font->ascender;

    /* try to fix broken entries */
    if (font->lly > font->ury)
        font->ury = font->lly + 200;
    if (font->llx > font->urx)
        font->urx = 1000;


    if (!ismem)
        pdc_free(p->pdc, pfm);

    return pdc_true;
}

pdc_bool
pdf_get_metrics_pfm(
    PDF *p,
    pdc_font *font,
    const char *fontname,
    pdc_encoding enc,
    const char *filename)
{
    pdc_file *pfmfile;

    /* open PFM file */
    pfmfile = pdf_fopen(p, filename, "PFM ", PDC_FILE_BINARY);
    if (pfmfile == NULL)
    {
	if (font->verbose_open)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return pdc_false;
    }

    /* Read PFM metrics */
    if (!pdf_parse_pfm(p, pfmfile, font))
    {
	pdc_set_errmsg(p->pdc, PDF_E_FONT_CORRUPT, "PFM", filename, 0, 0);

        if (font->verbose == pdc_true)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return pdc_false;
    }

    /* Check encoding */
    if (!pdf_check_pfm_encoding(p, font, fontname, enc))
        return pdc_false;

    if (!pdf_make_fontflag(p, font))
        return pdc_false;

    return pdc_true;
}
