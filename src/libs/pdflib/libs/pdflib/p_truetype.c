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

/* $Id: p_truetype.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib TrueType handling routines
 *
 */

#include "p_intern.h"
#include "p_font.h"
#include "p_truetype.h"

#ifdef PDF_TRUETYPE_SUPPORTED


void
tt_assert(PDF *p, tt_file *ttf)
{
    const char *filename = ttf->filename;
    const char *fontname = ttf->fontname;

    pdf_cleanup_tt(p, ttf); /* this will not free ttf->filename/fontname */

    if (filename)
        pdc_error(p->pdc, PDF_E_TT_ASSERT1, filename, 0, 0, 0);
    else if (fontname)
        pdc_error(p->pdc, PDF_E_TT_ASSERT1, fontname, 0, 0, 0);
    else
	pdc_error(p->pdc, PDF_E_TT_ASSERT2, 0, 0, 0, 0);
} /* tt_assert */

void
tt_error(PDF *p, tt_file *ttf)
{
    const char *filename = ttf->filename;
    const char *fontname = ttf->fontname;
    const char *qualifier = "TrueType";

    pdf_cleanup_tt(p, ttf); /* this will not free ttf->filename/fontname */

    if (filename)
        pdc_error(p->pdc, PDF_E_FONT_CORRUPT, qualifier, filename, 0, 0);
    else if (fontname)
        pdc_error(p->pdc, PDF_E_FONT_CORRUPT, qualifier, fontname, 0, 0);
    else
        pdc_error(p->pdc, PDF_E_FONT_CORRUPT, qualifier, "?", 0, 0);

} /* tt_error */

void
tt_seek(PDF *p, tt_file *ttf, long offset)
{
    if (ttf->incore)
    {
        TT_IOCHECK(p, ttf, ttf->img + (tt_ulong) offset <= ttf->end);
        ttf->pos = ttf->img + (tt_ulong) offset;
    }
    else
        TT_IOCHECK(p, ttf, pdc_fseek(ttf->fp, offset, SEEK_SET) == 0);
}

void
tt_read(PDF *p, tt_file *ttf, void *buf, unsigned int nbytes)
{
    if (ttf->incore)
    {
        TT_IOCHECK(p, ttf, ttf->pos + (tt_ulong) nbytes <= ttf->end);
        memcpy(buf, ttf->pos, (size_t) nbytes);
        ttf->pos += (tt_ulong) nbytes;
    }
    else
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, nbytes));
}

long
tt_tell(PDF *p, tt_file *ttf)
{
    (void) p;
    if (ttf->incore)
        return (long) (ttf->pos - ttf->img);
    else
        return pdc_ftell(ttf->fp);
}

tt_ushort
tt_get_ushort(PDF *p, tt_file *ttf)
{
    tt_byte *pos, buf[2];

    if (ttf->incore)
    {
        pos = ttf->pos;
        TT_IOCHECK(p, ttf, (ttf->pos += 2) <= ttf->end);
    }
    else
    {
        pos = buf;
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, 2));
    }

    return pdc_get_be_ushort(pos);
}

tt_short
tt_get_short(PDF *p, tt_file *ttf)
{
    tt_byte *pos, buf[2];

    if (ttf->incore)
    {
        pos = ttf->pos;
        TT_IOCHECK(p, ttf, (ttf->pos += 2) <= ttf->end);
    }
    else
    {
        pos = buf;
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, 2));
    }

    return pdc_get_be_short(pos);
}

tt_ulong
tt_get_ulong3(PDF *p, tt_file *ttf)
{
    tt_byte *pos, buf[3];

    if (ttf->incore)
    {
        pos = ttf->pos;
        TT_IOCHECK(p, ttf, (ttf->pos += 3) <= ttf->end);
    }
    else
    {
        pos = buf;
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, 3));
    }

    return pdc_get_be_ulong3(pos);
}

tt_ulong
tt_get_ulong(PDF *p, tt_file *ttf)
{
    tt_byte *pos, buf[4];

    if (ttf->incore)
    {
        pos = ttf->pos;
        TT_IOCHECK(p, ttf, (ttf->pos += 4) <= ttf->end);
    }
    else
    {
        pos = buf;
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, 4));
    }

    return pdc_get_be_ulong(pos);
}

tt_long
tt_get_long(PDF *p, tt_file *ttf)
{
    tt_byte *pos, buf[4];

    if (ttf->incore)
    {
        pos = ttf->pos;
        TT_IOCHECK(p, ttf, (ttf->pos += 4) <= ttf->end);
    }
    else
    {
        pos = buf;
        TT_IOCHECK(p, ttf, PDC_OK_FREAD(ttf->fp, buf, 4));
    }

    return pdc_get_be_long(pos);
}

tt_ulong
tt_get_offset(PDF *p, tt_file *ttf, tt_byte offsize)
{
    tt_byte buf;

    switch (offsize)
    {
        case 1:
        tt_read(p, ttf, &buf, 1);
        return (tt_ulong) buf;

        case 2:
        return (tt_ulong) tt_get_ushort(p, ttf);

        case 3:
        return (tt_ulong) tt_get_ulong3(p, ttf);

        case 4:
        return (tt_ulong) tt_get_ulong(p, ttf);
    }
    return 0;
}

static void
tt_get_dirent(PDF *p, tt_dirent *dirent, tt_file *ttf)
{
    tt_read(p, ttf, dirent->tag, 4);
    dirent->tag[4] = 0;
    dirent->checksum = tt_get_ulong(p, ttf);
    dirent->offset = tt_get_ulong(p, ttf);
    dirent->length = tt_get_ulong(p, ttf);
} /* tt_get_dirent */


int
tt_tag2idx(tt_file *ttf, char *tag)
{
    int i;

    for (i = 0; i < ttf->n_tables; ++i)
	if (strcmp(ttf->dir[i].tag, tag) == 0)
	    return i;

    return -1;
} /* tt_tag2idx */

static void
tt_get_cmap0(PDF *p, tt_file *ttf, tt_cmap0_6 *cm0_6)
{
    static const char *fn = "tt_get_cmap0";
    tt_ushort c;
    tt_byte buf[256];

    cm0_6->length	= tt_get_ushort(p, ttf);
    cm0_6->language	= tt_get_ushort(p, ttf);
    cm0_6->glyphIdArray = (tt_ushort *) 0;

    /* These are not used in format 0 */
    cm0_6->firstCode	= 0;
    cm0_6->entryCount	= 256;

    cm0_6->glyphIdArray = (tt_ushort *)
	pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * 256), fn);

    tt_read(p, ttf, buf, 256);

    for (c = 0; c < 256; c++)
	cm0_6->glyphIdArray[c] = (tt_ushort) buf[c];

} /* tt_get_cmap0 */

static void
tt_get_cmap6(PDF *p, tt_file *ttf, tt_cmap0_6 *cm0_6)
{
    static const char *fn = "tt_get_cmap6";
    tt_ushort c, last;

    cm0_6->length	= tt_get_ushort(p, ttf);
    cm0_6->language	= tt_get_ushort(p, ttf);
    cm0_6->firstCode	= tt_get_ushort(p, ttf);
    cm0_6->entryCount	= tt_get_ushort(p, ttf);
    cm0_6->glyphIdArray = (tt_ushort *) 0;

    last = (tt_ushort) (cm0_6->firstCode + cm0_6->entryCount);

    cm0_6->glyphIdArray = (tt_ushort *)
	pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * last), fn);

    /* default for codes outside the range specified in this table */
    for (c = 0; c < last; c++)
	cm0_6->glyphIdArray[c] = (tt_ushort) 0;

    for (c = cm0_6->firstCode; c < last; c++)
	cm0_6->glyphIdArray[c] = tt_get_ushort(p, ttf);

} /* tt_get_cmap6 */

static void
tt_get_cmap4(PDF *p, tt_file *ttf, tt_cmap4 *cm4)
{
    static const char *fn = "tt_get_cmap4";
    int		i, segs;

    /* the instruction order is critical for cleanup after exceptions!
    */
    cm4->endCount	= (tt_ushort *) 0;
    cm4->startCount	= (tt_ushort *) 0;
    cm4->idDelta	= (tt_short *)  0;
    cm4->idRangeOffs	= (tt_ushort *) 0;
    cm4->glyphIdArray	= (tt_ushort *) 0;

    cm4->format		= tt_get_ushort(p, ttf);
    TT_IOCHECK(p, ttf, cm4->format == 4);
    cm4->length		= tt_get_ushort(p, ttf);
    cm4->version	= tt_get_ushort(p, ttf);
    cm4->segCountX2	= tt_get_ushort(p, ttf);
    cm4->searchRange	= tt_get_ushort(p, ttf);
    cm4->entrySelector	= tt_get_ushort(p, ttf);
    cm4->rangeShift	= tt_get_ushort(p, ttf);

    segs = cm4->segCountX2 / 2;

    cm4->numGlyphIds  = (tt_ushort)(
        ((cm4->length - ( 16L + 8L * segs )) & 0xFFFFU) / 2);

    TT_IOCHECK(p, ttf, 0 <= cm4->numGlyphIds);

    cm4->endCount =
	(tt_ushort *)
	    pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * segs), fn);
    cm4->startCount =
	(tt_ushort *)
	    pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * segs), fn);
    cm4->idDelta =
	(tt_short *)
	    pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * segs), fn);
    cm4->idRangeOffs =
	(tt_ushort *)
	    pdc_malloc(p->pdc, (size_t) (sizeof (tt_ushort) * segs), fn);

    if (cm4->numGlyphIds)
    {
        cm4->glyphIdArray = (tt_ushort *)
            pdc_malloc(p->pdc,
                (size_t) (sizeof (tt_ushort) * cm4->numGlyphIds), fn);
    }

    for (i = 0; i < segs; ++i)
	cm4->endCount[i] = tt_get_ushort(p, ttf);

    TT_IOCHECK(p, ttf, cm4->endCount[segs - 1] == 0xFFFF);

    (void) tt_get_ushort(p, ttf);	/* padding */
    for (i = 0; i < segs; ++i)	cm4->startCount[i] = tt_get_ushort(p, ttf);
    for (i = 0; i < segs; ++i)	cm4->idDelta[i] = tt_get_short(p, ttf);
    for (i = 0; i < segs; ++i)	cm4->idRangeOffs[i] = tt_get_ushort(p, ttf);

    for (i = 0; i < cm4->numGlyphIds; ++i)
	cm4->glyphIdArray[i] = tt_get_ushort(p, ttf);

    /* empty cmap */
    if (segs == 1 && cm4->endCount[0] == cm4->startCount[0])
    {
        cm4->segCountX2 = 0;
        pdc_free(p->pdc, cm4->endCount);
        cm4->endCount = (tt_ushort *) 0;
        pdc_free(p->pdc, cm4->startCount);
        cm4->startCount = (tt_ushort *) 0;
        pdc_free(p->pdc, cm4->idDelta);
        cm4->idDelta = (tt_short *)  0;
        pdc_free(p->pdc, cm4->idRangeOffs);
        cm4->idRangeOffs = (tt_ushort *) 0;
        if (cm4->numGlyphIds)
        {
            pdc_free(p->pdc, cm4->glyphIdArray);
            cm4->glyphIdArray = (tt_ushort *) 0;
        }
    }

} /* tt_get_cmap4 */

/* convert unicode to glyph ID using cmap4.  */
static int
tt_unicode2gidx4(PDF *p, tt_file *ttf, unsigned int uv)
{
    tt_cmap4 *  cm4;
    int         segs;
    int         i;
    int         gidx = 0;

    TT_ASSERT(p, ttf, ttf->tab_cmap != (tt_tab_cmap *) 0);
    TT_ASSERT(p, ttf, ttf->tab_cmap->win != (tt_cmap4 *) 0);

    cm4 = ttf->tab_cmap->win;
    segs = cm4->segCountX2 / 2;

    for (i = 0; i < segs; ++i)
        if (uv <= cm4->endCount[i])
            break;

    TT_IOCHECK(p, ttf, i != segs);
    if (uv < cm4->startCount[i] || uv == 0xFFFF)
        return 0;

    if (cm4->idRangeOffs[i] == 0)
        gidx = (int)((uv + cm4->idDelta[i]) & 0xFFFF);
    else
    {
        int idx = (int) cm4->idRangeOffs[i] / 2
                       + (int) (uv - cm4->startCount[i]) - (segs - i);

        if (idx < 0 || idx >= cm4->numGlyphIds)
        {
            if (ttf->font->verbose)
                pdc_warning(p->pdc, PDF_E_TT_GLYPHIDNOTFOUND,
                            pdc_errprintf(p->pdc, "x%04X", uv),
                            ttf->fontname, 0, 0);
            return 0;
        }

        if (cm4->glyphIdArray[idx] == 0)
            return 0;
        else
            gidx = (int)((cm4->glyphIdArray[idx] + cm4->idDelta[i]) & 0xFFFF);
    }

    /* this is necessary because of some Mac fonts (e.g. Hiragino) */
    return (gidx < ttf->tab_maxp->numGlyphs) ? gidx : 0;

} /* tt_unicode2gidx4 */

/* convert 8-bit code to glyph ID */
static int
tt_code2gidx(PDF *p, tt_file *ttf, pdc_font *font, int code)
{
    pdc_encoding enc = font->encoding;
    pdc_ushort uv = 0;
    int gidx = 0;


    if (enc == pdc_macroman && ttf->tab_cmap->mac)
    {
        if (code > -1 && code < (int) (ttf->tab_cmap->mac->firstCode +
                                       ttf->tab_cmap->mac->entryCount))
            gidx = ttf->tab_cmap->mac->glyphIdArray[code];
    }
    else if (font->isstdlatin == pdc_false)
    {
        /* This would be an Apple font with an encoding different
         * from macroman.
         */
        if (!ttf->tab_OS_2)
        {
            const char *fontname = ttf->fontname;

            pdf_cleanup_tt(p, ttf);
            pdc_set_errmsg(p->pdc, PDF_E_TT_SYMBOLOS2, fontname, 0, 0, 0);

            if (font->verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

	    return -1;
        }

        /* e.g. WingDings has usFirstChar = 0xF020, but we must start
        ** at 0xF000. I haven't seen non-symbol fonts with a usFirstChar
        ** like that; perhaps we have to apply similar tricks then...
        */
        uv = (pdc_ushort) (code +
                 (ttf->tab_OS_2->usFirstCharIndex & 0xFF00));
        gidx = tt_unicode2gidx4(p, ttf, (unsigned int) uv);
    }
    else
    {
        pdc_encodingvector *ev = p->encodings[enc].ev;
        uv = ev->codes[code];
        if (uv)
        {
                gidx = tt_unicode2gidx4(p, ttf, (unsigned int) uv);
        }
    }

    return gidx;

} /* tt_code2gidx */

void
tt_get_tab_cmap(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_cmap";
    tt_tab_cmap *tp = (tt_tab_cmap *)
			pdc_malloc(p->pdc, sizeof (tt_tab_cmap), fn);
    int		idx = tt_tag2idx(ttf, pdf_str_cmap);
    int		i;
    tt_ulong	offset;
    tt_ushort	numEncTabs;
    tt_ushort	tableFormat;

    /* the instruction order is critical for cleanup after exceptions!
    */
    tp->win = (tt_cmap4 *) 0;
    tp->mac = (tt_cmap0_6 *) 0;
    ttf->tab_cmap = tp;

    TT_IOCHECK(p, ttf, idx != -1);
    offset = ttf->dir[idx].offset;
    tt_seek(p, ttf, (long) offset);

    (void) tt_get_ushort(p, ttf);	/* version */
    numEncTabs = tt_get_ushort(p, ttf);

    for (i = 0; i < numEncTabs; ++i)
    {
	tt_ushort platformID = tt_get_ushort(p, ttf);
	tt_ushort encodingID = tt_get_ushort(p, ttf);
	tt_ulong offsetEncTab = tt_get_ulong(p, ttf);
	tt_long pos = tt_tell(p, ttf);

	tt_seek(p, ttf, (long) (offset + offsetEncTab));

        if (platformID == tt_pfid_mac && encodingID == tt_wenc_symbol)
	{
	    tableFormat = tt_get_ushort(p, ttf);

	    /* we currently do not support cmaps
	    ** other than format 0 and 6 for Macintosh cmaps.
	    */

	    if (tableFormat == 0 && tp->mac == (tt_cmap0_6 *) 0)
	    {
		tp->mac = (tt_cmap0_6 *)
			    pdc_malloc(p->pdc, sizeof (tt_cmap0_6), fn);
		tp->mac->format = 0;
		tt_get_cmap0(p, ttf, tp->mac);
	    }
	    else if (tableFormat == 6 && tp->mac == (tt_cmap0_6 *) 0)
	    {
		tp->mac = (tt_cmap0_6 *)
			    pdc_malloc(p->pdc, sizeof (tt_cmap0_6), fn);
		tp->mac->format = 6;
		tt_get_cmap6(p, ttf, tp->mac);
	    }
	}
	else if (platformID == tt_pfid_win &&
		(encodingID == tt_wenc_symbol || encodingID == tt_wenc_text))
	{
	    TT_IOCHECK(p, ttf, tp->win == (tt_cmap4 *) 0);
	    tp->win = (tt_cmap4 *) pdc_malloc(p->pdc, sizeof (tt_cmap4), fn);
	    tt_get_cmap4(p, ttf, tp->win);
            if (tp->win->segCountX2)
                tp->win->encodingID = encodingID;
            else
            {
                pdc_free(p->pdc, tp->win);
                tp->win = (tt_cmap4 *) 0;
            }
	}

	tt_seek(p, ttf, pos);
    } /* for */
} /* tt_get_tab_cmap */

void
tt_get_tab_head(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_head";
    tt_tab_head *tp = (tt_tab_head *)
			pdc_malloc(p->pdc, sizeof (tt_tab_head), fn);
    int		idx = tt_tag2idx(ttf, pdf_str_head);

    ttf->tab_head = tp;
    TT_IOCHECK(p, ttf, idx != -1);
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->version			= tt_get_fixed(p, ttf);
    tp->fontRevision		= tt_get_fixed(p, ttf);
    tp->checkSumAdjustment	= tt_get_ulong(p, ttf);
    tp->magicNumber		= tt_get_ulong(p, ttf);
    tp->flags			= tt_get_ushort(p, ttf);
    tp->unitsPerEm		= tt_get_ushort(p, ttf);
    tp->created[1]		= tt_get_ulong(p, ttf);
    tp->created[0]		= tt_get_ulong(p, ttf);
    tp->modified[1]		= tt_get_ulong(p, ttf);
    tp->modified[0]		= tt_get_ulong(p, ttf);
    tp->xMin			= tt_get_fword(p, ttf);
    tp->yMin			= tt_get_fword(p, ttf);
    tp->xMax			= tt_get_fword(p, ttf);
    tp->yMax			= tt_get_fword(p, ttf);
    tp->macStyle		= tt_get_ushort(p, ttf);
    tp->lowestRecPPEM		= tt_get_ushort(p, ttf);
    tp->fontDirectionHint	= tt_get_short(p, ttf);
    tp->indexToLocFormat	= tt_get_short(p, ttf);
    tp->glyphDataFormat		= tt_get_short(p, ttf);
} /* tt_get_tab_head */

static void
tt_get_tab_hhea(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_hhea";
    tt_tab_hhea *tp = (tt_tab_hhea *)
			pdc_malloc(p->pdc, sizeof (tt_tab_hhea), fn);
    int		idx = tt_tag2idx(ttf, pdf_str_hhea);

    ttf->tab_hhea = tp;
    TT_IOCHECK(p, ttf, idx != -1);
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->version			= tt_get_fixed(p, ttf);
    tp->ascender		= tt_get_fword(p, ttf);
    tp->descender		= tt_get_fword(p, ttf);
    tp->lineGap			= tt_get_fword(p, ttf);
    tp->advanceWidthMax		= tt_get_ufword(p, ttf);
    tp->minLeftSideBearing	= tt_get_fword(p, ttf);
    tp->minRightSideBearing	= tt_get_fword(p, ttf);
    tp->xMaxExtent		= tt_get_fword(p, ttf);
    tp->caretSlopeRise		= tt_get_short(p, ttf);
    tp->caretSlopeRun		= tt_get_short(p, ttf);
    tp->res1			= tt_get_short(p, ttf);
    tp->res2			= tt_get_short(p, ttf);
    tp->res3			= tt_get_short(p, ttf);
    tp->res4			= tt_get_short(p, ttf);
    tp->res5			= tt_get_short(p, ttf);
    tp->metricDataFormat	= tt_get_short(p, ttf);
    tp->numberOfHMetrics	= tt_get_ushort(p, ttf);
} /* tt_get_tab_hhea */

static void
tt_get_tab_hmtx(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_hmtx";
    tt_tab_hmtx *tp = (tt_tab_hmtx *)
			pdc_malloc(p->pdc, sizeof (tt_tab_hmtx), fn);
    int		idx = tt_tag2idx(ttf, pdf_str_hmtx);
    int		n_metrics;
    int		n_lsbs;
    int		i;

    TT_ASSERT(p, ttf, ttf->tab_hhea != 0);
    TT_ASSERT(p, ttf, ttf->tab_maxp != 0);

    /* the instruction order is critical for cleanup after exceptions!
    */
    tp->metrics = 0;
    tp->lsbs = 0;
    ttf->tab_hmtx = tp;

    TT_IOCHECK(p, ttf, idx != -1);
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    n_metrics = ttf->tab_hhea->numberOfHMetrics;
    n_lsbs = ttf->tab_maxp->numGlyphs - n_metrics;

    TT_IOCHECK(p, ttf, n_metrics != 0);
    TT_IOCHECK(p, ttf, n_lsbs >= 0);
    tp->metrics = (tt_metric *)
	pdc_malloc(p->pdc, n_metrics * sizeof (tt_metric), fn);

    for (i = 0; i < n_metrics; ++i)
    {
	tp->metrics[i].advanceWidth = tt_get_ufword(p, ttf);
	tp->metrics[i].lsb = tt_get_fword(p, ttf);
    }

    if (n_lsbs == 0)
	tp->lsbs = (tt_fword *) 0;
    else
    {
	tp->lsbs = (tt_fword *)
			pdc_malloc(p->pdc, n_lsbs * sizeof (tt_fword), fn);
	for (i = 0; i < n_lsbs; ++i)
	    tp->lsbs[i] = tt_get_fword(p, ttf);
    }
} /* tt_get_tab_hmtx */



pdc_bool
tt_get_tab_CFF_(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_CFF_";
    int	idx = tt_tag2idx(ttf, pdf_str_CFF_);

    if (idx != -1)
    {
        /* CFF table found */
        ttf->tab_CFF_ = (tt_tab_CFF_ *)
			    pdc_malloc(p->pdc, sizeof (tt_tab_CFF_), fn);
        ttf->tab_CFF_->offset = ttf->dir[idx].offset;
        ttf->tab_CFF_->length = ttf->dir[idx].length;
    }
    else
    {
        idx = tt_tag2idx(ttf, pdf_str_glyf);
        if (idx == -1 || !ttf->dir[idx].length)
        {
            const char *fontname = ttf->fontname;
            pdc_bool verbose = ttf->font->verbose;

            pdf_cleanup_tt(p, ttf);
	    pdc_set_errmsg(p->pdc, PDF_E_TT_NOGLYFDESC, fontname, 0, 0, 0);

            if (verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

	    return pdc_false;
        }
        idx = -1;
    }


    return pdc_true;

} /* tt_get_tab_CFF_ */

static int
tt_gidx2width(PDF *p, tt_file *ttf, int gidx)
{
    TT_ASSERT(p, ttf, ttf->tab_hmtx != (tt_tab_hmtx *) 0);
    TT_ASSERT(p, ttf, ttf->tab_head != (tt_tab_head *) 0);
    TT_ASSERT(p, ttf, ttf->tab_hhea != (tt_tab_hhea *) 0);

    {
	int n_metrics = ttf->tab_hhea->numberOfHMetrics;

	if (gidx >= n_metrics)
	    gidx = n_metrics - 1;
        if (ttf->font->monospace)
            return ttf->font->monospace;
        else
            return PDF_TT2PDF(ttf->tab_hmtx->metrics[gidx].advanceWidth);
    }
} /* tt_gidx2width */


void
tt_get_tab_maxp(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_maxp";
    tt_tab_maxp *tp = (tt_tab_maxp *)
			pdc_malloc(p->pdc, sizeof (tt_tab_maxp), fn);
    int		idx = tt_tag2idx(ttf, pdf_str_maxp);

    ttf->tab_maxp = tp;
    TT_IOCHECK(p, ttf, idx != -1);
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->version			= tt_get_fixed(p, ttf);
    tp->numGlyphs		= tt_get_ushort(p, ttf);
    tp->maxPoints		= tt_get_ushort(p, ttf);
    tp->maxContours		= tt_get_ushort(p, ttf);
    tp->maxCompositePoints	= tt_get_ushort(p, ttf);
    tp->maxCompositeContours	= tt_get_ushort(p, ttf);
    tp->maxZones		= tt_get_ushort(p, ttf);
    tp->maxTwilightPoints	= tt_get_ushort(p, ttf);
    tp->maxStorage		= tt_get_ushort(p, ttf);
    tp->maxFunctionDefs		= tt_get_ushort(p, ttf);
    tp->maxInstructionDefs	= tt_get_ushort(p, ttf);
    tp->maxStackElements	= tt_get_ushort(p, ttf);
    tp->maxSizeOfInstructions	= tt_get_ushort(p, ttf);
    tp->maxComponentElements	= tt_get_ushort(p, ttf);
    tp->maxComponentDepth	= tt_get_ushort(p, ttf);
} /* tt_get_tab_maxp */

static pdc_bool
tt_get_tab_name(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_name";
    tt_tab_name *tp;
    int		idx;
    int		i, j, k, namid, irec, irec4 = -1, irec6 = -1;
    size_t      len;
    tt_nameref  *namerec = NULL, lastnamerec;
    char        *localname = NULL;
    tt_ulong	offs;

    idx = tt_tag2idx(ttf, pdf_str_name);

    /* not obligatory */
    if (idx == -1)
        return pdc_false;

    /* the instruction order is critical for cleanup after exceptions!
    */
    tp = (tt_tab_name *) pdc_malloc(p->pdc, sizeof (tt_tab_name), fn);
    tp->namerecords = NULL;
    tp->englishname4 = NULL;
    tp->englishname6 = NULL;
    ttf->tab_name = tp;
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->format = tt_get_ushort(p, ttf);

    /* Format 0 is the only document one, but some Apple fonts use 65535.
     * This is very consequent since it follows Microsoft's lead in
     * disregarding one's own published specifications.
     */
    TT_IOCHECK(p, ttf, (tp->format == 0 || tp->format == 65535));

    tp->numNameRecords = (tt_ushort)
        tt_get_offset(p, ttf, sizeof(tt_ushort));
    tp->offsetStrings = tt_get_ushort(p, ttf);
    offs = ttf->dir[idx].offset + tp->offsetStrings;

    len = tp->numNameRecords * sizeof (tt_nameref);
    tp->namerecords = (tt_nameref *) pdc_malloc(p->pdc, len, fn);

    for (i = 0; i < tp->numNameRecords; ++i)
    {
	tt_ushort platformID	= tt_get_ushort(p, ttf);
	tt_ushort encodingID	= tt_get_ushort(p, ttf);
	tt_ushort languageID	= tt_get_ushort(p, ttf);
	tt_ushort nameID	= tt_get_ushort(p, ttf);
	tt_ushort stringLength	= tt_get_ushort(p, ttf);
	tt_ushort stringOffset	= tt_get_ushort(p, ttf);

	(void)  encodingID;	/* avoid compiler warning */

        namerec = &tp->namerecords[i];
        namerec->platform = platformID;
        namerec->language = languageID;
        namerec->namid = nameID;
        namerec->length = stringLength;
        namerec->offset = stringOffset;
    }

    namid = 4;
    for (k = 0; k < 2; k++)
    {
        lastnamerec.platform = 0;
        lastnamerec.language = 0;
        lastnamerec.namid = 0;
        lastnamerec.length = 0;
        lastnamerec.offset = 0;

        for (i = 0; i < tp->numNameRecords; ++i)
        {
            namerec = &tp->namerecords[i];
            if (!namerec->length || namerec->namid != namid) continue;

            /* TTC font search */
            if (ttf->utf16fontname)
            {
                /* read font name */
                localname =
                    (char *) pdc_calloc(p->pdc, (size_t) namerec->length, fn);
                tt_seek(p, ttf, (long) (offs + namerec->offset));
                tt_read(p, ttf, localname, (unsigned int) namerec->length);

                if (namerec->platform == tt_pfid_win &&
                    namerec->length == ttf->fnamelen &&
                    !memcmp(localname, ttf->utf16fontname,
                            (size_t) ttf->fnamelen))
                {
                     /* font found */
                     pdc_free(p->pdc, localname);
                     return pdc_true;
                }
                pdc_free(p->pdc, localname);
            }

            /* search for the records with english names */
            else
            {
                /* we take the names from platformID 3 (Microsoft) or
                 * 1 (Macintosh). We favor Macintosh and then Microsoft
                 * with American English (LanguageID = 0x0409 = 1033)
                 */
                if ((lastnamerec.language != 0x0409 ||
                     lastnamerec.platform != tt_pfid_win) &&
                    (namerec->platform == tt_pfid_win ||
                     (namerec->platform == tt_pfid_mac &&
                      namerec->language == 0)))
                {
                    lastnamerec = *namerec;

                    /* We simulate always English */
                    if (namerec->platform == tt_pfid_mac)
                        lastnamerec.language = 0x0409;

                    if (namid == 4) irec4 = i;
                    if (namid == 6) irec6 = i;
                }
            }
        }
        namid = 6;
    }

    /* TTC font not found */
    if (ttf->utf16fontname) return pdc_false;

    /* English font names */
    namid = 4;
    irec = irec4;
    for (k = 0; k < 2; k++)
    {
        if (irec == -1) continue;
        namerec = &tp->namerecords[irec];

        /* read font name */
        len = (size_t) (namerec->length + 1);
        localname = (char *) pdc_calloc(p->pdc, (size_t) len, fn);
        tt_seek(p, ttf, (long) (offs + namerec->offset));
        tt_read(p, ttf, localname, (unsigned int) namerec->length);

        /* low byte picking */
        if (namerec->platform == tt_pfid_win)
	{
            for (j = 0; j < namerec->length / 2; j++)
            {
                /* We don't support wide character names */
                if (localname[2*j] != 0)
                {
                    pdc_free(p->pdc, localname);
                    localname = NULL;
                    break;
                }
                localname[j] = localname[2*j + 1];
            }
            if (localname)
                localname[j] = 0;
	}

        /* We observed this in EUDC fonts */
        if (localname && !strcmp(localname, "\060\060"))
        {
            pdc_free(p->pdc, localname);
            localname = NULL;
        }

        if (namid == 4 && localname)
            tp->englishname4 = localname;
        else if (namid == 6 && localname)
            tp->englishname6 = localname;

        namid = 6;
        irec = irec6;
    }

    /* font name 4 (full font name) is required */
    if (tp->englishname6 == NULL && tp->englishname4 == NULL)
    {
        const char *fontname = ttf->fontname;
        pdc_bool verbose = ttf->font->verbose;

        pdf_cleanup_tt(p, ttf);
        pdc_set_errmsg(p->pdc, PDF_E_TT_NONAME, fontname, 0, 0, 0);

        if (verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return pdc_false;
    }

    if (tp->englishname4 == NULL)
        tp->englishname4 = pdc_strdup(p->pdc, tp->englishname6);
    if (tp->englishname6 == NULL)
        tp->englishname6 = pdc_strdup(p->pdc, tp->englishname4);

    return pdc_true;
} /* tt_get_tab_name */

void
tt_get_tab_OS_2(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_OS_2";
    tt_tab_OS_2 *tp;
    int		idx;

    idx = tt_tag2idx(ttf, pdf_str_OS_2);

    /* Some Apple fonts don't have an OS/2 table */
    if (idx == -1)
	return;

    tp = (tt_tab_OS_2 *) pdc_malloc(p->pdc, sizeof (tt_tab_OS_2), fn);
    ttf->tab_OS_2 = tp;

    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->version = tt_get_ushort(p, ttf);
    tp->xAvgCharWidth = tt_get_short(p, ttf);
    tp->usWeightClass = tt_get_ushort(p, ttf);
    tp->usWidthClass = tt_get_ushort(p, ttf);
    tp->fsType = tt_get_ushort(p, ttf);
    tp->ySubscriptXSize = tt_get_short(p, ttf);
    tp->ySubscriptYSize = tt_get_short(p, ttf);
    tp->ySubscriptXOffset = tt_get_short(p, ttf);
    tp->ySubscriptYOffset = tt_get_short(p, ttf);
    tp->ySuperscriptXSize = tt_get_short(p, ttf);
    tp->ySuperscriptYSize = tt_get_short(p, ttf);
    tp->ySuperscriptXOffset = tt_get_short(p, ttf);
    tp->ySuperscriptYOffset = tt_get_short(p, ttf);
    tp->yStrikeoutSize = tt_get_short(p, ttf);
    tp->yStrikeoutPosition = tt_get_short(p, ttf);
    tp->sFamilyClass = tt_get_ushort(p, ttf);

    tt_read(p, ttf, tp->panose, 10);

    tp->ulUnicodeRange1 = tt_get_ulong(p, ttf);
    tp->ulUnicodeRange2 = tt_get_ulong(p, ttf);
    tp->ulUnicodeRange3 = tt_get_ulong(p, ttf);
    tp->ulUnicodeRange4 = tt_get_ulong(p, ttf);

    tt_read(p, ttf, tp->achVendID, 4);

    tp->fsSelection = tt_get_ushort(p, ttf);
    tp->usFirstCharIndex = tt_get_ushort(p, ttf);
    tp->usLastCharIndex = tt_get_ushort(p, ttf);
    tp->sTypoAscender = tt_get_short(p, ttf);
    tp->sTypoDescender = tt_get_short(p, ttf);
    tp->sTypoLineGap = tt_get_short(p, ttf);
    tp->usWinAscent = tt_get_ushort(p, ttf);
    tp->usWinDescent = tt_get_ushort(p, ttf);

    if (tp->version >= 1)
    {
	tp->ulCodePageRange[0] = tt_get_ulong(p, ttf);
	tp->ulCodePageRange[1] = tt_get_ulong(p, ttf);
    }
    else
    {
	tp->ulCodePageRange[0] = 0;
	tp->ulCodePageRange[1] = 0;
    }

    if (tp->version >= 2)
    {
	tp->sxHeight = tt_get_short(p, ttf);
	tp->sCapHeight = tt_get_short(p, ttf);
	tp->usDefaultChar = tt_get_ushort(p, ttf);
	tp->usBreakChar = tt_get_ushort(p, ttf);
	tp->usMaxContext = tt_get_ushort(p, ttf);
    }
    else
    {
	tp->sxHeight = 0;
	tp->sCapHeight = 0;
	tp->usDefaultChar = 0;
	tp->usBreakChar = 0;
	tp->usMaxContext = 0;
    }

    /* there are fonts with inconsistent usFirstCharIndex */
    if (ttf->tab_cmap && ttf->tab_cmap->win &&
        tp->usFirstCharIndex != ttf->tab_cmap->win->startCount[0])
        ttf->tab_OS_2->usFirstCharIndex = ttf->tab_cmap->win->startCount[0];

} /* tt_get_tab_OS_2 */

static void
tt_get_tab_post(PDF *p, tt_file *ttf)
{
    static const char *fn = "tt_get_tab_post";
    tt_tab_post *tp = (tt_tab_post *)
                        pdc_malloc(p->pdc, sizeof (tt_tab_post), fn);
    int         idx = tt_tag2idx(ttf, pdf_str_post);

    ttf->tab_post = tp;
    TT_IOCHECK(p, ttf, idx != -1);
    tt_seek(p, ttf, (long) ttf->dir[idx].offset);

    tp->formatType = tt_get_fixed(p, ttf);
    tp->italicAngle = tt_get_fixed(p, ttf) / (float) 65536.0;
    tp->underlinePosition = tt_get_fword(p, ttf);
    tp->underlineThickness = tt_get_fword(p, ttf);
    tp->isFixedPitch = tt_get_ulong(p, ttf);
    tp->minMemType42 = tt_get_ulong(p, ttf);
    tp->maxMemType42 = tt_get_ulong(p, ttf);
    tp->minMemType1 = tt_get_ulong(p, ttf);
    tp->maxMemType1 = tt_get_ulong(p, ttf);
} /* tt_get_tab_post */

tt_file *
pdf_init_tt(PDF *p)
{
    static const char *fn = "pdf_init_tt";

    tt_file *ttf = (tt_file *)
                pdc_malloc(p->pdc, (size_t) sizeof (tt_file), fn);

    ttf->check = pdc_false;
    ttf->incore = pdc_false;
    ttf->img = NULL;
    ttf->end = 0;
    ttf->pos = 0;

    ttf->fp = (pdc_file *) 0;
    ttf->n_tables = 0;
    ttf->offset = 0;
    ttf->dir = (tt_dirent *) 0;

    ttf->tab_cmap = (tt_tab_cmap *) 0;
    ttf->tab_head = (tt_tab_head *) 0;
    ttf->tab_hhea = (tt_tab_hhea *) 0;
    ttf->tab_hmtx = (tt_tab_hmtx *) 0;
    ttf->tab_maxp = (tt_tab_maxp *) 0;
    ttf->tab_name = (tt_tab_name *) 0;
    ttf->tab_post = (tt_tab_post *) 0;
    ttf->tab_OS_2 = (tt_tab_OS_2 *) 0;
    ttf->tab_CFF_ = (tt_tab_CFF_ *) 0;

    ttf->GID2Code = (tt_ushort *) 0;
    ttf->hasGlyphNames = 0;
    ttf->hasEncoding = 0;


    ttf->utf16fontname = (char *) 0;

    return ttf;

} /* pdf_init_tt */

#define PDF_O        ((char) 0x4f)           /* ASCII 'O'  */
#define PDF_T        ((char) 0x54)           /* ASCII 'T'  */

#define PDF_t        ((char) 0x74)           /* ASCII 't'  */
#define PDF_r        ((char) 0x72)           /* ASCII 'r'  */
#define PDF_u        ((char) 0x75)           /* ASCII 'u'  */
#define PDF_e        ((char) 0x65)           /* ASCII 'e'  */

#define PDF_c        ((char) 0x63)           /* ASCII 'c'  */
#define PDF_f        ((char) 0x66)           /* ASCII 'f'  */

pdc_bool
pdf_test_tt_font(tt_byte *img, tt_ulong *n_fonts)
{
    tt_ushort n_tables;

    /* The TrueType (including OpenType/TT) "version" is always 0x00010000 */
    if (!(img[0] == 0x00 && img[1] == 0x01 &&
          img[2] == 0x00 && img[3] == 0x00))
    {
        /* The OpenType/CFF version is always 'OTTO' */
        if (!(img[0] == PDF_O && img[1] == PDF_T &&
              img[2] == PDF_T && img[3] == PDF_O))
        {
            /* Old Apple fonts have 'true' */
            if (!(img[0] == PDF_t && img[1] == PDF_r &&
                  img[2] == PDF_u && img[3] == PDF_e))
            {
                /* TrueType Collection */
                if (n_fonts == NULL ||
                    !(img[0] == PDF_t && img[1] == PDF_t &&
                      img[2] == PDF_c && img[3] == PDF_f))
                    return pdc_false;

                /* Version is always 0x00010000 or 0x00020000  */
                if (!(img[4] == 0x00 && (img[5] == 0x01 || img[5] == 0x02) &&
                      img[6] == 0x00 && img[7] == 0x00))
                    return pdc_false;

                /* Number of fonts */
                *n_fonts = pdc_get_be_ulong(&img[8]);
                return pdc_true;
            }
        }
    }

    /* Number of tables */
    n_tables = pdc_get_be_ushort(&img[4]);
    if (n_tables < 8 && n_tables > 64)  /* max. 32 tables ? */
        return pdc_false;

    /* Further check of the next 6 bytes not implemented */

    return pdc_true;
}

pdc_bool
pdf_read_offset_tab(PDF *p, tt_file *ttf)
{
    static const char *fn = "pdf_get_tab_offset";
    tt_byte img[TT_OFFSETTAB_SIZE];
    int i;

    /* Check */
    tt_read(p, ttf, img, TT_OFFSETTAB_SIZE);
    if (pdf_test_tt_font(img, NULL) == pdc_false)
    {
        const char *filename = ttf->filename;
        pdc_bool verbose = ttf->font->verbose;

        pdf_cleanup_tt(p, ttf);
	pdc_set_errmsg(p->pdc, PDF_E_TT_NOFONT, filename, 0, 0, 0);

        if (verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }

    /* number of table directories */
    ttf->n_tables = pdc_get_be_ushort(&img[4]);

    /* set up table directory */
    ttf->dir = (tt_dirent *) pdc_malloc(p->pdc,
            (size_t) (ttf->n_tables * sizeof (tt_dirent)), fn);

    tt_seek(p, ttf, (long) (ttf->offset + TT_OFFSETTAB_SIZE));

    for (i = 0; i < ttf->n_tables; ++i)
        tt_get_dirent(p, ttf->dir + i, ttf);

    /* make sure this isn't a bitmap-only Mac font */
    if (tt_tag2idx(ttf, pdf_str_bhed) != -1)
    {
        const char *fontname = ttf->fontname;
        pdc_bool verbose = ttf->font->verbose;

        pdf_cleanup_tt(p, ttf);
	pdc_set_errmsg(p->pdc, PDF_E_TT_BITMAP, fontname, 0, 0, 0);

        if (verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }

    return pdc_true;

} /* pdf_read_offset_tab */

static pdc_bool
pdf_read_tt(PDF *p, tt_file *ttf, pdc_font *font)
{
    (void) font;

    if (pdf_read_offset_tab(p, ttf) == pdc_false)
        return pdc_false;

    /* These are all required TrueType tables; optional tables are not read.
    **
    ** There's no error return; pdf_cleanup_tt() is called by all subroutines
    ** before exceptions are thrown. LATER: It is a known bug that pdc_malloc()
    ** exceptions can't be caught and will lead to resource leaks in this
    ** context!
    */
    tt_get_tab_cmap(p, ttf);
    tt_get_tab_head(p, ttf);
    tt_get_tab_hhea(p, ttf);
    tt_get_tab_maxp(p, ttf);
    tt_get_tab_hmtx(p, ttf);    /* MUST be read AFTER hhea & maxp! */
    if (tt_get_tab_name(p, ttf) == pdc_false)
        return pdc_false;
    tt_get_tab_post(p, ttf);
    tt_get_tab_OS_2(p, ttf);    /* may be missing from some Apple fonts */

    /* this is an optional table, present only in OpenType fonts */
    if (tt_get_tab_CFF_(p, ttf) == pdc_false)
        return pdc_false;


    return pdc_true;

} /* pdf_read_tt */

void
pdf_cleanup_tt(PDF *p, tt_file *ttf)
{
    if (ttf->check == pdc_false && ttf->fp != (pdc_file *) 0)
        pdc_fclose(ttf->fp);

    if (ttf->dir != (tt_dirent *) 0)
        pdc_free(p->pdc, ttf->dir);
    ttf->dir = (tt_dirent *) 0;


    if (ttf->tab_head != (tt_tab_head *) 0)
        pdc_free(p->pdc, ttf->tab_head);
    if (ttf->tab_hhea != (tt_tab_hhea *) 0)
        pdc_free(p->pdc, ttf->tab_hhea);
    if (ttf->tab_maxp != (tt_tab_maxp *) 0)
        pdc_free(p->pdc, ttf->tab_maxp);
    if (ttf->tab_OS_2 != (tt_tab_OS_2 *) 0)
        pdc_free(p->pdc, ttf->tab_OS_2);
    if (ttf->tab_CFF_ != (tt_tab_CFF_ *) 0)
        pdc_free(p->pdc, ttf->tab_CFF_);
    if (ttf->tab_post != (tt_tab_post *) 0)
        pdc_free(p->pdc, ttf->tab_post);

    if (ttf->tab_cmap != (tt_tab_cmap *) 0)
    {
        if (ttf->tab_cmap->mac != (tt_cmap0_6 *) 0) {
            if (ttf->tab_cmap->mac->glyphIdArray)
                pdc_free(p->pdc, ttf->tab_cmap->mac->glyphIdArray);
            pdc_free(p->pdc, ttf->tab_cmap->mac);
        }

        if (ttf->tab_cmap->win != (tt_cmap4 *) 0)
        {
            tt_cmap4 *cm4 = (tt_cmap4 *) ttf->tab_cmap->win;

            if (cm4->endCount != 0)     pdc_free(p->pdc, cm4->endCount);
            if (cm4->startCount != 0)   pdc_free(p->pdc, cm4->startCount);
            if (cm4->idDelta != 0)      pdc_free(p->pdc, cm4->idDelta);
            if (cm4->idRangeOffs != 0)  pdc_free(p->pdc, cm4->idRangeOffs);
            if (cm4->glyphIdArray != 0) pdc_free(p->pdc, cm4->glyphIdArray);

            pdc_free(p->pdc, cm4);
        }

        pdc_free(p->pdc, ttf->tab_cmap);
    }

    if (ttf->tab_hmtx != (tt_tab_hmtx *) 0)
    {
        if (ttf->tab_hmtx->metrics != (tt_metric *) 0)
            pdc_free(p->pdc, ttf->tab_hmtx->metrics);
        if (ttf->tab_hmtx->lsbs != (tt_fword *) 0)
            pdc_free(p->pdc, ttf->tab_hmtx->lsbs);
        pdc_free(p->pdc, ttf->tab_hmtx);
    }

    if (ttf->tab_name != (tt_tab_name *) 0)
    {
        if (ttf->tab_name->namerecords != (tt_nameref *) 0)
            pdc_free(p->pdc, ttf->tab_name->namerecords);
        if (ttf->tab_name->englishname4 != (char *) 0)
            pdc_free(p->pdc, ttf->tab_name->englishname4);
        if (ttf->tab_name->englishname6 != (char *) 0)
            pdc_free(p->pdc, ttf->tab_name->englishname6);
        pdc_free(p->pdc, ttf->tab_name);
    }
    ttf->tab_name = (tt_tab_name *) 0;

    if (ttf->GID2Code != (tt_ushort *) 0)
        pdc_free(p->pdc, ttf->GID2Code);


    /* Note: do not clean up ttf->img since the data belongs to font->img
    */

    if (ttf->check == pdc_false)
        pdc_free(p->pdc, ttf);

} /* pdf_cleanup_tt */


static void
pdf_set_tt_fontvalues(PDF *p, tt_file *ttf, pdc_font *font)
{
    float       w;

    (void) p;

    font->llx           = (float) PDF_TT2PDF(ttf->tab_head->xMin);
    font->lly           = (float) PDF_TT2PDF(ttf->tab_head->yMin);
    font->urx           = (float) PDF_TT2PDF(ttf->tab_head->xMax);
    font->ury           = (float) PDF_TT2PDF(ttf->tab_head->yMax);

    font->italicAngle        = (float) ttf->tab_post->italicAngle;
    font->isFixedPitch       = (pdc_bool) ttf->tab_post->isFixedPitch;
    font->underlinePosition  = PDF_TT2PDF(ttf->tab_post->underlinePosition);
    font->underlineThickness = PDF_TT2PDF(ttf->tab_post->underlineThickness);

    if (ttf->tab_OS_2) {

        w               = ttf->tab_OS_2->usWeightClass / (float) 65;
        font->StdVW     = (int) (PDF_STEMV_MIN + w * w);

        /* some Apple fonts have zero values in the OS/2 table  */
        if (ttf->tab_OS_2->sTypoAscender)
            font->ascender      = PDF_TT2PDF(ttf->tab_OS_2->sTypoAscender);
        else
            font->ascender      = PDF_TT2PDF(ttf->tab_hhea->ascender);

        if (ttf->tab_OS_2->sTypoDescender)
            font->descender     = PDF_TT2PDF(ttf->tab_OS_2->sTypoDescender);
        else
            font->descender     = PDF_TT2PDF(ttf->tab_hhea->descender);

        /* sCapHeight and sxHeight are only valid if version >= 2 */
        if (ttf->tab_OS_2->version >= 2 && ttf->tab_OS_2->sCapHeight) {
            font->capHeight     = PDF_TT2PDF(ttf->tab_OS_2->sCapHeight);
        } else {
            font->capHeight     = font->ascender;       /* an educated guess */
        }

        if (ttf->tab_OS_2->version >= 2 && ttf->tab_OS_2->sxHeight) {
            font->xHeight       = PDF_TT2PDF(ttf->tab_OS_2->sxHeight);
        } else {
	    /* guess */
            font->xHeight       = (int) ((float) 0.66 * font->ascender);
        }

    } else {

#define PDF_MACSTYLE_BOLD     0x01

        if (ttf->tab_head->macStyle & PDF_MACSTYLE_BOLD)
            font->StdVW = PDF_STEMV_BOLD;
        else
            font->StdVW = PDF_STEMV_NORMAL;

        /* The OS/2 table is missing from some Apple fonts */
        font->ascender  = PDF_TT2PDF(ttf->tab_hhea->ascender);
        font->descender = PDF_TT2PDF(ttf->tab_hhea->descender);

        /* estimate the remaining values from the known ones */
        font->xHeight   = (int) ((float) 0.66 * font->descender);
        font->capHeight = font->ascender;
    }
}

#ifdef PDF_UNUSED

static pdc_bool
tt_supports_cp(char *name, unsigned *cp_range)
{
    typedef struct
    {
	const char *	s;
	int		i;
    } str_int;

    static const str_int n2i[] =
    {
	{ "1250",	TT_CP1250 },
	{ "1251",	TT_CP1251 },
	{ "1252",	TT_CP1252 },
	{ "1253",	TT_CP1253 },
	{ "1254",	TT_CP1254 },
	{ "1255",	TT_CP1255 },
	{ "1256",	TT_CP1256 },
	{ "1257",	TT_CP1257 },
	{ "1258",	TT_CP1258 },
	{ "1361",	TT_CP1361 },
	{ "437",	TT_CP437  },
	{ "708",	TT_CP708  },
	{ "737",	TT_CP737  },
	{ "775",	TT_CP775  },
	{ "850",	TT_CP850  },
	{ "852",	TT_CP852  },
	{ "855",	TT_CP855  },
	{ "857",	TT_CP857  },
	{ "860",	TT_CP860  },
	{ "861",	TT_CP861  },
	{ "862",	TT_CP862  },
	{ "863",	TT_CP863  },
	{ "864",	TT_CP864  },
	{ "865",	TT_CP865  },
	{ "866",	TT_CP866  },
	{ "869",	TT_CP869  },
	{ "874",	TT_CP874  },
	{ "932",	TT_CP932  },
	{ "936",	TT_CP936  },
	{ "949",	TT_CP949  },
	{ "950",	TT_CP950  },
    };

    int lo = 0;
    int hi = (sizeof n2i) / (sizeof (str_int));

    while (lo != hi)
    {
	int idx = (lo + hi) / 2;
	int cmp = strcmp(name, n2i[idx].s);

	if (cmp == 0)
	{
	    int i = n2i[idx].i;

	    return (cp_range[i > 31] & (1 << (i & 0x1F))) != 0;
	}

	if (cmp < 0)
	    hi = idx;
	else
	    lo + idx + 1;
    }

    return pdc_false;
} /* tt_supports_cp */

#endif	/* PDF_UNUSED */

pdc_bool
pdf_get_metrics_tt(PDF *p, pdc_font *font, const char *fontname,
                   pdc_encoding enc, const char *filename)
{
    static const char *fn = "pdf_get_metrics_tt";
    float       filesize;
    int         foundChars, code, ncodes, nwidths, gidx = 0;
    tt_file     *ttf;
    pdc_bool    retval;

    /*
     * Initialisation
     */
    ttf = pdf_init_tt(p);
    ttf->incore = pdc_true;
    ttf->img = (tt_byte *) font->img;
    ttf->pos = ttf->img;
    ttf->end = ttf->img + font->filelen;
    filesize = (float) (font->filelen / 1024.0);

    ttf->filename = filename;
    ttf->fontname = fontname;
    ttf->font = font;

    /*
     * Read font file
     */
    retval = pdf_read_tt(p, ttf, font);
    if (retval == pdc_false)
        return pdc_false;

    /*
     * Font type
     */
    if (ttf->tab_CFF_) {
	font->type = pdc_Type1C;
	font->cff_offset = (long) ttf->tab_CFF_->offset;
	font->cff_length = ttf->tab_CFF_->length;
    } else {
	font->type = pdc_TrueType;
    }

    /* These tables are not present in OT fonts */
    if (font->type != pdc_Type1C) {
	TT_IOCHECK(p, ttf, tt_tag2idx(ttf, pdf_str_glyf) != -1);
	TT_IOCHECK(p, ttf, tt_tag2idx(ttf, pdf_str_loca) != -1);
    }

    /* Number of Glyphs */
    if (ttf->tab_maxp->numGlyphs <= 1)
    {
        pdf_cleanup_tt(p, ttf);
        pdc_set_errmsg(p->pdc, PDF_E_TT_NOGLYFDESC, fontname, 0, 0, 0);

        if (font->verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }

    /*
     * Encoding
     */
    font->isstdlatin = pdc_true;
    if (ttf->tab_cmap->mac && !ttf->tab_cmap->win)
    {
        /* MacRoman font */
        if (enc >= pdc_builtin && enc != pdc_macroman)
        {
            if (font->verbose)
            {
                pdc_warning(p->pdc, PDF_E_FONT_FORCEENC,
                            pdf_get_encoding_name(p, pdc_macroman),
                            pdf_get_encoding_name(p, enc), fontname, 0);
            }
            enc = pdc_macroman;
        }
        if (!p->encodings[pdc_macroman].ev)
            p->encodings[pdc_macroman].ev =
                pdc_copy_core_encoding(p->pdc, "macroman");
    }
    else if (ttf->tab_cmap->win)
    {
        if (ttf->tab_cmap->win->encodingID == tt_wenc_symbol)
        {
            /* Symbol font */
            font->isstdlatin = pdc_false;

            /* 8-bit encoding: We enforce builtin encoding */
            if (enc >= 0)
            {
                if (font->verbose)
                {
                    pdc_warning(p->pdc, PDF_E_FONT_FORCEENC,
                                pdf_get_encoding_name(p, pdc_builtin),
                                pdf_get_encoding_name(p, enc), fontname, 0);
                }
                enc = pdc_builtin;
            }
        }
        else if (!ttf->tab_cmap->mac)
        {
            /* has no MacRoman cmap */
            font->hasnomac = pdc_true;
        }
    }
    else
    {
        pdf_cleanup_tt(p, ttf);
        pdc_set_errmsg(p->pdc, PDF_E_TT_BADCMAP, fontname, 0, 0, 0);

        if (font->verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }
    font->encoding = enc;


    /* builtin encoding but text font */
    if (enc == pdc_builtin && font->isstdlatin == pdc_true)
    {
        pdf_cleanup_tt(p, ttf);
	pdc_set_errmsg(p->pdc, PDF_E_FONT_BADENC,
            fontname, pdf_get_encoding_name(p, enc), 0, 0);

        if (font->verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }


    /* /FontName in FontDescriptor */
    font->name = pdc_strdup(p->pdc, ttf->tab_name->englishname4);

    /* /BaseFont name */
    font->ttname = pdc_strdup(p->pdc, ttf->tab_name->englishname6);


#define PDF_RESTRICTED_TT_EMBEDDING	0x02
    /* check embedding flags */
    if ((font->embedding) && ttf->tab_OS_2 &&
	ttf->tab_OS_2->fsType == PDF_RESTRICTED_TT_EMBEDDING)
    {
	pdf_cleanup_tt(p, ttf);
        pdc_set_errmsg(p->pdc, PDF_E_TT_EMBED, fontname, 0, 0, 0);

        if (font->verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }


    /* Save font values */
    pdf_set_tt_fontvalues(p, ttf, font);

    /* Allocate encoding dependent arrays */
    font->numOfGlyphs = ttf->tab_maxp->numGlyphs;
    ncodes = font->numOfCodes;
    nwidths = font->numOfCodes;


    font->code2GID = (pdc_ushort *) pdc_calloc(p->pdc,
                                   font->numOfCodes  * sizeof (pdc_ushort), fn);
    ttf->GID2Code = (tt_ushort *) pdc_calloc(p->pdc,
                                   font->numOfGlyphs  * sizeof (tt_ushort), fn);
    if (nwidths)
        font->widths = (int *) pdc_calloc(p->pdc, nwidths * sizeof(int), fn);


    /*
     * Build the char width tables for the font, set mappings GID <-> Code,
     * and count the characters.
     */
    foundChars = 0;
    for (code = 0; code < ncodes; code++)
    {
        gidx = tt_code2gidx(p, ttf, font, code);
        if (gidx == -1)
            return pdc_false;
        if (gidx)
        {
            /* we prefer the first occurence of gidx (SPACE vs. NBSP) */
            if (!ttf->GID2Code[gidx])
                ttf->GID2Code[gidx] = (tt_ushort) code;
            if (font->GID2code)
                font->GID2code[gidx] = (tt_ushort) code;
            foundChars++;
        }

        switch (enc)
        {

            default:
            font->widths[code] = tt_gidx2width(p, ttf, gidx);
            break;
        }

        font->code2GID[code] = (pdc_ushort) gidx;
    }

    /* No characters found */
    if (!foundChars)
    {
        pdf_cleanup_tt(p, ttf);
        pdc_set_errmsg(p->pdc, PDF_E_FONT_BADENC, fontname,
	    pdf_get_encoding_name(p, enc), 0, 0);

        if (font->verbose)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);

	return pdc_false;
    }
    else
    {


        pdf_cleanup_tt(p, ttf);
    }

    if (!pdf_make_fontflag(p, font))
        return pdc_false;

    return pdc_true;
}

static pdc_bool
pdf_check_and_read_ttc(PDF *p, pdc_file *fp,
                       const char *filename, const char *fontname,
                       pdc_font *font, tt_ulong n_fonts)
{
    static const char *fn = "pdf_check_and_read_ttc";
    tt_file *ttf;
    char *utf16fontname = NULL;
    char *fontname_p = (char *) fontname;
    pdc_bool retval = pdc_false;
    pdc_text_format textformat = pdc_bytes;
    pdc_text_format targettextformat = pdc_utf16be;
    int i, inlen, outlen;

    /* initialize */
    ttf = pdf_init_tt(p);
    ttf->check = pdc_true;
    ttf->fp = fp;
    ttf->filename = filename;
    ttf->fontname = fontname;
    ttf->font = font;

    /* create UTF-16-BE font name string for searching in font file */
    inlen = (int) strlen(fontname_p);
    if (pdc_is_utf8_unicode(fontname_p))
        textformat = pdc_utf8;
    if (pdc_convert_string(p->pdc, textformat, NULL,
                           (pdc_byte *) fontname_p, inlen,
                           &targettextformat, NULL,
                           (pdc_byte **) &utf16fontname, &outlen,
                           PDC_CONV_NOBOM, font->verbose))
    {
        pdf_cleanup_tt(p, ttf);
        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);
        return pdc_false;
    }

    /* search font */
    for (i = 0; i < (int)n_fonts; ++i)
    {
        if (i) pdf_cleanup_tt(p, ttf);

        tt_seek(p, ttf, (long) (TT_OFFSETTAB_SIZE + i * sizeof(tt_ulong)));
        ttf->offset = tt_get_ulong(p, ttf);
        tt_seek(p, ttf, (long) ttf->offset);

        /* Offset Table */
        if (!pdf_read_offset_tab(p, ttf))
            break;

        /* font name match in Naming Table */
        ttf->utf16fontname = utf16fontname;
        ttf->fnamelen = outlen;
        if (tt_get_tab_name(p, ttf))
            break;
    }
    pdc_free(p->pdc, utf16fontname);

    /* font found */
    if (i < (int)n_fonts)
    {
        tt_byte *pos;
        tt_ulong headlen, dirlen, tablen, length, offset;

        /* create file in memory */
        tablen = 0;
        dirlen = 4 * sizeof(tt_ulong);
        headlen = (tt_ulong) (TT_OFFSETTAB_SIZE + ttf->n_tables * dirlen);
        font->filelen = headlen;
        for (i = 0; i < ttf->n_tables; i++)
        {
            length = ttf->dir[i].length;
            if (length > tablen) tablen = length;
            font->filelen += length;
        }
        font->img = (pdc_byte *) pdc_malloc(p->pdc, font->filelen, fn);

        /* read font file */
        tt_seek(p, ttf, (long) ttf->offset);
        tt_read(p, ttf, font->img, headlen);
        pos = font->img + headlen;
        for (i = 0; i < ttf->n_tables; i++)
        {
            length = ttf->dir[i].length;
            tt_seek(p, ttf, (long) ttf->dir[i].offset);
            tt_read(p, ttf, pos, length);
            ttf->dir[i].offset = (unsigned int) (pos - font->img);
            pos += length;
        }

        /* correct offsets in Table Directory */
        pos = font->img + TT_OFFSETTAB_SIZE + 2 * sizeof(tt_ulong);
        for (i = 0; i < ttf->n_tables; i++)
        {
            offset = ttf->dir[i].offset;
            pos[0] = (tt_byte) (offset >> 24);
            pos[1] = (tt_byte) (offset >> 16);
            pos[2] = (tt_byte) (offset >> 8);
            pos[3] = (tt_byte) (offset);
            pos += dirlen;
        }
        retval = pdc_true;
    }
    else
    {
        pdc_set_errmsg(p->pdc, PDF_E_TTC_NOTFOUND, fontname, filename, 0, 0);
        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);
    }

    ttf->check = pdc_false;
    pdf_cleanup_tt(p, ttf);
    return retval;
}

int
pdf_check_tt_font(PDF *p, const char *filename, const char *fontname,
                  pdc_font *font)
{
    pdc_file   *fp;
    tt_byte     img[TT_OFFSETTAB_SIZE];
    pdc_bool    ismem = pdc_false;
    tt_ulong    n_fonts = 0;
    int         retval = pdc_undef;

    if ((fp = pdf_fopen(p, filename, "font ", PDC_FILE_BINARY)) == NULL)
    {
	if (font->verbose_open)
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);
    }
    else
    {
        retval = pdc_false;
        if (PDC_OK_FREAD(fp, img, TT_OFFSETTAB_SIZE))
        {
            retval = pdf_test_tt_font(img, &n_fonts);
            if (retval == pdc_true)
            {
                if (n_fonts > 1)
                {
                    retval = pdf_check_and_read_ttc(p, fp, filename, fontname,
                                                    font, n_fonts);
                }
                else
                {
                    font->img = (pdc_byte *)
                                   pdc_freadall(fp, &font->filelen, &ismem);
                    pdc_fclose(fp);
                }
                if (retval == pdc_true)
                {
                    if (font->filelen == 0)
                    {
			pdc_set_errmsg(p->pdc,
			    PDC_E_IO_NODATA, filename, 0, 0, 0);

                        if (font->verbose)
			    pdc_error(p->pdc, -1, 0, 0, 0, 0);

                        retval = pdc_false;
                    }
                    else if (ismem)
                    {
                        font->imgname = pdc_strdup(p->pdc, filename);
                        pdf_lock_pvf(p, font->imgname);
                    }
                }
                fp = NULL;
            }
        }
        if (fp) pdc_fclose(fp);
    }

    return retval;
}


#endif /* PDF_TRUETYPE_SUPPORTED */
