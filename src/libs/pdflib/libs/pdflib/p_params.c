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

/* $Id: p_params.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib parameter handling
 *
 */

#define P_PARAMS_C

#include "p_intern.h"
#include "p_font.h"
#include "p_image.h"

/*
 * PDF_get_parameter() and PDF_set_parameter() deal with strings,
 * PDF_get_value() and PDF_set_value() deal with numerical values.
 */

typedef struct
{
    char *	name;		/* parameter name */
    pdc_bool	mod_zero;	/* PDF_get_() modifier argument must be 0 */
    int		get_scope;	/* bit mask of legal states for PDF_get_() */
    int		set_scope;	/* bit mask of legal states for PDF_set_() */
} pdf_parm_descr;

static pdf_parm_descr parms[] =
{
#define pdf_gen_parm_descr	1
#include "p_params.h"
#undef	pdf_gen_parm_descr

    { "", 0, 0, 0 }
};

enum
{
#define pdf_gen_parm_enum	1
#include "p_params.h"
#undef	pdf_gen_parm_enum

    PDF_PARAMETER_LIMIT
};

static int
get_index(const char *key)
{
    int i;

    for (i = 0; i < PDF_PARAMETER_LIMIT; ++i)
	if (strcmp(key, parms[i].name) == 0)
	    return i;

    return -1;
}

static pdc_bool
pdf_bool_value(PDF *p, const char *key, const char *value)
{
    if (!strcmp(value, "true"))
	return pdc_true;

    if (!strcmp(value, "false"))
	return pdc_false;

    pdc_error(p->pdc, PDC_E_ILLARG_BOOL, key, value, 0, 0);

    return pdc_false;		/* compilers love it */
}

PDFLIB_API void PDFLIB_CALL
PDF_set_parameter(PDF *p, const char *key, const char *value)
{
    static const char fn[] = "PDF_set_parameter";
    pdc_usebox usebox = use_none;
    pdc_text_format textformat = pdc_auto;
    int i, k;

    if (key == NULL || !*key)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "key", 0, 0, 0);

    i = get_index(key);

    if (!pdf_enter_api(p, fn, (pdf_state) pdf_state_all,
	"(p[%p], \"%s\", \"%s\")\n", (void *) p, key, value))
	return;

    if (i == -1)
	pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);

    if ((p->state_stack[p->state_sp] & parms[i].set_scope) == 0)
	pdc_error(p->pdc, PDF_E_DOC_SCOPE_SET, key, pdf_current_scope(p), 0, 0);

    if (value == NULL)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "value", 0, 0, 0);

    switch (i)
    {
        case PDF_PARAMETER_PDIUSEBOX:
        case PDF_PARAMETER_VIEWAREA:
        case PDF_PARAMETER_VIEWCLIP:
        case PDF_PARAMETER_PRINTAREA:
        case PDF_PARAMETER_PRINTCLIP:
            k = pdc_get_keycode(value, pdf_usebox_keylist);
            if (k == PDC_KEY_NOTFOUND)
                pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
            usebox = (pdc_usebox) k;
            break;

        case PDF_PARAMETER_TEXTFORMAT:
        case PDF_PARAMETER_HYPERTEXTFORMAT:
            k = pdc_get_keycode(value, pdf_textformat_keylist);
            if (k == PDC_KEY_NOTFOUND)
                pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
            textformat = (pdc_text_format) k;
            break;
    }

    switch (i)
    {
        case PDF_PARAMETER_SEARCHPATH:
        case PDF_PARAMETER_FONTAFM:
	case PDF_PARAMETER_FONTPFM:
        case PDF_PARAMETER_FONTOUTLINE:
        case PDF_PARAMETER_HOSTFONT:
        case PDF_PARAMETER_ENCODING:
        case PDF_PARAMETER_ICCPROFILE:
        case PDF_PARAMETER_STANDARDOUTPUTINTENT:
	{
            pdf_add_resource(p, key, value);
            break;
        }

	case PDF_PARAMETER_FLUSH:
	    if (p->binding != NULL && strcmp(p->binding, "C++"))
		break;

	    if (!strcmp(value, "none"))
		p->flush = pdf_flush_none;
	    else if (!strcmp(value, "page"))
                p->flush = (pdf_flush_state) (p->flush | pdf_flush_page);
	    else if (!strcmp(value, "content"))
                p->flush = (pdf_flush_state) (p->flush | pdf_flush_content);
	    else if (!strcmp(value, "heavy"))
                p->flush = (pdf_flush_state) (p->flush | pdf_flush_heavy);
	    else
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);

	    break;

	case PDF_PARAMETER_DEBUG:
	{
	    const unsigned char *c;

	    for (c = (const unsigned char *) value; *c; c++) {
		p->debug[(int) *c] = 1;

		if (*c == 't') {
		    pdc_set_trace(p->pdc, "PDFlib " PDFLIB_VERSIONSTRING);
		}
	    }
	    break;
	}

	case PDF_PARAMETER_NODEBUG:
	{
	    const unsigned char *c;

	    for (c = (const unsigned char *) value; *c; c++) {
		if (*c == 't')
		    pdc_set_trace(p->pdc, NULL);

		p->debug[(int) *c] = 0;
	    }
	    break;
	}

        case PDF_PARAMETER_BINDING:
            if (!p->binding)
                p->binding = pdc_strdup(p->pdc, value);
            break;

        case PDF_PARAMETER_HASTOBEPOS:
            p->hastobepos = pdf_bool_value(p, key, value);
            break;

	case PDF_PARAMETER_UNDERLINE:
	    p->underline = pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_OVERLINE:
	    p->overline = pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_STRIKEOUT:
	    p->strikeout = pdf_bool_value(p, key, value);
	    break;

        case PDF_PARAMETER_KERNING:
            pdc_warning(p->pdc, PDF_E_UNSUPP_KERNING, 0, 0, 0, 0);
            break;

        case PDF_PARAMETER_AUTOSUBSETTING:
            pdc_warning(p->pdc, PDF_E_UNSUPP_SUBSET, 0, 0, 0, 0);
            break;

        case PDF_PARAMETER_AUTOCIDFONT:
            pdc_warning(p->pdc, PDF_E_UNSUPP_UNICODE, 0, 0, 0, 0);
            break;

        case PDF_PARAMETER_UNICODEMAP:
            pdc_warning(p->pdc, PDF_E_UNSUPP_UNICODE, 0, 0, 0, 0);
            break;

	case PDF_PARAMETER_MASTERPASSWORD:
	    pdc_warning(p->pdc, PDF_E_UNSUPP_CRYPT, 0, 0, 0, 0);
	    break;

	case PDF_PARAMETER_USERPASSWORD:
            pdc_warning(p->pdc, PDF_E_UNSUPP_CRYPT, 0, 0, 0, 0);
	    break;

	case PDF_PARAMETER_PERMISSIONS:
            pdc_warning(p->pdc, PDF_E_UNSUPP_CRYPT, 0, 0, 0, 0);
	    break;

	case PDF_PARAMETER_COMPATIBILITY:

	    if (!strcmp(value, "1.3"))
		p->compatibility = PDC_1_3;
	    else if (!strcmp(value, "1.4"))
		p->compatibility = PDC_1_4;
	    else if (!strcmp(value, "1.5"))
		p->compatibility = PDC_1_5;
	    else
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    break;

	case PDF_PARAMETER_PDFX:
	    pdc_warning(p->pdc, PDF_E_UNSUPP_PDFX, 0, 0, 0, 0);

	    break;


        case PDF_PARAMETER_RESOURCEFILE:
            if (p->resourcefilename)
            {
                pdc_free(p->pdc, p->resourcefilename);
                p->resourcefilename = NULL;
            }
            p->resourcefilename = pdc_strdup(p->pdc, value);
            p->resfilepending = pdc_true;
            break;

	case PDF_PARAMETER_PREFIX:
            if (p->prefix)
            {
                pdc_free(p->pdc, p->prefix);
                p->prefix = NULL;
            }
            /* because of downward compatibility */
            p->prefix = pdc_strdup(p->pdc, &value[value[0] == '/' ? 1 : 0]);
	    break;

	case PDF_PARAMETER_WARNING:
	    pdc_set_warnings(p->pdc, pdf_bool_value(p, key, value));
	    break;

        case PDF_PARAMETER_OPENWARNING:
            p->debug['o'] = (char) pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_FONTWARNING:
            p->debug['F'] = (char) pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_ICCWARNING:
            p->debug['I'] = (char) pdf_bool_value(p, key, value);
            break;

	case PDF_PARAMETER_IMAGEWARNING:
	    p->debug['i'] = (char) pdf_bool_value(p, key, value);
	    break;

        case PDF_PARAMETER_PDIWARNING:
            p->debug['p'] = (char) pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_HONORICCPROFILE:
            p->debug['e'] = (char) pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_GLYPHWARNING:
            p->debug['g'] = (char) pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_RENDERINGINTENT:
            k = pdc_get_keycode(value, gs_renderingintents);
            if (k == PDC_KEY_NOTFOUND)
                pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
            p->rendintent = (pdf_renderingintent) k;
            break;

        case PDF_PARAMETER_PRESERVEOLDPANTONENAMES:
            p->preserveoldpantonenames = pdf_bool_value(p, key, value);
            break;

        case PDF_PARAMETER_SPOTCOLORLOOKUP:
            p->spotcolorlookup = pdf_bool_value(p, key, value);
            break;

	case PDF_PARAMETER_INHERITGSTATE:
	    p->inheritgs = pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_PDISTRICT:
	    p->pdi_strict = pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_PDIUSEBOX:
            p->pdi_usebox = usebox;
            break;

	case PDF_PARAMETER_PASSTHROUGH:
	    p->debug['P'] = (char) !pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_HIDETOOLBAR:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= HideToolbar;
	    break;

	case PDF_PARAMETER_HIDEMENUBAR:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= HideMenubar;
	    break;

	case PDF_PARAMETER_HIDEWINDOWUI:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= HideWindowUI;
	    break;

	case PDF_PARAMETER_FITWINDOW:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= FitWindow;
	    break;

	case PDF_PARAMETER_CENTERWINDOW:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= CenterWindow;
	    break;

	case PDF_PARAMETER_DISPLAYDOCTITLE:
	    if (pdf_bool_value(p, key, value))
		p->ViewerPreferences.flags |= DisplayDocTitle;
	    break;

	case PDF_PARAMETER_NONFULLSCREENPAGEMODE:
	    if (!strcmp(value, "useoutlines")) {
		p->ViewerPreferences.flags |= NonFullScreenPageModeOutlines;
		p->ViewerPreferences.flags &= ~NonFullScreenPageModeThumbs;
	    } else if (!strcmp(value, "usethumbs")) {
		p->ViewerPreferences.flags &= ~NonFullScreenPageModeOutlines;
		p->ViewerPreferences.flags |= NonFullScreenPageModeThumbs;
	    } else if (!strcmp(value, "usenone")) {
		p->ViewerPreferences.flags &= ~NonFullScreenPageModeOutlines;
		p->ViewerPreferences.flags &= ~NonFullScreenPageModeThumbs;
	    } else {
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    }

	    break;

	case PDF_PARAMETER_DIRECTION:
	    if (!strcmp(value, "r2l")) {
		p->ViewerPreferences.flags |= DirectionR2L;
	    } else if (!strcmp(value, "l2r")) {
		p->ViewerPreferences.flags &= ~DirectionR2L;
	    } else {
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    }
	    break;

	case PDF_PARAMETER_VIEWAREA:
            p->ViewerPreferences.ViewArea = usebox;
	    break;

	case PDF_PARAMETER_VIEWCLIP:
            p->ViewerPreferences.ViewClip = usebox;
            break;

	case PDF_PARAMETER_PRINTAREA:
            p->ViewerPreferences.PrintArea = usebox;
            break;

	case PDF_PARAMETER_PRINTCLIP:
            p->ViewerPreferences.PrintClip = usebox;
            break;

        case PDF_PARAMETER_TOPDOWN:
            if (pdf_bool_value(p, key, value))
                p->ydirection = (float) -1.0;
            else
                p->ydirection = (float) 1.0;
            break;

        case PDF_PARAMETER_USERCOORDINATES:
            p->usercoordinates = pdf_bool_value(p, key, value);
            break;

	case PDF_PARAMETER_OPENACTION:
	    pdf_cleanup_destination(p, &p->open_action);

	    pdf_parse_destination_optlist(p,
                value, &p->open_action, 1, pdf_openaction);
	    break;

	case PDF_PARAMETER_OPENMODE:
	    if (!strcmp(value, "none")) {
		p->open_mode = open_none;
	    } else if (!strcmp(value, "bookmarks")) {
		p->open_mode = open_bookmarks;
	    } else if (!strcmp(value, "thumbnails")) {
		p->open_mode = open_thumbnails;
	    } else if (!strcmp(value, "fullscreen")) {
		p->open_mode = open_fullscreen;
	    } else {
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    }
	    break;

	case PDF_PARAMETER_BOOKMARKDEST:
	    pdf_cleanup_destination(p, &p->bookmark_dest);

	    pdf_parse_destination_optlist(p,
                value, &p->bookmark_dest, 0, pdf_bookmark);
	    break;

	case PDF_PARAMETER_FILLRULE:
	    if (!strcmp(value, "winding")) {
		p->fillrule = pdf_fill_winding;
	    } else if (!strcmp(value, "evenodd")) {
		p->fillrule = pdf_fill_evenodd;
	    } else {
		pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    }
	    break;

        case PDF_PARAMETER_TEXTFORMAT:
            p->textformat = textformat;
            break;

        case PDF_PARAMETER_HYPERTEXTFORMAT:
            p->hypertextformat = textformat;
            break;

        case PDF_PARAMETER_HYPERTEXTENCODING:
        {
            pdc_encoding enc;

            if (!*value)
            {
                enc = pdc_unicode;
            }
            else
            {
                enc = pdf_find_encoding(p, (const char *) value);
                if (enc == pdc_invalidenc)
                    enc = pdf_insert_encoding(p, (const char *) value);
                if (enc < 0 && enc != pdc_unicode)
                    pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
            }
            p->hypertextencoding = enc;
            break;
        }

	/* deprecated */
	case PDF_PARAMETER_NATIVEUNICODE:
	    (void) pdf_bool_value(p, key, value);
	    break;

	case PDF_PARAMETER_TRANSITION:
	    pdf_set_transition(p, value);
	    break;

	case PDF_PARAMETER_BASE:
	    if (p->base) {
		pdc_free(p->pdc, p->base);
		p->base = NULL;
	    }

	    p->base = pdc_strdup(p->pdc, value);
	    break;

	case PDF_PARAMETER_LAUNCHLINK_PARAMETERS:
	    if (p->launchlink_parameters) {
		pdc_free(p->pdc, p->launchlink_parameters);
		p->launchlink_parameters = NULL;
	    }
	    p->launchlink_parameters = pdc_strdup(p->pdc, value);
	    break;

	case PDF_PARAMETER_LAUNCHLINK_OPERATION:
	    if (p->launchlink_operation) {
		pdc_free(p->pdc, p->launchlink_operation);
		p->launchlink_operation = NULL;
	    }
	    p->launchlink_operation = pdc_strdup(p->pdc, value);
	    break;

	case PDF_PARAMETER_LAUNCHLINK_DEFAULTDIR:
	    if (p->launchlink_defaultdir) {
		pdc_free(p->pdc, p->launchlink_defaultdir);
		p->launchlink_defaultdir = NULL;
	    }
	    p->launchlink_defaultdir = pdc_strdup(p->pdc, value);
	    break;

	case PDF_PARAMETER_TRACE:
	    if (pdf_bool_value(p, key, value)) {
		p->debug['t'] = 1;
		pdc_set_trace(p->pdc, "PDFlib " PDFLIB_VERSIONSTRING);
	    } else {
		pdc_set_trace(p->pdc, NULL);
		p->debug['t'] = 0;
	    }
	    break;

	case PDF_PARAMETER_TRACEFILE:
	    pdc_set_tracefile(p->pdc, value);
	    break;

	case PDF_PARAMETER_TRACEMSG:
	    /* do nothing -- client-supplied string will show up in the trace */
	    break;

	case PDF_PARAMETER_SERIAL:
	case PDF_PARAMETER_LICENSE:
	    break;

	case PDF_PARAMETER_LICENSEFILE:
	    break;

	default:
	    pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);
	    break;
    } /* switch */
} /* PDF_set_parameter */

static float
pdf_value(PDF *p, const char *key, float value, int minver)
{
    if (p->compatibility < minver)
	pdc_error(p->pdc, PDC_E_PAR_VERSION,
	    key, pdc_errprintf(p->pdc, "%d.%d", minver/10, minver%10), 0, 0);

    return value;
}

static float
pdf_pos_value(PDF *p, const char *key, float value, int minver)
{
    if (p->compatibility < minver)
	pdc_error(p->pdc, PDC_E_PAR_VERSION,
	    key, pdc_errprintf(p->pdc, "%d.%d", minver/10, minver%10), 0, 0);

    if (value <= (float) 0)
	pdc_error(p->pdc, PDC_E_PAR_ILLVALUE,
	    pdc_errprintf(p->pdc, "%f", value), key, 0, 0);

    return value;
}

PDFLIB_API void PDFLIB_CALL
PDF_set_value(PDF *p, const char *key, float value)
{
    static const char fn[] = "PDF_set_value";
    int i;
    int ivalue = (int) value;

    if (key == NULL || !*key)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "key", 0, 0, 0);

    i = get_index(key);

    if (!pdf_enter_api(p, fn, (pdf_state) pdf_state_all,
	"(p[%p], \"%s\", %g)\n", (void *) p, key, value))
	return;

    if (i == -1)
	pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);

    if ((p->state_stack[p->state_sp] & parms[i].set_scope) == 0)
	pdc_error(p->pdc, PDF_E_DOC_SCOPE_SET, key, pdf_current_scope(p), 0, 0);

    switch (i)
    {
	case PDF_PARAMETER_COMPRESS:
	    if (ivalue < 0 || ivalue > 9)
		pdc_error(p->pdc, PDC_E_PAR_ILLVALUE,
		    pdc_errprintf(p->pdc, "%f", value), key, 0, 0);

	    if (pdc_get_compresslevel(p->out) != ivalue)
	    {
		/*
		 * We must restart the compression engine and start a new
		 * contents section if we're in the middle of a page.
		 */
		if (PDF_GET_STATE(p) == pdf_state_page) {
		    pdf_end_contents_section(p);
		    pdc_set_compresslevel(p->out, ivalue);
		    pdf_begin_contents_section(p);
		} else
		    pdc_set_compresslevel(p->out, ivalue);
	    }

	    break;

	case PDF_PARAMETER_FLOATDIGITS:
	    if (3 <= ivalue && ivalue <= 6)
		pdc_set_floatdigits(p->pdc, ivalue);
	    else
		pdc_error(p->pdc, PDC_E_PAR_ILLVALUE,
		    pdc_errprintf(p->pdc, "%d", ivalue), key, 0, 0);
	    break;

	case PDF_PARAMETER_PAGEWIDTH:
            if (p->ydirection == -1)
                pdc_error(p->pdc, PDF_E_PAGE_ILLCHGSIZE, 0, 0, 0, 0);
	    if (p->compatibility >= PDC_1_3 &&
		(value < PDF_ACRO4_MINPAGE || value > PDF_ACRO4_MAXPAGE))
		    pdc_warning(p->pdc, PDF_E_PAGE_SIZE_ACRO4, 0, 0, 0, 0);

	    p->width = pdf_pos_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_PAGEHEIGHT:
            if (p->ydirection == -1)
                pdc_error(p->pdc, PDF_E_PAGE_ILLCHGSIZE, 0, 0, 0, 0);
	    if (p->compatibility >= PDC_1_3 &&
		(value < PDF_ACRO4_MINPAGE || value > PDF_ACRO4_MAXPAGE))
		    pdc_warning(p->pdc, PDF_E_PAGE_SIZE_ACRO4, 0, 0, 0, 0);

	    p->height = pdf_pos_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_CROPBOX_LLX:
	    p->CropBox.llx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_CROPBOX_LLY:
	    p->CropBox.lly = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_CROPBOX_URX:
	    p->CropBox.urx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_CROPBOX_URY:
	    p->CropBox.ury = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_BLEEDBOX_LLX:
	    p->BleedBox.llx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_BLEEDBOX_LLY:
	    p->BleedBox.lly = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_BLEEDBOX_URX:
	    p->BleedBox.urx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_BLEEDBOX_URY:
	    p->BleedBox.ury = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_TRIMBOX_LLX:
	    p->TrimBox.llx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_TRIMBOX_LLY:
	    p->TrimBox.lly = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_TRIMBOX_URX:
	    p->TrimBox.urx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_TRIMBOX_URY:
	    p->TrimBox.ury = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_ARTBOX_LLX:
	    p->ArtBox.llx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_ARTBOX_LLY:
	    p->ArtBox.lly = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_ARTBOX_URX:
	    p->ArtBox.urx = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_ARTBOX_URY:
	    p->ArtBox.ury = pdf_value(p, key, value, PDC_1_3);
	    break;

	case PDF_PARAMETER_LEADING:
	    pdf_set_leading(p, value);
	    break;

	case PDF_PARAMETER_TEXTRISE:
	    pdf_set_text_rise(p, value);
	    break;

	case PDF_PARAMETER_HORIZSCALING:
	    pdf_set_horiz_scaling(p, value);
	    break;

	case PDF_PARAMETER_TEXTRENDERING:
	    pdf_set_text_rendering(p, (int) value);
	    break;

	case PDF_PARAMETER_CHARSPACING:
	    pdf_set_char_spacing(p, value);
	    break;

	case PDF_PARAMETER_WORDSPACING:
	    pdf_set_word_spacing(p, value);
	    break;

	case PDF_PARAMETER_DURATION:
	    pdf_set_duration(p, value);
	    break;

	case PDF_PARAMETER_DEFAULTGRAY:
	    break;

	case PDF_PARAMETER_DEFAULTRGB:
	    break;

	case PDF_PARAMETER_DEFAULTCMYK:
	    break;

        case PDF_PARAMETER_SETCOLOR_ICCPROFILEGRAY:
            break;

        case PDF_PARAMETER_SETCOLOR_ICCPROFILERGB:
            break;

        case PDF_PARAMETER_SETCOLOR_ICCPROFILECMYK:
            break;

        case PDF_PARAMETER_SUBSETLIMIT:
            pdc_warning(p->pdc, PDF_E_UNSUPP_SUBSET, 0, 0, 0, 0);
            break;

        case PDF_PARAMETER_SUBSETMINSIZE:
            pdc_warning(p->pdc, PDF_E_UNSUPP_SUBSET, 0, 0, 0, 0);
            break;

	default:
	    pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);
	    break;
    } /* switch */
} /* PDF_set_value */

PDFLIB_API float PDFLIB_CALL
PDF_get_value(PDF *p, const char *key, float mod)
{
    static const char fn[] = "PDF_get_value";
    int i = -1;
    int imod = (int) mod;
    float result = (float) 0;

    /* some parameters can be retrieved with p == 0.
    */
    if (key != NULL && (i = get_index(key)) != -1)
    {
	switch (i)
	{
	    case PDF_PARAMETER_MAJOR:
		result = PDFLIB_MAJORVERSION;
		return result;

	    case PDF_PARAMETER_MINOR:
		result = PDFLIB_MINORVERSION;
		return result;

	    case PDF_PARAMETER_REVISION:
		result = PDFLIB_REVISION;
		return result;
	} /* switch */
    }

    if (!pdf_enter_api(p, fn, (pdf_state) pdf_state_all,
	"(p[%p], \"%s\", %g)", (void *) p, key, mod))
	return (float) 0;

    if (i == -1)
        pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);

    if ((p->state_stack[p->state_sp] & parms[i].get_scope) == 0)
	pdc_error(p->pdc, PDF_E_DOC_SCOPE_GET, key, pdf_current_scope(p), 0, 0);

    if (key == NULL || !*key)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "key", 0, 0, 0);

    if (parms[i].mod_zero && mod != (float) 0)
	pdc_error(p->pdc, PDC_E_PAR_ILLVALUE,
	    pdc_errprintf(p->pdc, "%f", mod), key, 0, 0);

    switch (i)
    {
        case PDF_PARAMETER_IMAGEWIDTH:
        case PDF_PARAMETER_IMAGEHEIGHT:
        case PDF_PARAMETER_RESX:
        case PDF_PARAMETER_RESY:
            PDF_INPUT_HANDLE(p, imod)
            pdf_check_handle(p, imod, pdc_imagehandle);
            break;

        case PDF_PARAMETER_FONTMAXCODE:
        case PDF_PARAMETER_CAPHEIGHT:
        case PDF_PARAMETER_ASCENDER:
        case PDF_PARAMETER_DESCENDER:
            PDF_INPUT_HANDLE(p, imod)
            pdf_check_handle(p, imod, pdc_fonthandle);
            break;
    }

    switch (i)
    {
        case PDF_PARAMETER_PAGEWIDTH:
            result = p->width;
            break;

        case PDF_PARAMETER_PAGEHEIGHT:
            result = p->height;
            break;

        case PDF_PARAMETER_IMAGEWIDTH:
            result = (float) (p->images[imod].width);
            break;

        case PDF_PARAMETER_IMAGEHEIGHT:
            result = (float) fabs((double) (p->images[imod].height));
            break;

        case PDF_PARAMETER_RESX:
            result = (float) (p->images[imod].dpi_x);
            break;

        case PDF_PARAMETER_RESY:
            result = (float) (p->images[imod].dpi_y);
            break;

        case PDF_PARAMETER_IMAGE_ICCPROFILE:
            break;

        case PDF_PARAMETER_ICCCOMPONENTS:
            break;

        case PDF_PARAMETER_CURRENTX:
	    result = (float) (p->gstate[p->sl].x);
	    break;

	case PDF_PARAMETER_CURRENTY:
	    result = (float) (p->gstate[p->sl].y);
	    break;

	case PDF_PARAMETER_TEXTX:
	    result = (float) (p->tstate[p->sl].m.e);
	    break;

	case PDF_PARAMETER_TEXTY:
	    result = (float) (p->tstate[p->sl].m.f);
	    break;

	case PDF_PARAMETER_WORDSPACING:
	    result = (float) (p->tstate[p->sl].w);
	    break;

	case PDF_PARAMETER_CHARSPACING:
	    result = (float) (p->tstate[p->sl].c);
	    break;

	case PDF_PARAMETER_HORIZSCALING:
            result = pdf_get_horiz_scaling(p);
	    break;

	case PDF_PARAMETER_TEXTRISE:
	    result = (float) (p->tstate[p->sl].rise);
	    break;

	case PDF_PARAMETER_LEADING:
	    result = (float) (p->tstate[p->sl].l);
	    break;

	case PDF_PARAMETER_TEXTRENDERING:
	    result = (float) (p->tstate[p->sl].mode);
	    break;

	case PDF_PARAMETER_FONT:
	    result = (float) (pdf_get_font(p));
            if (p->hastobepos) result += 1; \
	    break;

        case PDF_PARAMETER_MONOSPACE:
            result = (float) pdf_get_monospace(p);
            break;

        case PDF_PARAMETER_FONTSIZE:
            result = (float) (pdf_get_fontsize(p));
            break;

        case PDF_PARAMETER_FONTMAXCODE:
            result = (float) (p->fonts[imod].numOfCodes - 1);
            break;

	case PDF_PARAMETER_CAPHEIGHT:
	    result = (float) (p->fonts[imod].capHeight / (float) 1000);
	    break;

	case PDF_PARAMETER_ASCENDER:
	    result = (float) (p->fonts[imod].ascender / (float) 1000);
	    break;

	case PDF_PARAMETER_DESCENDER:
	    result = (float) (p->fonts[imod].descender / (float) 1000);
	    break;

        case PDF_PARAMETER_SUBSETLIMIT:
            break;

	default:
	    pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);
	    break;
    } /* switch */

    pdc_trace(p->pdc, "[%g]\n", result);
    return result;
} /* PDF_get_value */

PDFLIB_API const char * PDFLIB_CALL
PDF_get_parameter(PDF *p, const char *key, float mod)
{
    static const char fn[] = "PDF_get_parameter";
    int i = -1;
    const char *result = "";

    /* some parameters can be retrieved with p == 0.
    */
    if (key != NULL && (i = get_index(key)) != -1)
    {
	switch (i)
	{
	    case PDF_PARAMETER_VERSION:
		result = PDFLIB_VERSIONSTRING;
		return result;

	    case PDF_PARAMETER_PDI:
		result = "false";
		return result;
	} /* switch */
    }

    if (!pdf_enter_api(p, fn, (pdf_state) pdf_state_all,
	"(p[%p], \"%s\", %g);", (void *) p, key, mod))
        return "";

    if (key == NULL || !*key)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "key", 0, 0, 0);

    if (i == -1)
        pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);

    if ((p->state_stack[p->state_sp] & parms[i].get_scope) == 0)
	pdc_error(p->pdc, PDF_E_DOC_SCOPE_GET, key, pdf_current_scope(p), 0, 0);

    if (parms[i].mod_zero && mod != (float) 0)
	pdc_error(p->pdc, PDC_E_PAR_ILLPARAM,
	    pdc_errprintf(p->pdc, "%f", mod), key, 0, 0);

    switch (i)
    {
	case PDF_PARAMETER_FONTNAME:
	    result = pdf_get_fontname(p);
	    break;

        case PDF_PARAMETER_FONTSTYLE:
            result = pdf_get_fontstyle(p);
            break;

        case PDF_PARAMETER_FONTENCODING:
            result = pdf_get_fontencoding(p);
            break;

	case PDF_PARAMETER_UNDERLINE:
            result = PDC_BOOLSTR(p->underline);
	    break;

	case PDF_PARAMETER_OVERLINE:
            result = PDC_BOOLSTR(p->overline);
	    break;

	case PDF_PARAMETER_STRIKEOUT:
            result = PDC_BOOLSTR(p->strikeout);
	    break;

	case PDF_PARAMETER_INHERITGSTATE:
            result = PDC_BOOLSTR(p->inheritgs);
	    break;

	case PDF_PARAMETER_SCOPE:
	    switch (p->state_stack[p->state_sp]) {
		case pdf_state_object:	result = "object";	break;
		case pdf_state_document:result = "document";	break;
		case pdf_state_page:	result = "page";	break;
		case pdf_state_pattern:	result = "pattern";	break;
		case pdf_state_template:result = "template";	break;
		case pdf_state_path:	result = "path";	break;
		default:		result = "(unknown)";	break;
	    }
	    break;

        case PDF_PARAMETER_TEXTFORMAT:
            result = pdc_get_keyword(p->textformat, pdf_textformat_keylist);
            break;

        case PDF_PARAMETER_HYPERTEXTFORMAT:
            result = pdc_get_keyword(p->hypertextformat,pdf_textformat_keylist);
            break;

        case PDF_PARAMETER_HYPERTEXTENCODING:
            result = pdf_get_encoding_name(p, p->hypertextencoding);
            break;

	default:
	    pdc_error(p->pdc, PDC_E_PAR_UNKNOWNKEY, key, 0, 0, 0);
	    break;
    } /* switch */

    pdc_trace(p->pdc, "[%s]\n", result);
    return result;
} /* PDF_get_parameter */
