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

/* $Id: p_annots.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib routines for annnotations
 *
 */

#include "p_intern.h"

/* Annotation types */
typedef enum {
    ann_text, ann_locallink,
    ann_pdflink, ann_weblink,
    ann_launchlink, ann_attach
} pdf_annot_type;

/* icons for file attachments and text annotations */
typedef enum {
    icon_file_graph, icon_file_paperclip,
    icon_file_pushpin, icon_file_tag,

    icon_text_comment, icon_text_insert,
    icon_text_note, icon_text_paragraph,
    icon_text_newparagraph, icon_text_key,
    icon_text_help
} pdf_icon;

/* Annotations */
struct pdf_annot_s {
    pdf_annot_type	type;		/* used for all annotation types */
    pdc_rectangle	rect;		/* used for all annotation types */
    pdc_id		obj_id;		/* used for all annotation types */
    pdf_annot		*next;		/* used for all annotation types */

    pdf_icon		icon;		/* attach and text */
    char		*filename;	/* attach, launchlink, pdflink,weblink*/
    char		*contents;	/* text, attach, pdflink */
    char		*mimetype;	/* attach */
    char		*parameters;	/* launchlink */
    char		*operation;	/* launchlink */
    char		*defaultdir;	/* launchlink */

    char		*title;		/* text */
    int			open;		/* text */
    pdf_dest		dest;		/* locallink, pdflink */

    /* -------------- annotation border style and color -------------- */
    pdf_border_style	border_style;
    float		border_width;
    float		border_red;
    float		border_green;
    float		border_blue;
    float		border_dash1;
    float		border_dash2;
};

static const char *pdf_border_style_names[] = {
    "S",	/* solid border */
    "D",	/* dashed border */
    "B",	/* beveled (three-dimensional) border */
    "I",	/* inset border */
    "U"		/* underlined border */
};

static const char *pdf_icon_names[] = {
    /* embedded file icon names */
    "Graph", "Paperclip", "Pushpin", "Tag",

    /* text annotation icon names */
    "Comment", "Insert", "Note", "Paragraph", "NewParagraph", "Key", "Help"
};

/* flags for annotation properties */
typedef enum {
    pdf_ann_flag_invisible	= 1,
    pdf_ann_flag_hidden		= 2,
    pdf_ann_flag_print		= 4,
    pdf_ann_flag_nozoom		= 8,
    pdf_ann_flag_norotate	= 16,
    pdf_ann_flag_noview		= 32,
    pdf_ann_flag_readonly	= 64
} pdf_ann_flag;

void
pdf_init_annots(PDF *p)
{
    /* annotation border style defaults */
    p->border_style	= border_solid;
    p->border_width	= (float) 1.0;
    p->border_red	= (float) 0.0;
    p->border_green	= (float) 0.0;
    p->border_blue	= (float) 0.0;
    p->border_dash1	= (float) 3.0;
    p->border_dash2	= (float) 3.0;

    /* auxiliary function parameters */
    p->launchlink_parameters	= NULL;
    p->launchlink_operation	= NULL;
    p->launchlink_defaultdir	= NULL;
}

void
pdf_cleanup_annots(PDF *p)
{
    if (p->launchlink_parameters) {
	pdc_free(p->pdc, p->launchlink_parameters);
	p->launchlink_parameters = NULL;
    }

    if (p->launchlink_operation) {
	pdc_free(p->pdc, p->launchlink_operation);
	p->launchlink_operation = NULL;
    }

    if (p->launchlink_defaultdir) {
	pdc_free(p->pdc, p->launchlink_defaultdir);
	p->launchlink_defaultdir = NULL;
    }
}

static void
pdf_init_annot(PDF *p, pdf_annot *ann)
{
    (void) p;

    ann->next           = NULL;
    ann->filename       = NULL;
    ann->mimetype       = NULL;
    ann->contents       = NULL;
    ann->mimetype       = NULL;
    ann->parameters     = NULL;
    ann->operation      = NULL;
    ann->defaultdir     = NULL;
    ann->title          = NULL;
}


/* Write annotation border style and color */
static void
pdf_write_border_style(PDF *p, pdf_annot *ann)
{
    /* don't write the default values */
    if (ann->border_style == border_solid &&
        ann->border_width == (float) 1.0 &&
        ann->border_red == (float) 0.0 &&
        ann->border_green == (float) 0.0 &&
        ann->border_blue == (float) 0.0 &&
        ann->border_dash1 == (float) 3.0 &&
        ann->border_dash2 == (float) 3.0)
        return;

    if (ann->type != ann_attach) {
	pdc_puts(p->out, "/BS");
	pdc_begin_dict(p->out);			/* BS dict */
	pdc_puts(p->out, "/Type/Border\n");

	/* Acrobat 6 requires this entry, and does not use /S/S as default */
	pdc_printf(p->out, "/S/%s\n",
	    pdf_border_style_names[ann->border_style]);

	/* Acrobat 6 requires this entry */
	pdc_printf(p->out, "/W %f\n", ann->border_width);

	if (ann->border_style == border_dashed)
	    pdc_printf(p->out, "/D[%f %f]\n",
		ann->border_dash1, ann->border_dash2);

	pdc_end_dict(p->out);			/* BS dict */

	/* Write the Border key in old-style PDF 1.1 format */
	pdc_printf(p->out, "/Border[0 0 %f", ann->border_width);

	if (ann->border_style == border_dashed &&
	    (ann->border_dash1 != (float) 0.0 || ann->border_dash2 !=
	    (float) 0.0))
	    /* set dashed border */
	    pdc_printf(p->out, "[%f %f]", ann->border_dash1, ann->border_dash2);

	pdc_puts(p->out, "]\n");

    }

    /* write annotation color */
    pdc_printf(p->out, "/C[%f %f %f]\n",
		    ann->border_red, ann->border_green, ann->border_blue);
}

void
pdf_write_annots_root(PDF *p)
{
    pdf_annot *ann;

    /* Annotations array */
    if (p->annots) {
	pdc_puts(p->out, "/Annots[");

	for (ann = p->annots; ann != NULL; ann = ann->next) {
	    ann->obj_id = pdc_alloc_id(p->out);
	    pdc_printf(p->out, "%ld 0 R ", ann->obj_id);
	}

	pdc_puts(p->out, "]\n");
    }
}

void
pdf_write_page_annots(PDF *p)
{
    pdf_annot	*ann;

    for (ann = p->annots; ann != NULL; ann = ann->next) {

	pdc_begin_obj(p->out, ann->obj_id);	/* Annotation object */
	pdc_begin_dict(p->out);		/* Annotation dict */


	pdc_puts(p->out, "/Type/Annot\n");
	switch (ann->type) {
	    case ann_text:
		pdc_puts(p->out, "/Subtype/Text\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		if (ann->open)
		    pdc_puts(p->out, "/Open true\n");

		if (ann->icon != icon_text_note)	/* note is default */
		    pdc_printf(p->out, "/Name/%s\n", pdf_icon_names[ann->icon]);

		/* Contents key is required, but may be empty */
		pdc_puts(p->out, "/Contents");

		if (ann->contents) {
		    pdc_put_pdfunistring(p->out, ann->contents);
                    pdc_puts(p->out, "\n");
		} else
		    pdc_puts(p->out, "()\n"); /* empty contents is OK */

		/* title is optional */
		if (ann->title) {
		    pdc_puts(p->out, "/T");
		    pdc_put_pdfunistring(p->out, ann->title);
                    pdc_puts(p->out, "\n");
		}

		break;

	    case ann_locallink:
		pdc_puts(p->out, "/Subtype/Link\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		/* preallocate page object id for a later page */
		if (ann->dest.page > p->current_page) {
		    while (ann->dest.page >= p->pages_capacity)
			pdf_grow_pages(p);

		    /* if this page has already been used as a link target
		     * it will already have an object id.
		     */
		    if (p->pages[ann->dest.page] == PDC_BAD_ID)
			p->pages[ann->dest.page] = pdc_alloc_id(p->out);
		}

		pdc_puts(p->out, "/Dest");
		pdf_write_destination(p, &ann->dest);
		pdc_puts(p->out, "\n");

		break;

	    case ann_pdflink:
		pdc_puts(p->out, "/Subtype/Link\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		pdc_puts(p->out, "/A");
		pdc_begin_dict(p->out);			/* A dict */
		pdc_puts(p->out, "/Type/Action/S/GoToR\n");

		pdc_puts(p->out, "/D");
		pdf_write_destination(p, &ann->dest);
		pdc_puts(p->out, "\n");

		pdc_puts(p->out, "/F");
		pdc_begin_dict(p->out);			/* F dict */
		pdc_puts(p->out, "/Type/Filespec\n");
		pdc_puts(p->out, "/F");
		pdc_put_pdfstring(p->out, ann->filename,
		    (int)strlen(ann->filename));
		pdc_puts(p->out, "\n");
		pdc_end_dict(p->out);			/* F dict */

		pdc_end_dict(p->out);			/* A dict */

		break;

	    case ann_launchlink:
		pdc_puts(p->out, "/Subtype/Link\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		pdc_puts(p->out, "/A");
		pdc_begin_dict(p->out);			/* A dict */
		pdc_puts(p->out, "/Type/Action/S/Launch\n");

		if (ann->parameters || ann->operation || ann->defaultdir) {
		    pdc_puts(p->out, "/Win");
		    pdc_begin_dict(p->out);			/* Win dict */
		    pdc_printf(p->out, "/F");
		    pdc_put_pdfstring(p->out, ann->filename,
			(int)strlen(ann->filename));
		    pdc_puts(p->out, "\n");
		    if (ann->parameters) {
			pdc_printf(p->out, "/P");
			pdc_put_pdfstring(p->out, ann->parameters,
			    (int)strlen(ann->parameters));
			pdc_puts(p->out, "\n");
			pdc_free(p->pdc, ann->parameters);
			ann->parameters = NULL;
		    }
		    if (ann->operation) {
			pdc_printf(p->out, "/O");
			pdc_put_pdfstring(p->out, ann->operation,
			    (int)strlen(ann->operation));
			pdc_puts(p->out, "\n");
			pdc_free(p->pdc, ann->operation);
			ann->operation = NULL;
		    }
		    if (ann->defaultdir) {
			pdc_printf(p->out, "/D");
			pdc_put_pdfstring(p->out, ann->defaultdir,
			    (int)strlen(ann->defaultdir));
			pdc_puts(p->out, "\n");
			pdc_free(p->pdc, ann->defaultdir);
			ann->defaultdir = NULL;
		    }
		    pdc_end_dict(p->out);			/* Win dict */
		} else {
		    pdc_puts(p->out, "/F");
		    pdc_begin_dict(p->out);			/* F dict */
		    pdc_puts(p->out, "/Type/Filespec\n");
		    pdc_printf(p->out, "/F");
		    pdc_put_pdfstring(p->out, ann->filename,
			(int)strlen(ann->filename));
		    pdc_puts(p->out, "\n");
		    pdc_end_dict(p->out);			/* F dict */
		}

		pdc_end_dict(p->out);			/* A dict */

		break;

	    case ann_weblink:
		pdc_puts(p->out, "/Subtype/Link\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		pdc_puts(p->out, "/A<</S/URI/URI");
		pdc_put_pdfstring(p->out, ann->filename,
		    (int)strlen(ann->filename));
		pdc_puts(p->out, ">>\n");
		break;

	    case ann_attach:
		pdc_puts(p->out, "/Subtype/FileAttachment\n");
		pdc_printf(p->out, "/Rect[%f %f %f %f]\n",
		    ann->rect.llx, ann->rect.lly, ann->rect.urx, ann->rect.ury);

		pdf_write_border_style(p, ann);

		if (ann->icon != icon_file_pushpin)	/* pushpin is default */
		    pdc_printf(p->out, "/Name/%s\n",
		    	pdf_icon_names[ann->icon]);

		if (ann->title) {
		    pdc_puts(p->out, "/T");
		    pdc_put_pdfunistring(p->out, ann->title);
                    pdc_puts(p->out, "\n");
		}

		if (ann->contents) {
		    pdc_puts(p->out, "/Contents");
		    pdc_put_pdfunistring(p->out, ann->contents);
                    pdc_puts(p->out, "\n");
		}

		/* the icon is too small without these flags (=28) */
		pdc_printf(p->out, "/F %d\n",
			pdf_ann_flag_print |
			pdf_ann_flag_nozoom |
			pdf_ann_flag_norotate);

		pdc_puts(p->out, "/FS");
		pdc_begin_dict(p->out);			/* FS dict */
		pdc_puts(p->out, "/Type/Filespec\n");

		pdc_puts(p->out, "/F");
		pdc_put_pdfstring(p->out, ann->filename,
		    (int)strlen(ann->filename));
		pdc_puts(p->out, "\n");

		/* alloc id for the actual embedded file stream */
		ann->obj_id = pdc_alloc_id(p->out);
		pdc_printf(p->out, "/EF<</F %ld 0 R>>\n", ann->obj_id);
		pdc_end_dict(p->out);			/* FS dict */

		break;

	    default:
		pdc_error(p->pdc, PDF_E_INT_BADANNOT,
		    pdc_errprintf(p->pdc, "%d", ann->type), 0, 0, 0);
	}

	pdc_end_dict(p->out);		/* Annotation dict */
	pdc_end_obj(p->out);		/* Annotation object */
    }

    /* Write the actual embedded files with preallocated ids */
    for (ann = p->annots; ann != NULL; ann = ann->next) {
	pdc_id		length_id;
	PDF_data_source src;

	if (ann->type != ann_attach)
	    continue;

	pdc_begin_obj(p->out, ann->obj_id);	/* EmbeddedFile */
	pdc_puts(p->out, "<</Type/EmbeddedFile\n");

	if (ann->mimetype) {
	    pdc_puts(p->out, "/Subtype");
            pdc_put_pdfname(p->out, ann->mimetype, strlen(ann->mimetype));
	    pdc_puts(p->out, "\n");
	}

	if (pdc_get_compresslevel(p->out))
		pdc_puts(p->out, "/Filter/FlateDecode\n");

	length_id = pdc_alloc_id(p->out);
	pdc_printf(p->out, "/Length %ld 0 R\n", length_id);
	pdc_end_dict(p->out);		/* F dict */

	/* write the file in the PDF */
	src.private_data = (void *) ann->filename;
	src.init	= pdf_data_source_file_init;
	src.fill	= pdf_data_source_file_fill;
	src.terminate	= pdf_data_source_file_terminate;
	src.length	= (long) 0;
	src.offset	= (long) 0;

	pdf_copy_stream(p, &src, pdc_true);	/* embedded file stream */

	pdc_end_obj(p->out);			/* EmbeddedFile object */

	pdc_put_pdfstreamlength(p->out, length_id);

	if (p->flush & pdf_flush_content)
	    pdc_flush_stream(p->out);
    }
}

void
pdf_init_page_annots(PDF *p)
{
    p->annots = NULL;
}

void
pdf_cleanup_page_annots(PDF *p)
{
    pdf_annot *ann, *old;

    for (ann = p->annots; ann != (pdf_annot *) NULL; /* */ ) {
	switch (ann->type) {
	    case ann_text:
		if (ann->contents)
		    pdc_free(p->pdc, ann->contents);
		if (ann->title)
		    pdc_free(p->pdc, ann->title);
		break;

	    case ann_locallink:
		pdf_cleanup_destination(p, &ann->dest);
		break;

	    case ann_launchlink:
		pdc_free(p->pdc, ann->filename);
		break;

	    case ann_pdflink:
		pdf_cleanup_destination(p, &ann->dest);
		pdc_free(p->pdc, ann->filename);
		break;

	    case ann_weblink:
		pdc_free(p->pdc, ann->filename);
		break;

	    case ann_attach:
		pdf_unlock_pvf(p, ann->filename);
		pdc_free(p->pdc, ann->filename);
		if (ann->contents)
		    pdc_free(p->pdc, ann->contents);
		if (ann->title)
		    pdc_free(p->pdc, ann->title);
		if (ann->mimetype)
		    pdc_free(p->pdc, ann->mimetype);
		break;

	    default:
		pdc_error(p->pdc, PDF_E_INT_BADANNOT,
		    pdc_errprintf(p->pdc, "%d", ann->type), 0, 0, 0);
	}
	old = ann;
	ann = old->next;
	pdc_free(p->pdc, old);
    }
    p->annots = NULL;
}

/* Insert new annotation at the end of the annots chain */
static void
pdf_add_annot(PDF *p, pdf_annot *ann)
{
    pdf_annot *last;

    /* fetch current border state from p */
    ann->border_style	= p->border_style;
    ann->border_width	= p->border_width;
    ann->border_red	= p->border_red;
    ann->border_green	= p->border_green;
    ann->border_blue	= p->border_blue;
    ann->border_dash1	= p->border_dash1;
    ann->border_dash2	= p->border_dash2;

    ann->next = NULL;

    if (p->annots == NULL)
	p->annots = ann;
    else {
	for (last = p->annots; last->next != NULL; /* */ )
	    last = last->next;
	last->next = ann;
    }
}

static void
pdf_init_rectangle(PDF *p, pdf_annot *ann,
                   float llx, float lly, float urx, float ury)
{
    pdc_rect_init(&ann->rect, llx, lly, urx, ury);
    if (p->usercoordinates == pdc_true)
        pdc_rect_transform(&p->gstate[p->sl].ctm, &ann->rect, &ann->rect);
}




/* Attach an arbitrary file to the PDF. Note that the actual
 * embedding takes place in PDF_end_page().
 * description, author, and mimetype may be NULL.
 */

static const pdc_keyconn pdf_icon_attach_keylist[] =
{
    {"graph",     icon_file_graph},
    {"paperclip", icon_file_paperclip},
    {"pushpin",   icon_file_pushpin},
    {"tag",       icon_file_tag},
    {NULL, 0}
};

static void
pdf__attach_file(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *filename,
    const char *description,
    int len_descr,
    const char *author,
    int len_auth,
    const char *mimetype,
    const char *icon)
{
    static const char fn[] = "pdf__attach_file";
    pdf_annot *ann;
    pdc_file *attfile;

    if (filename == NULL || *filename == '\0')
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    if ((attfile = pdf_fopen(p, filename, "attachment ", 0)) == NULL)
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    pdf_lock_pvf(p, filename);
    pdc_fclose(attfile);

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->type	  = ann_attach;
    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    if (icon == NULL || !*icon)
        icon = "pushpin";
    ann->icon = (pdf_icon) pdc_get_keycode(icon, pdf_icon_attach_keylist);
    if (ann->icon == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_ILLARG_STRING, "icon", icon, 0, 0);

    ann->filename = (char *) pdc_strdup(p->pdc, filename);

    ann->contents = pdf_convert_hypertext(p, description, len_descr);

    ann->title = pdf_convert_hypertext(p, author, len_auth);

    if (mimetype != NULL) {
	ann->mimetype = (char *) pdc_strdup(p->pdc, mimetype);
    }

    pdf_add_annot(p, ann);
}

PDFLIB_API void PDFLIB_CALL
PDF_attach_file(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *filename,
    const char *description,
    const char *author,
    const char *mimetype,
    const char *icon)
{
    static const char fn[] = "PDF_attach_file";

    if (pdf_enter_api(p, fn, pdf_state_page,
        "(p[%p], %g, %g, %g, %g, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\")\n",
        (void *) p, llx, lly, urx, ury, filename,
        pdc_strprint(p->pdc, description, 0),
        pdc_strprint(p->pdc, author, 0), mimetype, icon))
    {
        int len_descr = description ? (int) pdc_strlen(description) : 0;
        int len_auth = author ? (int) pdc_strlen(author) : 0;
        pdf__attach_file(p, llx, lly, urx, ury, filename,
            description, len_descr, author, len_auth, mimetype, icon) ;
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_attach_file2(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *filename,
    int reserved,
    const char *description,
    int len_descr,
    const char *author,
    int len_auth,
    const char *mimetype,
    const char *icon)
{
    static const char fn[] = "PDF_attach_file2";

    if (pdf_enter_api(p, fn, pdf_state_page,
        "(p[%p], %g, %g, %g, %g, \"%s\", %d, \"%s\", %d, "
        "\"%s\", %d, \"%s\", \"%s\")\n",
        (void *) p, llx, lly, urx, ury, filename, reserved,
        pdc_strprint(p->pdc, description, len_descr), len_descr,
        pdc_strprint(p->pdc, author, len_auth), len_auth, mimetype, icon))
    {
        pdf__attach_file(p, llx, lly, urx, ury, filename,
            description, len_descr, author, len_auth, mimetype, icon) ;
    }
}

static const pdc_keyconn pdf_icon_note_keylist[] =
{
    {"comment",      icon_text_comment},
    {"insert",       icon_text_insert},
    {"note",         icon_text_note},
    {"paragraph",    icon_text_paragraph},
    {"newparagraph", icon_text_newparagraph},
    {"key",          icon_text_key},
    {"help",         icon_text_help},
    {NULL, 0}
};

static void
pdf__add_note(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *contents,
    int len_cont,
    const char *title,
    int len_title,
    const char *icon,
    int open)
{
    static const char fn[] = "pdf__add_note";
    pdf_annot *ann;

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->type	  = ann_text;
    ann->open	  = open;
    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    if (icon == NULL || !*icon)
        icon = "note";
    ann->icon = (pdf_icon) pdc_get_keycode(icon, pdf_icon_note_keylist);
    if (ann->icon == PDC_KEY_NOTFOUND)
        pdc_error(p->pdc, PDC_E_ILLARG_STRING, "icon", icon, 0, 0);

    /* title may be NULL */
    ann->title = pdf_convert_hypertext(p, title, len_title);

    /* It is legal to create an empty text annnotation */
    ann->contents = pdf_convert_hypertext(p, contents, len_cont);

    pdf_add_annot(p, ann);
}

PDFLIB_API void PDFLIB_CALL
PDF_add_note(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *contents,
    const char *title,
    const char *icon,
    int open)
{
    static const char fn[] = "PDF_add_note";

    if (pdf_enter_api(p, fn, pdf_state_page,
        "(p[%p], %g, %g, %g, %g, \"%s\", \"%s\", \"%s\", %d)\n",
        (void *) p, llx, lly, urx, ury,
        pdc_strprint(p->pdc, contents, 0),
        pdc_strprint(p->pdc, title, 0), icon, open))
    {
        int len_cont = contents ? (int) pdc_strlen(contents) : 0;
        int len_title = title ? (int) pdc_strlen(title) : 0;
        pdf__add_note(p, llx, lly, urx, ury, contents, len_cont,
                      title, len_title, icon, open);
    }
}

PDFLIB_API void PDFLIB_CALL
PDF_add_note2(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *contents,
    int len_cont,
    const char *title,
    int len_title,
    const char *icon,
    int open)
{
    static const char fn[] = "PDF_add_note2";

    if (pdf_enter_api(p, fn, pdf_state_page,
        "(p[%p], %g, %g, %g, %g, \"%s\", %d, \"%s\", %d, \"%s\", %d)\n",
        (void *) p, llx, lly, urx, ury,
        pdc_strprint(p->pdc, contents, len_cont), len_cont,
        pdc_strprint(p->pdc, title, len_title), len_title,
        icon, open))
    {
        pdf__add_note(p, llx, lly, urx, ury, contents, len_cont,
                      title, len_title, icon, open);
    }
}

/* Add a link to another PDF file */
PDFLIB_API void PDFLIB_CALL
PDF_add_pdflink(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *filename,
    int page,
    const char *optlist)
{
    static const char fn[] = "PDF_add_pdflink";
    pdf_annot *ann;

    if (!pdf_enter_api(p, fn, pdf_state_page,
	"(p[%p], %g, %g, %g, %g, \"%s\", %d, \"%s\")\n",
    	(void *) p, llx, lly, urx, ury, filename, page, optlist))
    {
	return;
    }

    if (filename == NULL)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->filename = pdc_strdup(p->pdc, filename);

    ann->type = ann_pdflink;

    pdf_parse_destination_optlist(p, optlist, &ann->dest, page, pdf_remotelink);

    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    pdf_add_annot(p, ann);
}

/* Add a link to another file of an arbitrary type */
PDFLIB_API void PDFLIB_CALL
PDF_add_launchlink(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *filename)
{
    static const char fn[] = "PDF_add_launchlink";
    pdf_annot *ann;

    if (!pdf_enter_api(p, fn, pdf_state_page,
	"(p[%p], %g, %g, %g, %g, \"%s\")\n",
	(void *)p, llx, lly, urx, ury, filename))
    {
	return;
    }

    if (filename == NULL)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "filename", 0, 0, 0);

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->filename = pdc_strdup(p->pdc, filename);

    ann->type	  = ann_launchlink;

    if (p->launchlink_parameters) {
	ann->parameters = p->launchlink_parameters;
	p->launchlink_parameters = NULL;
    }

    if (p->launchlink_operation) {
	ann->operation = p->launchlink_operation;
	p->launchlink_operation = NULL;
    }

    if (p->launchlink_defaultdir) {
	ann->defaultdir = p->launchlink_defaultdir;
	p->launchlink_defaultdir = NULL;
    }

    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    pdf_add_annot(p, ann);
}

/* Add a link to a destination in the current PDF file */
PDFLIB_API void PDFLIB_CALL
PDF_add_locallink(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    int page,
    const char *optlist)
{
    static const char fn[] = "PDF_add_locallink";
    pdf_annot *ann;

    if (!pdf_enter_api(p, fn, pdf_state_page,
	"(p[%p], %g, %g, %g, %g, %d, \"%s\")\n",
    	(void *) p, llx, lly, urx, ury, page, optlist))
    {
	return;
    }

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->type = ann_locallink;

    pdf_parse_destination_optlist(p, optlist, &ann->dest, page, pdf_locallink);

    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    pdf_add_annot(p, ann);
}

/* Add a link to an arbitrary Internet resource (URL) */
PDFLIB_API void PDFLIB_CALL
PDF_add_weblink(
    PDF *p,
    float llx,
    float lly,
    float urx,
    float ury,
    const char *url)
{
    static const char fn[] = "PDF_add_weblink";
    pdf_annot *ann;

    if (!pdf_enter_api(p, fn, pdf_state_page,
	"(p[%p], %g, %g, %g, %g, \"%s\")\n",
	(void *) p, llx, lly, urx, ury, url))
    {
	return;
    }

    if (url == NULL || *url == '\0')
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "url", 0, 0, 0);

    ann = (pdf_annot *) pdc_malloc(p->pdc, sizeof(pdf_annot), fn);

    pdf_init_annot(p, ann);

    ann->filename = pdc_strdup(p->pdc, url);

    ann->type	  = ann_weblink;
    pdf_init_rectangle(p, ann, llx, lly, urx, ury);

    pdf_add_annot(p, ann);
}

PDFLIB_API void PDFLIB_CALL
PDF_set_border_style(PDF *p, const char *style, float width)
{
    static const char fn[] = "PDF_set_border_style";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page),
	"(p[%p], \"%s\", %g)\n", (void *) p, style, width))
    {
	return;
    }

    if (style == NULL)
	p->border_style = border_solid;
    else if (!strcmp(style, "solid"))
	p->border_style = border_solid;
    else if (!strcmp(style, "dashed"))
	p->border_style = border_dashed;
    else
	pdc_error(p->pdc, PDC_E_ILLARG_STRING, "style", style, 0, 0);

    if (width < 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "width", pdc_errprintf(p->pdc, "%f", width), 0, 0);

    p->border_width = width;
}

PDFLIB_API void PDFLIB_CALL
PDF_set_border_color(PDF *p, float red, float green, float blue)
{
    static const char fn[] = "PDF_set_border_color";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page),
	"(p[%p], %g, %g, %g)\n", (void *) p, red, green, blue))
    {
	return;
    }

    if (red < 0.0 || red > 1.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "red", pdc_errprintf(p->pdc, "%f", red), 0, 0);
    if (green < 0.0 || green > 1.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "green", pdc_errprintf(p->pdc, "%f", green), 0, 0);
    if (blue < 0.0 || blue > 1.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "blue", pdc_errprintf(p->pdc, "%f", blue), 0, 0);

    p->border_red = red;
    p->border_green = green;
    p->border_blue = blue;
}

PDFLIB_API void PDFLIB_CALL
PDF_set_border_dash(PDF *p, float b, float w)
{
    static const char fn[] = "PDF_set_border_dash";

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_document | pdf_state_page),
	"(p[%p], %g, %g)\n", (void *) p, b, w))
    {
	return;
    }

    if (b < 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "b", pdc_errprintf(p->pdc, "%f", b), 0, 0);

    if (w < 0.0)
	pdc_error(p->pdc, PDC_E_ILLARG_FLOAT,
	    "w", pdc_errprintf(p->pdc, "%f", w), 0, 0);

    p->border_dash1 = b;
    p->border_dash2 = w;
}
