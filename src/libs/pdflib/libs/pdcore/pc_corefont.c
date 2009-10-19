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

/* $Id: pc_corefont.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib in-core font and basic font metric functions
 *
 */

#include "pc_util.h"
#include "pc_corefont.h"

/* PDF basic font names */
static const char *pdc_base14_names[] =
{
  "Courier",
  "Courier-Bold",
  "Courier-Oblique",
  "Courier-BoldOblique",
  "Helvetica",
  "Helvetica-Bold",
  "Helvetica-Oblique",
  "Helvetica-BoldOblique",
  "Symbol",
  "Times-Roman",
  "Times-Bold",
  "Times-Italic",
  "Times-BoldItalic",
  "ZapfDingbats"
};

const char *
pdc_get_base14_name(int slot)
{
    if (slot < (int)(sizeof(pdc_base14_names) / sizeof(pdc_base14_names[0])))
        return(pdc_base14_names[slot]);
    else
        return(NULL);
}

#ifdef PDF_BUILTINMETRIC_SUPPORTED

/* Basic fonts core metrics */
#include "pc_coremetr.h"
static const pdc_core_metric *pdc_core_metrics[] =
{
    &pdc_core_metric_01,
    &pdc_core_metric_02,
    &pdc_core_metric_03,
    &pdc_core_metric_04,
    &pdc_core_metric_05,
    &pdc_core_metric_06,
    &pdc_core_metric_07,
    &pdc_core_metric_08,
    &pdc_core_metric_09,
    &pdc_core_metric_10,
    &pdc_core_metric_11,
    &pdc_core_metric_12,
    &pdc_core_metric_13,
    &pdc_core_metric_14
};

#endif /* PDF_BUILTINMETRIC_SUPPORTED */

const pdc_core_metric *
pdc_get_core_metric(const char *fontname)
{
#ifdef PDF_BUILTINMETRIC_SUPPORTED
    const pdc_core_metric *metric = NULL;
    int slot;

    for (slot = 0;
         slot < (int)(sizeof(pdc_core_metrics) / sizeof(pdc_core_metrics[0]));
         slot++)
    {
        metric = pdc_core_metrics[slot];
        if (!strcmp(metric->name, fontname))
            return metric;
    }
#endif /* PDF_BUILTINMETRIC_SUPPORTED */
    return(NULL);
}

void
pdc_init_core_metric(pdc_core *pdc, pdc_core_metric *metric)
{
    (void) pdc;

    metric->name                  = NULL;
    metric->flags                 = 0L;
    metric->type                  = pdc_Type1;
    metric->charcoll              = cc_none;

    metric->italicAngle           = 0;
    metric->isFixedPitch          = pdc_false;
    metric->llx                   = (float) -50;
    metric->lly                   = (float) -200;
    metric->urx                   = (float) 1000;
    metric->ury                   = (float) 900;
    metric->underlinePosition     = -100;
    metric->underlineThickness    = 50;
    metric->ascender              = 800;
    metric->descender             = -200;
    metric->capHeight             = 700;
    metric->xHeight               = 0;
    metric->StdHW                 = 0;
    metric->StdVW                 = 0;
    metric->numOfInter            = 0;
    metric->ciw                   = NULL;
    metric->numOfGlyphs           = 0;
    metric->glw                   = NULL;

}

void
pdc_cleanup_core_metric(pdc_core *pdc, pdc_core_metric *metric)
{
    if (metric == NULL)
        return;

    if (metric->name)
        pdc_free(pdc, metric->name);

    if (metric->glw != NULL) {
        pdc_free(pdc, metric->glw);
    }

}


/*
 * Fill up core metric struct from font struct
 */
void
pdc_fill_core_metric(pdc_core *pdc, pdc_font *font, pdc_core_metric *metric)
{
    static const char fn[] = "pdc_fill_core_metric";

    /* Fill up core metric struct */
    metric->name = pdc_strdup(pdc, font->name);
    metric->flags = font->flags;
    metric->type = font->type;
    metric->charcoll = font->charcoll;
    metric->italicAngle = font->italicAngle;
    metric->isFixedPitch = font->isFixedPitch;
    metric->llx = font->llx;
    metric->lly = font->lly;
    metric->urx = font->urx;
    metric->ury = font->ury;
    metric->underlinePosition = font->underlinePosition;
    metric->underlineThickness = font->underlineThickness;
    metric->capHeight = font->capHeight;
    metric->xHeight = font->xHeight;
    metric->ascender = font->ascender;
    metric->descender = font->descender;
    metric->StdVW = font->StdVW;

    /* Generate Glyph width array */
    metric->numOfGlyphs = font->numOfGlyphs;
    if (metric->numOfGlyphs)
    {
        metric->glw = (pdc_glyphwidth *) pdc_calloc(pdc,
                          font->numOfGlyphs * sizeof(pdc_glyphwidth), fn);
        memcpy(metric->glw, font->glw,
               font->numOfGlyphs * sizeof(pdc_glyphwidth));
    }

}


/*
 * Fill up font metric struct from core metric struct
 */
void
pdc_fill_font_metric(pdc_core *pdc, pdc_font *font,
                     const pdc_core_metric *metric)
{
    static const char fn[] = "pdc_fill_font_metric";

    /* Fill up font metric struct. Font struct must be initialized */
    font->name = pdc_strdup(pdc, metric->name);
    font->flags = metric->flags;
    font->type = metric->type;
    font->charcoll = metric->charcoll;
    font->italicAngle = metric->italicAngle;
    font->isFixedPitch = metric->isFixedPitch;
    font->llx = metric->llx;
    font->lly = metric->lly;
    font->urx = metric->urx;
    font->ury = metric->ury;
    font->underlinePosition = metric->underlinePosition;
    font->underlineThickness = metric->underlineThickness;
    font->capHeight = metric->capHeight;
    font->xHeight = metric->xHeight;
    font->ascender = metric->ascender;
    font->descender = metric->descender;
    font->StdVW = metric->StdVW;
    font->StdHW = metric->StdHW;

    if (metric->numOfInter && font->codeSize > 1)
    {
        int i;

        /* Fill up the code intervals for glyph widths */
        font->numOfWidths = metric->numOfInter;
        font->widthsTab = (pdc_widthdata *) pdc_calloc(pdc,
                                font->numOfWidths * sizeof(pdc_widthdata), fn);
        for (i = 0; i < font->numOfWidths; i++)
        {
            font->widthsTab[i].startcode = metric->ciw[i].startcode;
            font->widthsTab[i].width = (int) metric->ciw[i].width;
        }
    }

    /* Generate Glyph width array */
    font->numOfGlyphs = metric->numOfGlyphs;
    if (font->numOfGlyphs)
    {
        font->glw = (pdc_glyphwidth *) pdc_calloc(pdc,
                          metric->numOfGlyphs * sizeof(pdc_glyphwidth), fn);
        memcpy(font->glw, metric->glw,
               metric->numOfGlyphs * sizeof(pdc_glyphwidth));
    }

}




