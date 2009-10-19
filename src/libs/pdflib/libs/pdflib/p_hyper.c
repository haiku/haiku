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

/* $Id: p_hyper.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines for hypertext stuff: bookmarks, document info, transitions
 *
 */

#define P_HYPER_C

#include "p_intern.h"


/* We can't work with pointers in the outline objects because
 * the complete outline block may be reallocated. Therefore we use
 * this simple mechanism for achieving indirection.
 */
#define COUNT(index)	(p->outlines[index].count)
#define OPEN(index)	(p->outlines[index].open)
#define LAST(index)	(p->outlines[index].last)
#define PARENT(index)	(p->outlines[index].parent)
#define FIRST(index)	(p->outlines[index].first)
#define SELF(index)	(p->outlines[index].self)
#define PREV(index)	(p->outlines[index].prev)
#define NEXT(index)	(p->outlines[index].next)

struct pdf_outline_s {
    pdc_id		self;		/* id of this outline object */
    pdc_id		prev;		/* previous entry at this level */
    pdc_id		next;		/* next entry at this level */
    int			parent;		/* ancestor's index */
    int			first;		/* first sub-entry */
    int			last;		/* last sub-entry */
    char 		*text;		/* bookmark text */
    int			count;		/* number of open sub-entries */
    int			open;		/* whether or not to display children */
    pdf_dest		dest;		/* outline destination */
};

struct pdf_info_s {
    char		*key;		/* ASCII string */
    char		*value;		/* Unicode string */
    pdf_info		*next;		/* next info entry */
};

struct pdf_name_s {
    pdc_id		obj_id;		/* id of this name object */
    char *		name;		/* name string */
    int 		len;		/* length of name string */
};

void
pdf_init_outlines(PDF *p)
{
    p->outline_count	= 0;
}

/* Free outline entries */
void
pdf_cleanup_outlines(PDF *p)
{
    int i;

    if (!p->outlines || p->outline_count == 0)
	return;

    /* outlines[0] is the outline root object */
    for (i = 0; i <= p->outline_count; i++)
    {
	if (p->outlines[i].text)
	    pdc_free(p->pdc, p->outlines[i].text);
        pdf_cleanup_destination(p, &p->outlines[i].dest);
    }

    pdc_free(p->pdc, (void*) p->outlines);

    p->outlines = NULL;
}

static const pdc_keyconn pdf_type_keylist[] =
{
    {"fixed",   	fixed},
    {"fitwindow",   	fitwindow},
    {"fitwidth",   	fitwidth},
    {"fitheight",   	fitheight},
    {"fitrect",   	fitrect},
    {"fitvisible",   	fitvisible},
    {"fitvisiblewidth", fitvisiblewidth},
    {"fitvisibleheight",fitvisibleheight},
    {"nameddest",	nameddest},
    {"file",	        filedest},
    {NULL, 0}
};

#define PDF_MAXDESTLEN	64000

static const pdc_defopt pdf_destination_options[] =
{
    {"fitbbox", pdc_booleanlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, NULL},

    {"fitheight", pdc_booleanlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, NULL},

    {"fitpage", pdc_booleanlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, NULL},

    {"fitwidth", pdc_booleanlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, NULL},

    {"retain", pdc_booleanlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, NULL},

    {"type", pdc_keywordlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0, pdf_type_keylist},

    {"name", pdc_stringlist, PDC_OPT_NONE, 1, 1, 0.0, PDF_MAXDESTLEN, NULL},

    /* page 0 means "current page" for PDF_add_bookmark() */
    {"page", pdc_integerlist, PDC_OPT_NONE, 1, 1, 0, INT_MAX, NULL},

    /* Acrobat 5 supports a maximum zoom of 1600%, but we allow some more */
    {"zoom", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0.0, 10000, NULL},

    {"left", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0.0, PDF_ACRO4_MAXPAGE, NULL},

    {"right", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0.0, PDF_ACRO4_MAXPAGE, NULL},

    {"bottom", pdc_floatlist, PDC_OPT_REQUIRIF1, 1, 1, 0.0, PDF_ACRO4_MAXPAGE,
     NULL},

    {"top", pdc_floatlist, PDC_OPT_NONE, 1, 1, 0.0, PDF_ACRO4_MAXPAGE, NULL},

    {"color", pdc_floatlist, PDC_OPT_NONE, 1, 3, 0.0, 1.0, NULL},

    {"fontstyle", pdc_keywordlist, PDC_OPT_NONE, 1, 1, 0.0, 0.0,
     pdf_fontstyle_keylist},

    {"filename", pdc_stringlist, PDC_OPT_NONE, 1, 1, 0.0, PDF_FILENAMELEN,
     NULL},

    PDC_OPT_TERMINATE
};

void
pdf_init_destination(PDF *p, pdf_dest *dest)
{
    (void) p;

    dest->type = fitwindow;
    dest->remote = pdc_false;
    dest->page = 0;
    dest->left = -1;
    dest->right = -1;
    dest->bottom = -1;
    dest->top = -1;
    dest->zoom = -1;
    dest->name = NULL;
    dest->color[0] = (float) 0.0;
    dest->color[1] = (float) 0.0;
    dest->color[2] = (float) 0.0;
    dest->fontstyle = pdc_Normal;
    dest->filename = NULL;
}

static void
pdf_copy_destination(PDF *p, pdf_dest *dest, pdf_dest *source)
{
    *dest = *source;
    if (source->name)
        dest->name = pdc_strdup(p->pdc, source->name);
    if (source->filename)
        dest->filename = pdc_strdup(p->pdc, source->filename);
}

void
pdf_cleanup_destination(PDF *p, pdf_dest *dest)
{
    if (dest->name)
        pdc_free(p->pdc, dest->name);
    dest->name = NULL;
    if (dest->filename)
        pdc_free(p->pdc, dest->filename);
    dest->filename = NULL;
}

void
pdf_parse_destination_optlist(
    PDF *p,
    const char *optlist,
    pdf_dest *dest,
    int page,
    pdf_destuse destuse)
{
    pdc_resopt *results;
    const char *keyword;
    const char *type_name;
    char **name = NULL;
    char **filename = NULL;
    int inum, minpage;
    pdc_bool boolval;

    /* Defaults */
    pdf_init_destination(p, dest);
    dest->page = page;

    /* parse option list */
    results = pdc_parse_optionlist(p->pdc, optlist, pdf_destination_options,
                                   NULL, pdc_true);

    if (pdc_get_optvalues(p->pdc, "fitbbox", results, &boolval, NULL) &&
        boolval == pdc_true)
        dest->type = fitvisible;

    if (pdc_get_optvalues(p->pdc, "fitheight", results, &boolval, NULL) &&
        boolval == pdc_true)
        dest->type = fitheight;

    if (pdc_get_optvalues(p->pdc, "fitpage", results, &boolval, NULL) &&
        boolval == pdc_true)
        dest->type = fitwindow;

    if (pdc_get_optvalues(p->pdc, "fitwidth", results, &boolval, NULL) &&
        boolval == pdc_true)
        dest->type = fitwidth;

    if (pdc_get_optvalues(p->pdc, "retain", results, &boolval, NULL) &&
        boolval == pdc_true)
        dest->type = fixed;

    if (pdc_get_optvalues(p->pdc, "type", results, &inum, NULL))
        dest->type = (pdf_desttype) inum;
    type_name = pdc_get_keyword(dest->type, pdf_type_keylist);

    keyword = "name";
    if (pdc_get_optvalues(p->pdc, keyword, results, NULL, (void **) &name) &&
        dest->type != nameddest)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "page";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->page, NULL) &&
        dest->type == filedest)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "zoom";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->zoom, NULL) &&
        dest->type != fixed)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "left";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->left, NULL) &&
        (dest->type == fitwindow  || dest->type == fitwidth ||
         dest->type == fitvisible || dest->type == fitvisiblewidth ||
         dest->type == nameddest  || dest->type == filedest))
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "right";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->right, NULL) &&
        dest->type != fitrect)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "bottom";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->bottom, NULL) &&
        dest->type != fitrect)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "top";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->top, NULL) &&
        (dest->type == fitwindow  || dest->type == fitheight ||
         dest->type == fitvisible || dest->type == fitvisibleheight ||
         dest->type == nameddest  || dest->type == filedest))
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    keyword = "color";
    if (pdc_get_optvalues(p->pdc, keyword, results, &dest->color, NULL) &&
        destuse != pdf_bookmark)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORELEM, keyword, 0, 0, 0);

    keyword = "fontstyle";
    if (pdc_get_optvalues(p->pdc, keyword, results, &inum, NULL))
    {
        dest->fontstyle = (pdc_fontstyle) inum;
        if (destuse != pdf_bookmark)
            pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORELEM, keyword, 0, 0, 0);
    }

    keyword = "filename";
    if (pdc_get_optvalues(p->pdc, keyword, results, NULL, (void **) &filename)
        && dest->type != filedest)
        pdc_warning(p->pdc, PDF_E_HYP_OPTIGNORE_FORTYPE, keyword, type_name,
                    0, 0);

    pdc_cleanup_optionlist(p->pdc, results);

    switch (dest->type)
    {
        case fitwidth:
        /* Trick: we don't know the height of a future page yet,
         * so we use a "large" value for top which will do for
         * most pages. If it doesn't work, not much harm is done.
         */
        if (dest->top == -1)
            dest->top = 10000;
        break;

        case fitrect:
        case fitheight:
        case fitvisiblewidth:
        case fitvisibleheight:
        if (dest->left == -1)
            dest->left = 0;
        if (dest->bottom == -1)
            dest->bottom = 0;
        if (dest->right == -1)
            dest->right = 1000;
        if (dest->top == -1)
            dest->top = 1000;
        break;

        case nameddest:
        if (destuse == pdf_nameddest)
        {
            pdc_cleanup_stringlist(p->pdc, name);
            pdc_error(p->pdc, PDC_E_OPT_ILLKEYWORD, "type", type_name, 0, 0);
        }
        if (name)
        {
            dest->name = name[0];
            pdc_free(p->pdc, name);
            name = NULL;
        }
        else
        {
            pdc_error(p->pdc, PDC_E_OPT_NOTFOUND, "name", 0, 0, 0);
        }
        break;

        case filedest:
        if (destuse != pdf_bookmark)
        {
            pdc_cleanup_stringlist(p->pdc, filename);
            pdc_error(p->pdc, PDC_E_OPT_ILLKEYWORD, "type", type_name, 0, 0);
        }
        if (filename)
        {
            dest->filename = filename[0];
            pdc_free(p->pdc, filename);
            filename = NULL;
        }
        else
        {
            pdc_error(p->pdc, PDC_E_OPT_NOTFOUND, "filename", 0, 0, 0);
        }
        break;

        default:
        break;
    }

    if (name)
        pdc_cleanup_stringlist(p->pdc, name);
    if (filename)
        pdc_cleanup_stringlist(p->pdc, filename);

    /* check for minpage */
    minpage = (destuse == pdf_bookmark) ? 0 : 1;
    switch (destuse)
    {
        case pdf_nameddest:
        case pdf_locallink:
        if (dest->page == 0)
            dest->page = p->current_page;
        case pdf_bookmark:
        case pdf_openaction:
        case pdf_remotelink:
        if (dest->page < minpage)
        {
            pdc_error(p->pdc, PDC_E_ILLARG_HANDLE, "page",
                      pdc_errprintf(p->pdc, "%d", dest->page), 0, 0);
        }
        break;
    }

    /* remote flag */
    if (destuse == pdf_remotelink)
        dest->remote = pdc_true;
}

void
pdf_write_destination(PDF *p, pdf_dest *dest)
{
    if (dest->type == nameddest) {
	pdc_put_pdfstring(p->out, dest->name, (int) strlen(dest->name));
	pdc_puts(p->out, "\n");
	return;
    }

    pdc_puts(p->out, "[");

    if (dest->remote) {
	pdc_printf(p->out, "%d", dest->page - 1);	/* zero-based */

    } else {
	/* preallocate page object id for a later page */
	if (dest->page > p->current_page) {
	    while (dest->page >= p->pages_capacity)
		pdf_grow_pages(p);

	    /* if this page has already been used as a link target
	     * it will already have an object id. Otherwise we allocate one.
	     */
	    if (p->pages[dest->page] == PDC_BAD_ID)
		p->pages[dest->page] = pdc_alloc_id(p->out);
	}
	pdc_printf(p->out, "%ld 0 R", p->pages[dest->page]);
    }

    switch (dest->type) {

	case fixed:
	pdc_puts(p->out, "/XYZ ");

	if (dest->left != -1)
	    pdc_printf(p->out, "%f ", dest->left);
	else
	    pdc_puts(p->out, "null ");

	if (dest->top != -1)
	    pdc_printf(p->out, "%f ", dest->top);
	else
	    pdc_puts(p->out, "null ");

	if (dest->zoom != -1)
	    pdc_printf(p->out, "%f", dest->zoom);
	else
	    pdc_puts(p->out, "null");

	break;

	case fitwindow:
	pdc_puts(p->out, "/Fit");
	break;

	case fitwidth:
        pdc_printf(p->out, "/FitH %f", dest->top);
	break;

	case fitheight:
        pdc_printf(p->out, "/FitV %f", dest->left);
	break;

	case fitrect:
	pdc_printf(p->out, "/FitR %f %f %f %f",
	    dest->left, dest->bottom, dest->right, dest->top);
	break;

	case fitvisible:
	pdc_puts(p->out, "/FitB");
	break;

	case fitvisiblewidth:
	pdc_printf(p->out, "/FitBH %f", dest->top);
	break;

	case fitvisibleheight:
	pdc_printf(p->out, "/FitBV %f", dest->left);
	break;

	default:
	break;
    }

    pdc_puts(p->out, "]");
}

static void
pdf_write_outline_dict(PDF *p, int entry)
{
    pdf_dest *dest = &p->outlines[entry].dest;

    pdc_begin_obj(p->out, SELF(entry));   /* outline object */
    pdc_begin_dict(p->out);

    pdc_printf(p->out, "/Parent %ld 0 R\n", SELF(PARENT(entry)));

    /* outline destination */
    if (dest->filename == NULL)
    {
        pdc_puts(p->out, "/Dest");
        pdf_write_destination(p, dest);
        pdc_puts(p->out, "\n");
    }
    else
    {
        pdc_puts(p->out, "/A");
        pdc_begin_dict(p->out);                 /* A dict */
        pdc_puts(p->out, "/Type/Action/S/Launch\n");
        pdc_puts(p->out, "/F");
        pdc_begin_dict(p->out);                 /* F dict */
        pdc_puts(p->out, "/Type/Filespec\n");
        pdc_printf(p->out, "/F");
        pdc_put_pdfstring(p->out, dest->filename,
            (int) strlen(dest->filename));
        pdc_puts(p->out, "\n");
        pdc_end_dict(p->out);                   /* F dict */
        pdc_end_dict(p->out);                   /* A dict */
    }

    pdc_puts(p->out, "/Title");	/* outline text */
    pdc_put_pdfunistring(p->out, p->outlines[entry].text);
    pdc_puts(p->out, "\n");

    if (PREV(entry))
	pdc_printf(p->out, "/Prev %ld 0 R\n", PREV(entry));
    if (NEXT(entry))
	pdc_printf(p->out, "/Next %ld 0 R\n", NEXT(entry));

    if (FIRST(entry)) {
	pdc_printf(p->out, "/First %ld 0 R\n", SELF(FIRST(entry)));
	pdc_printf(p->out, "/Last %ld 0 R\n", SELF(LAST(entry)));
    }
    if (COUNT(entry)) {
	if (OPEN(entry))
	    pdc_printf(p->out, "/Count %d\n", COUNT(entry));	/* open */
	else
	    pdc_printf(p->out, "/Count %d\n", -COUNT(entry));/* closed */
    }

    /* Color */
    if (dest->color[0] != 0.0 || dest->color[1] != 0.0 || dest->color[2] != 0.0)
        pdc_printf(p->out, "/C [%f %f %f]\n",
                   dest->color[0], dest->color[1], dest->color[2]);

    /* FontStyle */
    if (dest->fontstyle != pdc_Normal)
    {
        int fontstyle = 0;
        if (dest->fontstyle == pdc_Bold)
            fontstyle = 2;
        if (dest->fontstyle == pdc_Italic)
            fontstyle = 1;
        if (dest->fontstyle == pdc_BoldItalic)
            fontstyle = 3;
        pdc_printf(p->out, "/F %d\n", fontstyle);
    }

    pdc_end_dict(p->out);
    pdc_end_obj(p->out);			/* outline object */
}

void
pdf_write_outlines(PDF *p)
{
    int i;

    if (p->outline_count == 0)		/* no outlines: return */
	return;

    pdc_begin_obj(p->out, p->outlines[0].self);	/* root outline object */
    pdc_begin_dict(p->out);

    if (p->outlines[0].count != 0)
	pdc_printf(p->out, "/Count %d\n", COUNT(0));
    pdc_printf(p->out, "/First %ld 0 R\n", SELF(FIRST(0)));
    pdc_printf(p->out, "/Last %ld 0 R\n", SELF(LAST(0)));

    pdc_end_dict(p->out);
    pdc_end_obj(p->out);			/* root outline object */

#define PDF_FLUSH_AFTER_MANY_OUTLINES	1000	/* ca. 50-100 KB */
    for (i = 1; i <= p->outline_count; i++) {
	/* reduce memory usage for many outline entries */
	if (i % PDF_FLUSH_AFTER_MANY_OUTLINES)
	    pdc_flush_stream(p->out);

	pdf_write_outline_dict(p, i);
    }
}

void
pdf_write_outline_root(PDF *p)
{
    if (p->outline_count != 0)
	pdc_printf(p->out, "/Outlines %ld 0 R\n", p->outlines[0].self);
}

static int
pdf__add_bookmark(PDF *p, const char *text, int len, int parent, int open)
{
    pdf_outline *self;                  /* newly created outline */
    char *outtext;

    if (parent < 0 || parent > p->outline_count)
        pdc_error(p->pdc, PDC_E_ILLARG_INT,
            "parent", pdc_errprintf(p->pdc, "%d", parent), 0, 0);

    /* convert text string */
    outtext = pdf_convert_hypertext(p, text, len);
    if (!outtext)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "text", 0, 0, 0);

    /* create the root outline object */
    if (p->outline_count == 0) {
        p->outlines = (pdf_outline *) pdc_calloc(p->pdc,
            sizeof(pdf_outline) * OUTLINE_CHUNKSIZE, "PDF_add_bookmark");
        p->outline_capacity = OUTLINE_CHUNKSIZE;

        /* populate the root outline object */
        p->outlines[0].self     = pdc_alloc_id(p->out);
        p->outlines[0].count    = 0;
        p->outlines[0].parent   = 0;
        p->outlines[0].open     = 1;

        /* set the open mode show bookmarks if we have at least one,
         * and the client didn't already set his own open mode.
         */
        if (p->open_mode == open_auto)
            p->open_mode = open_bookmarks;
    }

    /*
     * It's crucial to increase p->outline_count only after
     * successfully having realloc()ed. Otherwise the error handler
     * may try to free too much if the realloc goes wrong.
     */
    if (p->outline_count+1 >= p->outline_capacity) { /* used up all space */
        p->outlines = (pdf_outline *) pdc_realloc(p->pdc, p->outlines,
                        sizeof(pdf_outline) * 2 * p->outline_capacity,
                        "PDF_add_bookmark");
        p->outline_capacity *= 2;
    }

    p->outline_count++;

    self = &p->outlines[p->outline_count];

    self->text          = outtext;

    /* If the global parameter doesn't say otherwise we link to the
     * current page.
     */
    pdf_copy_destination(p, &self->dest, &p->bookmark_dest);

    /* 0 is a shortcut for "current page" */
    if (self->dest.page == 0)
	self->dest.page = p->current_page;

    self->self          = pdc_alloc_id(p->out);
    self->first         = 0;
    self->last          = 0;
    self->prev          = 0;
    self->next          = 0;
    self->count         = 0;
    self->open          = open;
    self->parent        = parent;

    /* insert new outline at the end of the chain or start a new chain */
    if (FIRST(parent) == 0) {
        FIRST(parent) = p->outline_count;
    } else {
        self->prev = SELF(LAST(parent));
        NEXT(LAST(parent))= self->self;
    }

    /* insert new outline as last child of parent in all cases */
    LAST(parent) = p->outline_count;

    /* increase the number of open sub-entries for all relevant ancestors */
    do {
        COUNT(parent)++;
    } while (OPEN(parent) && (parent = PARENT(parent)) != 0);

    return (p->outline_count);          /* caller may use this as handle */
}

PDFLIB_API int PDFLIB_CALL
PDF_add_bookmark(PDF *p, const char *text, int parent, int open)
{
    static const char fn[] = "PDF_add_bookmark";
    int retval = 0;

    if (pdf_enter_api(p, fn, pdf_state_page, "(p[%p], \"%s\", %d, %d)",
        (void *) p, pdc_strprint(p->pdc, text, 0), parent, open))
    {
        int len = text ? (int) pdc_strlen(text) : 0;
        retval = pdf__add_bookmark(p, text, len, parent, open);
    }
    pdc_trace(p->pdc, "[%d]\n", retval);
    return retval;
}

PDFLIB_API int PDFLIB_CALL
PDF_add_bookmark2(PDF *p, const char *text, int len, int parent, int open)
{
    static const char fn[] = "PDF_add_bookmark2";
    int retval = 0;

    if (pdf_enter_api(p, fn, pdf_state_page, "(p[%p], \"%s\", %d, %d, %d)",
        (void *) p, pdc_strprint(p->pdc, text, len), len, parent, open))
    {
        retval = pdf__add_bookmark(p, text, len, parent, open);
    }
    pdc_trace(p->pdc, "[%d]\n", retval);
    return retval;
}

void
pdf_feed_digest_info(PDF *p)
{
    pdf_info        *info;

    if (p->Keywords)
	pdc_update_digest(p->out,
	    (unsigned char *) p->Keywords, strlen(p->Keywords));

    if (p->Subject)
	pdc_update_digest(p->out,
	    (unsigned char *) p->Subject, strlen(p->Subject));

    if (p->Title)
	pdc_update_digest(p->out,
	    (unsigned char *) p->Title, strlen(p->Title));

    if (p->Creator)
	pdc_update_digest(p->out,
	    (unsigned char *) p->Creator, strlen(p->Creator));

    if (p->Author)
	pdc_update_digest(p->out,
	    (unsigned char *) p->Author, strlen(p->Author));

    if (p->userinfo) {
        for (info = p->userinfo; info != NULL; info = info->next) {
            pdc_update_digest(p->out,
                (unsigned char *) info->key, strlen(info->key));
        }
    }
}

void
pdf_init_info(PDF *p)
{
    p->Keywords		= NULL;
    p->Subject		= NULL;
    p->Title		= NULL;
    p->Creator		= NULL;
    p->Author		= NULL;
    p->userinfo		= NULL;

    pdc_get_timestr(p->time_str);
}

#define PDF_TRAPPED_TRUE      "\124\162\165\145"
#define PDF_TRAPPED_FALSE     "\106\141\154\163\145"
#define PDF_TRAPPED_UNKNOWN   "\125\156\153\156\157\167\156"

/* Set Info dictionary entries */
static void
pdf__set_info(PDF *p, const char *key, const char *value, int len)
{
    static const char fn[] = "pdf__set_info";
    char *key_buf, *val_buf;
    pdf_info *newentry;

    if (key == NULL || !*key)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "key", 0, 0, 0);

    if (!strcmp(key, "Producer") || !strcmp(key, "CreationDate") ||
	!strcmp(key, "ModDate"))
	pdc_error(p->pdc, PDC_E_ILLARG_STRING, "key", key, 0, 0);

    /* convert text string */
    val_buf = pdf_convert_hypertext(p, value, len);
    if (!val_buf)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "value", 0, 0, 0);

    /* key_buf will be converted in pdf_printf() */
    key_buf = pdc_strdup(p->pdc, key);
    if (!strcmp(key_buf, "Keywords")) {
	if (p->Keywords)
	    pdc_free(p->pdc, p->Keywords);
	p->Keywords = val_buf;
    } else if (!strcmp(key_buf, "Subject")) {
	if (p->Subject)
	    pdc_free(p->pdc, p->Subject);
	p->Subject = val_buf;
    } else if (!strcmp(key_buf, "Title")) {
	if (p->Title)
	    pdc_free(p->pdc, p->Title);
	p->Title = val_buf;
    } else if (!strcmp(key_buf, "Creator")) {
	if (p->Creator)
	    pdc_free(p->pdc, p->Creator);
	p->Creator = val_buf;
    } else if (!strcmp(key_buf, "Author")) {
	if (p->Author)
	    pdc_free(p->pdc, p->Author);
	p->Author = val_buf;
    } else { /* user-defined keyword */
	/* special handling required for "Trapped" */
	if (!strcmp(key_buf, "Trapped")) {
            if (strcmp(val_buf, PDF_TRAPPED_TRUE) &&
                strcmp(val_buf, PDF_TRAPPED_FALSE) &&
                strcmp(val_buf, PDF_TRAPPED_UNKNOWN))
                {
		    pdc_free(p->pdc, val_buf);
		    pdc_free(p->pdc, key_buf);
		    pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, value, key, 0, 0);
	    }
	}

	newentry = (pdf_info *)
	    pdc_malloc(p->pdc, sizeof(pdf_info), fn);
	newentry->key	= key_buf;
	newentry->value	= val_buf;
	newentry->next	= p->userinfo;

	/* ordering doesn't matter so we insert at the beginning */
	p->userinfo	= newentry;

	return;
    }

    pdc_free(p->pdc, key_buf);
}

/* Set Info dictionary entries */
PDFLIB_API void PDFLIB_CALL
PDF_set_info(PDF *p, const char *key, const char *value)
{
    static const char fn[] = "PDF_set_info";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page),
        "\t(p[%p], \"%s\", \"%s\")\n",
        (void *) p, key, pdc_strprint(p->pdc, value, 0)))
    {
        int len = value ? (int) pdc_strlen(value) : 0;
        pdf__set_info(p, key, value, len);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_set_info2(PDF *p, const char *key, const char *value, int len)
{
    static const char fn[] = "PDF_set_info2";

    if (pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page),
        "(p[%p], \"%s\", \"%s\", %d)\n",
        (void *) p, key, pdc_strprint(p->pdc, value, len), len))
    {
        pdf__set_info(p, key, value, len);
    }
}

static void
pdf_init_names(PDF *p)
{
    int i;

    p->names_number = 0;
    p->names_capacity = NAMES_CHUNKSIZE;

    p->names = (pdf_name *) pdc_malloc(p->pdc,
        sizeof(pdf_name) * p->names_capacity, "pdf_init_names");

    for (i = 0; i < p->names_capacity; i++) {
        p->names[i].obj_id = PDC_BAD_ID;
        p->names[i].name = NULL;
        p->names[i].len = 0;
    }
}

static void
pdf_grow_names(PDF *p)
{
    int i;

    p->names = (pdf_name *) pdc_realloc(p->pdc, p->names,
        sizeof(pdf_name) * 2 * p->names_capacity, "pdf_grow_names");

    for (i = p->names_capacity; i < 2 * p->names_capacity; i++) {
        p->names[i].obj_id = PDC_BAD_ID;
        p->names[i].name = NULL;
        p->names[i].len = 0;
    }

    p->names_capacity *= 2;
}

void
pdf_cleanup_names(PDF *p)
{
    int i;

    if (p->names == NULL)
	return;

    for (i = 0; i < p->names_number; i++) {
	pdc_free(p->pdc, p->names[i].name);
    }

    pdc_free(p->pdc, p->names);
    p->names = NULL;
}

static int
name_compare( const void*  a, const void*  b)
{
    pdf_name *p1 = (pdf_name *) a;
    pdf_name *p2 = (pdf_name *) b;

    return strcmp(p1->name, p2->name);
}

pdc_id
pdf_write_names(PDF *p)
{
    pdc_id ret;
    int i;

    /* nothing to do */
    if (p->names_number == 0)
	return PDC_BAD_ID;


    ret = pdc_begin_obj(p->out, PDC_NEW_ID);	/* Names object */

    qsort(p->names, (size_t) p->names_number, sizeof(pdf_name), name_compare);

    pdc_begin_dict(p->out);			/* Node dict */

    /* TODO: construct a balanced tree instead of a linear list */
    pdc_puts(p->out, "/Limits[");
    pdc_put_pdfstring(p->out, p->names[0].name, p->names[0].len);
    pdc_put_pdfstring(p->out, p->names[p->names_number-1].name,
	    p->names[p->names_number-1].len);
    pdc_puts(p->out, "]\n");

    pdc_puts(p->out, "/Names[");

    for (i = 0; i < p->names_number; i++) {
	pdc_put_pdfstring(p->out, p->names[i].name, p->names[i].len);
	pdc_printf(p->out, "%ld 0 R\n", p->names[i].obj_id);
    }

    pdc_puts(p->out, "]");

    pdc_end_dict(p->out);			/* Node dict */

    pdc_end_obj(p->out);			/* Names object */

    return ret;
}

PDFLIB_API void PDFLIB_CALL
PDF_add_nameddest(
    PDF *p,
    const char *name,
    int reserved,
    const char *optlist)
{
    static const char fn[] = "PDF_add_nameddest";
    pdf_dest dest;
    int i;

    if (!pdf_enter_api(p, fn,
	(pdf_state) (pdf_state_page | pdf_state_document),
        "(p[%p], %s, %d, \"%s\")\n",
        (void *) p, name, reserved, optlist))
    {
        return;
    }

    if (!name || !*name)
        pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "name", 0, 0, 0);

    if (p->names == NULL)
        pdf_init_names(p);

    for (i = 0; i < p->names_number; i++) {
	if (!strcmp(p->names[i].name, name))
	    /* silently ignore duplicate destination names */
	    return;
    }

    if (p->names_number == p->names_capacity)
	pdf_grow_names(p);

    pdf_parse_destination_optlist(p, optlist, &dest, 0, pdf_nameddest);

    /* interrupt the content stream if we are on a page */
    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_end_contents_section(p);
    						/* Dest object */
    p->names[p->names_number].obj_id = pdc_begin_obj(p->out, PDC_NEW_ID);
    p->names[p->names_number].name = (char *) pdc_strdup(p->pdc, name);
    p->names[p->names_number].len = (int) strlen(name);
    p->names_number++;

    pdc_begin_dict(p->out);			/* Destination dict */

    pdc_puts(p->out, "/D");
    pdf_write_destination(p, &dest);
    pdc_puts(p->out, "\n");

    pdc_end_dict(p->out);			/* Destination dict */
    pdc_end_obj(p->out);			/* Dest object */

    /* continue the contents stream */
    if (PDF_GET_STATE(p) == pdf_state_page)
	pdf_begin_contents_section(p);
}



pdc_id
pdf_write_info(PDF *p)
{
    char producer[256];
    pdf_info	*info;
    pdc_id	info_id;
    char *	product = "PDFlib Lite";

    info_id = pdc_begin_obj(p->out, PDC_NEW_ID);	/* Info object */

    pdc_begin_dict(p->out);

    /*
     * Although it would be syntactically correct, we must not remove
     * the space characters after the dictionary keys since this
     * would break the PDF properties feature in Windows Explorer.
     */

    if (p->Keywords) {
	pdc_puts(p->out, "/Keywords ");
	pdc_put_pdfunistring(p->out, p->Keywords);
        pdc_puts(p->out, "\n");
    }
    if (p->Subject) {
	pdc_puts(p->out, "/Subject ");
	pdc_put_pdfunistring(p->out, p->Subject);
        pdc_puts(p->out, "\n");
    }
    if (p->Title) {
	pdc_puts(p->out, "/Title ");
	pdc_put_pdfunistring(p->out, p->Title);
        pdc_puts(p->out, "\n");
    }
    if (p->Creator) {
	pdc_puts(p->out, "/Creator ");
	pdc_put_pdfunistring(p->out, p->Creator);
        pdc_puts(p->out, "\n");
    }
    if (p->Author) {
	pdc_puts(p->out, "/Author ");
	pdc_put_pdfunistring(p->out, p->Author);
        pdc_puts(p->out, "\n");
    }
    if (p->userinfo) {
	for (info = p->userinfo; info != NULL; info = info->next) {
            pdc_put_pdfname(p->out, info->key, strlen(info->key));
            pdc_puts(p->out, " ");

	    if (strcmp(info->key, "Trapped")) {
		pdc_put_pdfunistring(p->out, info->value);
	    } else {
		/* "Trapped" value is already encoded in PDFDocEncoding */
                pdc_put_pdfname(p->out, info->value, strlen(info->value));
	    }

            pdc_puts(p->out, "\n");
	}
    }

    pdc_puts(p->out, "/CreationDate ");
    pdc_put_pdfstring(p->out, p->time_str, (int) strlen(p->time_str));
    pdc_puts(p->out, "\n");


    /*
     * If you change the /Producer entry your license to use
     * PDFlib will be void!
     */

    if (p->binding)
	sprintf(producer, "%s %s (%s/%s)", product,
	    PDFLIB_VERSIONSTRING, p->binding, PDF_PLATFORM);
    else
	sprintf(producer, "%s %s (%s)", product,
	    PDFLIB_VERSIONSTRING, PDF_PLATFORM);

    pdc_puts(p->out, "/Producer ");
    pdc_put_pdfstring(p->out, producer, (int) strlen(producer));
    pdc_puts(p->out, "\n");

    pdc_end_dict(p->out);
    pdc_end_obj(p->out);			/* Info object */

    return info_id;
}

void
pdf_cleanup_info(PDF *p)
{
    pdf_info	*info, *last;

    /* Free Info dictionary entries */
    if (p->Keywords) {
	pdc_free(p->pdc, p->Keywords);
	p->Keywords = NULL;
    }
    if (p->Subject) {
	pdc_free(p->pdc, p->Subject);
	p->Subject = NULL;
    }
    if (p->Title) {
	pdc_free(p->pdc, p->Title);
	p->Title = NULL;
    }
    if (p->Creator) {
	pdc_free(p->pdc, p->Creator);
	p->Creator = NULL;
    }
    if (p->Author) {
	pdc_free(p->pdc, p->Author);
	p->Author = NULL;
    }
    if (p->userinfo) {
	for (info = p->userinfo; info != NULL; /* */) {
	    last = info;
	    info = info->next;

	    pdc_free(p->pdc, last->key);
	    pdc_free(p->pdc, last->value);
	    pdc_free(p->pdc, last);
	}
	p->userinfo = NULL;
    }
}

/* Page transition effects */

static const pdc_keyconn pdf_transition_keylist[] =
{
    {"none",      trans_none},
    {"split",     trans_split},
    {"blinds",    trans_blinds},
    {"box",       trans_box},
    {"wipe",      trans_wipe},
    {"dissolve",  trans_dissolve},
    {"glitter",   trans_glitter},
    {"replace",   trans_replace},
    {NULL, 0}
};

static const pdc_keyconn pdf_transition_pdfkeylist[] =
{
    {"",          trans_none},
    {"Split",     trans_split},
    {"Blinds",    trans_blinds},
    {"Box",       trans_box},
    {"Wipe",      trans_wipe},
    {"Dissolve",  trans_dissolve},
    {"Glitter",   trans_glitter},
    {"R",         trans_replace},
    {NULL, 0}
};

void
pdf_init_transition(PDF *p)
{
    p->transition = trans_none;
    p->duration = 0;
}

/* set page display duration for current and future pages */
void
pdf_set_duration(PDF *p, float t)
{
    p->duration = t;
}

void
pdf_write_page_transition(PDF *p)
{
    if (p->transition != trans_none) {
	pdc_puts(p->out, "/Trans");
	pdc_begin_dict(p->out);
	pdc_printf(p->out, "/S/%s",
            pdc_get_keyword(p->transition, pdf_transition_pdfkeylist));

	pdc_end_dict(p->out);
    }
}

/* set transition mode for current and future pages */

void
pdf_set_transition(PDF *p, const char *type)
{
    if (type == NULL || !*type)
	type = "none";
    p->transition =
        (pdf_transition) pdc_get_keycode(type, pdf_transition_keylist);
    if (p->transition == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_PAR_ILLPARAM, type, "transition", 0, 0);
}

char *
pdf_convert_hypertext(PDF *p, const char *text, int len)
{
    pdc_encoding enc;
    pdc_encodingvector *ev = NULL;
    pdc_encodingvector *ev_pdfdoc = NULL;
    pdc_byte *outtext = NULL;
    pdc_text_format textformat = pdc_utf16be;
    int outlen;

    if (text && len == 0)
        len = (int) strlen(text);
    if (text == NULL || len <= 0)
        return NULL;

    /* encoding struct available */
    if (p->encodings)
    {
        /* User encoding */
        enc = p->hypertextencoding;
        if (enc == pdc_invalidenc)
            enc = pdf_find_encoding(p, "auto");
        if (enc == pdc_invalidenc)
            enc = pdf_insert_encoding(p, "auto");
        if (enc >= 0)
            ev = p->encodings[enc].ev;

        /* PDFDocEncoding */
        ev_pdfdoc = p->encodings[pdc_pdfdoc].ev;
        if (ev_pdfdoc == NULL)
        {
            pdf_find_encoding(p, "pdfdoc");
            ev_pdfdoc = p->encodings[pdc_pdfdoc].ev;
        }
    }

    /* Convert hypertext string */
    pdc_convert_string(p->pdc, p->hypertextformat, ev, (pdc_byte *)text, len,
                       &textformat, ev_pdfdoc, &outtext, &outlen,
                       PDC_CONV_WITHBOM | PDC_CONV_TRYBYTES, pdc_true);

    return (char *) outtext;
}
