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

/* $Id: p_basic.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib general routines
 *
 */


#include "p_intern.h"
#include "p_font.h"
#include "p_image.h"


#if !defined(WIN32) && !defined(OS2)
#include <unistd.h>
#endif

#if (defined(WIN32) || defined(OS2)) && !defined(WINCE)
#include <fcntl.h>
#include <io.h>
#endif

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winbase.h>
#undef WIN32_LEAN_AND_MEAN
#endif

static pdc_error_info   pdf_errors[] =
{
#define         pdf_genInfo     1
#include        "p_generr.h"
};

#define N_PDF_ERRORS    (sizeof pdf_errors / sizeof (pdc_error_info))


static const PDFlib_api PDFlib = {
    /* version numbers for checking the DLL against client code */
    sizeof(PDFlib_api),		/* size of this structure */

    PDFLIB_MAJORVERSION,	/* PDFlib major version number */
    PDFLIB_MINORVERSION,	/* PDFlib minor version number */
    PDFLIB_REVISION,		/* PDFlib revision number */

    0,				/* reserved */

    /* p_annots.c: */
    PDF_add_launchlink,
    PDF_add_locallink,
    PDF_add_note,
    PDF_add_note2,
    PDF_add_pdflink,
    PDF_add_weblink,
    PDF_attach_file,
    PDF_attach_file2,
    PDF_set_border_color,
    PDF_set_border_dash,
    PDF_set_border_style,
    /* p_basic.c: */
    PDF_begin_page,
    PDF_boot,
    PDF_close,
    PDF_delete,
    PDF_end_page,
    PDF_get_api,
    PDF_get_apiname,
    PDF_get_buffer,
    PDF_get_errmsg,
    PDF_get_errnum,
    PDF_get_majorversion,
    PDF_get_minorversion,
    PDF_get_opaque,
    PDF_new,
    PDF_new2,
    PDF_open_file,
    PDF_open_fp,
    PDF_open_mem,
    PDF_shutdown,
    pdf_jbuf,
    pdf_exit_try,
    pdf_catch,
    pdf_rethrow,
    /* p_block.c: */
    PDF_fill_imageblock,
    PDF_fill_pdfblock,
    PDF_fill_textblock,
    /* p_color.c: */
    PDF_makespotcolor,
    PDF_setcolor,
    PDF_setgray,
    PDF_setgray_stroke,
    PDF_setgray_fill,
    PDF_setrgbcolor,
    PDF_setrgbcolor_fill,
    PDF_setrgbcolor_stroke,
    /* p_draw.c: */
    PDF_arc,
    PDF_arcn,
    PDF_circle,
    PDF_clip,
    PDF_closepath,
    PDF_closepath_fill_stroke,
    PDF_closepath_stroke,
    PDF_curveto,
    PDF_endpath,
    PDF_fill,
    PDF_fill_stroke,
    PDF_lineto,
    PDF_moveto,
    PDF_rect,
    PDF_stroke,
    /* p_encoding.c: */
    PDF_encoding_set_char,
    /* p_font.c: */
    PDF_findfont,
    PDF_load_font,
    PDF_setfont,
    /* p_gstate.c */
    PDF_concat,
    PDF_initgraphics,
    PDF_restore,
    PDF_rotate,
    PDF_save,
    PDF_scale,
    PDF_setdash,
    PDF_setdashpattern,
    PDF_setflat,
    PDF_setlinecap,
    PDF_setlinejoin,
    PDF_setlinewidth,
    PDF_setmatrix,
    PDF_setmiterlimit,
    PDF_setpolydash,
    PDF_skew,
    PDF_translate,
    /* p_hyper.c: */
    PDF_add_bookmark,
    PDF_add_bookmark2,
    PDF_add_nameddest,
    PDF_set_info,
    PDF_set_info2,
    /* p_icc.c: */
    PDF_load_iccprofile,
    /* p_image.c: */
    PDF_add_thumbnail,
    PDF_close_image,
    PDF_fit_image,
    PDF_load_image,
    PDF_open_CCITT,
    PDF_open_image,
    PDF_open_image_file,
    PDF_place_image,
    /* p_kerning.c: */
    /* p_params.c: */
    PDF_get_parameter,
    PDF_get_value,
    PDF_set_parameter,
    PDF_set_value,
    /* p_pattern.c: */
    PDF_begin_pattern,
    PDF_end_pattern,
    /* p_pdi.c: */
    PDF_close_pdi,
    PDF_close_pdi_page,
    PDF_fit_pdi_page,
    PDF_get_pdi_parameter,
    PDF_get_pdi_value,
    PDF_open_pdi,
    PDF_open_pdi_callback,
    PDF_open_pdi_page,
    PDF_place_pdi_page,
    PDF_process_pdi,
    /* p_resource.c: */
    PDF_create_pvf,
    PDF_delete_pvf,
    /* p_shading.c: */
    PDF_shading,
    PDF_shading_pattern,
    PDF_shfill,
    /* p_template.c: */
    PDF_begin_template,
    PDF_end_template,
    /* p_text.c: */
    PDF_continue_text,
    PDF_continue_text2,
    PDF_fit_textline,
    PDF_set_text_pos,
    PDF_show,
    PDF_show2,
    PDF_show_boxed,
    PDF_show_xy,
    PDF_show_xy2,
    PDF_stringwidth,
    PDF_stringwidth2,
    /* p_type3.c: */
    PDF_begin_font,
    PDF_begin_glyph,
    PDF_end_font,
    PDF_end_glyph,
    /* p_xgstate.c: */
    PDF_create_gstate,
    PDF_set_gstate
};


PDFLIB_API void PDFLIB_CALL
PDF_boot(void)
{
    /* nothing yet */
}

PDFLIB_API void PDFLIB_CALL
PDF_shutdown(void)
{
    /* nothing yet */
}

PDFLIB_API const PDFlib_api * PDFLIB_CALL
PDF_get_api(void)
{
    return &PDFlib;
}


#if (defined(WIN32) || defined(__CYGWIN)) && defined(PDFLIB_EXPORTS)

/*
 * DLL entry function as required by Visual C++.
 * It is currently not necessary on Windows, but will eventually
 * be used to boot thread-global resources for PDFlib
 * (mainly font-related stuff).
 */
BOOL WINAPI
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

BOOL WINAPI
DllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void) hModule;
    (void) lpReserved;

    switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
	    PDF_boot();
	    break;
	case DLL_THREAD_ATTACH:
	    break;
	case DLL_THREAD_DETACH:
	    break;
	case DLL_PROCESS_DETACH:
	    PDF_shutdown();
	    break;
    }

    return TRUE;
}
#endif	/* WIN32 && PDFLIB_EXPORT */

PDFLIB_API int PDFLIB_CALL
PDF_get_majorversion(void)
{
    return PDFLIB_MAJORVERSION;
}

PDFLIB_API int PDFLIB_CALL
PDF_get_minorversion(void)
{
    return PDFLIB_MINORVERSION;
}

pdc_bool
pdf_enter_api(PDF *p, const char *funame, pdf_state s, const char *fmt, ...)
{
    va_list args;

    /* check whether the client completely screwed up */
    if (p == NULL || p->magic != PDC_MAGIC)
	return pdc_false;

    va_start(args, fmt);

    if (p->debug['t'])
        pdc_trace_api(p->pdc, funame, fmt, args);

    va_end(args);

    /* pdc_enter_api() will return pdc_false if the core is in error state.
    */
    if (!pdc_enter_api(p->pdc, funame))
	return pdc_false;

    /* check whether we are in a valid scope */
    if ((p->state_stack[p->state_sp] & s) == 0)
    {
	/* pdc_error() will NOT throw an exception (and simply return instead)
	** if the core is already in error state.
	*/
	pdc_error(p->pdc, PDF_E_DOC_SCOPE, pdf_current_scope(p), 0, 0, 0);

	return pdc_false;
    }

    return pdc_true;
}

static pdc_bool
pdf_enter_api2(PDF *p, const char *funame, const char *fmt, ...)
{
    va_list args;

    /* check whether the client completely screwed up */
    if (p == NULL || p->magic != PDC_MAGIC)
	return pdc_false;

    va_start(args, fmt);

    if (p->debug['t'])
	pdc_trace_api(p->pdc, funame, fmt, args);

    va_end(args);
    return pdc_true;
}


PDFLIB_API int PDFLIB_CALL
PDF_get_errnum(PDF *p)
{
    static const char fn[] = "PDF_get_errnum";
    int ret;

    if (!pdf_enter_api2(p, fn, "(p[%p])", (void *) p))
    {
	pdc_trace(p->pdc, "[0]\n");
	return 0;
    }

    ret = pdc_get_errnum(p->pdc);
    pdc_trace(p->pdc, "[%d]\n", ret);
    return ret;
}

PDFLIB_API const char * PDFLIB_CALL
PDF_get_errmsg(PDF *p)
{
    static const char fn[] = "PDF_get_errmsg";
    const char *ret;

    if (!pdf_enter_api2(p, fn, "(p[%p])", (void *) p))
    {
	pdc_trace(p->pdc, "[NULL]\n");
	return (char *) 0;
    }

    ret = pdc_get_errmsg(p->pdc);
    pdc_trace(p->pdc, "\n[\"%s\"]\n", ret);
    return ret;
}

PDFLIB_API const char * PDFLIB_CALL
PDF_get_apiname(PDF *p)
{
    static const char fn[] = "PDF_get_apiname";
    const char *ret;

    if (!pdf_enter_api2(p, fn, "(p[%p])", (void *) p))
    {
	pdc_trace(p->pdc, "[NULL]\n");
	return (char *) 0;
    }

    ret = pdc_get_apiname(p->pdc);
    pdc_trace(p->pdc, "[\"%s\"]\n", ret);
    return ret;
}

PDFLIB_API void * PDFLIB_CALL
PDF_get_opaque(PDF *p)
{
    static const char fn[] = "PDF_get_opaque";

    if (p == NULL || p->magic != PDC_MAGIC)
	return ((void *) NULL);

    pdc_trace(p->pdc, "/* ");

    if (!pdf_enter_api2(p, fn, "(p[%p])", (void *) p))
    {
	return ((void *) NULL);
    }

    pdc_trace(p->pdc, "[%p] */\n", p->opaque);

    return p->opaque;
}

const char *
pdf_current_scope(PDF *p)
{
    /* This array must be kept in sync with the pdf_state enum in p_intern.h */
    static const char *scope_names[] =
    {
	"object",
	"document",
	"page",
	"pattern",
	"template",
	"path",
	"gstate",
	"font",
	"glyph",
	"error"
    };

    int i;

    for (i = 0; i < (int)(sizeof(scope_names)/sizeof(char *)); ++i)
        if (PDF_GET_STATE(p) == (pdf_state) (1 << i))
	    return scope_names[i];

    pdc_error(p->pdc, PDF_E_INT_BADSCOPE,
	pdc_errprintf(p->pdc, " (0x%08X)", PDF_GET_STATE(p)), 0, 0, 0);

    return (char *) 0;	/* be happy, compiler! */
}

/* p may be NULL on the first call - we don't use it anyway */
static void *
default_malloc(PDF *p, size_t size, const char *caller)
{
    void *ret = malloc(size);

    (void) p;
    (void) caller;
#ifdef DEBUG
    if (p != NULL && p->debug['m'])
	fprintf(stderr, "%p malloced, size %d from %s, page %d\n",
		ret, (int) size, caller, p->current_page);
#endif

    return ret;
}

static void *
default_realloc(PDF *p, void *mem, size_t size, const char *caller)
{
    void *ret = realloc(mem, size);

    (void) p;
    (void) caller;
#ifdef DEBUG
    if (p->debug['r'])
	fprintf(stderr, "%p realloced to %p, %d from %s, page %d\n",
		mem, ret, (int) size, caller, p->current_page);
#endif

    return ret;
}

static void
default_free(PDF *p, void *mem)
{
    (void) p;

#ifdef DEBUG
    if (p->debug['f'])
	fprintf(stderr, "%p freed, page %d\n", mem, p->current_page);
#endif

    free(mem);
}

/* This is the easy version with the default handlers */
PDFLIB_API PDF * PDFLIB_CALL
PDF_new(void)
{
    return PDF_new2(NULL, NULL, NULL, NULL, NULL);
}

/* This is the spiced-up version with user-defined error and memory handlers */

PDFLIB_API PDF * PDFLIB_CALL
PDF_new2(
    void  (*errorhandler)(PDF *p, int type, const char *msg),
    void* (*allocproc)(PDF *p, size_t size, const char *caller),
    void* (*reallocproc)(PDF *p, void *mem, size_t size, const char *caller),
    void  (*freeproc)(PDF *p, void *mem),
    void   *opaque)
{
    PDF *	p;
    pdc_core *	pdc;

    /* If allocproc is NULL, all entries are supplied internally by PDFlib */
    if (allocproc == NULL) {
	allocproc	= default_malloc;
	reallocproc	= default_realloc;
	freeproc	= default_free;
    }

    p = (PDF *) (*allocproc) (NULL, sizeof(PDF), "PDF_new");

    if (p == NULL)
	return NULL;

    /*
     * Guard against crashes when PDF_delete is called without any
     * PDF_open_*() in between.
     */
    memset((void *)p, 0, (size_t) sizeof(PDF));

    /* these two are required by PDF_get_opaque() */
    p->magic = PDC_MAGIC;
    p->opaque = opaque;

    pdc = pdc_init_core(
	(pdc_error_fp) errorhandler,
	(pdc_alloc_fp) allocproc,
	(pdc_realloc_fp) reallocproc,
	(pdc_free_fp) freeproc, p);

    if (pdc == NULL)
    {
	(*freeproc)(p, p);
	return NULL;
    }

    pdc_register_errtab(pdc, PDC_ET_PDFLIB, pdf_errors, N_PDF_ERRORS);

    p->freeproc		= freeproc;
    p->pdc		= pdc;
    p->compatibility	= PDC_1_4;

    p->errorhandler	= errorhandler;

    p->flush		= pdf_flush_page;

    p->inheritgs	= pdc_false;

    p->hypertextencoding= pdc_invalidenc;
    p->hypertextformat  = pdc_auto;
    p->textformat       = pdc_auto;
    p->currtext         = NULL;





    p->resfilepending   = pdc_true;
    p->resourcefilename = NULL;
    p->resources        = NULL;
    p->prefix           = NULL;
    p->filesystem       = NULL;
    p->binding          = NULL;
    p->hastobepos       = pdc_false;
    p->rendintent       = AutoIntent;
    p->preserveoldpantonenames = pdc_false;
    p->spotcolorlookup  = pdc_true;
    p->ydirection       = (float) 1.0;
    p->usercoordinates  = pdc_false;
    p->names		= NULL;
    p->names_capacity	= 0;
    p->xobjects		= NULL;
    p->pdi		= NULL;
    p->pdi_strict	= pdc_false;
    p->pdi_sbuf		= NULL;
    p->state_sp		= 0;
    PDF_SET_STATE(p, pdf_state_object);

    /* all debug flags are cleared by default because of the above memset... */

    /* ...but warning messages for non-fatal errors should be set,
     * as well as font warnings -- the client must explicitly disable these.
     */
    p->debug['e'] = pdc_true;
    p->debug['F'] = pdc_true;
    p->debug['I'] = pdc_true;

    pdf_init_info(p);
    pdf_init_encodings(p);

    p->out	= (pdc_output *)pdc_boot_output(p->pdc);

    return p;
}

static void pdf_cleanup_contents(PDF *p);

/*
 * PDF_delete must be called for cleanup in case of error,
 * or when the client is done producing PDF.
 * It should never be called more than once for a given PDF, although
 * we try to guard against duplicated calls.
 *
 * Note: all pdf_cleanup_*() functions may safely be called multiple times.
 */

PDFLIB_API void PDFLIB_CALL
PDF_delete(PDF *p)
{
    static const char fn[] = "PDF_delete";

    if (!pdf_enter_api2(p, fn, "(p[%p])\n", (void *) p))
	return;

    /*
     * Clean up page-related stuff if necessary. Do not raise
     * an error here since we may be called from the error handler.
     */
    if (PDF_GET_STATE(p) != pdf_state_object) {
	pdc_close_output(p->out);
	pdf_cleanup_contents(p);
    }
    pdf_cleanup_encodings(p);

    /* close the output stream.
     * This can't be done in PDF_close() because the caller may fetch
     * the buffer only after PDF_close()ing the document.
     */
    pdc_cleanup_output(p->out);
    pdf_cleanup_resources(p);           /* release the resources tree */
    pdf_cleanup_filesystem(p);          /* release the file system */


    if (p->out)
	pdc_free(p->pdc, p->out);

    if (p->binding)
	pdc_free(p->pdc, p->binding);

    if (p->resourcefilename)
        pdc_free(p->pdc, p->resourcefilename);

    if (p->prefix)
        pdc_free(p->pdc, p->prefix);

    /* we never reach this point if (p->pdc == NULL).
    */
    pdc_delete_core(p->pdc);

    /* free the PDF structure and try to protect against duplicated calls */

    p->magic = 0L;		/* we don't reach this with the wrong magic */
    (*p->freeproc)(p, (void *) p);
}

static void
pdf_init_document(PDF *p)
{
    static const char *fn = "pdf_init_document";
    int i;

    p->contents_ids_capacity = CONTENTS_CHUNKSIZE;
    p->contents_ids = (pdc_id *) pdc_malloc(p->pdc,
	    sizeof(pdc_id) * p->contents_ids_capacity, fn);

    p->pages_capacity = PAGES_CHUNKSIZE;
    p->pages = (pdc_id *)
		pdc_malloc(p->pdc, sizeof(pdc_id) * p->pages_capacity, fn);

    /* mark ids to allow for pre-allocation of page ids */
    for (i = 0; i < p->pages_capacity; i++)
	p->pages[i] = PDC_BAD_ID;

    p->pnodes_capacity = PNODES_CHUNKSIZE;
    p->pnodes = (pdc_id *)
		pdc_malloc(p->pdc, sizeof(pdc_id) * p->pnodes_capacity, fn);

    p->current_pnode	= 0;
    p->current_pnode_kids = 0;

    p->current_page	= 0;
    p->open_mode	= open_auto;
    p->base		= NULL;

    p->ViewerPreferences.flags		= 0;
    p->ViewerPreferences.ViewArea	= use_none;
    p->ViewerPreferences.ViewClip	= use_none;
    p->ViewerPreferences.PrintArea	= use_none;
    p->ViewerPreferences.PrintClip	= use_none;

    pdf_init_destination(p, &p->open_action);
    pdf_init_destination(p, &p->bookmark_dest);

}

static void
pdf_init_contents(PDF *p)
{
    /*
     * None of these functions must call pdc_alloc_id() or generate
     * any output since the output machinery is not yet initialized!
     * If required, such calls must go in pdf_init_contents2() below.
     */

    pdf_init_document(p);
    pdf_init_images(p);
    pdf_init_xobjects(p);
    pdf_init_fonts(p);
    pdf_init_transition(p);
    pdf_init_outlines(p);
    pdf_init_annots(p);
    pdf_init_colorspaces(p);
    pdf_init_pattern(p);
    pdf_init_shadings(p);
    pdf_init_extgstate(p);

    /* clients may set char/word spacing and horizontal scaling outside pages
    ** for PDF_stringwidth() calculations, and they may set a color for use
    ** in PDF_makespotcolor(). Here we set the defaults.
    */
    p->sl = 0;
    pdf_init_tstate(p);
    pdf_init_cstate(p);

}

static void
pdf_init_contents2(PDF *p)
{
    /* second part of initialization, including pdcore services */

    p->pnodes[0]	= pdc_alloc_id(p->out);

    PDF_SET_STATE(p, pdf_state_document);
}

static void
pdf_feed_digest(PDF *p, unsigned char *custom, unsigned int len)
{
    pdc_init_digest(p->out);

    if (custom)
	pdc_update_digest(p->out, custom, len);

    pdc_update_digest(p->out, (unsigned char *) &p, sizeof(PDF*));

    pdc_update_digest(p->out, (unsigned char *) p, sizeof(PDF));

    pdc_update_digest(p->out,
	(unsigned char *) p->time_str, strlen(p->time_str));

    pdf_feed_digest_info(p);


    pdc_finish_digest(p->out);
}

#if defined(_MSC_VER) && defined(_MANAGED)
#pragma unmanaged
#endif
PDFLIB_API int PDFLIB_CALL
PDF_open_file(PDF *p, const char *filename)
{
    static const char fn[] = "PDF_open_file";

    if (!pdf_enter_api(p, fn, pdf_state_object, "(p[%p], \"%s\")",
	(void *) p, filename))
    {
        PDF_RETURN_BOOLEAN(p, -1);
    }

    pdf_init_contents(p);

    if (filename)
	pdf_feed_digest(p,
	    (unsigned char*) filename, (unsigned int) strlen(filename));
    else
	pdf_feed_digest(p, NULL, 0);

    if (!pdc_init_output((void *) p, p->out, filename, NULL, NULL,
	p->compatibility)) {
	pdc_set_errmsg(p->pdc, pdc_get_fopen_errnum(p->pdc, PDC_E_IO_WROPEN),
	    "PDF ", filename, 0, 0);

        if (p->debug['o'])
            pdc_warning(p->pdc, -1, 0, 0, 0, 0);

        PDF_RETURN_BOOLEAN(p, -1);
    }

    pdf_init_contents2(p);

    PDF_RETURN_BOOLEAN(p, 1);
}
#if defined(_MSC_VER) && defined(_MANAGED)
#pragma managed
#endif

PDFLIB_API int PDFLIB_CALL
PDF_open_fp(PDF *p, FILE *fp)
{
    static const char fn[] = "PDF_open_fp";

    if (!pdf_enter_api(p, fn, pdf_state_object, "(p[%p], fp[%p])",
	(void *) p, fp))
    {
        PDF_RETURN_BOOLEAN(p, -1);
    }

    if (fp == NULL)
    {
        PDF_RETURN_BOOLEAN(p, -1);
    }

/*
 * It is the callers responsibility to open the file in binary mode,
 * but it doesn't hurt to make sure it really is.
 * The Intel version of the Metrowerks compiler doesn't have setmode().
 */
#if !defined(__MWERKS__) && (defined(WIN32) || defined(OS2))
#if defined WINCE
    _setmode(fileno(fp), _O_BINARY);
#else
    setmode(fileno(fp), O_BINARY);
#endif
#endif

    pdf_init_contents(p);

    pdf_feed_digest(p, (unsigned char*) fp, sizeof(FILE));

    (void) pdc_init_output((void *) p, p->out, NULL, fp, NULL,
	p->compatibility);

    pdf_init_contents2(p);

    PDF_RETURN_BOOLEAN(p, 1);
}

/*
 * The external callback interface requires a PDF* as the first argument,
 * while the internal interface uses pdc_output* and doesn't know about PDF*.
 * We use a wrapper to bridge the gap, and store the PDF* within the
 * pdc_output structure opaquely.
 */

static size_t
writeproc_wrapper(pdc_output *out, void *data, size_t size)
{
    PDF *p = (PDF *) pdc_get_opaque(out);

    return (p->writeproc)(p, data, size);
}

PDFLIB_API void PDFLIB_CALL
PDF_open_mem(PDF *p, size_t (*i_writeproc)(PDF *p, void *data, size_t size))
{
    static const char fn[] = "PDF_open_mem";
    size_t (*writeproc)(PDF *, void *, size_t) = i_writeproc;

    if (!pdf_enter_api(p, fn, pdf_state_object,
	"(p[%p], wp[%p])\n", (void *) p, (void *) writeproc))
	return;

    if (writeproc == NULL)
	pdc_error(p->pdc, PDC_E_ILLARG_EMPTY, "writeproc", 0, 0, 0);

    p->writeproc = writeproc;

    pdf_init_contents(p);

    pdf_feed_digest(p, (unsigned char*) &writeproc,
	(unsigned int) sizeof(writeproc));

    (void) pdc_init_output((void *) p, p->out, NULL, NULL, writeproc_wrapper,
	p->compatibility);

    pdf_init_contents2(p);
}

/*
 * The caller must use the contents of the returned buffer before
 * calling the next PDFlib function.
 */

PDFLIB_API const char * PDFLIB_CALL
PDF_get_buffer(PDF *p, long *size)
{
    static const char fn[] = "PDF_get_buffer";
    const char *ret;

    if (!pdf_enter_api(p, fn,
        (pdf_state) (pdf_state_object | pdf_state_document),
	"(p[%p], &size[%p])", (void *) p, (void *) size)) {
	*size = (long) 0;
	return (const char) NULL;
    }

    ret = pdc_get_stream_contents(p->out, size);

    pdc_trace(p->pdc, "[%p, size=%ld]\n", (void *) (ret), *size);
    return ret;
}

static void
pdf_write_pnode(PDF *p,
	pdc_id node_id,
	pdc_id parent_id,
	pdc_id *kids,
	int n_kids,
	int n_pages)
{
    pdc_begin_obj(p->out, node_id);
    pdc_begin_dict(p->out);
    pdc_puts(p->out, "/Type/Pages\n");
    pdc_printf(p->out, "/Count %d\n", n_pages);

    if (parent_id != PDC_BAD_ID)
	pdc_printf(p->out, "/Parent %ld 0 R\n", parent_id);

    pdc_puts(p->out, "/Kids[");

    do
    {
	pdc_printf(p->out, "%ld 0 R", *kids++);
        pdc_puts(p->out, "\n");
    } while (--n_kids > 0);

    pdc_puts(p->out, "]");
    pdc_end_dict(p->out);
    pdc_end_obj(p->out);
}

#define N_KIDS	10

static pdc_id
pdf_get_pnode_id(PDF *p)
{
    static const char fn[] = "pdf_get_pnode_id";

    if (p->current_pnode_kids == N_KIDS)
    {
	if (++p->current_pnode == p->pnodes_capacity)
	{
	    p->pnodes_capacity *= 2;
	    p->pnodes = (pdc_id *) pdc_realloc(p->pdc, p->pnodes,
			sizeof (pdc_id) * p->pnodes_capacity, fn);
	}

	p->pnodes[p->current_pnode] = pdc_alloc_id(p->out);
	p->current_pnode_kids = 1;
    }
    else
	++p->current_pnode_kids;

    return p->pnodes[p->current_pnode];
}

static pdc_id
pdf_make_tree(PDF *p,
    pdc_id parent_id,
    pdc_id *pnodes,
    pdc_id *pages,
    int n_pages)
{
    if (n_pages <= N_KIDS)
    {
	/* this is a near-to-leaf node. use the pre-allocated id
	** from p->pnodes.
	*/
	pdf_write_pnode(p, *pnodes, parent_id, pages, n_pages, n_pages);
	return *pnodes;
    }
    else
    {
	pdc_id node_id = pdc_alloc_id(p->out);
	pdc_id kids[N_KIDS];
	int n_kids, rest;
        int tpow = N_KIDS;
	int i;

        /* tpow < n_pages <= tpow*N_KIDS
	*/
        while (tpow * N_KIDS < n_pages)
            tpow *= N_KIDS;

        n_kids = n_pages / tpow;
        rest = n_pages % tpow;

        for (i = 0; i < n_kids; ++i, pnodes += tpow / N_KIDS, pages += tpow)
	{
            kids[i] = pdf_make_tree(p, node_id, pnodes, pages, tpow);
	}

	if (rest)
	{
	    kids[i] = pdf_make_tree(p, node_id, pnodes, pages, rest);
	    ++n_kids;
	}

	pdf_write_pnode(p, node_id, parent_id, kids, n_kids, n_pages);
	return node_id;
    }
}

static pdc_id
pdf_write_pages_and_catalog(PDF *p)
{
    pdc_id pages_id;
    pdc_id root_id;
    pdc_id names_id;

    pages_id =
	pdf_make_tree(p, PDC_BAD_ID, p->pnodes, p->pages+1, p->current_page);


    names_id = pdf_write_names(p);

    root_id = pdc_begin_obj(p->out, PDC_NEW_ID);  /* Catalog or Root object */

    pdc_begin_dict(p->out);
    pdc_puts(p->out, "/Type/Catalog\n");


    if (p->ViewerPreferences.flags ||
	p->ViewerPreferences.ViewArea != use_none ||
	p->ViewerPreferences.ViewClip != use_none ||
	p->ViewerPreferences.PrintArea != use_none ||
	p->ViewerPreferences.PrintClip != use_none)
    {
	pdc_printf(p->out, "/ViewerPreferences\n");
	pdc_begin_dict(p->out);

	if (p->ViewerPreferences.flags & HideToolbar)
	    pdc_printf(p->out, "/HideToolbar true\n");

	if (p->ViewerPreferences.flags & HideMenubar)
	    pdc_printf(p->out, "/HideMenubar true\n");

	if (p->ViewerPreferences.flags & HideWindowUI)
	    pdc_printf(p->out, "/HideWindowUI true\n");

	if (p->ViewerPreferences.flags & FitWindow)
	    pdc_printf(p->out, "/FitWindow true\n");

	if (p->ViewerPreferences.flags & CenterWindow)
	    pdc_printf(p->out, "/CenterWindow true\n");

	if (p->ViewerPreferences.flags & DisplayDocTitle)
	    pdc_printf(p->out, "/DisplayDocTitle true\n");

	if (p->ViewerPreferences.flags & NonFullScreenPageModeOutlines)
	    pdc_printf(p->out, "/NonFullScreenPageMode/UseOutlines\n");
	else if (p->ViewerPreferences.flags & NonFullScreenPageModeThumbs)
	    pdc_printf(p->out, "/NonFullScreenPageMode/UseThumbs\n");

	if (p->ViewerPreferences.flags & DirectionR2L)
	    pdc_printf(p->out, "/Direction/R2L\n");

	if (p->ViewerPreferences.ViewArea == use_media)
	    pdc_printf(p->out, "/ViewArea/MediaBox\n");
	if (p->ViewerPreferences.ViewArea == use_bleed)
	    pdc_printf(p->out, "/ViewArea/BleedBox\n");
	else if (p->ViewerPreferences.ViewArea == use_trim)
	    pdc_printf(p->out, "/ViewArea/TrimBox\n");
	else if (p->ViewerPreferences.ViewArea == use_art)
	    pdc_printf(p->out, "/ViewArea/ArtBox\n");

	if (p->ViewerPreferences.ViewClip == use_media)
	    pdc_printf(p->out, "/ViewClip/MediaBox\n");
	if (p->ViewerPreferences.ViewClip == use_bleed)
	    pdc_printf(p->out, "/ViewClip/BleedBox\n");
	else if (p->ViewerPreferences.ViewClip == use_trim)
	    pdc_printf(p->out, "/ViewClip/TrimBox\n");
	else if (p->ViewerPreferences.ViewClip == use_art)
	    pdc_printf(p->out, "/ViewClip/ArtBox\n");

	if (p->ViewerPreferences.PrintArea == use_media)
	    pdc_printf(p->out, "/PrintArea/MediaBox\n");
	if (p->ViewerPreferences.PrintArea == use_bleed)
	    pdc_printf(p->out, "/PrintArea/BleedBox\n");
	else if (p->ViewerPreferences.PrintArea == use_trim)
	    pdc_printf(p->out, "/PrintArea/TrimBox\n");
	else if (p->ViewerPreferences.PrintArea == use_art)
	    pdc_printf(p->out, "/PrintArea/ArtBox\n");

	if (p->ViewerPreferences.PrintClip == use_media)
	    pdc_printf(p->out, "/PrintClip/MediaBox\n");
	if (p->ViewerPreferences.PrintClip == use_bleed)
	    pdc_printf(p->out, "/PrintClip/BleedBox\n");
	else if (p->ViewerPreferences.PrintClip == use_trim)
	    pdc_printf(p->out, "/PrintClip/TrimBox\n");
	else if (p->ViewerPreferences.PrintClip == use_art)
	    pdc_printf(p->out, "/PrintClip/ArtBox\n");

	pdc_end_dict(p->out);				/* ViewerPreferences */
    }

    /*
     * specify the open action (display of the first page)
     * default: top of the first page at default zoom level
     */
    if (p->open_action.type != fitwindow ||
        p->open_action.zoom != -1 ||
        p->open_action.page != 0) {
	    pdc_puts(p->out, "/OpenAction");
	    pdf_write_destination(p, &p->open_action);
	    pdc_puts(p->out, "\n");
    }

    /*
     * specify the document's open mode
     * default = open_none: open document with neither bookmarks nor
     * thumbnails visible
     */
    if (p->open_mode == open_bookmarks) {
	pdc_printf(p->out, "/PageMode/UseOutlines\n");

    } else if (p->open_mode == open_thumbnails) {
	pdc_printf(p->out, "/PageMode/UseThumbs\n");

    } else if (p->open_mode == open_fullscreen) {
	pdc_printf(p->out, "/PageMode/FullScreen\n");
    }

    if (p->base) {
	pdc_puts(p->out, "/URI<</Base");
        pdc_put_pdfstring(p->out, p->base, (int) strlen(p->base));
	pdc_end_dict(p->out);
    }

    						/* Pages object */
    pdc_printf(p->out, "/Pages %ld 0 R\n", pages_id);

    if (names_id != PDC_BAD_ID) {
	pdc_printf(p->out, "/Names");
	pdc_begin_dict(p->out);				/* Names */
	pdc_printf(p->out, "/Dests %ld 0 R\n", names_id);
	pdc_end_dict(p->out);				/* Names */
    }

    pdf_write_outline_root(p);

    pdc_end_dict(p->out);				/* Catalog */
    pdc_end_obj(p->out);

    return root_id;
}

static void
pdf_cleanup_contents(PDF *p)
{
    /* clean up all document-related stuff */
    if (p->contents_ids) {
	pdc_free(p->pdc, p->contents_ids);
	p->contents_ids = NULL;
    }
    if (p->pages) {
	pdc_free(p->pdc, p->pages);
	p->pages = NULL;
    }
    if (p->base) {
	pdc_free(p->pdc, p->base);
	p->base = NULL;
    }
    if (p->pnodes) {
	pdc_free(p->pdc, p->pnodes);
	p->pnodes = NULL;
    }

    if (p->currtext) {
        pdc_free(p->pdc, p->currtext);
        p->currtext = NULL;
    }

    pdf_cleanup_destination(p, &p->open_action);
    pdf_cleanup_destination(p, &p->bookmark_dest);

    pdf_cleanup_info(p);
    pdf_cleanup_fonts(p);
    pdf_cleanup_outlines(p);
    pdf_cleanup_annots(p);
    pdf_cleanup_names(p);
    pdf_cleanup_colorspaces(p);
    pdf_cleanup_pattern(p);
    pdf_cleanup_shadings(p);
    pdf_cleanup_images(p);
    pdf_cleanup_xobjects(p);
    pdf_cleanup_extgstates(p);
}

PDFLIB_API void PDFLIB_CALL
PDF_close(PDF *p)
{
    static const char fn[] = "PDF_close";

    pdc_id info_id;
    pdc_id root_id;

    if (!pdf_enter_api(p, fn, pdf_state_document, "(p[%p])\n", (void *) p))
	return;

    if (PDF_GET_STATE(p) != pdf_state_error) {
	if (p->current_page == 0)
	    pdc_error(p->pdc, PDF_E_DOC_EMPTY, 0, 0, 0, 0);

	/* Write all pending document information up to xref table + trailer */
	info_id = pdf_write_info(p);
	pdf_write_doc_fonts(p);			/* font objects */
	pdf_write_doc_colorspaces(p);		/* color space resources */
	pdf_write_doc_extgstates(p);		/* ExtGState resources */
	root_id = pdf_write_pages_and_catalog(p);
	pdf_write_outlines(p);
	pdc_write_xref_and_trailer(p->out, info_id, root_id);
    }

    pdc_close_output(p->out);

    /* Don't call pdc_cleanup_output() here because we may still need
     * the buffer contents for PDF_get_buffer() after PDF_close().
     */
    pdf_cleanup_contents(p);

    /* UPR stuff not released here but in PDF_delete() */

    PDF_SET_STATE(p, pdf_state_object);
}

void
pdf_begin_contents_section(PDF *p)
{
    if (p->contents != c_none)
	return;

    if (p->next_content >= p->contents_ids_capacity) {
	p->contents_ids_capacity *= 2;
	p->contents_ids = (pdc_id *) pdc_realloc(p->pdc, p->contents_ids,
			    sizeof(pdc_id) * p->contents_ids_capacity,
			    "pdf_begin_contents_section");
    }

    p->contents_ids[p->next_content] = pdc_begin_obj(p->out, PDC_NEW_ID);
    p->contents	= c_page;
    pdc_begin_dict(p->out);
    p->length_id = pdc_alloc_id(p->out);
    pdc_printf(p->out, "/Length %ld 0 R\n", p->length_id);

    if (pdc_get_compresslevel(p->out))
	    pdc_puts(p->out, "/Filter/FlateDecode\n");

    pdc_end_dict(p->out);

    pdc_begin_pdfstream(p->out);

    p->next_content++;
}

void
pdf_end_contents_section(PDF *p)
{
    if (p->contents == c_none)
	return;

    pdf_end_text(p);
    p->contents = c_none;

    pdc_end_pdfstream(p->out);
    pdc_end_obj(p->out);

    pdc_put_pdfstreamlength(p->out, p->length_id);
}

PDFLIB_API void PDFLIB_CALL
PDF_begin_page(PDF *p, float width, float height)
{
    static const char fn[] = "PDF_begin_page";

    if (!pdf_enter_api(p, fn, pdf_state_document, "(p[%p], %g, %g)\n",
	(void *) p, width, height))
	return;

    if (width <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "width", 0, 0, 0);

    if (height <= 0)
	pdc_error(p->pdc, PDC_E_ILLARG_POSITIVE, "height", 0, 0, 0);

    if ((height < PDF_ACRO4_MINPAGE || width < PDF_ACRO4_MINPAGE ||
	height > PDF_ACRO4_MAXPAGE || width > PDF_ACRO4_MAXPAGE))
	    pdc_warning(p->pdc, PDF_E_PAGE_SIZE_ACRO4, 0, 0, 0, 0);


    if (++(p->current_page) >= p->pages_capacity)
	pdf_grow_pages(p);

    /* no id has been preallocated */
    if (p->pages[p->current_page] == PDC_BAD_ID)
	p->pages[p->current_page] = pdc_alloc_id(p->out);

    p->height		= height;
    p->width		= width;
    p->thumb_id		= PDC_BAD_ID;
    p->next_content	= 0;
    p->contents 	= c_none;
    p->sl		= 0;

    pdc_rect_init(&p->CropBox, (float) 0, (float) 0, (float) 0, (float) 0);
    pdc_rect_init(&p->BleedBox, (float) 0, (float) 0, (float) 0, (float) 0);
    pdc_rect_init(&p->TrimBox, (float) 0, (float) 0, (float) 0, (float) 0);
    pdc_rect_init(&p->ArtBox, (float) 0, (float) 0, (float) 0, (float) 0);

    PDF_SET_STATE(p, pdf_state_page);

    pdf_init_page_annots(p);
    pdf_init_tstate(p);
    pdf_init_gstate(p);
    pdf_init_cstate(p);

    pdf_begin_contents_section(p);


    /* top-down y coordinates */
    pdf_set_topdownsystem(p, height);
}

PDFLIB_API void PDFLIB_CALL
PDF_end_page(PDF *p)
{
    static const char fn[] = "PDF_end_page";
    int idx;


    if (!pdf_enter_api(p, fn, pdf_state_page, "(p[%p])\n", (void *) p))
	return;


    /* check whether PDF_save() and PDF_restore() calls are balanced */
    if (p->sl > 0)
	pdc_error(p->pdc, PDF_E_GSTATE_UNMATCHEDSAVE, 0, 0, 0, 0);

    /* restore text parameter and color defaults for out-of-page usage.
    */
    pdf_init_tstate(p);
    pdf_init_cstate(p);

    pdf_end_contents_section(p);

    /* Page object */
    pdc_begin_obj(p->out, p->pages[p->current_page]);

    pdc_begin_dict(p->out);
    pdc_puts(p->out, "/Type/Page\n");
    pdc_printf(p->out, "/Parent %ld 0 R\n", pdf_get_pnode_id(p));

    p->res_id = pdc_alloc_id(p->out);
    pdc_printf(p->out, "/Resources %ld 0 R\n", p->res_id);

    pdc_printf(p->out, "/MediaBox[0 0 %f %f]\n", p->width, p->height);


    if (!pdc_rect_isnull(&p->CropBox))
    {
	if (p->CropBox.urx <= p->CropBox.llx ||
	    p->CropBox.ury <= p->CropBox.lly)
		pdc_error(p->pdc, PDF_E_PAGE_BADBOX, "CropBox",
		    pdc_errprintf(p->pdc, "%f %f %f %f",
		    p->CropBox.llx, p->CropBox.lly,
		    p->CropBox.urx, p->CropBox.ury), 0, 0);

	pdc_printf(p->out, "/CropBox[%f %f %f %f]\n",
	    p->CropBox.llx, p->CropBox.lly, p->CropBox.urx, p->CropBox.ury);
    }

    if (!pdc_rect_isnull(&p->BleedBox))
    {
	if (p->BleedBox.urx <= p->BleedBox.llx ||
	    p->BleedBox.ury <= p->BleedBox.lly)
		pdc_error(p->pdc, PDF_E_PAGE_BADBOX, "BleedBox",
		    pdc_errprintf(p->pdc, "%f %f %f %f",
		    p->BleedBox.llx, p->BleedBox.lly,
		    p->BleedBox.urx, p->BleedBox.ury), 0, 0);

	pdc_printf(p->out, "/BleedBox[%f %f %f %f]\n",
	    p->BleedBox.llx, p->BleedBox.lly, p->BleedBox.urx, p->BleedBox.ury);
    }

    if (!pdc_rect_isnull(&p->TrimBox))
    {
	if (p->TrimBox.urx <= p->TrimBox.llx ||
	    p->TrimBox.ury <= p->TrimBox.lly)
		pdc_error(p->pdc, PDF_E_PAGE_BADBOX, "TrimBox",
		    pdc_errprintf(p->pdc, "%f %f %f %f",
		    p->TrimBox.llx, p->TrimBox.lly,
		    p->TrimBox.urx, p->TrimBox.ury), 0, 0);

	pdc_printf(p->out, "/TrimBox[%f %f %f %f]\n",
	    p->TrimBox.llx, p->TrimBox.lly, p->TrimBox.urx, p->TrimBox.ury);
    }

    if (!pdc_rect_isnull(&p->ArtBox))
    {
	if (p->ArtBox.urx <= p->ArtBox.llx ||
	    p->ArtBox.ury <= p->ArtBox.lly)
		pdc_error(p->pdc, PDF_E_PAGE_BADBOX, "ArtBox",
		    pdc_errprintf(p->pdc, "%f %f %f %f",
		    p->ArtBox.llx, p->ArtBox.lly,
		    p->ArtBox.urx, p->ArtBox.ury), 0, 0);

	pdc_printf(p->out, "/ArtBox[%f %f %f %f]\n",
	    p->ArtBox.llx, p->ArtBox.lly, p->ArtBox.urx, p->ArtBox.ury);
    }

    /*
     * The duration can be placed in the transition dictionary (/D)
     * or in the page dictionary (/Dur). We put it here so it can
     * be used without setting a transition effect.
     */

    if (p->duration > 0)
	pdc_printf(p->out, "/Dur %f\n", p->duration);

    pdf_write_page_transition(p);

    pdc_puts(p->out, "/Contents[");
    for (idx = 0; idx < p->next_content; idx++) {
	pdc_printf(p->out, "%ld 0 R", p->contents_ids[idx]);
	pdc_putc(p->out, (char) (idx + 1 % 8 ? PDF_SPACE : PDF_NEWLINE));
    }
    pdc_puts(p->out, "]\n");

    /* Thumbnail image */
    if (p->thumb_id != PDC_BAD_ID)
	pdc_printf(p->out, "/Thumb %ld 0 R\n", p->thumb_id);

    pdf_write_annots_root(p);

    pdc_end_dict(p->out);			/* Page object */
    pdc_end_obj(p->out);

    pdf_write_page_annots(p);			/* Annotation dicts */

    pdc_begin_obj(p->out, p->res_id);		/* resource object */
    pdc_begin_dict(p->out);			/* resource dict */

    pdf_write_page_fonts(p);			/* Font resources */

    pdf_write_page_colorspaces(p);		/* Color space resources */

    pdf_write_page_pattern(p);			/* Pattern resources */

    pdf_write_page_shadings(p);			/* Shading resources */

    pdf_write_xobjects(p);			/* XObject resources */

    pdf_write_page_extgstates(p);		/* ExtGState resources */

    pdc_end_dict(p->out);			/* resource dict */
    pdc_end_obj(p->out);			/* resource object */

    /* Free all page-related resources */
    pdf_cleanup_page_annots(p);

    PDF_SET_STATE(p, pdf_state_document);

    if (p->flush & pdf_flush_page)
	pdc_flush_stream(p->out);
}

void
pdf_grow_pages(PDF *p)
{
    int i;

    p->pages_capacity *= 2;
    p->pages = (pdc_id *) pdc_realloc(p->pdc, p->pages,
		sizeof(pdc_id) * p->pages_capacity, "pdf_grow_pages");
    for (i = p->pages_capacity/2; i < p->pages_capacity; i++)
	p->pages[i] = PDC_BAD_ID;
}

/* ------------------- exception handling ------------------- */

PDFLIB_API pdf_jmpbuf * PDFLIB_CALL
pdf_jbuf(PDF *p)
{
    return (pdf_jmpbuf *) pdc_jbuf(p->pdc);
}

PDFLIB_API void PDFLIB_CALL
pdf_exit_try(PDF *p)
{
    if (p == NULL || p->magic != PDC_MAGIC)
	return;

    pdc_exit_try(p->pdc);
}

PDFLIB_API int PDFLIB_CALL
pdf_catch(PDF *p)
{
    if (p == NULL || p->magic != PDC_MAGIC)
	return pdc_false;

    return pdc_catch_extern(p->pdc);
}

PDFLIB_API void PDFLIB_CALL
pdf_rethrow(PDF *p)
{
    if (p == NULL || p->magic != PDC_MAGIC)
	return;

    pdc_rethrow(p->pdc);
}

PDFLIB_API void PDFLIB_CALL
pdf_throw(PDF *p, const char *parm1, const char *parm2, const char *parm3)
{
    pdc_enter_api(p->pdc, "pdf_throw");

    pdc_error(p->pdc, PDF_E_INT_WRAPPER, parm1, parm2, parm3, NULL);
}

/* -------------------------- handle check -------------------------------*/

void
pdf_check_handle(PDF *p, int handle, pdc_opttype type)
{
    int minval = 0, maxval = 0;
    pdc_bool empty = pdc_false;

    switch (type)
    {
        case pdc_colorhandle:
        maxval = p->colorspaces_number - 1;
        break;


        case pdc_fonthandle:
        maxval = p->fonts_number - 1;
        break;

        case pdc_gstatehandle:
        maxval = p->extgstates_number;
        break;


        case pdc_imagehandle:
        maxval = p->images_capacity - 1;
        if (handle >= minval && handle <= maxval &&
            (!p->images[handle].in_use ||
              p->xobjects[p->images[handle].no].type == pdi_xobject))
            empty = pdc_true;
        break;

        case pdc_pagehandle:
        maxval = p->images_capacity - 1;
        if (handle >= minval && handle <= maxval &&
            (!p->images[handle].in_use ||
              p->xobjects[p->images[handle].no].type != pdi_xobject))
            empty = pdc_true;
        break;

        case pdc_patternhandle:
        maxval = p->pattern_number - 1;
        break;

        case pdc_shadinghandle:
        maxval = p->shadings_number - 1;
        break;

        default:
        break;
    }

    if (handle < minval || handle > maxval || empty)
    {
        const char *stemp1 = pdc_errprintf(p->pdc, "%s",
                                           pdc_get_handletype(type));
        const char *stemp2 = pdc_errprintf(p->pdc, "%d",
                                           p->hastobepos ? handle + 1 : handle);
        pdc_error(p->pdc, PDC_E_ILLARG_HANDLE, stemp1, stemp2, 0, 0);
    }
}
