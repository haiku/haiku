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

/* $Id: p_encoding.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib encoding handling routines
 *
 */

#include "p_intern.h"
#include "p_font.h"

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif /* WIN32 */



void
pdf__encoding_set_char(PDF *p, const char *encoding, int slot,
                       const char *glyphname, int uv)
{
    int enc;
    pdc_encodingvector *ev;
    char given;

    for (enc = (int) pdc_invalidenc + 1; enc < (int) pdc_firstvarenc; enc++)
    {
        if (!strcmp(encoding, pdc_get_fixed_encoding_name((pdc_encoding) enc)))
            pdc_error(p->pdc, PDF_E_ENC_CANTCHANGE, encoding, 0, 0, 0);
    }

    if (uv == 0)
    {
        given = 1;
        uv = (int) pdf_insert_glyphname(p, glyphname);
    }
    else if (!glyphname || !*glyphname)
    {
        given = 0;
        glyphname = pdf_insert_unicode(p, (pdc_ushort) uv);
    }
    else
    {
        const char *reg_glyphname;
        pdc_ushort reg_uv;

        given = 1;
        reg_glyphname = pdc_unicode2adobe((pdc_ushort) uv);
        if (reg_glyphname)
        {
            if (strcmp(reg_glyphname, glyphname) &&
                p->debug['F'] == pdc_true)
            {
		pdc_warning(p->pdc, PDF_E_ENC_BADGLYPH,
		    glyphname,
		    pdc_errprintf(p->pdc, "0x%04X", uv),
		    reg_glyphname, 0);
            }

            /* We take the registered name */
        }
        else
        {
            reg_uv = pdc_adobe2unicode(glyphname);
            if (reg_uv && reg_uv != (pdc_ushort) uv &&
                p->debug['F'] == pdc_true)
            {
		pdc_error(p->pdc, PDF_E_ENC_BADUNICODE,
		    pdc_errprintf(p->pdc, "0x%04X", uv), glyphname,
		    pdc_errprintf(p->pdc, "0x%04X", reg_uv), 0);
            }

            /* We register the new unicode value */
            pdf_register_glyphname(p, glyphname, (pdc_ushort) uv);
        }
    }

    /* search for a registered encoding */
    enc = pdf_find_encoding(p, encoding);
    if (enc >= p->encodings_capacity)
        pdf_grow_encodings(p);

    if (enc == pdc_invalidenc)
    {
        enc = p->encodings_number;
        p->encodings[enc].ev = pdc_new_encoding(p->pdc, encoding);
        p->encodings[enc].ev->flags |= PDC_ENC_USER;
        p->encodings[enc].ev->flags |= PDC_ENC_SETNAMES;
        p->encodings[enc].ev->flags |= PDC_ENC_ALLOCCHARS;
        p->encodings_number++;
    }
    else if (!(p->encodings[enc].ev->flags & PDC_ENC_USER))
    {
	pdc_error(p->pdc, PDF_E_ENC_CANTCHANGE, encoding, 0, 0, 0);
    }
    else if (p->encodings[enc].ev->flags & PDC_ENC_USED)
    {
	pdc_error(p->pdc, PDF_E_ENC_INUSE, encoding, 0, 0, 0);
    }

    /* Free character name */
    ev = p->encodings[enc].ev;
    if (ev->chars[slot] != NULL)
        pdc_free(p->pdc, ev->chars[slot]);

    /* Saving */
    ev->codes[slot] = (pdc_ushort) uv;
    if (glyphname != NULL)
        ev->chars[slot] = pdc_strdup(p->pdc, glyphname);
    ev->given[slot] = given;
}

PDFLIB_API void PDFLIB_CALL
PDF_encoding_set_char(PDF *p, const char *encoding, int slot,
                      const char *glyphname, int uv)
{
    static const char fn[] = "PDF_encoding_set_char";

    if (!pdf_enter_api(p, fn, pdf_state_all,
        "(p[%p], \"%s\", %d, \"%s\", 0x%04X)\n",
        (void *) p, encoding, slot, glyphname, uv))
    {
        return;
    }

    if (!encoding || !*encoding)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "encoding", 0, 0, 0);

    if (slot < 0 || slot > 255)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

    if (uv < 0 || uv > PDC_MAX_UNICODE)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "uv", pdc_errprintf(p->pdc, "%d", uv), 0, 0);

    if (!glyphname || !*glyphname)
    {
        if (uv == 0)
            pdc_error(p->pdc, PDF_E_ENC_GLYPHORCODE, 0, 0, 0, 0);
    }

    pdf__encoding_set_char(p, encoding, slot, glyphname, uv);
}

PDFLIB_API const char * PDFLIB_CALL
PDF_encoding_get_glyphname(PDF *p, const char *encoding, int slot)
{
    static const char fn[] = "PDF_encoding_get_glyphname";
    const char *name = NULL;
    pdc_encoding enc;

    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", 0x%04X)", (void *) p, encoding, slot))
    {
        name = ".notdef";
        pdc_trace(p->pdc, "[%s]\n", name);
        return name;
    }

    /* search for a registered encoding */
    enc = pdf_find_encoding(p, encoding);

    /* Unicode, glyphid or 8-bit encoding */
    if (enc == pdc_unicode)
    {
        if (slot < 0 || slot > PDC_MAX_UNICODE)
            pdc_error(p->pdc, PDC_E_ILLARG_INT,
                "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

        name = pdf_unicode2glyphname(p, (pdc_ushort) slot);
    }
    else if (enc == pdc_glyphid)
    {
        int font = pdf_get_font(p);
        if (font > -1)
        {
            if (slot < 0 || slot >= p->fonts[font].numOfCodes)
                pdc_error(p->pdc, PDC_E_ILLARG_INT,
                    "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

            if (p->fonts[font].encoding != pdc_glyphid)
                pdc_error(p->pdc, PDF_E_ENC_BADFONT,
                    pdc_errprintf(p->pdc, "%d", font),
                    pdf_get_encoding_name(p, enc), 0, 0);

            if (p->fonts[font].GID2Name)
                name = (const char *) p->fonts[font].GID2Name[slot];
            else if (p->fonts[font].GID2code)
                name = pdf_unicode2glyphname(p,
                           (pdc_ushort) p->fonts[font].GID2code[slot]);
        }
    }
    else
    {
        if (slot < 0 || slot >= 256)
	    pdc_error(p->pdc, PDC_E_ILLARG_INT,
		"slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

        /* cid or builtin */
        if (enc < 0)
	    pdc_error(p->pdc, PDF_E_ENC_CANTQUERY, encoding, 0, 0, 0);

        if (enc >= p->encodings_number)
	    pdc_error(p->pdc, PDF_E_ENC_NOTFOUND, encoding, 0, 0, 0);

        pdf_set_encoding_glyphnames(p, enc);

        if (p->encodings[enc].ev->chars[slot])
            name = p->encodings[enc].ev->chars[slot];
    }

    if (!name)
        name = ".notdef";
    pdc_trace(p->pdc, "[%s]\n", name);
    return name;
}

PDFLIB_API int PDFLIB_CALL
PDF_encoding_get_unicode(PDF *p, const char *encoding, int slot)
{
    static const char fn[] = "PDF_encoding_get_unicode";
    pdc_encoding enc;
    int code = 0;

    if (!pdf_enter_api(p, fn, pdf_state_all,
	"(p[%p], \"%s\", 0x%04X)", (void *) p, encoding, slot))
    {
        pdc_trace(p->pdc, "[%d]\n", code);
        return code;
    }

    /* search for a registered encoding */
    enc = pdf_find_encoding(p, encoding);

    /* Unicode, glyphid or 8-bit encoding */
    if (enc == pdc_unicode)
    {
        if (slot < 0 || slot > PDC_MAX_UNICODE)
            pdc_error(p->pdc, PDC_E_ILLARG_INT,
                "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

        code = slot;
    }
    else if (enc == pdc_glyphid)
    {
        int font = pdf_get_font(p);
        if (font > -1 && p->fonts[font].GID2code)
        {
            if (slot < 0 || slot >= p->fonts[font].numOfCodes)
                pdc_error(p->pdc, PDC_E_ILLARG_INT,
                    "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

            if (p->fonts[font].encoding != pdc_glyphid)
                pdc_error(p->pdc, PDF_E_ENC_BADFONT,
                    pdc_errprintf(p->pdc, "%d", font),
                    pdf_get_encoding_name(p, enc), 0, 0);

            code = p->fonts[font].GID2code[slot];
        }
    }
    else
    {
        if (slot < 0 || slot > 255)
            pdc_error(p->pdc, PDC_E_ILLARG_INT,
                "slot", pdc_errprintf(p->pdc, "%d", slot), 0, 0);

        /* cid or builtin */
        if (enc < 0)
            pdc_error(p->pdc, PDF_E_ENC_CANTQUERY, encoding, 0, 0, 0);

        if (enc >= p->encodings_number)
            pdc_error(p->pdc, PDF_E_ENC_NOTFOUND, encoding, 0, 0, 0);

        code = (int) p->encodings[enc].ev->codes[slot];
    }

    pdc_trace(p->pdc, "[%d]\n", code);
    return code;
}


/*
 * Try to read an encoding from file.
 *
 * Return value: allocated encoding struct
 *               NULL: error
 */

pdc_encodingvector *
pdf_read_encoding(PDF *p, const char *encoding, const char *filename)
{
    pdc_encodingvector  *ev = NULL;
    pdc_file            *fp;
    char               **linelist;
    char                *line, glyphname[128];
    int                 slot, nlines = 0, l, i_uv;
    pdc_ushort          uv;

    enum state {
        s_init, s_names, s_codes
    } stat = s_init;

    if ((fp = pdf_fopen(p, filename, "encoding ", 0)) == NULL)
    {
	if (p->debug['F'])
	    pdc_error(p->pdc, -1, 0, 0, 0, 0);
    }
    else
    {
        nlines = pdc_read_textfile(p->pdc, fp, &linelist);
        pdc_fclose(fp);
    }
    if (!nlines)
        return ev;

    ev = pdc_new_encoding(p->pdc, encoding);
    if (ev == NULL)
        return ev;

    for (l = 0; l < nlines; l++)
    {
        line = linelist[l];
        if (stat == s_init)
        {
            if (sscanf(line, "0x%x", &i_uv) == 1)
                stat = s_codes;
            else
                stat = s_names;
        }

        if (stat == s_names)
        {
            i_uv = 0;
            if ((sscanf(line, "%s 0x%x 0x%x", glyphname, &slot, &i_uv) != 3 &&
                 sscanf(line, "%s %d 0x%x", glyphname, &slot, &i_uv) != 3) ||
                 slot < 0 || slot >= 256 ||
                 i_uv < 0 || i_uv > PDC_MAX_UNICODE)
            {
                if ((sscanf(line, "%s 0x%x", glyphname, &slot) != 2 &&
                     sscanf(line, "%s %d", glyphname, &slot) != 2) ||
                     slot < 0 || slot >= 256)
                {
                    const char *stemp = pdc_errprintf(p->pdc, "%s", line);
                    pdc_cleanup_encoding(p->pdc, ev);
                    pdc_cleanup_stringlist(p->pdc, linelist);
                    ev = NULL;
                    if (p->debug['F'] == pdc_true)
			pdc_error(p->pdc, PDF_E_ENC_BADLINE,
                                  filename, stemp, 0, 0);
                    else
                        return ev;
                }
                uv = pdf_insert_glyphname(p, glyphname);
            }
            else
            {
                uv = (pdc_ushort) i_uv;
            }

            ev->codes[slot] = uv;
            ev->chars[slot] = pdc_strdup(p->pdc, glyphname);
            ev->given[slot] = 1;
        }
        else
        {
            /* stat == s_codes
            */
            if ((sscanf(line, "0x%x 0x%x", &i_uv, &slot) != 2 &&
                 sscanf(line, "0x%x %d", &i_uv, &slot) != 2) ||
                 slot < 0 || slot >= 256 ||
                 i_uv < 0 || i_uv > PDC_MAX_UNICODE)
            {
                const char *stemp = pdc_errprintf(p->pdc, "%s", line);
                pdc_cleanup_encoding(p->pdc, ev);
                pdc_cleanup_stringlist(p->pdc, linelist);
                ev = NULL;
                if (p->debug['F'] == pdc_true)
		    pdc_error(p->pdc, PDF_E_ENC_BADLINE,
			      filename, stemp, 0, 0);
                else
                    return ev;
            }

            uv = (pdc_ushort) i_uv;
            ev->codes[slot] = uv;
            ev->chars[slot] = (char *) pdf_insert_unicode(p, uv);
        }
    }

    ev->flags |= PDC_ENC_FILE;
    ev->flags |= PDC_ENC_SETNAMES;
    if (stat == s_names)
        ev->flags |= PDC_ENC_ALLOCCHARS;

    pdc_cleanup_stringlist(p->pdc, linelist);

    return ev;
}

/*
 * Try to generate an encoding from an Unicode code page.
 *
 * Return value: allocated encoding struct
 *               NULL: error
 */

pdc_encodingvector *
pdf_generate_encoding(PDF *p, const char *encoding)
{
    pdc_encodingvector *ev = NULL;
    pdc_ushort          uv;
    int                 i_uv1, i_uv2, slot;
    size_t              len;

    /* first unicode offset */
    len = strlen(encoding);
    if (len >= 6 && !strncmp(encoding, "U+", 2) &&
        sscanf(&encoding[2], "%x", &i_uv1) == 1)
    {
        if (i_uv1 < 0 || i_uv1 > PDC_MAX_UNICODE)
            return ev;

        /* second unicode offset */
        i_uv2 = -1;
        if (len >= 12)
        {
            char *se = strstr(&encoding[6], "U+");
            if (se && sscanf(&se[2], "%x", &i_uv2) == 1)
            {
                if (i_uv2 < 0 || i_uv2 > PDC_MAX_UNICODE)
                    return ev;
            }
        }
        uv = (pdc_ushort) i_uv1;
        ev = pdc_new_encoding(p->pdc, encoding);
        if (ev != NULL)
        {
            for (slot = 0; slot < 256; slot++)
            {
                if (i_uv2 > -1 && slot == 128)
                    uv = (pdc_ushort) i_uv2;
                ev->codes[slot] = uv;
                ev->chars[slot] = (char *) pdf_insert_unicode(p, uv);
                uv++;
            }
            ev->flags |= PDC_ENC_GENERATE;
            ev->flags |= PDC_ENC_SETNAMES;
        }
    }

    return ev;
}

static const char *
pdf_subst_encoding_name(PDF *p, const char *encoding, char *buffer)
{
    (void) p;
    (void) buffer;

    /* special case for the platform-specific host encoding */
    if (!strcmp(encoding, "host") || !strcmp(encoding, "auto"))
    {

#if defined(PDFLIB_EBCDIC)
        return "ebcdic";

#elif defined(MAC)
        return "macroman";

#elif defined(WIN32)
        UINT cpident = GetACP();
        if (!strcmp(encoding, "auto"))
        {
            if (cpident == 10000)
                strcpy(buffer, "macroman");
            else if (cpident == 20924)
                strcpy(buffer, "ebcdic");
            else if (cpident >= 28590 && cpident <= 28599)
                sprintf(buffer, "iso8859-%d", cpident-28590);
            else if (cpident >= 28600 && cpident <= 28609)
                sprintf(buffer, "iso8859-%d", cpident-28600+10);
            else if (cpident == 1200 || cpident == 1201)
                strcpy(buffer, "unicode");
            else
            {
                CPINFO CPInfo ;

                if (GetCPInfo(cpident, (LPCPINFO) &CPInfo) &&
                    CPInfo.MaxCharSize == 1)
                {
                        sprintf(buffer, "cp%d", cpident);
                }
                else
                {
                    pdc_error(p->pdc, PDF_E_ENC_UNSUPP,
                              pdc_errprintf(p->pdc, "%d", cpident), 0, 0, 0);
                }
            }
            encoding = buffer;
        }
        else
        {
            return "winansi";
        }
#endif
    }

    /* These encodings will be mapped to winansi */
    if (!strcmp(encoding, "host") ||
        !strcmp(encoding, "auto")  ||
        !strcmp(encoding, "cp1252"))
        return "winansi";

    return encoding;
}

pdc_encoding
pdf_insert_encoding(PDF *p, const char *encoding)
{
    char *filename;
    char buffer[32];
    pdc_encodingvector *ev = NULL;
    pdc_encoding enc = (pdc_encoding) p->encodings_number;

    /* substituting encoding name */
    encoding = pdf_subst_encoding_name(p, encoding, buffer);

    /* check for an user-defined encoding */
    filename =  pdf_find_resource(p, "Encoding", encoding);
    if (filename)
        ev = pdf_read_encoding(p, encoding, filename);
    if (ev == NULL)
    {
        /* check for a generate encoding */
        ev = pdf_generate_encoding(p, encoding);
        if (ev == NULL)
        {
            {
		pdc_set_errmsg(p->pdc, PDF_E_ENC_NOTFOUND, encoding, 0, 0, 0);

		if (p->debug['F'])
		    pdc_error(p->pdc, -1, 0, 0, 0, 0);

		return pdc_invalidenc;
            }
        }
    }
    p->encodings[enc].ev = ev;
    p->encodings_number++;

    return enc;
}

pdc_encoding
pdf_find_encoding(PDF *p, const char *encoding)
{
    int slot;
    char buffer[32];

    /* substituting encoding name */
    encoding = pdf_subst_encoding_name(p, encoding, buffer);

    /* search for a fixed encoding */
    for (slot = (pdc_encoding) pdc_invalidenc + 1;
         slot < (pdc_encoding) pdc_firstvarenc; slot++)
    {
        if (!strcmp(encoding, pdc_get_fixed_encoding_name((pdc_encoding) slot)))
        {
            /* copy in-core encoding at fixed slots */
            if (slot >= 0 && p->encodings[slot].ev == NULL)
               p->encodings[slot].ev = pdc_copy_core_encoding(p->pdc, encoding);
            return((pdc_encoding) slot);
        }
    }

    /* search for a user defined encoding */
    for (slot = (pdc_encoding) pdc_firstvarenc;
         slot < p->encodings_number; slot++)
    {
        if (p->encodings[slot].ev->apiname != NULL &&
            !strcmp(encoding, p->encodings[slot].ev->apiname))
            return((pdc_encoding) slot);
    }

    if (slot >= p->encodings_capacity)
        pdf_grow_encodings(p);

    /* search for an in-core encoding */
    p->encodings[slot].ev = pdc_copy_core_encoding(p->pdc, encoding);
    if (p->encodings[slot].ev != NULL)
    {
        p->encodings_number++;
        return((pdc_encoding) slot);
    }

    return (pdc_invalidenc);
}

const char *
pdf_get_encoding_name(PDF *p, pdc_encoding enc)
{
    const char *apiname = pdc_get_fixed_encoding_name(enc);
    if (!apiname[0] && enc >= 0)
        apiname = (const char *) p->encodings[enc].ev->apiname;
    return apiname;
}

void
pdf_set_encoding_glyphnames(PDF *p, pdc_encoding enc)
{
    pdc_encodingvector *ev = p->encodings[enc].ev;
    int slot;
    pdc_ushort uv;

    if (!(ev->flags & PDC_ENC_SETNAMES))
    {
        ev->flags |= PDC_ENC_SETNAMES;
        for (slot = 0; slot < 256; slot++)
        {
            uv = ev->codes[slot];
            ev->chars[slot] = (char *)pdf_unicode2glyphname(p, uv);
        }
    }
}

pdc_bool
pdf_get_encoding_isstdflag(PDF *p, pdc_encoding enc)
{
    pdc_encodingvector *ev = p->encodings[enc].ev;
    int slot;
    pdc_bool isstd = pdc_true;

    if (!(ev->flags & PDC_ENC_INCORE) && !(ev->flags & PDC_ENC_STDNAMES))
    {
        for (slot = 0; slot < 256; slot++)
        {
            if (!(ev->flags & PDC_ENC_SETNAMES))
                ev->chars[slot] =
                    (char *) pdf_unicode2glyphname(p, ev->codes[slot]);
            if (isstd == pdc_true && ev->chars[slot])
            {
                isstd = pdc_is_std_charname((char *) ev->chars[slot]);
                if (isstd == pdc_false && (ev->flags & PDC_ENC_SETNAMES))
                    break;
            }
        }
        ev->flags |= PDC_ENC_SETNAMES;
        if (isstd == pdc_true)
            ev->flags |= PDC_ENC_STDNAMES;
    }

    return (ev->flags & PDC_ENC_STDNAMES) ? pdc_true : pdc_false;
}

pdc_ushort
pdf_glyphname2unicode(PDF *p, const char *glyphname)
{
    int lo, hi;

    /* Private glyph name table available */
    if (p->glyph_tab_size)
    {
        /* Searching for private glyph name */
        lo = 0;
        hi = p->glyph_tab_size;
        while (lo < hi)
        {
            int i = (lo + hi) / 2;
            int cmp = strcmp(glyphname, p->name2unicode[i].glyphname);

            if (cmp == 0)
                return p->name2unicode[i].code;

            if (cmp < 0)
                hi = i;
            else
                lo = i + 1;
        }
    }

    /* Searching for glyph name in AGL */
    return pdc_adobe2unicode(glyphname);
}

pdc_ushort
pdf_register_glyphname(PDF *p, const char *glyphname, pdc_ushort uv)
{
    static const char fn[] = "pdf_register_glyphname";
    char buf[16];
    int slot, slotname, slotuv;

    if (!glyphname)
        return 0;

    /* Allocate private glyphname tables */
    if (!p->glyph_tab_capacity)
    {
        p->next_unicode = (pdc_ushort) 0xE000;  /* Begin PUA */
        p->glyph_tab_size = 0;
        p->glyph_tab_capacity = PRIVGLYPHS_CHUNKSIZE;
        p->unicode2name = (pdc_glyph_tab *) pdc_malloc(p->pdc,
            sizeof(pdc_glyph_tab) * p->glyph_tab_capacity, fn);
        p->name2unicode = (pdc_glyph_tab *) pdc_malloc(p->pdc,
            sizeof(pdc_glyph_tab) * p->glyph_tab_capacity, fn);
    }

    /* Re-allocate private glyphname tables */
    if (p->glyph_tab_size == p->glyph_tab_capacity)
    {
        int n = p->glyph_tab_capacity + PRIVGLYPHS_CHUNKSIZE;
        p->unicode2name = (pdc_glyph_tab *) pdc_realloc(p->pdc,
            p->unicode2name, n * sizeof(pdc_glyph_tab), fn);
        p->name2unicode = (pdc_glyph_tab *) pdc_realloc(p->pdc,
            p->name2unicode, n * sizeof(pdc_glyph_tab), fn);
        p->glyph_tab_capacity = n;
    }

    /* Set reasonable glyph name "unixxxx" */
    if (!glyphname)
    {
        sprintf(buf, "uni%04X", uv);
        glyphname = buf;
    }

    /* Find slot so that new glyphname is sorted in to name table */
    for (slot = 0; slot < p->glyph_tab_size; slot++)
    {
        if (strcmp(glyphname, p->name2unicode[slot].glyphname) < 0)
            break;
    }
    slotname = slot;
    if (slot < p->glyph_tab_size)
    {
        for (slot = p->glyph_tab_size; slot > slotname; slot--)
        {
            p->name2unicode[slot].code = p->name2unicode[slot-1].code;
            p->name2unicode[slot].glyphname =
                p->name2unicode[slot-1].glyphname;
        }
    }

    /* Set reasonable unicode value in the case of "unixxxx" glyph names */
    if (!uv)
    {
        if (!strncmp(glyphname, "uni", 3))
        {
            int i;
            if (sscanf(glyphname, "uni%x", &i) == 1)
                uv = (pdc_ushort) i;
        }
        if (!uv)
            uv = p->next_unicode;
    }

    if (uv >= p->next_unicode)
    {
        slotuv = p->glyph_tab_size;
        p->next_unicode = (pdc_ushort) (uv + 1);
    }
    else
    {
        /* Find slot so that new unicode is sorted in to unicode table */
        for (slot = 0; slot < p->glyph_tab_size; slot++)
        {
            if (uv < p->unicode2name[slot].code)
                break;
        }
        slotuv = slot;
        if (slot < p->glyph_tab_size)
        {
            for (slot = p->glyph_tab_size; slot > slotuv; slot--)
            {
                p->unicode2name[slot].code = p->unicode2name[slot-1].code;
                p->unicode2name[slot].glyphname =
                    p->unicode2name[slot-1].glyphname;
            }
        }
    }

    p->name2unicode[slotname].code = uv;
    p->name2unicode[slotname].glyphname = pdc_strdup(p->pdc, glyphname);
    p->unicode2name[slotuv].code = uv;
    p->unicode2name[slotuv].glyphname = p->name2unicode[slotname].glyphname;

    p->glyph_tab_size += 1;

    return uv;
}

pdc_ushort
pdf_insert_glyphname(PDF *p, const char *glyphname)
{
    pdc_ushort uv;

    uv = pdf_glyphname2unicode(p, glyphname);
    if (!uv)
        uv = pdf_register_glyphname(p, glyphname, 0);
    return uv;
}

const char *
pdf_insert_unicode(PDF *p, pdc_ushort uv)
{
    const char *glyphname;

    glyphname = pdf_unicode2glyphname(p, uv);
    if (!glyphname)
    {
        pdf_register_glyphname(p, NULL, uv);
        glyphname = pdf_unicode2glyphname(p, uv);
    }
    return glyphname;
}

const char *
pdf_unicode2glyphname(PDF *p, pdc_ushort uv)
{
    int lo, hi;

    /* Private unicode table available */
    if (p->glyph_tab_size)
    {
        /* Searching for private Unicode */
        lo = 0;
        hi = p->glyph_tab_size;
        while (lo < hi)
        {
            int i = (lo + hi) / 2;

            if (uv == p->unicode2name[i].code)
                return p->unicode2name[i].glyphname;

            if (uv < p->unicode2name[i].code)
                hi = i;
            else
                lo = i + 1;
        }
    }

    /* Searching for unicode value in AGL */
    return pdc_unicode2adobe(uv);
}

void
pdf_init_encoding_ids(PDF *p)
{
    int slot;

    for (slot = 0; slot < p->encodings_capacity; slot++)
    {
        p->encodings[slot].id = PDC_BAD_ID;
        p->encodings[slot].tounicode_id = PDC_BAD_ID;
    }
}

void
pdf_init_encodings(PDF *p)
{
    static const char fn[] = "pdf_init_encodings";
    int slot;

    p->encodings_capacity = ENCODINGS_CHUNKSIZE;

    p->encodings = (pdf_encoding *) pdc_malloc(p->pdc,
                        sizeof(pdf_encoding) * p->encodings_capacity, fn);

    for (slot = 0; slot < p->encodings_capacity; slot++)
        p->encodings[slot].ev = NULL;

    /* we must reserve the first 4 slots for standard encodings, because
     * the program identify their by index of p->encodings array!
     */
    p->encodings_number = 4;

    /* init private glyphname tables */
    p->unicode2name = NULL;
    p->name2unicode = NULL;
    p->glyph_tab_capacity = 0;
}

void
pdf_grow_encodings(PDF *p)
{
    static const char fn[] = "pdf_grow_encodings";
    int slot;

    p->encodings = (pdf_encoding *) pdc_realloc(p->pdc, p->encodings,
                       sizeof(pdf_encoding) * 2 * p->encodings_capacity, fn);

    p->encodings_capacity *= 2;
    for (slot = p->encodings_number; slot < p->encodings_capacity; slot++)
    {
        p->encodings[slot].ev = NULL;
        p->encodings[slot].id = PDC_BAD_ID;
        p->encodings[slot].tounicode_id = PDC_BAD_ID;
    }
}

void
pdf_cleanup_encodings(PDF *p)
{
    int slot;

    if (!p->encodings)
        return;

    for (slot = 0; slot < p->encodings_number; slot++)
    {
        if (p->encodings[slot].ev)
            pdc_cleanup_encoding(p->pdc, p->encodings[slot].ev);
    }

    if (p->encodings)
        pdc_free(p->pdc, p->encodings);
    p->encodings = NULL;

    /* clean up private glyphname tables */
    if (p->unicode2name)
    {
        for (slot = 0; slot < p->glyph_tab_size; slot++)
            pdc_free(p->pdc, (char *)p->unicode2name[slot].glyphname);

        if (p->unicode2name)
            pdc_free(p->pdc, p->unicode2name);
        p->unicode2name = NULL;
    }

    if (p->name2unicode)
        pdc_free(p->pdc, p->name2unicode);
    p->name2unicode = NULL;
    p->glyph_tab_capacity = 0;
}

