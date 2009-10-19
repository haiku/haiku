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

/* $Id: pc_font.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Basic font functions
 *
 */

#include "pc_util.h"
#include "pc_font.h"

void
pdc_init_font_struct(pdc_core *pdc, pdc_font *font)
{
    /*
     * Fill in some reasonable default values in global font info in
     * case they're missing from the metrics data.
     */

    (void) pdc;

    memset(font, 0, sizeof(pdc_font));

    font->verbose               = pdc_true;
    font->verbose_open          = pdc_true;
    font->obj_id                = PDC_BAD_ID;
    font->type                  = pdc_Type1;
    font->style                 = pdc_Normal;
    font->encoding              = pdc_builtin;
    font->charcoll              = cc_none;

    font->italicAngle           = 0;
    font->isFixedPitch          = pdc_false;
    font->llx                   = (float) -50;
    font->lly                   = (float) -200;
    font->urx                   = (float) 1000;
    font->ury                   = (float) 900;
    font->underlinePosition     = -100;
    font->underlineThickness    = 50;
    font->capHeight             = 700;
    font->xHeight               = 0;
    font->ascender              = 800;
    font->descender             = -200;
    font->StdVW                 = 0;
    font->StdHW                 = 0;
    font->monospace             = 0;

    font->codeSize              = 1;
    font->numOfCodes            = 256;
    font->lastCode              = -1;
}

void
pdc_cleanup_font_struct(pdc_core *pdc, pdc_font *font)
{
    int i;

    if (font == NULL)
        return;

    if (font->img != NULL && font->imgname == NULL)
        pdc_free(pdc, font->img);

    if (font->imgname != NULL)
        pdc_free(pdc, font->imgname);

    if (font->name != NULL)
        pdc_free(pdc, font->name);

    if (font->apiname != NULL)
        pdc_free(pdc, font->apiname);

    if (font->ttname != NULL)
        pdc_free(pdc, font->ttname);

    if (font->fontfilename != NULL)
        pdc_free(pdc, font->fontfilename);

    if (font->cmapname != NULL) {
        pdc_free(pdc, font->cmapname);
    }

    if (font->encapiname != NULL) {
        pdc_free(pdc, font->encapiname);
    }

    if (font->glw != NULL) {
        pdc_free(pdc, font->glw);
    }

    if (font->pkd != NULL) {
        pdc_free(pdc, font->pkd);
    }

    if (font->GID2Name != NULL) {
        if (font->names_tbf) {
            for (i = 0; i < font->numOfGlyphs; i++) {
                if (font->GID2Name[i] != NULL)
                    pdc_free(pdc, font->GID2Name[i]);
            }
        }
        pdc_free(pdc, font->GID2Name);
    }

    if (font->GID2code != NULL) {
        pdc_free(pdc, font->GID2code);
    }

    if (font->code2GID != NULL) {
        pdc_free(pdc, font->code2GID);
    }

    if (font->usedGIDs != NULL) {
        pdc_free(pdc, font->usedGIDs);
    }

    if (font->widthsTab != NULL) {
        pdc_free(pdc, font->widthsTab);
    }

    if (font->widths != NULL) {
        pdc_free(pdc, font->widths);
    }

    if (font->usedChars != NULL) {
        pdc_free(pdc, font->usedChars);
    }

    if (font->t3font != NULL) {
        pdc_cleanup_t3font_struct(pdc, font->t3font);
        pdc_free(pdc, font->t3font);
    }
}

void
pdc_cleanup_t3font_struct(pdc_core *pdc, pdc_t3font *t3font)
{
    int i;

    if (t3font->fontname != NULL)
        pdc_free(pdc, t3font->fontname);

    for (i = 0; i < t3font->next_glyph; i++) {
        if (t3font->glyphs[i].name)
            pdc_free(pdc, t3font->glyphs[i].name);
    }

    pdc_free(pdc, t3font->glyphs);
}
