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

/* $Id: p_font.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib font handling routines
 *
 */

#define P_FONT_C

#include "p_intern.h"
#include "p_font.h"
#include "p_truetype.h"


static const pdc_keyconn pdf_extension_names[] =
{
    {".ttf", 1},
    {".otf", 1},
    {".afm", 2},
    {".pfm", 3},
    {".ttc", 1},
    {".TTF", 1},
    {".OTF", 1},
    {".AFM", 2},
    {".PFM", 3},
    {".TTC", 1},
    {".pfa", 4},
    {".pfb", 4},
    {".PFA", 4},
    {".PFB", 4},
    {NULL, 0}
};

static const pdc_keyconn pdf_fontstyle_pdfkeys[] =
{
    {"Normal",     pdc_Normal},
    {"Bold",       pdc_Bold},
    {"Italic",     pdc_Italic},
    {"BoldItalic", pdc_BoldItalic},
    {NULL, 0}
};

const char *
pdf_get_fontname(PDF *p)
{
    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "fontname", 0, 0, 0);

    return p->fonts[p->tstate[p->sl].f].name;
}

const char *
pdf_get_fontstyle(PDF *p)
{
    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "fontstyle", 0, 0, 0);

    return pdc_get_keyword(p->fonts[p->tstate[p->sl].f].style,
                           pdf_fontstyle_keylist);
}

const char *
pdf_get_fontencoding(PDF *p)
{
    pdc_font *font;
    const char *ret;

    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "fontencoding", 0, 0, 0);

    font = &p->fonts[p->tstate[p->sl].f];
    switch (font->encoding)
    {
        case pdc_cid:
            ret = (const char *) font->cmapname;
            break;

        default:
            ret = pdf_get_encoding_name(p, font->encoding);
    }

    return ret;
}

int
pdf_get_monospace(PDF *p)
{
    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "fontstyle", 0, 0, 0);

    return p->fonts[p->tstate[p->sl].f].monospace;
}

int
pdf_get_font(PDF *p)
{
    if (p->fonts_number == 0 || p->tstate[p->sl].f == -1) /* no font set */
        pdc_error(p->pdc, PDF_E_TEXT_NOFONT_PAR, "font", 0, 0, 0);

    return p->tstate[p->sl].f;
}

static void
pdf_cleanup_font(PDF *p, pdc_font *font)
{
    if (font->imgname)
        pdf_unlock_pvf(p, font->imgname);
    pdc_cleanup_font_struct(p->pdc, font);
}

void
pdf_cleanup_fonts(PDF *p)
{
    int slot;

    if (p->fonts)
    {
        for (slot = 0; slot < p->fonts_number; slot++)
            pdf_cleanup_font(p, &p->fonts[slot]);

        if (p->fonts)
            pdc_free(p->pdc, p->fonts);
        p->fonts = NULL;
    }
}

void
pdf_init_fonts(PDF *p)
{
    p->fonts_number     = 0;
    p->fonts_capacity   = FONTS_CHUNKSIZE;

    p->fonts = (pdc_font *) pdc_calloc(p->pdc,
                sizeof(pdc_font) * p->fonts_capacity, "pdf_init_fonts");

    p->t3font = (pdc_t3font *) NULL;

    pdf_init_encoding_ids(p);
}

void
pdf_grow_fonts(PDF *p)
{
    p->fonts = (pdc_font *) pdc_realloc(p->pdc, p->fonts,
                sizeof(pdc_font) * 2 * p->fonts_capacity, "pdf_grow_fonts");

    p->fonts_capacity *= 2;
}

int
pdf_init_newfont(PDF *p)
{
    pdc_font *font;
    int slot;

    /* slot for font struct */
    slot = p->fonts_number;
    if (slot >= p->fonts_capacity)
        pdf_grow_fonts(p);

    /* initialize font struct */
    font = &p->fonts[slot];
    pdc_init_font_struct(p->pdc, font);

    /* inherite global settings */
    font->verbose = p->debug['F'];
    font->verbose_open = font->verbose;

    return slot;
}

static pdc_bool
pdf_get_metrics_core(PDF *p, pdc_font *font, const char *fontname,
                     pdc_encoding enc)
{
    const pdc_core_metric *metric;

    metric = pdc_get_core_metric(fontname);
    if (metric != NULL)
    {
        /* Fill up the font struct */
        pdc_fill_font_metric(p->pdc, font, metric);
        font->encoding = enc;

        /* Process metric data */
        if (pdf_process_metrics_data(p, font, fontname))
        {
            if (!pdf_make_fontflag(p, font))
                return pdc_false;

            if (font->monospace)
            {
                pdc_set_errmsg(p->pdc, PDC_E_OPT_IGNORED, "monospace", 0, 0, 0);
                if (font->verbose == pdc_true)
                    pdc_error(p->pdc, -1, 0, 0, 0, 0);
                return pdc_false;
            }
            return pdc_true;
        }
    }
    return pdc_undef;
}

pdc_bool
pdf_make_fontflag(PDF *p, pdc_font *font)
{
    (void) p;

    if (font->type != pdc_Type3)
    {
        if (font->isFixedPitch)
            font->flags |= FIXEDWIDTH;

        if (font->isstdlatin == pdc_true ||
            font->encoding == pdc_winansi ||
            font->encoding == pdc_macroman ||
            font->encoding == pdc_ebcdic)
            font->flags |= ADOBESTANDARD;
        else
            font->flags |= SYMBOL;

        if (font->italicAngle < 0 ||
            font->style == pdc_Italic || font->style == pdc_BoldItalic)
            font->flags |= ITALIC;
        if (font->italicAngle == 0 && font->flags & ITALIC)
            font->italicAngle = PDC_DEF_ITALICANGLE;

        /* heuristic to identify (small) caps fonts */
        if (font->name &&
            (strstr(font->name, "Caps") ||
            !strcmp(font->name + strlen(font->name) - 2, "SC")))
            font->flags |= SMALLCAPS;

        if (font->style == pdc_Bold || font->style == pdc_BoldItalic)
            font->StdVW = PDF_STEMV_BOLD;

        if (strstr(font->name, "Bold") || font->StdVW > PDF_STEMV_SEMIBOLD)
            font->flags |= FORCEBOLD;
    }

    if (font->style != pdc_Normal &&
        (font->embedding || font->type == pdc_Type1 ||
         font->type == pdc_MMType1 || font->type == pdc_Type3))
    {
        pdc_set_errmsg(p->pdc, PDC_E_OPT_IGNORED, "fontstyle", 0, 0, 0);
        if (font->verbose == pdc_true)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);
        return pdc_false;
    }
    return pdc_true;
}

#define PDF_KERNING_FLAG PDC_OPT_UNSUPP

#define PDF_SUBSETTING_FLAG  PDC_OPT_UNSUPP
#define PDF_AUTOSUBSETT_FLAG PDC_OPT_UNSUPP

#define PDF_AUTOCIDFONT_FLAG PDC_OPT_UNSUPP

/* definitions of image options */
static const pdc_defopt pdf_load_font_options[] =
{
    {"fontwarning", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"embedding", pdc_booleanlist, 0, 1, 1, 0.0, 0.0, NULL},

    {"kerning", pdc_booleanlist, PDF_KERNING_FLAG, 1, 1, 0.0, 0.0, NULL},

    {"subsetting", pdc_booleanlist, PDF_SUBSETTING_FLAG, 1, 1, 0.0, 0.0,
     NULL},

    {"autosubsetting", pdc_booleanlist, PDF_AUTOSUBSETT_FLAG, 1, 1, 0.0, 0.0,
     NULL},

    {"subsetlimit", pdc_floatlist, PDF_SUBSETTING_FLAG, 1, 1, 0.0, 100.0,
     NULL},

    {"subsetminsize", pdc_floatlist, PDF_SUBSETTING_FLAG, 1, 1,
     0.0, PDC_FLOAT_MAX, NULL},

    {"autocidfont", pdc_booleanlist, PDF_AUTOCIDFONT_FLAG, 1, 1, 0.0, 0.0,
     NULL},

    {"unicodemap", pdc_booleanlist, PDF_AUTOCIDFONT_FLAG, 1, 1, 0.0, 0.0,
     NULL},

    {"fontstyle", pdc_keywordlist, 0, 1, 1, 0.0, 0.0,
     pdf_fontstyle_keylist},

    {"monospace", pdc_integerlist, 0, 1, 1, 1.0, 2048.0,
     NULL},

    PDC_OPT_TERMINATE
};

int
pdf__load_font(PDF *p, const char *fontname, int inlen,
               const char *encoding, const char *optlist)
{
    static const char fn[] = "pdf__load_font";
    pdc_resopt *results;
    pdc_encoding enc = pdc_invalidenc;
    pdc_bool fontwarning;
    pdc_bool kret;
    int slot = -1;
    size_t len;
    int i;
    int retval, inum;
    pdc_font *font;
    const char *extension = NULL;
    char *fontname_p = NULL;
    char *outfilename = NULL;
    char *filename = NULL, testfilename[PDF_MAX_FONTNAME + 5];
    char *mmparam, mastername[PDF_MAX_FONTNAME + 1];

    if (encoding == NULL || *encoding == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "encoding", 0, 0, 0);

    if (fontname == NULL)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "fontname", 0, 0, 0);

    /* Converting fontname to UTF-8 */
    if (inlen)
    {
        pdc_byte *utf8fontname;
        pdc_text_format textformat = pdc_bytes2;
        pdc_text_format targettextformat = pdc_utf8;
        int outlen;

        if (pdc_convert_string(p->pdc, textformat, NULL,
                               (pdc_byte *) fontname, inlen,
                               &targettextformat, NULL,
                               &utf8fontname, &outlen,
                               PDC_CONV_WITHBOM | PDC_CONV_TRY7BYTES,
                               p->debug['F']))
        {
            if (p->debug['F'])
                pdc_error(p->pdc, -1, 0, 0, 0, 0);
            return -1;
        }

        fontname = pdc_errprintf(p->pdc, "%s", utf8fontname);
        pdc_free(p->pdc, utf8fontname);
    }

    if (*fontname == '\0')
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "fontname", 0, 0, 0);

    /* slot for new font struct */
    slot = pdf_init_newfont(p);
    font = &p->fonts[slot];

    /* parsing optlist */
    font->verbose = p->debug['F'];
    if (optlist && strlen(optlist))
    {
        results = pdc_parse_optionlist(p->pdc, optlist, pdf_load_font_options,
                                       NULL, font->verbose);
        if (!results)
            return -1;

        /* save and check options */
        pdc_get_optvalues(p->pdc, "fontwarning", results,
                          &font->verbose, NULL);

        pdc_get_optvalues(p->pdc, "embedding", results,
                          &font->embedding, NULL);




        if (pdc_get_optvalues(p->pdc, "fontstyle", results, &inum, NULL))
            font->style = (pdc_fontstyle) inum;

        pdc_get_optvalues(p->pdc, "monospace", results,
                          &font->monospace, NULL);

        pdc_cleanup_optionlist(p->pdc, results);
    }

    /* search for a registered encoding */
    slot = -1;
    kret = pdc_false;
    fontwarning = p->debug['F'];
    p->debug['F'] = (char) font->verbose;
    enc = pdf_find_encoding(p, encoding);

    if (enc == pdc_unicode || enc == pdc_glyphid)
        pdc_error(p->pdc, PDF_E_UNSUPP_UNICODE, 0, 0, 0, 0);

    if (enc == pdc_invalidenc)
    {
        /* check the known CMap names */
        slot = pdf_handle_cidfont(p, fontname, encoding);
        if (slot == -1)
        {
            /* look for a new encoding */
            enc = pdf_insert_encoding(p, encoding);
            if (enc == pdc_invalidenc)
                kret = pdc_true;
        }
        else
	{
            kret = pdc_true;
	    if (slot == -2)	/* error occurred in pdf_handle_cidfont() */
		slot = -1;
	}
    }
    p->debug['F'] = (char) fontwarning;
    if (kret)
        return slot;

    /*
     * Look whether font is already in the cache.
     * If a font with same encoding and same relevant options is found,
     * return its descriptor.
     * If a Type 3 font with the same name but different encoding
     * is found, make a copy in a new slot and attach the requested encoding.
     */

    for (slot = 0; slot < p->fonts_number; slot++)
    {
        if (!strcmp(p->fonts[slot].apiname, fontname) &&
            p->fonts[slot].style == font->style)
        {
            if (p->fonts[slot].type == pdc_Type3)
            {
                if (enc < 0 )
                {
		    pdc_set_errmsg(p->pdc, PDF_E_FONT_BADENC,
			p->fonts[slot].name,
			pdf_get_encoding_name(p, enc), 0, 0);

                    if (font->verbose == pdc_true)
                        pdc_error(p->pdc, -1, 0, 0, 0, 0);

                    return -1;
                }
                if (p->fonts[slot].encoding != enc)
                    slot = pdf_handle_t3font(p, fontname, enc, slot);

                return slot;
            }
            else if (p->fonts[slot].monospace == font->monospace)
            {
                if (p->fonts[slot].encoding == enc)
                {
                    return slot;
                }
                else if (p->fonts[slot].encoding >= 0)
                {
                    char *encname, *adaptname;
                    int kc;

                    /* Comparing apiname of encoding */
                    if (!strcmp(encoding, p->fonts[slot].encapiname))
                        return slot;

                    /* Name of adapted to font encoding */
                    encname = (char *) pdf_get_encoding_name(p, enc);
                    len = strlen(encname) + 1 + strlen(fontname) + 1;
                    adaptname = (char *) pdc_malloc(p->pdc, len, fn);
                    strcpy(adaptname, encname);
                    strcat(adaptname, PDC_ENC_MODSEPAR);
                    strcat(adaptname, fontname);
                    kc = strcmp(adaptname,
                            pdf_get_encoding_name(p, p->fonts[slot].encoding));
                    pdc_free(p->pdc, adaptname);
                    if (!kc)
                        return slot;
                }
            }
        }
    }


    /* Multiple Master handling:
     * - strip MM parameters to build the master name
     * - the master name is used to find the metrics
     * - the instance name (client-supplied font name) is used in all places
     * - although the master name is used for finding the metrics, the
     *   instance name is stored in the font struct.
     */

    len = strlen(fontname);
    if (len > PDF_MAX_FONTNAME)
    {
	pdc_set_errmsg(p->pdc, PDC_E_ILLARG_TOOLONG, "fontname",
	    pdc_errprintf(p->pdc, "%d", PDF_MAX_FONTNAME), 0, 0);

        if (font->verbose)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);

        return -1;
    }
    strcpy(mastername, fontname);

    /* A Multiple Master font was requested */
    if ((mmparam = strstr(mastername, "MM_")) != NULL)
    {
        if (font->embedding)
        {
	    pdc_set_errmsg(p->pdc, PDF_E_FONT_EMBEDMM, fontname, 0, 0, 0);

            if (font->verbose)
                pdc_error(p->pdc, -1, 0, 0, 0, 0);

            return -1;
        }
        mmparam[2] = '\0';      /* strip the parameter from the master name */
    }

    /* Font with vertical writing mode */
    fontname_p = mastername;
    if (mastername[0] == '@')
    {
        font->vertical = pdc_true;
        fontname_p = &mastername[1];
    }

    /* API font name */
    font->apiname = pdc_strdup(p->pdc, fontname);

    /* Font file search hierarchy
     * - Check "FontOutline" resource entry and check TrueType font
     * - Check "FontAFM" resource entry
     * - Check "FontPFM" resource entry
     * - Check "HostFont" resource entry
     * - Check available in-core metrics
     * - Check host font
     */
    retval = pdc_false;
    while (1)
    {
#ifdef PDF_TRUETYPE_SUPPORTED
        /* Check specified TrueType file */
        filename = pdf_find_resource(p, "FontOutline", fontname_p);
        if (filename) {
            outfilename = filename;
            retval = pdf_check_tt_font(p, filename, fontname_p, font);
            if (retval == pdc_undef) {
                retval = pdc_false;
                break;
            }
            if (retval == pdc_true) {
                retval = pdf_get_metrics_tt(p, font, fontname_p, enc, filename);
                break;
            }
        }
#endif /* PDF_TRUETYPE_SUPPORTED */

        /* Check specified AFM file */
        filename = pdf_find_resource(p, "FontAFM", fontname_p);
        if (filename) {
            retval = pdf_get_metrics_afm(p, font, fontname_p, enc, filename);
            break;
        }

        /* Check specified PFM file */
        filename = pdf_find_resource(p, "FontPFM", fontname_p);
        if (filename) {
            retval = pdf_get_metrics_pfm(p, font, fontname_p, enc, filename);
            break;
        }



        /* Check available in-core metrics */
        retval = pdf_get_metrics_core(p, font, fontname_p, enc);
        if (retval != pdc_undef) break;
        retval = pdc_false;


        /* Search for a metric file */
        font->verbose_open = pdc_false;
        filename = testfilename;
        for (i = 0; i < 100; i++)
        {
            extension = pdf_extension_names[i].word;
            if (!extension) break;
            strcpy(testfilename, fontname_p);
            strcat(testfilename, extension);
            switch (pdf_extension_names[i].code)
            {
                case 1:
                if (pdf_check_tt_font(p, filename, fontname_p, font) ==
                    pdc_true)
                    retval = pdf_get_metrics_tt(p, font, fontname_p, enc,
                                                filename);
                break;

                case 2:
                retval = pdf_get_metrics_afm(p, font, fontname_p, enc,
                                             filename);
                break;

                case 3:
                retval = pdf_get_metrics_pfm(p, font, fontname_p, enc,
                                             filename);
                break;
            }
            if (retval == pdc_true)
            {
                if (pdf_extension_names[i].code == 1)
                    outfilename = filename;
                break;
            }
        }

        if (retval == pdc_false)
	{
            pdc_set_errmsg(p->pdc, PDF_E_FONT_NOMETRICS, fontname, 0, 0, 0);

	    if (font->verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);
	}
        break;
    }

    if (retval == pdc_false) {
        pdf_cleanup_font(p, font);
        return -1;
    }

    /* store instance name instead of master name in the font structure */
    if (mmparam) {
        pdc_free(p->pdc, font->name);
        font->name = pdc_strdup(p->pdc, fontname);
    }

    /* If embedding was requested, check font file (or raise an exception) */
    font->verbose_open = font->verbose;
    if (font->embedding) {
        if (font->img == NULL) {
            pdc_file *fp = NULL;
            if (outfilename) {
                if ((fp = pdf_fopen(p, outfilename, "font ", 0)) == NULL)
		{
                    if (font->verbose)
			pdc_error(p->pdc, -1, 0, 0, 0, 0);

                    retval = pdc_false;
                }
                if ((font->type == pdc_Type1 || font->type == pdc_MMType1) &&
                    (pdf_t1check_fontfile(p, font, fp) == pdc_undef)) {
                    pdc_fclose(fp);
                    retval = pdc_false;
                }
            } else {
                outfilename = testfilename;
                for (i = 0; i < 100; i++) {
                    extension = pdf_extension_names[i].word;
                    if (!extension) break;
                    strcpy(testfilename, fontname_p);
                    strcat(testfilename, extension);
                    if (pdf_extension_names[i].code == 4 &&
                        (fp = pdf_fopen(p, outfilename, NULL, 0)) != NULL) {
                        if (pdf_t1check_fontfile(p, font, fp) != pdc_undef)
                            break;
                        pdc_fclose(fp);
                        fp = NULL;
                    }
                }
                if (fp == NULL)
		{
		    pdc_set_errmsg(p->pdc, PDF_E_FONT_NOOUTLINE, fontname,
			0, 0, 0);

                    if (font->verbose)
                        pdc_error(p->pdc, -1, 0, 0, 0, 0);

                    retval = pdc_false;
                }
            }
            if (retval == pdc_true) {
                if (pdc_file_isvirtual(fp) == pdc_true) {
                    font->imgname = pdc_strdup(p->pdc, outfilename);
                    pdf_lock_pvf(p, font->imgname);
                }
                pdc_fclose(fp);
                font->fontfilename = pdc_strdup(p->pdc, outfilename);
            }
        }
    } else if (font->img) {
        if (!font->imgname) {
            pdc_free(p->pdc, font->img);
        } else {
            pdf_unlock_pvf(p, font->imgname);
            pdc_free(p->pdc, font->imgname);
            font->imgname = NULL;
        }
        font->img = NULL;
        font->filelen = 0;
    }

    if (retval && font->monospace && font->embedding)
    {
        pdc_set_errmsg(p->pdc, PDC_E_OPT_IGNORED, "monospace", 0, 0, 0);
        if (font->verbose == pdc_true)
            pdc_error(p->pdc, -1, 0, 0, 0, 0);
        retval = pdc_false;
    }

    if (retval == pdc_false) {
        pdf_cleanup_font(p, font);
        return -1;
    }

    /* Now everything is fine; fill the remaining font cache entries */

    p->fonts_number++;

    font->verbose_open = pdc_true;

    font->encapiname = pdc_strdup(p->pdc, encoding);

    if (enc >= 0)
        p->encodings[enc].ev->flags |= PDC_ENC_USED;

    font->obj_id = pdc_alloc_id(p->out);

    return slot;
}

PDFLIB_API int PDFLIB_CALL
PDF_load_font(PDF *p, const char *fontname, int len,
              const char *encoding, const char *optlist)
{
    static const char fn[] = "PDF_load_font";
    int slot = -1;

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content),
        "(p[%p], \"%s\", %d, \"%s\", \"%s\")",
        (void *) p, pdc_strprint(p->pdc, fontname, len), len, encoding,optlist))
    {
        slot = pdf__load_font(p, fontname, len, encoding, optlist);
    }

    PDF_RETURN_HANDLE(p, slot)
}

PDFLIB_API int PDFLIB_CALL
PDF_findfont(PDF *p, const char *fontname, const char *encoding, int embed)
{
    static const char fn[] = "PDF_findfont";
    char optlist[256];
    int slot = -1;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content),
        "(p[%p], \"%s\", \"%s\", %d)", (void *) p, fontname, encoding, embed))
    {
        PDF_RETURN_HANDLE(p, slot)
    }

    if (embed < 0 || embed > 1)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "embed", pdc_errprintf(p->pdc, "%d", embed), 0, 0);

    optlist[0] = 0;
    if (embed)
        strcat(optlist, "embedding true ");

    slot = pdf__load_font(p, fontname, 0, encoding, (const char *) optlist);

    PDF_RETURN_HANDLE(p, slot)
}

PDFLIB_API int PDFLIB_CALL
PDF_get_glyphid(PDF *p, int font, int code)
{
    static const char fn[] = "PDF_get_glyphid";
    int gid = 0;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_content | pdf_state_path),
        "(p[%p], %d, %04X)", (void *) p, font, code))
    {
        pdc_trace(p->pdc, "[%d]\n", gid);
        return gid;
    }

    PDF_INPUT_HANDLE(p, font)
    if (font < 0 || font >= p->fonts_number)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "font", pdc_errprintf(p->pdc, "%d", font), 0, 0);

    if (code < 0 || code >= p->fonts[font].numOfCodes)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "code", pdc_errprintf(p->pdc, "%d", code), 0, 0);

    if (p->fonts[font].code2GID)
        gid = p->fonts[font].code2GID[code];
    else
        pdc_error(p->pdc, PDF_E_FONT_NOGLYPHID, p->fonts[font].apiname, 0, 0,0);

    pdc_trace(p->pdc, "[%d]\n", gid);
    return gid;
}


static void
pdf_write_fontdescriptor(
    PDF *p,
    pdc_font *font,
    pdc_id fontdescriptor_id,
    pdc_id fontfile_id)
{
    /*
     * Font descriptor object
     */
    pdc_begin_obj(p->out, fontdescriptor_id);   /* font descriptor obj */
    pdc_begin_dict(p->out);                     /* font descriptor dict */

    pdc_puts(p->out, "/Type/FontDescriptor\n");
    pdc_printf(p->out, "/Ascent %d\n", font->ascender);
    pdc_printf(p->out, "/CapHeight %d\n", font->capHeight);
    pdc_printf(p->out, "/Descent %d\n", font->descender);
    pdc_printf(p->out, "/Flags %ld\n", font->flags);
    pdc_printf(p->out, "/FontBBox[%d %d %d %d]\n",
        (int) font->llx, (int) font->lly, (int) font->urx, (int) font->ury);

    pdc_printf(p->out, "/FontName");
    pdc_put_pdfname(p->out, font->name, strlen(font->name));
    pdc_puts(p->out, "\n");

    pdc_printf(p->out, "/ItalicAngle %d\n", (int) (font->italicAngle));
    pdc_printf(p->out, "/StemV %d\n", font->StdVW);

    if (font->StdHW > 0)
        pdc_printf(p->out, "/StemH %d\n", font->StdHW);

    if (font->xHeight > 0)
        pdc_printf(p->out, "/XHeight %d\n", font->xHeight);

    if (fontfile_id != PDC_BAD_ID)
    {
        switch(font->type)
        {
            case pdc_Type1:
            case pdc_MMType1:
            pdc_printf(p->out, "/FontFile %ld 0 R\n", fontfile_id);
            break;

#ifdef PDF_TRUETYPE_SUPPORTED
            case pdc_TrueType:
            case pdc_CIDFontType2:
            pdc_printf(p->out, "/FontFile2 %ld 0 R\n", fontfile_id);
            break;

            case pdc_Type1C:
            case pdc_CIDFontType0:
            pdc_printf(p->out, "/FontFile3 %ld 0 R\n", fontfile_id);
            break;
#endif /* PDF_TRUETYPE_SUPPORTED */

            default:
            break;
        }
    }

    pdc_end_dict(p->out);                       /* font descriptor dict */
    pdc_end_obj(p->out);                        /* font descriptor obj */
}

static void
pdf_put_font(PDF *p, pdc_font *font)
{
    const char        *fontname = font->name;
    pdc_id             fontdescriptor_id = PDC_BAD_ID;
    pdc_id             fontfile_id = PDC_BAD_ID;
    pdc_id             encoding_id = PDC_BAD_ID;
    const char *Adobe_str = "\101\144\157\142\145";
    pdc_id             descendant_id = PDC_BAD_ID;
    pdc_encoding       enc = font->encoding;
    pdc_bool           base_font = pdc_false;
    pdc_bool           comp_font = pdc_false;
    float              a = (float) 1.0;
    PDF_data_source    src;
    int                slot, i, j;

    /*
     * This Type3 font has been defined, but never used. Ignore it.
     * However, the font's object id has already been allocated.
     * Write a dummy object in order to avoid a complaint
     * of the object machinery.
     */
    if (enc == pdc_invalidenc)
    {
        pdc_begin_obj(p->out, font->obj_id);            /* dummy font obj */
        pdc_printf(p->out, "null %% unused Type 3 font '%s'\n",
                   font->apiname);
        pdc_end_obj(p->out);
        return;
    }


    /* ID for embedded font */
    if (font->embedding)
    {
        switch(font->type)
        {
            case pdc_Type1:
            case pdc_MMType1:
            case pdc_TrueType:
            case pdc_CIDFontType2:
            case pdc_Type1C:
            case pdc_CIDFontType0:
            fontfile_id = pdc_alloc_id(p->out);
            break;

            default:
            break;
        }
    }
    else if (font->type == pdc_Type1)
    {
        const char *basename;

        /* check whether we have one of the base 14 fonts */
        for (slot = 0; ; slot++)
        {
            basename = pdc_get_base14_name(slot);
            if (basename == NULL)
                break;
            if (!strcmp(basename, fontname))
            {
                base_font = pdc_true;
                break;
            }
        }
    }

    /*
     * Font dictionary
     */
    pdc_begin_obj(p->out, font->obj_id);                /* font obj */
    pdc_begin_dict(p->out);                             /* font dict */
    pdc_puts(p->out, "/Type/Font\n");

    /* /Subtype */
    switch (font->type)
    {
        case pdc_Type1:
        pdc_puts(p->out, "/Subtype/Type1\n");
        break;

        case pdc_MMType1:
        pdc_puts(p->out, "/Subtype/MMType1\n");
        break;

        case pdc_TrueType:
        pdc_puts(p->out, "/Subtype/TrueType\n");
        fontname = font->ttname;
        break;

        case pdc_Type1C:
        fontname = font->ttname;
        pdc_puts(p->out, "/Subtype/Type1\n");
        break;

        case pdc_CIDFontType2:
        case pdc_CIDFontType0:
        fontname = font->ttname;
        pdc_puts(p->out, "/Subtype/Type0\n");
        comp_font = pdc_true;
        break;

        case pdc_Type3:
        pdc_puts(p->out, "/Subtype/Type3\n");
        break;
    }

    /* /Name */
    if (font->type == pdc_Type3)
    {
        /*
         * The name is optional, but if we include it it will show up
         * in Acrobat's font info box. However, if the same font name
         * is used with different encodings Acrobat 4 will not be
         * able to distinguish both. For this reason we add the
         * encoding name to make the font name unique.
         */
        pdc_puts(p->out, "/Name");
        pdc_put_pdfname(p->out, fontname, strlen(fontname));
        pdc_puts(p->out, "\n");
    }

    /* /BaseFont */
    switch (font->type)
    {
        case pdc_Type1:
        case pdc_MMType1:
        case pdc_TrueType:
        case pdc_Type1C:
        case pdc_CIDFontType2:
        case pdc_CIDFontType0:
        {
            pdc_puts(p->out, "/BaseFont");
            pdc_put_pdfname(p->out, fontname, strlen(fontname));
            if (font->cmapname)
                pdc_printf(p->out, "-%s", font->cmapname);
            if (font->style != pdc_Normal && !comp_font)
                pdc_printf(p->out, ",%s",
                    pdc_get_keyword(font->style, pdf_fontstyle_pdfkeys));
            pdc_puts(p->out, "\n");
        }
        break;

        /* /FontBBox, /FontMatrix, /CharProcs /Resources */
        case pdc_Type3:
        pdc_printf(p->out, "/FontBBox[%f %f %f %f]\n",
            font->t3font->bbox.llx, font->t3font->bbox.lly,
            font->t3font->bbox.urx, font->t3font->bbox.ury);

        pdc_printf(p->out, "/FontMatrix[%f %f %f %f %f %f]\n",
            font->t3font->matrix.a, font->t3font->matrix.b,
            font->t3font->matrix.c, font->t3font->matrix.d,
            font->t3font->matrix.e, font->t3font->matrix.f);
        pdc_printf(p->out, "/CharProcs %ld 0 R\n", font->t3font->charprocs_id);
        pdc_printf(p->out, "/Resources %ld 0 R\n", font->t3font->res_id);

        /* We must apply a correctional factor since Type 3 fonts not
         * necessarily use 1000 units per em. We apply the correction
         * here, and store the 1000-based width values in the font in
         * order to speed up PDF_stringwidth().
         */
        a = (float) 1000 * font->t3font->matrix.a;
        break;
    }

    /* /FontDescriptor, /FirstChar, /LastChar, /Widths */
    switch (font->type)
    {
        case pdc_Type1:
        if (base_font == pdc_true) break;
        case pdc_MMType1:
        case pdc_TrueType:
        case pdc_Type1C:
        {
            fontdescriptor_id = pdc_alloc_id(p->out);
            pdc_printf(p->out, "/FontDescriptor %ld 0 R\n", fontdescriptor_id);
        }
        case pdc_Type3:
        {

            pdc_puts(p->out, "/FirstChar 0\n");
            pdc_puts(p->out, "/LastChar 255\n");

            pdc_puts(p->out, "/Widths[\n");
            for (i = 0; i < 16; i++)
            {
                for (j = 0; j < 16; j++)
                    pdc_printf(p->out, " %d",
                               (int) ((font->widths[16*i + j]) / a + 0.5));
                pdc_puts(p->out, "\n");
            }
            pdc_puts(p->out, "]\n");
        }
        break;

        default:
        break;
    }

    /* /Encoding */
    switch (font->type)
    {
        case pdc_Type1:
        case pdc_MMType1:
        case pdc_TrueType:
        case pdc_Type1C:
        if (enc == pdc_winansi)
        {
            pdc_printf(p->out, "/Encoding/WinAnsiEncoding\n");
            break;
        }
        if (enc == pdc_macroman && font->hasnomac == pdc_false)
        {
            pdc_printf(p->out, "/Encoding/MacRomanEncoding\n");
            break;
        }
        case pdc_Type3:
        if (enc >= 0)
        {
            if (p->encodings[enc].id == PDC_BAD_ID)
                p->encodings[enc].id = pdc_alloc_id(p->out);
            encoding_id = p->encodings[enc].id;
        }
        if (encoding_id != PDC_BAD_ID)
            pdc_printf(p->out, "/Encoding %ld 0 R\n", encoding_id);
        break;

        case pdc_CIDFontType2:
        case pdc_CIDFontType0:
        if (font->cmapname)
            pdc_printf(p->out, "/Encoding/%s\n", font->cmapname);
        break;
    }


    /* /DescendantFonts */
    if (comp_font == pdc_true)
    {
        descendant_id = pdc_alloc_id(p->out);
        pdc_printf(p->out, "/DescendantFonts[%ld 0 R]\n", descendant_id);
    }

    pdc_end_dict(p->out);                               /* font dict */
    pdc_end_obj(p->out);                                /* font obj */

    /*
     * Encoding dictionary
     */
    if (encoding_id != PDC_BAD_ID)
    {
        char *charname;

        pdc_begin_obj(p->out, encoding_id);             /* encoding obj */
        pdc_begin_dict(p->out);                         /* encoding dict */

        pdc_puts(p->out, "/Type/Encoding\n");

        {
            pdc_encodingvector *ev = p->encodings[enc].ev;
            pdc_encodingvector *evb = NULL;
            int islatin1 =
                !strncmp((const char *) ev->apiname, "iso8859-1",
                         strlen("iso8859-1"));

            pdf_set_encoding_glyphnames(p, enc);

            if (!strncmp(ev->apiname, PDC_ENC_MODWINANSI,
                         strlen(PDC_ENC_MODWINANSI))
                || islatin1)
            {
                pdc_puts(p->out, "/BaseEncoding/WinAnsiEncoding\n");
                evb = p->encodings[pdc_winansi].ev;
                if (!evb)
                    p->encodings[pdc_winansi].ev =
                        pdc_copy_core_encoding(p->pdc, "winansi");
            }
            if (!strncmp(ev->apiname, PDC_ENC_MODMACROMAN,
                         strlen(PDC_ENC_MODMACROMAN)))
            {
                pdc_puts(p->out, "/BaseEncoding/MacRomanEncoding\n");
                evb = p->encodings[pdc_macroman].ev;
                if (!evb)
                    p->encodings[pdc_macroman].ev =
                        pdc_copy_core_encoding(p->pdc, "macroman");
            }

            if (evb && font->type != pdc_Type3)
            {
                int iv = -1;
                for (i = 0; i < font->numOfCodes; i++)
                {
                    if ((ev->chars[i] && !evb->chars[i]) ||
                        (!ev->chars[i] && evb->chars[i]) ||
                        (ev->chars[i] && evb->chars[i] &&
                         strcmp(ev->chars[i], evb->chars[i])))
                    {
                        if (iv == -1)
                            pdc_puts(p->out, "/Differences[");
                        if (i > iv + 1)
                            pdc_printf(p->out, "%d", i);
                        charname = (char *)
                            ev->chars[i] ? ev->chars[i] : (char *) ".notdef";
                        pdc_put_pdfname(p->out, charname, strlen(charname));
                        pdc_puts(p->out, "\n");
                        iv = i;
                    }
                }
                if (iv > -1)
                    pdc_puts(p->out, "]\n");
            }
            else
            {
                pdc_puts(p->out, "/Differences[0");
                for (i = 0; i < font->numOfCodes; i++)
                {
                    charname = (char *) ev->chars[i] ?
                                   ev->chars[i] :  (char *) ".notdef";
                    pdc_put_pdfname(p->out, charname, strlen(charname));
                    pdc_puts(p->out, "\n");
                }
                pdc_puts(p->out, "]\n");
            }
        }

        pdc_end_dict(p->out);                           /* encoding dict */
        pdc_end_obj(p->out);                            /* encoding obj */

        if (p->flush & pdf_flush_content)
            pdc_flush_stream(p->out);
    }


    /*
     * CID fonts dictionary
     */
    if (descendant_id != PDC_BAD_ID)
    {
        const char *tmpstr;

        pdc_begin_obj(p->out, descendant_id);           /* CID font obj */
        pdc_begin_dict(p->out);                         /* CID font dict */
        pdc_puts(p->out, "/Type/Font\n");

        /* /Subtype */
        if (font->type == pdc_CIDFontType0)
            pdc_puts(p->out, "/Subtype/CIDFontType0\n");
        if (font->type == pdc_CIDFontType2)
            pdc_puts(p->out, "/Subtype/CIDFontType2\n");

        /* /BaseFont */
        pdc_puts(p->out, "/BaseFont");
        pdc_put_pdfname(p->out, fontname, strlen(fontname));
        if (font->style != pdc_Normal)
            pdc_printf(p->out, ",%s",
                pdc_get_keyword(font->style, pdf_fontstyle_pdfkeys));
        pdc_puts(p->out, "\n");

        /* /CIDSystemInfo */
        tmpstr = pdf_get_ordering_cid(p, font);
        pdc_puts(p->out, "/CIDSystemInfo<</Registry");
        pdc_put_pdfstring(p->out, Adobe_str, (int) strlen(Adobe_str));
        pdc_puts(p->out, "/Ordering");
        pdc_put_pdfstring(p->out, tmpstr, (int) strlen(tmpstr));
        pdc_printf(p->out, "/Supplement %d>>\n",
                   pdf_get_supplement_cid(p, font));

        /* /FontDescriptor */
        fontdescriptor_id = pdc_alloc_id(p->out);
        pdc_printf(p->out, "/FontDescriptor %ld 0 R\n", fontdescriptor_id);

        /* /DW /W */
#ifdef PDF_CJKFONTWIDTHS_SUPPORTED
        if (enc == pdc_cid)
            pdf_put_cidglyph_widths(p, font);
#endif /* PDF_CJKFONTWIDTHS_SUPPORTED */


        pdc_end_dict(p->out);                           /* CID font dict */
        pdc_end_obj(p->out);                            /* CID font obj */
    }

    /*
     * FontDescriptor dictionary
     */
    if (fontdescriptor_id != PDC_BAD_ID)
        pdf_write_fontdescriptor(p, font, fontdescriptor_id, fontfile_id);

    /*
     * Font embedding
     */
    if (fontfile_id != PDC_BAD_ID)
    {
        pdc_id    length_id = PDC_BAD_ID;
        pdc_id    length1_id = PDC_BAD_ID;
        pdc_id    length2_id = PDC_BAD_ID;
        pdc_id    length3_id = PDC_BAD_ID;
        pdc_bool  hexencode = pdc_false;
        pdc_bool  compress = pdc_false;

        /* Prepare embedding */
        switch(font->type)
        {
            case pdc_Type1:
            case pdc_MMType1:
            {
                pdf_make_t1src(p, font, &src);
                length1_id = pdc_alloc_id(p->out);
                length2_id = pdc_alloc_id(p->out);
                length3_id = pdc_alloc_id(p->out);
            }
            break;

#ifdef PDF_TRUETYPE_SUPPORTED
            case pdc_TrueType:
            case pdc_CIDFontType2:
            {
                length1_id = pdc_alloc_id(p->out);
            }
            case pdc_Type1C:
            case pdc_CIDFontType0:
            {
                src.private_data = (void *) font->fontfilename;
                if (font->fontfilename)
                {
                    /* Read the font from file */
                    src.init = pdf_data_source_file_init;
                    src.fill = pdf_data_source_file_fill;
                    src.terminate = pdf_data_source_file_terminate;
                    switch(font->type)
                    {
                        case pdc_TrueType:
                        case pdc_CIDFontType2:
                        src.offset = (long) 0;
                        src.length = (long) 0;
                        break;

                        case pdc_Type1C:
                        case pdc_CIDFontType0:
                        src.offset = font->cff_offset;
                        src.length = (long) font->cff_length;
                        break;

                        default:
                        break;
                    }
                }
                else
                {
                    /* Read the font from memory */
                    src.init = NULL;
                    src.fill = pdf_data_source_buf_fill;
                    src.terminate = NULL;
                    switch(font->type)
                    {
                        case pdc_TrueType:
                        case pdc_CIDFontType2:
                        src.buffer_start = font->img;
                        src.buffer_length = font->filelen;
                        break;

                        case pdc_Type1C:
                        case pdc_CIDFontType0:
                        src.buffer_start = font->img + font->cff_offset;
                        src.buffer_length = font->cff_length;
                        break;

                        default:
                        break;
                    }
                    src.bytes_available = 0;
                    src.next_byte = NULL;
                }
            }
            break;
#endif /* PDF_TRUETYPE_SUPPORTED */

            default:
            break;
        }

        /* Embedded font stream dictionary */
        pdc_begin_obj(p->out, fontfile_id);     /* Embedded font stream obj */
        pdc_begin_dict(p->out);                 /* Embedded font stream dict */

        /* /Length, /Filter */
        length_id = pdc_alloc_id(p->out);
        pdc_printf(p->out, "/Length %ld 0 R\n", length_id);
        switch(font->type)
        {
            case pdc_Type1:
            case pdc_MMType1:
            if (p->debug['a'])
            {
                hexencode = pdc_true;
                pdc_puts(p->out, "/Filter/ASCIIHexDecode\n");
            }
            break;

#ifdef PDF_TRUETYPE_SUPPORTED
            case pdc_TrueType:
            case pdc_CIDFontType2:
            case pdc_Type1C:
            case pdc_CIDFontType0:
            if (font->filelen != 0L)
            {
                compress = pdc_true;
                if (pdc_get_compresslevel(p->out))
                    pdc_puts(p->out, "/Filter/FlateDecode\n");
            }
            break;
#endif /* PDF_TRUETYPE_SUPPORTED */

            default:
            break;
        }

        /* /Length1, /Length2, Length3 */
        if (length1_id != PDC_BAD_ID)
            pdc_printf(p->out, "/Length1 %ld 0 R\n", length1_id);
        if (length2_id != PDC_BAD_ID)
            pdc_printf(p->out, "/Length2 %ld 0 R\n", length2_id);
        if (length3_id != PDC_BAD_ID)
            pdc_printf(p->out, "/Length3 %ld 0 R\n", length3_id);

#ifdef PDF_TRUETYPE_SUPPORTED
        /* /Subtype */
        if(font->type == pdc_Type1C)
            pdc_puts(p->out, "/Subtype/Type1C\n");
        if (font->type == pdc_CIDFontType0)
            pdc_puts(p->out, "/Subtype/CIDFontType0C\n");
#endif /* PDF_TRUETYPE_SUPPORTED */

        pdc_end_dict(p->out);                   /* Embedded font stream dict */

        /* Stream */
        if (hexencode == pdc_true)
            pdf_ASCIIHexEncode(p, &src);
        else
            pdf_copy_stream(p, &src, compress);

        pdc_end_obj(p->out);                    /* Embedded font stream obj */

        pdc_put_pdfstreamlength(p->out, length_id);

        /* Length objects */
        switch(font->type)
        {
            case pdc_Type1:
            case pdc_MMType1:
            pdf_put_length_objs(p, &src, length1_id, length2_id, length3_id);
            break;

#ifdef PDF_TRUETYPE_SUPPORTED
            case pdc_TrueType:
            case pdc_CIDFontType2:
            if (compress)
            {
                pdc_begin_obj(p->out, length1_id);      /* Length1 obj */
                pdc_printf(p->out, "%ld\n", (long) font->filelen);
                pdc_end_obj(p->out);                    /* Length1 obj */
            }
            else
            {
                /* same as /Length */
                pdc_put_pdfstreamlength(p->out, length1_id);
            }
            break;
#endif /* PDF_TRUETYPE_SUPPORTED */

            default:
            break;
        }
    }

    if (p->flush & pdf_flush_content)
        pdc_flush_stream(p->out);
}

void
pdf_write_doc_fonts(PDF *p)
{
    int slot;

    /* output pending font objects */
    for (slot = 0; slot < p->fonts_number; slot++)
    {
        switch(p->fonts[slot].type)
        {
            case pdc_Type1:
            case pdc_MMType1:
#ifdef PDF_TRUETYPE_SUPPORTED
            case pdc_TrueType:
            case pdc_CIDFontType2:
            case pdc_Type1C:
#endif /* PDF_TRUETYPE_SUPPORTED */
            case pdc_CIDFontType0:
            case pdc_Type3:
            pdf_put_font(p, &p->fonts[slot]);
            break;

            default:
            break;
        }
    }
}

void
pdf_write_page_fonts(PDF *p)
{
    int i, total = 0;

    /* This doesn't really belong here, but all modules which write
     * font resources also need this, so we include it here.
     * Note that keeping track of ProcSets is considered obsolete
     * starting with PDF 1.4, so we always include the full set.
     */

    pdc_puts(p->out, "/ProcSet[/PDF/ImageB/ImageC/ImageI/Text]\n");

    for (i = 0; i < p->fonts_number; i++)
        if (p->fonts[i].used_on_current_page == pdc_true)
            total++;

    if (total > 0)
    {
        pdc_puts(p->out, "/Font");

        pdc_begin_dict(p->out);         /* font resource dict */

        for (i = 0; i < p->fonts_number; i++)
            if (p->fonts[i].used_on_current_page == pdc_true) {
                p->fonts[i].used_on_current_page = pdc_false;   /* reset */
                pdc_printf(p->out, "/F%d %ld 0 R\n", i, p->fonts[i].obj_id);
            }

        pdc_end_dict(p->out);           /* font resource dict */
    }
}
