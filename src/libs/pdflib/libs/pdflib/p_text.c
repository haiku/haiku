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

/* $Id: p_text.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib text routines
 *
 */

#define P_TEXT_C

#include "p_intern.h"
#include "p_font.h"

/* ------------------------ Text object operators ------------------------ */

static void
pdf_begin_text(PDF *p)
{
    if (p->contents != c_text)
    {
	p->contents = c_text;
	p->textparams_done = pdc_false;

	pdc_puts(p->out, "BT\n");

	/* BT resets the current point, text matrix, and line matrix */
	p->gstate[p->sl].x = (float) 0.0;
	p->gstate[p->sl].y = (float) 0.0;
    }
}

static void
pdf_put_textmatrix(PDF *p)
{
    pdc_matrix *m = &p->tstate[p->sl].m;

    pdc_printf(p->out, "%f %f %f %f %f %f Tm\n",
                       m->a, m->b, m->c, m->d, m->e, m->f);
    p->tstate[p->sl].me = p->tstate[p->sl].m.e;
    p->tstate[p->sl].potm = pdc_false;
}

void
pdf_end_text(PDF *p)
{
    if (p->contents != c_text)
	return;

    p->contents	= c_page;

    pdc_puts(p->out, "ET\n");
    p->tstate[p->sl].potm = pdc_true;
}

/* ------------------------ Text state operators ------------------------ */

/* Initialize the text state at the beginning of each page */
void
pdf_init_tstate(PDF *p)
{
    pdf_tstate *ts;

    ts = &p->tstate[p->sl];

    ts->c	= (float) 0;
    ts->w	= (float) 0;
    ts->h       = (float) 1;
    ts->l	= (float) 0;
    ts->f	= -1;
    ts->fs	= (float) 0;

    ts->m.a	= (float) 1;
    ts->m.b	= (float) 0;
    ts->m.c	= (float) 0;
    ts->m.d	= (float) 1;
    ts->m.e	= (float) 0;
    ts->m.f     = (float) 0;
    ts->me      = (float) 0;

    ts->mode	= 0;
    ts->rise	= (float) 0;

    ts->lm.a	= (float) 1;
    ts->lm.b	= (float) 0;
    ts->lm.c	= (float) 0;
    ts->lm.d	= (float) 1;
    ts->lm.e	= (float) 0;
    ts->lm.f    = (float) 0;

    ts->potm    = pdc_false;

    p->underline	= pdc_false;
    p->overline		= pdc_false;
    p->strikeout	= pdc_false;
    p->textparams_done	= pdc_false;
}

/* reset the text state */
void
pdf_reset_tstate(PDF *p)
{
    float ydirection = p->ydirection;
    p->ydirection = (float) 1.0;

    pdf_set_horiz_scaling(p, 100);
    pdf_set_leading(p, 0);
    pdf_set_text_rise(p, 0);
    pdf_end_text(p);

    p->ydirection = ydirection;
}

/* character spacing for justified lines */
void
pdf_set_char_spacing(PDF *p, float spacing)
{
    spacing *= p->ydirection;
    if (PDF_GET_STATE(p) == pdf_state_document)
    {
	p->tstate[p->sl].c = spacing;
	return;
    }

    /*
     * We take care of spacing values != 0 in the text output functions,
     * but must explicitly reset here.
     */
    if (spacing == (float) 0) {
	pdf_begin_text(p);
	pdc_printf(p->out, "%f Tc\n", spacing);
    }

    p->tstate[p->sl].c = spacing;
    p->textparams_done = pdc_false;
}

/* word spacing for justified lines */
void
pdf_set_word_spacing(PDF *p, float spacing)
{
    spacing *= p->ydirection;
    if (PDF_GET_STATE(p) == pdf_state_document)
    {
	p->tstate[p->sl].w = spacing;
	return;
    }

    /*
     * We take care of spacing values != 0 in the text output functions,
     * but must explicitly reset here.
     */
    if (spacing == (float) 0) {
	pdf_begin_text(p);
	pdc_printf(p->out, "%f Tw\n", spacing);
    }

    p->tstate[p->sl].w = spacing;
    p->textparams_done = pdc_false;
}

void
pdf_set_horiz_scaling(PDF *p, float scale)
{
    if (scale == (float) 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "horizscaling", pdc_errprintf(p->pdc, "%f", scale), 0, 0);

    scale *= p->ydirection;
    if (PDF_GET_STATE(p) == pdf_state_document)
    {
	p->tstate[p->sl].h = scale / (float) 100.0;
	return;
    }

    if (scale == 100 * p->tstate[p->sl].h && !PDF_FORCE_OUTPUT())
	return;

    pdf_begin_text(p);
    pdc_printf(p->out, "%f Tz\n", scale);

    p->tstate[p->sl].h = scale / (float) 100.0;
}

float
pdf_get_horiz_scaling(PDF *p)
{
   return (float) (p->ydirection * p->tstate[p->sl].h * 100);
}

void
pdf_set_leading(PDF *p, float l)
{
    l *= p->ydirection;
    if (l == p->tstate[p->sl].l && !PDF_FORCE_OUTPUT())
	return;

    p->tstate[p->sl].l = l;
    p->textparams_done = pdc_false;
}

void
pdf_set_text_rendering(PDF *p, int mode)
{
    if (mode < 0 || mode > PDF_LAST_TRMODE)
	pdc_error(p->pdc, PDC_E_ILLARG_INT,
	    "textrendering", pdc_errprintf(p->pdc, "%d", mode), 0, 0);

    if (mode == p->tstate[p->sl].mode && !PDF_FORCE_OUTPUT())
	return;

    pdf_begin_text(p);
    pdc_printf(p->out, "%d Tr\n", mode);

    p->tstate[p->sl].mode = mode;
}

void
pdf_set_text_rise(PDF *p, float rise)
{
    rise *= p->ydirection;
    if (PDF_GET_STATE(p) == pdf_state_document)
    {
	p->tstate[p->sl].rise = rise;
	return;
    }

    if (rise == p->tstate[p->sl].rise && !PDF_FORCE_OUTPUT())
	return;

    pdf_begin_text(p);
    pdc_printf(p->out, "%f Ts\n", rise);

    p->tstate[p->sl].rise = rise;
}

/* Text positioning operators */

PDFLIB_API void PDFLIB_CALL
PDF_set_text_matrix(PDF *p,
    float a, float b, float c, float d, float e, float f)
{
    static const char fn[] = "PDF_set_text_matrix";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_content | pdf_state_document),
	"(p[%p], %g, %g, %g, %g, %g, %g)\n", (void *) p, a, b, c, d, e, f))
    {
	return;
    }

    p->tstate[p->sl].m.a = a;
    p->tstate[p->sl].m.b = b;
    p->tstate[p->sl].m.c = c;
    p->tstate[p->sl].m.d = d;
    p->tstate[p->sl].m.e = e;
    p->tstate[p->sl].m.f = f;

    if (PDF_GET_STATE(p) != pdf_state_document)
    {
	pdf_begin_text(p);
        pdf_put_textmatrix(p);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_set_text_pos(PDF *p, float x, float y)
{
    static const char fn[] = "PDF_set_text_pos";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %g, %g)\n",
	(void *) p, x, y))
	return;

    p->tstate[p->sl].m.e = x;
    p->tstate[p->sl].m.f = y;

    p->tstate[p->sl].lm.e = x;
    p->tstate[p->sl].lm.f = y;

    pdf_begin_text(p);
    pdf_put_textmatrix(p);
}

/* String width calculations */

static float
pdf_str_width(PDF *p, pdc_byte *text, int len, int charlen,
              int font, float fontsize)
{
    int i;
    pdc_font *currfont = &p->fonts[font];
    pdc_ushort uv;
    pdc_ushort *ustext = (pdc_ushort *) text;
    float chwidth = (float) 0.0, width = (float) 0.0;
    pdc_bool haswidths = (currfont->widths != NULL);

    /* We cannot handle empty strings and fonts with unknown
     * code size - especially CID fonts with no UCS-2 CMap */
    if (!len || !currfont->codeSize)
        return width;

    for (i = 0; i < len/charlen; i++)
    {
        if (charlen == 1)
            uv = (pdc_ushort) text[i];
        else
            uv = ustext[i];

        /* take word spacing parameter into account at each blank */
        if (currfont->codeSize == 1 && uv == (pdc_ushort) PDF_SPACE)
            width += p->tstate[p->sl].w;

        /* start by adding in the width of the character */
        if (currfont->monospace)
        {
            chwidth = (float) currfont->monospace;
        }
        else if (haswidths)
        {
            chwidth = (float) currfont->widths[uv];
        }
        width += fontsize * chwidth / ((float) 1000);


	/* now add the character spacing parameter */
	width += p->tstate[p->sl].c;
    }

    /* take current text matrix and horizontal scaling factor into account */
    width = width * p->tstate[p->sl].m.a * p->tstate[p->sl].h;

    return width;
}

/* Convert and check input text string.
 * The function returns a pointer to an allocated buffer
 * containing the converted string.
 * The caller is responsible for freeing this buffer.
 * If return value is NULL error was occurred.
 */
static pdc_byte *
pdf_check_textstring(PDF *p, const char *text, int len, int font,
                     pdc_bool textflow, pdc_bool calcwidth,
                     int *outlen, int *outcharlen)
{
    pdc_font *currfont = &p->fonts[font];
    pdc_byte *outtext;
    pdc_text_format textformat, targettextformt;
    int maxlen, charlen = 1;
    int codesize = abs(currfont->codeSize);

    (void) textflow;

    /* Convert to 8-bit or UTF-16 text string */
    textformat = p->textformat;
    if (textformat == pdc_auto && codesize <= 1)
        textformat = pdc_bytes;
    else if (textformat == pdc_auto2 && codesize <= 1)
        textformat = pdc_bytes2;
    targettextformt = pdc_utf16;
    pdc_convert_string(p->pdc, textformat, NULL, (pdc_byte *)text, len,
                       &targettextformt, NULL, &outtext, outlen,
                       PDC_CONV_NOBOM | PDC_CONV_KEEPBYTES, pdc_true);
    textformat = targettextformt;

    if (outtext && *outlen)
    {
        /* 2 byte storage length of a character */
        if (textformat == pdc_utf16)
        {
            if (codesize <= 1 && currfont->encoding == pdc_builtin)
                pdc_error(p->pdc, PDF_E_FONT_BADTEXTFORM, 0, 0, 0, 0);
            charlen = 2;
        }

        /* Maximal text string length - found out emprirically! */
        maxlen = (codesize == 1) ? PDF_MAXARRAYSIZE : PDF_MAXDICTSIZE;
        if (!calcwidth && *outlen > maxlen)
        {
            pdc_warning(p->pdc, PDF_E_TEXT_TOOLONG,
                pdc_errprintf(p->pdc, "%d", maxlen), 0, 0, 0);
            *outlen = maxlen;
        }

    }
    *outcharlen = charlen;

    return outtext;
}

float
pdf__stringwidth(PDF *p, const char *text, int len, int font, float fontsize)
{
    pdc_byte *utext;
    int charlen;
    float result = (float) 0.0;

    if (text && len == 0)
        len = (int) strlen(text);
    if (text == NULL || len <= 0)
        return result;

    pdf_check_handle(p, font, pdc_fonthandle);

    if (fontsize == (float) 0.0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT, "fontsize", "0", 0, 0);

    /* convert text string */
    fontsize *= p->ydirection;
    utext = pdf_check_textstring(p, text, len, font, pdc_false, pdc_true,
                                 &len, &charlen);
    if (utext)
    {
        p->currtext = utext;
        result = pdf_str_width(p, utext, len, charlen, font, fontsize);
        p->currtext = NULL;
        pdc_free(p->pdc, utext);
    }
    return result;
}

PDFLIB_API float PDFLIB_CALL
PDF_stringwidth(PDF *p, const char *text, int font, float fontsize)
{
    static const char fn[] = "PDF_stringwidth";
    float result = (float) 0.0;

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content | pdf_state_path),
        "(p[%p], \"%s\", %d, %g)",
        (void *) p, pdc_strprint(p->pdc, text, 0), font, fontsize))
    {
        int len = text ? (int) strlen(text) : 0;
        PDF_INPUT_HANDLE(p, font)
        result = pdf__stringwidth(p, text, len, font, fontsize);
    }
    pdc_trace(p->pdc, "[%g]\n", result);
    return result;
}

PDFLIB_API float PDFLIB_CALL
PDF_stringwidth2(PDF *p, const char *text, int len, int font, float fontsize)
{
    static const char fn[] = "PDF_stringwidth2";
    float result = (float) 0.0;

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content | pdf_state_path),
        "(p[%p], \"%s\", %d, %d, %g)",
        (void *) p, pdc_strprint(p->pdc, text, len), len, font, fontsize))
    {
        PDF_INPUT_HANDLE(p, font)
        result = pdf__stringwidth(p, text, len, font, fontsize);
    }
    pdc_trace(p->pdc, "[%g]\n", result);
    return result;
}


/* ------------------------ Text control functions ------------------------ */

static void
pdf_underline(PDF *p, float x, float y, float length)
{
    float delta_y, xscale, yscale;
    double linewidth;
    pdf_tstate *ts;

    ts = &p->tstate[p->sl];

    xscale = (float) fabs((double) ts->m.a);
    yscale = (float) fabs((double) ts->m.d);

    linewidth = fabs(ts->fs * p->fonts[ts->f].underlineThickness / 1000.0 *
                     ts->h * (xscale > yscale ?  xscale : yscale));

    delta_y = (ts->fs * p->fonts[ts->f].underlinePosition / 1000 + ts->rise) *
            (float) fabs((double) ts->h) * (xscale > yscale ?  xscale : yscale);

    pdf__save(p);

    pdf__setlinewidth(p, (float) linewidth);
    pdf__setlinecap(p, 0);
    pdf__setdash(p, 0, 0);
    pdf__moveto(p, x,          y + delta_y);
    pdf__lineto(p, x + length, y + delta_y);
    pdf__stroke(p);

    pdf__restore(p);
}

static void
pdf_overline(PDF *p, float x, float y, float length)
{
    float delta_y, xscale, yscale, lineheight;
    double linewidth;
    pdf_tstate *ts;

    ts = &p->tstate[p->sl];

    xscale = (float) fabs((double) ts->m.a);
    yscale = (float) fabs((double) ts->m.d);

    linewidth = fabs(ts->fs * p->fonts[ts->f].underlineThickness / 1000.0 *
                     ts->h * (xscale > yscale ?  xscale : yscale));

    lineheight = ((float) p->fonts[ts->f].ascender/1000) * ts->fs;

    delta_y = (ts->fs * p->fonts[ts->f].underlinePosition / 1000 - ts->rise) *
	    (float) fabs((double) ts->h) * (xscale > yscale ?  xscale : yscale);

    pdf__save(p);

    pdf__setlinewidth(p, (float)linewidth);
    pdf__setlinecap(p, 0);
    pdf__setdash(p, 0, 0);
    pdf__moveto(p, x,          y + lineheight - delta_y);
    pdf__lineto(p, x + length, y + lineheight - delta_y);
    pdf__stroke(p);

    pdf__restore(p);
}

static void
pdf_strikeout(PDF *p, float x, float y, float length)
{
    float delta_y, xscale, yscale, lineheight;
    double linewidth;
    pdf_tstate *ts;

    ts = &p->tstate[p->sl];

    xscale = (float) fabs((double) ts->m.a);
    yscale = (float) fabs((double) ts->m.d);

    linewidth = fabs(ts->fs * p->fonts[ts->f].underlineThickness / 1000.0 *
                     ts->h * (xscale > yscale ?  xscale : yscale));

    lineheight = ((float) p->fonts[ts->f].ascender/1000) * ts->fs;

    delta_y = (ts->fs * p->fonts[ts->f].underlinePosition / 1000 + ts->rise) *
	    (float) fabs((double) ts->h) * (xscale > yscale ?  xscale : yscale);
    pdf__save(p);

    pdf__setlinewidth(p, (float)linewidth);
    pdf__setlinecap(p, 0);
    pdf__setdash(p, 0, 0);
    pdf__moveto(p, x,          y + lineheight/2 + delta_y);
    pdf__lineto(p, x + length, y + lineheight/2 + delta_y);
    pdf__stroke(p);

    pdf__restore(p);
}

/* ------------------------ font operator ------------------------ */

void
pdf__setfont(PDF *p, int font, float fontsize)
{
    /* make font the current font */
    p->fonts[font].used_on_current_page = pdc_true;
    p->tstate[p->sl].fs = p->ydirection * fontsize;
    p->tstate[p->sl].f = font;

    pdf_begin_text(p);
    pdc_printf(p->out, "/F%d %f Tf\n", font, p->tstate[p->sl].fs);
    pdf_set_leading(p, fontsize);
}

PDFLIB_API void PDFLIB_CALL
PDF_setfont(PDF *p, int font, float fontsize)
{
    static const char fn[] = "PDF_setfont";

    if (!pdf_enter_api(p, fn, pdf_state_content, "(p[%p], %d, %g)\n",
        (void *) p, font, fontsize))
        return;

    /* Check parameters */
    PDF_INPUT_HANDLE(p, font)
    pdf_check_handle(p, font, pdc_fonthandle);

    if (fontsize == (float) 0.0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT, "fontsize", "0", 0, 0);

    pdf__setfont(p, font, fontsize);
}

float
pdf_get_fontsize(PDF *p)
{
    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
	pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "fontsize", 0, 0, 0);


    return p->ydirection * p->tstate[p->sl].fs;
}

/* ------------------------ Text rendering operators ------------------------ */


void
pdf_put_textstring(PDF *p, pdc_byte *text, int len, int charlen)
{
    (void) charlen;

    pdc_put_pdfstring(p->out, (const char *) text, len);
}


static void
pdf_output_text(PDF *p, pdc_byte *text, int len, int charlen)
{
    {
        pdf_put_textstring(p, text, len, charlen);
        pdc_puts(p->out, "Tj\n");
    }
}

#define PDF_RENDERMODE_FILLCLIP 4

static void
pdf_place_text(PDF *p, pdc_byte *utext, int len, int charlen,
               float x, float y, float width, pdc_bool cont)
{
    pdf_tstate *ts = &p->tstate[p->sl];

    if (width)
    {
        if (p->underline)
            pdf_underline(p, x, y, width);
        if (p->overline)
            pdf_overline(p, x, y, width);
        if (p->strikeout)
            pdf_strikeout(p, x, y, width);
    }

    pdf_begin_text(p);
    if (ts->potm)
        pdf_put_textmatrix(p);

    /* set leading, word, and character spacing if required */
    if (!p->textparams_done)
    {
        pdc_printf(p->out, "%f TL\n", ts->l);

        if (ts->w != (float) 0)
            pdc_printf(p->out,"%f Tw\n", ts->w);

        if (ts->c != (float) 0)
            pdc_printf(p->out,"%f Tc\n", ts->c);

        p->textparams_done = pdc_true;
    }

    /* text output */
    if (!cont)
    {
        pdf_output_text(p, utext, len, charlen);
    }
    else
    {
        {
            pdf_put_textstring(p, utext, len, charlen);
            pdc_puts(p->out, "'\n");
        }
    }

    if (ts->mode >= PDF_RENDERMODE_FILLCLIP)
        pdf_end_text(p);

    ts->m.e += width;
    if (cont) ts->m.f -= ts->l;
}

void
pdf_show_text(
    PDF *p,
    const char *text,
    int len,
    const float *x_p,
    const float *y_p,
    pdc_bool cont)
{
    static const char *fn = "pdf_show_text";
    pdf_tstate *ts = &p->tstate[p->sl];
    pdc_byte *utext = NULL;
    int charlen = 1;
    float x, y, width;

    if (x_p != NULL)
    {
        x = *x_p;
        y = *y_p;
        ts->m.e = x;
        ts->m.f = y;
        ts->lm.e = x;
        ts->lm.f = y;
        ts->potm = pdc_true;
    }
    else if (cont)
    {
        ts->m.e = ts->lm.e;
        x = ts->m.e;
        y = ts->m.f - ts->l;

        /* we must output line begin if necessary */
        if (fabs((double) (ts->me - ts->lm.e)) > PDC_FLOAT_PREC)
            ts->potm = pdc_true;
    }
    else
    {
        x = ts->m.e;
        y = ts->m.f;
    }

    if (text && len == 0)
        len = (int) strlen(text);
    if (text == NULL || len <= 0)
    {
        if (cont)
            len = 0;
        else
            return;
    }

    /* no font set */
    if (ts->f == -1)
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT, 0, 0, 0, 0);

    width = 0;
    if (len)
    {
        /* convert text string */
        utext = pdf_check_textstring(p, text, len, ts->f, pdc_false, pdc_false,
                                     &len, &charlen);
        if (!utext)
            return;

        p->currtext = utext;

        /* length of text */
        width = pdf_str_width(p, utext, len, charlen, ts->f, ts->fs);
    }
    else
    {
        utext = (pdc_byte *) pdc_calloc(p->pdc, 2, fn);
        p->currtext = utext;
    }

    /* place text */
    pdf_place_text(p, utext, len, charlen, x, y, width, cont);

    p->currtext = NULL;
    if (utext)
        pdc_free(p->pdc, utext);
}

PDFLIB_API void PDFLIB_CALL
PDF_show(PDF *p, const char *text)
{
    static const char fn[] = "PDF_show";
    if (pdf_enter_api(p, fn, pdf_state_content, "(p[%p], \"%s\")\n",
        (void *) p, pdc_strprint(p->pdc, text, 0)))
    {
        int len = text ? (int) strlen(text) : 0;
        pdf_show_text(p, text, len, NULL, NULL, pdc_false);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_show2(PDF *p, const char *text, int len)
{
    static const char fn[] = "PDF_show2";
    if (pdf_enter_api(p, fn, pdf_state_content,
	"(p[%p], \"%s\", %d)\n",
        (void *) p, pdc_strprint(p->pdc, text, len), len))
    {
        pdf_show_text(p, text, len, NULL, NULL, pdc_false);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_show_xy(PDF *p, const char *text, float x, float y)
{
    static const char fn[] = "PDF_show_xy";
    if (pdf_enter_api(p, fn, pdf_state_content, "(p[%p], \"%s\", %g, %g)\n",
        (void *) p, pdc_strprint(p->pdc, text, 0), x, y))
    {
        int len = text ? (int) strlen(text) : 0;
        pdf_show_text(p, text, len, &x, &y, pdc_false);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_show_xy2(PDF *p, const char *text, int len, float x, float y)
{
    static const char fn[] = "PDF_show_xy2";
    if (pdf_enter_api(p, fn, pdf_state_content, "(p[%p], \"%s\", %d, %g, %g)\n",
        (void *) p, pdc_strprint(p->pdc, text, len), len, x, y))
    {
        pdf_show_text(p, text, len, &x, &y, pdc_false);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_continue_text(PDF *p, const char *text)
{
    static const char fn[] = "PDF_continue_text";
    if (pdf_enter_api(p, fn, pdf_state_content, "(p[%p], \"%s\")\n",
        (void *) p, pdc_strprint(p->pdc, text, 0)))
    {
        int len = text ? (int) strlen(text) : 0;
        pdf_show_text(p, text, len, NULL, NULL, pdc_true);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_continue_text2(PDF *p, const char *text, int len)
{
    static const char fn[] = "PDF_continue_text2";
    if (pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], \"%s\", %d)\n",
        (void *) p, pdc_strprint(p->pdc, text, len), len))
    {
        pdf_show_text(p, text, len, NULL, NULL, pdc_true);
    }
}


/* ----------------------- Text fitting routines ------------------------ */

#define PDF_KERNING_FLAG PDC_OPT_UNSUPP

/* definitions of fit text options */
static const pdc_defopt pdf_fit_textline_options[] =
{
    {"font", pdc_fonthandle, PDC_OPT_NONE, 1, 1,
      0, 0, NULL},

    {"fontsize", pdc_floatlist, PDC_OPT_REQUIRIF1 | PDC_OPT_NOZERO, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"textrendering", pdc_integerlist, PDC_OPT_NONE, 1, 1,
      0, PDF_LAST_TRMODE, NULL},

    {"wordspacing", pdc_floatlist, PDC_OPT_NONE, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"charspacing", pdc_floatlist, PDC_OPT_NONE, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"horizscaling", pdc_floatlist,  PDC_OPT_NOZERO, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"textrise", pdc_floatlist, PDC_OPT_NONE, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"underline", pdc_booleanlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, NULL},

    {"overline", pdc_booleanlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, NULL},

    {"strikeout", pdc_booleanlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, NULL},

    {"kerning", pdc_booleanlist, PDF_KERNING_FLAG, 1, 1,
      0.0, 0.0, NULL},

    {"textformat", pdc_keywordlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, pdf_textformat_keylist},

    {"margin", pdc_floatlist, PDC_OPT_NONE, 1, 2,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"orientate", pdc_keywordlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, pdf_orientate_keylist},

    {"boxsize", pdc_floatlist, PDC_OPT_NONE, 2, 2,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"rotate", pdc_floatlist, PDC_OPT_NONE, 1, 1,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"position", pdc_floatlist, PDC_OPT_NONE, 1, 2,
      PDC_FLOAT_MIN, PDC_FLOAT_MAX, NULL},

    {"fitmethod", pdc_keywordlist, PDC_OPT_NONE, 1, 1,
      0.0, 0.0, pdf_fitmethod_keylist},

    {"distortionlimit", pdc_floatlist, PDC_OPT_NONE, 1, 1,
      0.0, 100.0, NULL},

    PDC_OPT_TERMINATE
};

void
pdf__fit_textline(PDF *p, const char *text, int len, float x, float y,
                  const char *optlist)
{
    pdc_byte *utext = (pdc_byte *) "";
    pdc_resopt *results;
    pdc_clientdata data;
    pdc_text_format textformat_save = p->textformat;
    pdc_bool underline_save = p->underline;
    pdc_bool overline_save = p->overline;
    pdc_bool strikeout_save = p->strikeout;
    int charlen;

    int font;
    float fontsize;
    int textrendering, ntextrendering;
    float textrise, wordspacing, charspacing, horizscaling;
    int nfontsize, ntextrise, nwordspacing, ncharspacing, nhorizscaling;
    float margin[2], boxsize[2], position[2], angle, distortionlimit;
    pdc_fitmethod method;
    int inum, orientangle, indangle;

    if (text && len == 0)
        len = (int) strlen(text);
    if (text == NULL || len <= 0)
        return;

    /* defaults */
    font = p->tstate[p->sl].f;
    fontsize = p->tstate[p->sl].fs;
    orientangle = 0;
    margin[0] = margin[1] = (float) 0.0;
    boxsize[0] = boxsize[1] = (float) 0.0;
    position[0] = position[1] = (float) 0.0;
    angle = (float) 0.0;
    method = pdc_nofit;
    distortionlimit = (float) 75.0;

    /* parsing optlist */
    data.compatibility = p->compatibility;
    data.maxfont = p->fonts_number - 1;
    data.hastobepos = p->hastobepos;
    results = pdc_parse_optionlist(p->pdc, optlist, pdf_fit_textline_options,
                                   &data, pdc_true);

    /* save options */
    pdc_get_optvalues(p->pdc, "font", results, &font, NULL);

    nfontsize = pdc_get_optvalues(p->pdc, "fontsize", results,
                                  &fontsize, NULL);

    ntextrendering = pdc_get_optvalues(p->pdc, "textrendering", results,
                                       &textrendering, NULL);

    nwordspacing = pdc_get_optvalues(p->pdc, "wordspacing", results,
                                     &wordspacing, NULL);

    ncharspacing = pdc_get_optvalues(p->pdc, "charspacing", results,
                                     &charspacing, NULL);

    nhorizscaling = pdc_get_optvalues(p->pdc, "horizscaling", results,
                                      &horizscaling, NULL);

    ntextrise = pdc_get_optvalues(p->pdc, "textrise", results,
                                  &textrise, NULL);

    pdc_get_optvalues(p->pdc, "underline", results, &p->underline, NULL);

    pdc_get_optvalues(p->pdc, "overline", results, &p->overline, NULL);

    pdc_get_optvalues(p->pdc, "strikeout", results, &p->strikeout, NULL);


    if (pdc_get_optvalues(p->pdc, "textformat", results, &inum, NULL))
        p->textformat = (pdc_text_format) inum;

    if (1 == pdc_get_optvalues(p->pdc, "margin", results, margin, NULL))
        margin[1] = margin[0];

    pdc_get_optvalues(p->pdc, "orientate", results, &orientangle, NULL);

    pdc_get_optvalues(p->pdc, "boxsize", results, boxsize, NULL);

    pdc_get_optvalues(p->pdc, "rotate", results, &angle, NULL);

    pdc_get_optvalues(p->pdc, "distortionlimit", results,
                      &distortionlimit, NULL);

    if (1 == pdc_get_optvalues(p->pdc, "position", results, position, NULL))
        position[1] = position[0];

    if (pdc_get_optvalues(p->pdc, "fitmethod", results, &inum, NULL))
        method = (pdc_fitmethod) inum;

    pdc_cleanup_optionlist(p->pdc, results);

    /* save graphic state */
    pdf__save(p);

    while (1)
    {
        pdf_tstate *ts = &p->tstate[p->sl];
        pdc_matrix m;
        pdc_vector elemsize, elemscale, elemmargin, relpos, polyline[5];
        pdc_box fitbox, elembox;
        pdc_scalar ss, minfscale;
        float width, height, tx = 0, ty = 0;

        /* font size */
        if (nfontsize)
        {
            pdf__setfont(p, font, fontsize);
            fontsize = p->tstate[p->sl].fs;
        }

        /* no font set */
        if (p->tstate[p->sl].f == -1)
            pdc_error(p->pdc, PDF_E_TEXT_NOFONT, 0, 0, 0, 0);

        /* convert text string */
        utext = pdf_check_textstring(p, text, len, font, pdc_false, pdc_false,
                                     &len, &charlen);
        if (utext == NULL || len == 0)
            break;
        p->currtext = utext;

        /* text rendering */
        if (ntextrendering)
            pdf_set_text_rendering(p, textrendering);

        /* options for text width and height */
        if (nwordspacing)
            pdf_set_word_spacing(p, wordspacing);
        if (ncharspacing)
            pdf_set_char_spacing(p, charspacing);
        if (nhorizscaling)
            pdf_set_horiz_scaling(p, horizscaling);
        if (ntextrise)
            pdf_set_text_rise(p, textrise);

        /* minimal horizontal scaling factor */
        minfscale = distortionlimit / 100.0;

        /* text width */
        width = pdf_str_width(p, utext, len, charlen, font, fontsize);
        if (width < PDF_SMALLREAL)
            break;
        elemmargin.x = margin[0];
        elemsize.x = width + 2 * elemmargin.x;

        /* text height */
        height = (float) fabs((p->fonts[ts->f].capHeight / 1000.0f) * ts->fs);
        elemmargin.y = margin[1];
        elemsize.y = height + 2 * elemmargin.y;

        /* orientation */
        indangle = orientangle / 90;
        if (indangle % 2)
        {
            ss = elemsize.x;
            elemsize.x = elemsize.y;
            elemsize.y = ss;
        }

        /* box for fitting */
        fitbox.ll.x = 0;
        fitbox.ll.y = 0;
        fitbox.ur.x = boxsize[0];
        fitbox.ur.y = boxsize[1];

        /* relative position */
        relpos.x = position[0] / 100.0;
        relpos.y = position[1] / 100.0;

        /* calculate image box */
        pdc_place_element(method, minfscale, &fitbox, &relpos,
                          &elemsize, &elembox, &elemscale);

        /* reference point */
        pdc_translation_matrix(x, y, &m);
        pdf_concat_raw(p, &m);

        /* clipping */
        if (method == pdc_clip || method == pdc_slice)
        {
            pdf__rect(p, 0, 0, boxsize[0], boxsize[1]);
            pdf__clip(p);
        }

        /* optional rotation */
        if (fabs((double)(angle)) > PDC_FLOAT_PREC)
        {
            pdc_rotation_matrix(p->ydirection * angle, &m);
            pdf_concat_raw(p, &m);
        }

        /* translation of element box */
        elembox.ll.y *= p->ydirection;
        elembox.ur.y *= p->ydirection;
        pdc_box2polyline(&elembox, polyline);
        tx = (float) polyline[indangle].x;
        ty = (float) polyline[indangle].y;
        pdc_translation_matrix(tx, ty, &m);
        pdf_concat_raw(p, &m);

        /* orientation of text */
        if (orientangle != 0)
        {
            pdc_rotation_matrix(p->ydirection * orientangle, &m);
            pdf_concat_raw(p, &m);
            if (indangle % 2)
            {
                ss = elemscale.x;
                elemscale.x = elemscale.y;
                elemscale.y = ss;
            }
        }

        if (elemscale.x != 1 || elemscale.y != 1)
        {
            pdc_scale_matrix((float) elemscale.x, (float) elemscale.y, &m);
            pdf_concat_raw(p, &m);
        }

        /* place text */
        x = (float) elemmargin.x;
        y = (float) elemmargin.y;
        p->tstate[p->sl].m.e = x;
        p->tstate[p->sl].m.f = y;
        p->tstate[p->sl].lm.e = x;
        p->tstate[p->sl].lm.f = y;
        p->tstate[p->sl].potm = pdc_true;
        pdf_place_text(p, utext, len, charlen, x, y, width, pdc_false);

        break;
    }

    /* restore graphic state */
    pdf__restore(p);
    p->underline = underline_save;
    p->overline = overline_save;
    p->strikeout = strikeout_save;
    p->textformat = textformat_save;
    p->currtext = NULL;
    if (utext)
        pdc_free(p->pdc, utext);
}

PDFLIB_API void PDFLIB_CALL
PDF_fit_textline(PDF *p, const char *text, int len, float x, float y,
                 const char *optlist)
{
    static const char fn[] = "PDF_fit_textline";

    if (pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], \"%s\", %d, %g, %g, \"%s\")\n",
        (void *) p, pdc_strprint(p->pdc, text, len), len, x, y, optlist))
    {
        pdf__fit_textline(p, text, len, x, y, optlist);
    }
}

/* ----------------------- Text formatting routines ------------------------ */

/* text alignment modes */
typedef enum { pdf_align_left, pdf_align_right, pdf_align_center,
	pdf_align_justify, pdf_align_fulljustify
} pdf_alignment;

/* this helper function returns the width of the null-terminated string
** 'text' for the current font and size EXCLUDING the last character's
** additional charspacing.
*/
static float
pdf_swidth(PDF *p, const char *text)
{
    pdf_tstate	*ts = &p->tstate[p->sl];

    return pdf_str_width(p, (pdc_byte *)text, (int) strlen(text), 1,
                         ts->f, ts->fs) - ts->c * ts->m.a * ts->h;
}

static void
pdf_show_aligned(PDF *p, const char *text, float x, float y, pdf_alignment mode)
{
    if (!text)
	return;

    switch (mode) {
	case pdf_align_left:
	case pdf_align_justify:
	case pdf_align_fulljustify:
	    /* nothing extra here... */
	    break;

	case pdf_align_right:
	    x -= pdf_swidth(p, text);
	    break;

	case pdf_align_center:
	    x -= pdf_swidth(p, text) / 2;
	    break;
    }

    pdf_show_text(p, text, (int) strlen(text), &x, &y, pdc_false);
}

static int
pdf__show_boxed(
    PDF *p,
    const char *text,
    int totallen,
    float left,
    float bottom,
    float width,
    float height,
    const char *hmode,
    const char *feature)
{
    float	textwidth, old_word_spacing, curx, cury;
    pdc_bool	prematureexit;	/* return because box is too small */
    int		curTextPos;	/* character currently processed */
    int		lastdone;	/* last input character processed */
    int         toconv = totallen;
    pdf_tstate *ts = &p->tstate[p->sl];
    pdc_byte *utext = NULL;
    pdc_text_format textformat = p->textformat;
    pdf_alignment mode = pdf_align_left;
    pdc_bool blind = pdc_false;

    if (hmode == NULL || *hmode == '\0')
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "hmode", 0, 0, 0);

    if (!strcmp(hmode, "left"))
	mode = pdf_align_left;
    else if (!strcmp(hmode, "right"))
	mode = pdf_align_right;
    else if (!strcmp(hmode, "center"))
	mode = pdf_align_center;
    else if (!strcmp(hmode, "justify"))
	mode = pdf_align_justify;
    else if (!strcmp(hmode, "fulljustify"))
	mode = pdf_align_fulljustify;
    else
	pdc_error(p->pdc, PDC_E_ILLARG_STRING, "hmode", hmode, 0, 0);

    if (feature != NULL && *feature != '\0')
    {
	if (!strcmp(feature, "blind"))
	    blind = pdc_true;
	else
	    pdc_error(p->pdc, PDC_E_ILLARG_STRING, "feature", feature, 0, 0);
    }

    /* no font set */
    if (ts->f == -1)
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT, 0, 0, 0, 0);

    if (width == 0 && height != 0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
            "width", pdc_errprintf(p->pdc, "%f", width), 0, 0);

    if (width != 0 && height == 0)
        pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
            "height", pdc_errprintf(p->pdc, "%f", height), 0, 0);

    /* we cannot handle several encodings */
    if (p->fonts[ts->f].encoding == pdc_unicode)
    {
        pdc_error(p->pdc, PDF_E_DOC_FUNCUNSUPP, "Unicode", 0, 0, 0);
    }

    if (p->fonts[ts->f].encoding == pdc_glyphid)
    {
        pdc_error(p->pdc, PDF_E_DOC_FUNCUNSUPP, "glyphid", 0, 0, 0);
    }

    if (p->fonts[ts->f].encoding == pdc_cid)
    {
	pdc_error(p->pdc, PDF_E_DOC_FUNCUNSUPP, "CID", 0, 0, 0);
    }

    if (p->fonts[ts->f].encoding == pdc_ebcdic)
    {
	pdc_error(p->pdc, PDF_E_DOC_FUNCUNSUPP, "EBCDIC", 0, 0, 0);
    }

    /* convert text string */
    if (toconv)
    {
        int charlen;
        utext = pdf_check_textstring(p, text, totallen, ts->f, pdc_true,
                                     pdc_true, &totallen, &charlen);
        if (!utext)
            return 0;

        utext[totallen] = 0;
        text = (const char *) utext;
        p->textformat = pdc_bytes;
    }

    /* text length */
    if (!totallen)
        totallen = (int) strlen(text);

    /* special case for a single aligned line */
    if (width == 0 && height == 0)
    {
        if (!blind)
            pdf_show_aligned(p, text, left, bottom, mode);

        if (toconv)
        {
            pdc_free(p->pdc, utext);
            p->textformat = textformat;
        }
        return 0;
    }

    old_word_spacing = ts->w;

    curx = left;
    cury = bottom + p->ydirection * height;
    prematureexit = pdc_false;
    curTextPos = 0;
    lastdone = 0;

    /* switch curx for right and center justification */
    if (mode == pdf_align_right)
	curx += width;
    else if (mode == pdf_align_center)
	curx += (width / 2);

#define	MAX_CHARS_IN_LINE	2048

    /* loop until all characters processed, or box full */

    while ((curTextPos < totallen) && !prematureexit) {
	/* buffer for constructing the line */
	char	linebuf[MAX_CHARS_IN_LINE];
	int	curCharsInLine = 0;	/* # of chars in constructed line */
	int	lastWordBreak = 0;	/* the last seen space char */
	int	wordBreakCount = 0;	/* # of blanks in this line */

	/* loop over the input string */
	while (curTextPos < totallen) {
	    if (curCharsInLine >= MAX_CHARS_IN_LINE)
		pdc_error(p->pdc, PDC_E_ILLARG_TOOLONG, "(text line)",
		    pdc_errprintf(p->pdc, "%d", MAX_CHARS_IN_LINE-1), 0, 0);

	    /* abandon DOS line-ends */
	    if (text[curTextPos] == PDF_RETURN &&
		text[curTextPos+1] == PDF_NEWLINE)
		    curTextPos++;

	    /* if it's a forced line break draw the line */
	    if (text[curTextPos] == PDF_NEWLINE ||
		text[curTextPos] == PDF_RETURN) {

		cury -= ts->l;			/* adjust cury by leading */

                if (p->ydirection * (cury - bottom) < 0) {
		    prematureexit = pdc_true;	/* box full */
		    break;
		}

                linebuf[curCharsInLine] = 0;    /* terminate the line */

		/* check whether the line is too long */
		ts->w = (float) 0.0;
                textwidth = pdf_swidth(p, linebuf);

		/* the forced break occurs too late for this line */
		if (textwidth > width) {
		    if (wordBreakCount == 0) {	/* no blank found */
			prematureexit = pdc_true;
			break;
		    }
                    linebuf[lastWordBreak] = 0;   /* terminate at last blank */
		    if (curTextPos > 0 && text[curTextPos-1] == PDF_RETURN)
			--curTextPos;
		    curTextPos -= (curCharsInLine - lastWordBreak);

		    if (!blind) {
                        textwidth = pdf_swidth(p, linebuf);
			if (wordBreakCount != 1 &&
				(mode == pdf_align_justify ||
				mode == pdf_align_fulljustify)) {
			    pdf_set_word_spacing(p,
				p->ydirection * (width - textwidth) /
				((wordBreakCount - 1) * ts->h * ts->m.a));
			}
			else
			    pdf_set_word_spacing(p, (float) 0.0);
                        pdf_show_aligned(p, linebuf, curx, cury, mode);
		    }

		} else if (!blind) {

		    if (mode == pdf_align_fulljustify && wordBreakCount > 0) {
			pdf_set_word_spacing(p,
                            p->ydirection * (width - textwidth) /
			    (wordBreakCount * ts->h * ts->m.a));
		    } else {
			pdf_set_word_spacing(p, (float) 0.0);
		    }

                    pdf_show_aligned(p, linebuf, curx, cury, mode);
		}

		lastdone = curTextPos;
		curCharsInLine = lastWordBreak = wordBreakCount = 0;
		curTextPos++;

	    } else if (text[curTextPos] == PDF_SPACE) {
                linebuf[curCharsInLine] = 0;    /* terminate the line */
		ts->w = (float) 0.0;

		/* line too long ==> break at last blank */
                if (pdf_swidth(p, linebuf) > width) {
		    cury -= ts->l;		/* adjust cury by leading */

                    if (p->ydirection * (cury - bottom) < 0) {
			prematureexit = pdc_true; 	/* box full */
			break;
		    }

                    linebuf[lastWordBreak] = 0; /* terminate at last blank */
		    curTextPos -= (curCharsInLine - lastWordBreak - 1);

		    if (lastWordBreak == 0)
			curTextPos--;

		    /* LATER: * force break if wordBreakCount == 1,
		     * i.e., no blank
		     */
		    if (wordBreakCount == 0) {
			prematureexit = pdc_true;
			break;
		    }

		    /* adjust word spacing for full justify */
		    if (wordBreakCount != 1 && (mode == pdf_align_justify ||
					    mode == pdf_align_fulljustify)) {
			ts->w = (float) 0.0;
                        textwidth = pdf_swidth(p, linebuf);
			if (!blind) {
			    pdf_set_word_spacing(p,
                                p->ydirection * (width - textwidth) /
				((wordBreakCount - 1) * ts->h * ts->m.a));
			}
		    }
		    else if (!blind) {
			pdf_set_word_spacing(p, (float) 0.0);
		    }


		    lastdone = curTextPos;
		    if (!blind)
                        pdf_show_aligned(p, linebuf, curx, cury, mode);
		    curCharsInLine = lastWordBreak = wordBreakCount = 0;

		} else {
		    /* blank found, and line still fits */
		    wordBreakCount++;
		    lastWordBreak = curCharsInLine;
		    linebuf[curCharsInLine++] = text[curTextPos++];
		}

	    } else {
		/* regular character ==> store in buffer */
		linebuf[curCharsInLine++] = text[curTextPos++];
	    }
	}

	if (prematureexit) {
	    break;		/* box full */
	}

	/* if there is anything left in the buffer, draw it */
	if (curTextPos >= totallen && curCharsInLine != 0) {
	    cury -= ts->l;		/* adjust cury for line height */

            if (p->ydirection * (cury - bottom) < 0) {
		prematureexit = pdc_true; 	/* box full */
		break;
	    }

            linebuf[curCharsInLine] = 0;        /* terminate the line */

	    /* check if the last line is too long */
	    ts->w = (float) 0.0;
            textwidth = pdf_swidth(p, linebuf);

	    if (textwidth > width) {
		if (wordBreakCount == 0)
		{
		    prematureexit = pdc_true;
		    break;
		}

                linebuf[lastWordBreak] = 0;     /* terminate at last blank */
		curTextPos -= (curCharsInLine - lastWordBreak - 1);

		/* recalculate the width */
                textwidth = pdf_swidth(p, linebuf);

		/* adjust word spacing for full justify */
		if (wordBreakCount != 1 && (mode == pdf_align_justify ||
					    mode == pdf_align_fulljustify)) {
		    if (!blind) {
			pdf_set_word_spacing(p,
                            p->ydirection * (width - textwidth) /
			    ((wordBreakCount - 1) * ts->h * ts->m.a));
		    }
		}

	    } else if (!blind) {

		if (mode == pdf_align_fulljustify && wordBreakCount) {
		    pdf_set_word_spacing(p,
                            p->ydirection * (width - textwidth) /
			    (wordBreakCount * ts->h * ts->m.a));
		} else {
		    pdf_set_word_spacing(p, (float) 0.0);
		}
	    }

	    lastdone = curTextPos;
	    if (!blind)
                pdf_show_aligned(p, linebuf, curx, cury, mode);
	    curCharsInLine = lastWordBreak = wordBreakCount = 0;
	}
    }

    if (!blind)
	pdf_set_word_spacing(p, old_word_spacing);

    /* return number of remaining characters  */

    while (text[lastdone] == PDF_SPACE)
	++lastdone;

    if ((text[lastdone] == PDF_RETURN ||
	text[lastdone] == PDF_NEWLINE) && text[lastdone+1] == 0)
	    ++lastdone;

    if (toconv)
    {
        pdc_free(p->pdc, utext);
        p->textformat = textformat;
    }

    return (int) (totallen - lastdone);
}

PDFLIB_API int PDFLIB_CALL
PDF_show_boxed(
    PDF *p,
    const char *text,
    float left,
    float bottom,
    float width,
    float height,
    const char *hmode,
    const char *feature)
{
    static const char fn[] = "PDF_show_boxed";
    int remchars;

    if (!pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], \"%s\", %g, %g, %g, %g, \"%s\", \"%s\")",
        (void *) p, pdc_strprint(p->pdc, text, 0),
        left, bottom, width, height, hmode, feature))
    {
        return 0;
    }

    if (text == NULL || *text == '\0') {
        pdc_trace(p->pdc, "[%d]\n", 0);
        return 0;
    }

    remchars = pdf__show_boxed(p, text, 0, left, bottom,
                              width, height, hmode, feature);

    pdc_trace(p->pdc, "[%d]\n", remchars);
    return remchars;
}

PDFLIB_API int PDFLIB_CALL
PDF_show_boxed2(
    PDF *p,
    const char *text,
    int len,
    float left,
    float bottom,
    float width,
    float height,
    const char *hmode,
    const char *feature)
{
    static const char fn[] = "PDF_show_boxed2";
    int remchars;

    if (!pdf_enter_api(p, fn, pdf_state_content,
        "(p[%p], \"%s\", %d, %g, %g, %g, %g, \"%s\", \"%s\")",
        (void *) p, pdc_strprint(p->pdc, text, len), len,
        left, bottom, width, height, hmode, feature))
    {
        return 0;
    }

    if (text == NULL || len == 0) {
        pdc_trace(p->pdc, "[%d]\n", 0);
        return 0;
    }

    remchars = pdf__show_boxed(p, text, len, left, bottom,
                               width, height, hmode, feature);

    pdc_trace(p->pdc, "[%d]\n", remchars);
    return remchars;
}

