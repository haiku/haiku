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

/* $Id: p_cid.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib CID font handling routines
 *
 */

#include "p_intern.h"
#include "p_font.h"
#include "p_cid.h"

/* returns font handle,
** or -1 if no font found,
** or -2 in case of an error.
*/
int
pdf_handle_cidfont(PDF *p, const char *fontname, const char *encoding)
{
    pdc_font *font = &p->fonts[p->fonts_number];
    int slot, cmap, metric;

    /*
     * Look whether font is already in the cache.
     * If font with same name and encoding is found,
     * return its descriptor.
     */

    for (slot = 0; slot < p->fonts_number; slot++) {
        if (p->fonts[slot].encoding == pdc_cid &&
            p->fonts[slot].style == font->style &&
            !strcmp(p->fonts[slot].name, fontname) &&
            !strcmp(p->fonts[slot].cmapname, encoding))
            return slot;
    }

    /* Check the requested CMap */
    for (cmap = 0; cmap < NUMBER_OF_CMAPS; cmap++)
        if (!strcmp(cmaps[cmap].name, encoding))
            break;

    /* Unknown CMap */
    if (cmap == NUMBER_OF_CMAPS)
        return -1;

    /* Check whether this CMap is supported in the desired PDF version */
    if (cmaps[cmap].compatibility > p->compatibility)
    {
	pdc_set_errmsg(p->pdc, PDF_E_DOC_PDFVERSION,
	    encoding, pdc_errprintf(p->pdc, "%d.%d",
	    p->compatibility/10, p->compatibility % 10), 0, 0);

        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return -2;
    }

    /* Check whether the font name is among the known CID fonts */
    for (metric = 0; metric < SIZEOF_CID_METRICS; metric++) {
        if (!strcmp(pdf_cid_metrics[metric].name, fontname))
            break;
    }

    /* Unknown font */
    if (metric == SIZEOF_CID_METRICS)
    {
	pdc_set_errmsg(p->pdc, PDF_E_CJK_NOSTANDARD, fontname, 0, 0, 0);

        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return -2;
    }

    /* Selected CMap and font don't match */
    if (cmaps[cmap].charcoll != cc_identity &&
        cmaps[cmap].charcoll != pdf_cid_metrics[metric].charcoll)
    {
	pdc_set_errmsg(p->pdc, PDF_E_FONT_BADENC, fontname, encoding, 0, 0);

        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return -2;
    }

    /* For Unicode capable language wrappers only UCS2 CMaps allowed */
    if (cmaps[cmap].codesize == 0 && p->textformat == pdc_auto2)
    {
	pdc_set_errmsg(p->pdc, PDF_E_FONT_NEEDUCS2, encoding, fontname, 0, 0);

        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return -2;
    }

    p->fonts_number++;

    /* Code size of CMap */
    font->codeSize = 0;
    font->numOfCodes = 0;

    /* Fill up the font struct */
    pdc_fill_font_metric(p->pdc, font, &pdf_cid_metrics[metric]);

    /* Now everything is fine; fill the remaining entries */
    font->encoding = pdc_cid;
    font->cmapname = pdc_strdup(p->pdc, encoding);
    font->ttname = pdc_strdup(p->pdc, fontname);
    font->apiname = pdc_strdup(p->pdc, fontname);
    font->obj_id = pdc_alloc_id(p->out);
    font->embedding = pdc_false;
    font->kerning = pdc_false;
    font->subsetting = pdc_false;
    font->autocidfont = pdc_false;
    font->unicodemap = pdc_false;
    font->autosubsetting = pdc_false;

    /* return valid font descriptor */
    return slot;
}

const char *
pdf_get_ordering_cid(PDF *p, pdc_font *font)
{
    (void) p;
    return pdc_get_keyword(font->charcoll, charcoll_names); /* in ASCII */
}

int
pdf_get_supplement_cid(PDF *p, pdc_font *font)
{
    int cmap, supplement;

    for (cmap = 0; cmap < NUMBER_OF_CMAPS; cmap++)
        if (!strcmp(cmaps[cmap].name, font->cmapname))
            break;

    switch (p->compatibility)
    {
        case PDC_1_3:
        supplement = cmaps[cmap].supplement13;
        break;

        default:
        supplement = cmaps[cmap].supplement14;
        break;
    }

    return supplement;
}

#ifdef PDF_CJKFONTWIDTHS_SUPPORTED
void
pdf_put_cidglyph_widths(PDF *p, pdc_font *font)
{
    int slot;

    /* Check whether the font name is among the known CID fonts */
    for (slot = 0; slot < SIZEOF_CID_METRICS; slot++) {
        if (!strcmp(pdf_cid_metrics[slot].name, font->apiname))
            break;
    }
    pdc_printf(p->out, "/DW %d\n",
        font->monospace ? font->monospace : PDF_DEFAULT_CIDWIDTH);
    if (!font->monospace)
    {
        if (slot < SIZEOF_CID_METRICS && pdf_cid_width_arrays[slot]) {
            pdc_puts(p->out, pdf_cid_width_arrays[slot]);
        }
    }
}
#endif /* PDF_CJKFONTWIDTHS_SUPPORTED */
