/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2003 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: pdflib.h,v 1.3 2003/09/18 20:20:45 laplace Exp $
 *
 * PDFlib public function and constant declarations
 *
 */

#ifndef PDFLIB_H
#define PDFLIB_H

/*
 * ----------------------------------------------------------------------
 * Setup, mostly Windows calling conventions and DLL stuff
 * ----------------------------------------------------------------------
 */

#include <stdio.h>

#ifdef WIN32

#if !defined(PDFLIB_CALL)
#define PDFLIB_CALL	__cdecl
#endif

#if !defined(PDFLIB_API)
#ifdef PDFLIB_EXPORTS
#define PDFLIB_API __declspec(dllexport) /* prepare a DLL (internal use only) */

#elif defined(PDFLIB_DLL)
#define PDFLIB_API __declspec(dllimport) /* PDFlib clients: import PDFlib DLL */

#else	/* !PDFLIB_DLL */
#define PDFLIB_API /* */	/* default: generate or use static library */

#endif	/* !PDFLIB_DLL */
#endif /* !PDFLIB_API */

#else	/* !WIN32 */

#if ((defined __IBMC__ || defined __IBMCPP__) && defined __DLL__ && defined OS2)
    #define PDFLIB_CALL _Export
    #define PDFLIB_API
#endif	/* IBM VisualAge C++ DLL */

#ifndef PDFLIB_CALL
#define PDFLIB_CALL
#endif
#ifndef PDFLIB_API
#define PDFLIB_API
#endif

#endif	/* !WIN32 */


/* export all symbols for a shared library on the Mac */
#if defined(__MWERKS__) && defined(PDFLIB_EXPORTS)
#pragma export on
#endif


/* Make our declarations C++ compatible */
#ifdef __cplusplus
extern "C" {
#endif


/* Define the basic PDF type. This is used opaquely at the API level. */
#if !defined(PDF) || defined(ACTIVEX)
typedef struct PDF_s PDF;
#endif /* !PDF */

/* The API structure with function pointers. */
typedef struct PDFlib_api_s PDFlib_api;

/*
 * The version defines below may be used to check the version of the
 * include file against the library.
 */

/* do not change this (version.sh will do it for you :) */
#define PDFLIB_MAJORVERSION	5	/* PDFlib major version number */
#define PDFLIB_MINORVERSION	0	/* PDFlib minor version number */
#define PDFLIB_REVISION		2	/* PDFlib revision number */
#define PDFLIB_VERSIONSTRING	"5.0.2"	/* The whole bunch */


/* {{{ p_annots.c: file attachments, notes, and links
 * ----------------------------------------------------------------------
 */

/* Add a launch annotation (to a target of arbitrary file type). */
PDFLIB_API void PDFLIB_CALL
PDF_add_launchlink(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename);

/* Add a link annotation to a target within the current PDF file. */
PDFLIB_API void PDFLIB_CALL
PDF_add_locallink(PDF *p, float llx, float lly, float urx, float ury,
    int page, const char *optlist);

/* Add a note annotation. icon is one of of "comment", "insert", "note",
 * "paragraph", "newparagraph", "key", or "help". */
PDFLIB_API void PDFLIB_CALL
PDF_add_note(PDF *p, float llx, float lly, float urx, float ury,
    const char *contents, const char *title, const char *icon, int open);

PDFLIB_API void PDFLIB_CALL
PDF_add_note2(PDF *p, float llx, float lly, float urx, float ury,
    const char *contents, int len_cont, const char *title, int len_title,
    const char *icon, int open);

/* Add a file link annotation (to a PDF target). */
PDFLIB_API void PDFLIB_CALL
PDF_add_pdflink(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename, int page, const char *optlist);

/* Add a weblink annotation to a target URL on the Web. */
PDFLIB_API void PDFLIB_CALL
PDF_add_weblink(PDF *p, float llx, float lly, float urx, float ury,
    const char *url);

/* Add a file attachment annotation. icon is one of "graph", "paperclip",
 * "pushpin", or "tag". */
PDFLIB_API void PDFLIB_CALL
PDF_attach_file(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename, const char *description, const char *author,
    const char *mimetype, const char *icon);

PDFLIB_API void PDFLIB_CALL
PDF_attach_file2(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename, int reserved, const char *description, int len_descr,
    const char *author, int len_auth, const char *mimetype, const char *icon);

/* Set the border color for all kinds of annotations. */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_color(PDF *p, float red, float green, float blue);

/* Set the border dash style for all kinds of annotations. See PDF_setdash(). */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_dash(PDF *p, float b, float w);

/* Set the border style for all kinds of annotations. style is "solid" or
 * "dashed". */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_style(PDF *p, const char *style, float width);
/* }}} */


/* {{{ p_basic.c: general functions
 * ----------------------------------------------------------------------
 */

/* Add a new page to the document. */
PDFLIB_API void PDFLIB_CALL
PDF_begin_page(PDF *p, float width, float height);

/* Boot PDFlib (recommended although currently not required). */
PDFLIB_API void PDFLIB_CALL
PDF_boot(void);

/* Close the generated PDF file, and release all document-related resources. */
PDFLIB_API void PDFLIB_CALL
PDF_close(PDF *p);

/* Delete the PDFlib object, and free all internal resources. */
PDFLIB_API void PDFLIB_CALL
PDF_delete(PDF *p);

/* Finish the page. */
PDFLIB_API void PDFLIB_CALL
PDF_end_page(PDF *p);

/* Retrieve a structure with PDFlib API function pointers (mainly for DLLs).
 * Although this function is published here, it is not supposed to be used
 * directly by clients. Use PDF_boot_dll() (in pdflibdl.c) instead. */
PDFLIB_API const PDFlib_api * PDFLIB_CALL
PDF_get_api(void);

/* Get the name of the API function which threw the last exception or failed. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_apiname(PDF *p);

/* Get the contents of the PDF output buffer. The result must be used by
 * the client before calling any other PDFlib function. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_buffer(PDF *p, long *size);

/* Get the descriptive text of the last thrown exception, or the reason of
 * a failed function call. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_errmsg(PDF *p);

/* Get the number of the last thrown exception, or the reason of a failed
 * function call. */
PDFLIB_API int PDFLIB_CALL
PDF_get_errnum(PDF *p);

/* Depreceated: use PDF_get_value() */
PDFLIB_API int PDFLIB_CALL
PDF_get_majorversion(void);

/* Depreceated: use PDF_get_value() */
PDFLIB_API int PDFLIB_CALL
PDF_get_minorversion(void);

/* Fetch the opaque application pointer stored in PDFlib. */
PDFLIB_API void * PDFLIB_CALL
PDF_get_opaque(PDF *p);

/* Create a new PDFlib object, using default error handling and memory
 management. */
PDFLIB_API PDF * PDFLIB_CALL
PDF_new(void);

/* Create a new PDFlib object with client-supplied error handling and memory
 * allocation routines. */
typedef void  (*errorproc_t)(PDF *p1, int errortype, const char *msg);
typedef void* (*allocproc_t)(PDF *p2, size_t size, const char *caller);
typedef void* (*reallocproc_t)(PDF *p3,
		void *mem, size_t size, const char *caller);
typedef void  (*freeproc_t)(PDF *p4, void *mem);

PDFLIB_API PDF * PDFLIB_CALL
PDF_new2(errorproc_t errorhandler, allocproc_t allocproc,
	reallocproc_t reallocproc, freeproc_t freeproc, void *opaque);

/* Create a new PDF file using the supplied file name. */
PDFLIB_API int PDFLIB_CALL
PDF_open_file(PDF *p, const char *filename);

/* Open a new PDF file associated with p, using the supplied file handle. */
PDFLIB_API int PDFLIB_CALL
PDF_open_fp(PDF *p, FILE *fp);

/* Open a new PDF in memory, and install a callback for fetching the data. */
typedef size_t (*writeproc_t)(PDF *p1, void *data, size_t size);
PDFLIB_API void PDFLIB_CALL
PDF_open_mem(PDF *p, writeproc_t writeproc);

/* Shut down PDFlib (recommended although currently not required). */
PDFLIB_API void PDFLIB_CALL
PDF_shutdown(void);

/*
 * Error classes are deprecated; use PDF_TRY/PDF_CATCH instead.
 * Note that old-style error handlers are still supported, but
 * they will always receive type PDF_NonfatalError (for warnings)
 * or PDF_UnknownError (for other exceptions).
 */

#define PDF_MemoryError    1
#define PDF_IOError        2
#define PDF_RuntimeError   3
#define PDF_IndexError     4
#define PDF_TypeError      5
#define PDF_DivisionByZero 6
#define PDF_OverflowError  7
#define PDF_SyntaxError    8
#define PDF_ValueError     9
#define PDF_SystemError   10

#define PDF_NonfatalError 11
#define PDF_UnknownError  12

/* }}} */


/*{{{ p_block.c: Variable Data Processing with blocks (requires the PDI library)
 * --------------------------------------------------------------------------
 */

/* Process an image block according to its properties. */
PDFLIB_API int PDFLIB_CALL
PDF_fill_imageblock(PDF *p, int page, const char *blockname,
    int image, const char *optlist);

/* Process a PDF block according to its properties. */
PDFLIB_API int PDFLIB_CALL
PDF_fill_pdfblock(PDF *p, int page, const char *blockname,
    int contents, const char *optlist);

/* Process a text block according to its properties. */
PDFLIB_API int PDFLIB_CALL
PDF_fill_textblock(PDF *p, int page, const char *blockname,
    const char *text, int len, const char *optlist);
/* }}} */


/* {{{ p_color.c: color handling
 * ----------------------------------------------------------------------
 */

/* Make a named spot color from the current color. */
PDFLIB_API int PDFLIB_CALL
PDF_makespotcolor(PDF *p, const char *spotname, int reserved);

/* Set the current color space and color. fstype is "fill", "stroke",
or "fillstroke".
 */
PDFLIB_API void PDFLIB_CALL
PDF_setcolor(PDF *p, const char *fstype, const char *colorspace,
    float c1, float c2, float c3, float c4);

/* The following six functions are deprecated, use PDF_setcolor() instead. */
PDFLIB_API void PDFLIB_CALL
PDF_setgray(PDF *p, float gray);

PDFLIB_API void PDFLIB_CALL
PDF_setgray_fill(PDF *p, float gray);

PDFLIB_API void PDFLIB_CALL
PDF_setgray_stroke(PDF *p, float gray);

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor(PDF *p, float red, float green, float blue);

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_fill(PDF *p, float red, float green, float blue);

PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_stroke(PDF *p, float red, float green, float blue);
/* }}} */


/* {{{ p_draw.c: path construction, painting, and clipping
 * ----------------------------------------------------------------------
 */

/* Draw a counterclockwise circular arc from alpha to beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_arc(PDF *p, float x, float y, float r, float alpha, float beta);

/* Draw a clockwise circular arc from alpha to beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_arcn(PDF *p, float x, float y, float r, float alpha, float beta);

/* Draw a circle with center (x, y) and radius r. */
PDFLIB_API void PDFLIB_CALL
PDF_circle(PDF *p, float x, float y, float r);

/* Use the current path as clipping path. */
PDFLIB_API void PDFLIB_CALL
PDF_clip(PDF *p);

/* Close the current path. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath(PDF *p);

/* Close the path, fill, and stroke it. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath_fill_stroke(PDF *p);

/* Close the path, and stroke it. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath_stroke(PDF *p);

/* Draw a Bezier curve from the current point, using 3 more control points. */
PDFLIB_API void PDFLIB_CALL
PDF_curveto(PDF *p,
    float x_1, float y_1, float x_2, float y_2, float x_3, float y_3);

/* End the current path without filling or stroking it. */
PDFLIB_API void PDFLIB_CALL
PDF_endpath(PDF *p);

/* Fill the interior of the path with the current fill color. */
PDFLIB_API void PDFLIB_CALL
PDF_fill(PDF *p);

/* Fill and stroke the path with the current fill and stroke color. */
PDFLIB_API void PDFLIB_CALL
PDF_fill_stroke(PDF *p);

/* Draw a line from the current point to (x, y). */
PDFLIB_API void PDFLIB_CALL
PDF_lineto(PDF *p, float x, float y);

/* Draw a line from the current point to (cp + (x, y)) (unsupported). */
PDFLIB_API void PDFLIB_CALL
PDF_rlineto(PDF *p, float x, float y);

/* Set the current point. */
PDFLIB_API void PDFLIB_CALL
PDF_moveto(PDF *p, float x, float y);

/* Draw a Bezier curve from the current point using relative coordinates
 (unsupported). */
PDFLIB_API void PDFLIB_CALL
PDF_rcurveto(PDF *p,
    float x_1, float y_1, float x_2, float y_2, float x_3, float y_3);

/* Draw a rectangle at lower left (x, y) with width and height. */
PDFLIB_API void PDFLIB_CALL
PDF_rect(PDF *p, float x, float y, float width, float height);

/* Set the new current point relative the old current point (unsupported). */
PDFLIB_API void PDFLIB_CALL
PDF_rmoveto(PDF *p, float x, float y);

/* Stroke the path with the current color and line width, and clear it. */
PDFLIB_API void PDFLIB_CALL
PDF_stroke(PDF *p);

/* }}} */


/* {{{ p_encoding.c: encoding handling
 * ----------------------------------------------------------------------
 */

/* Request a glyph name from a custom encoding (unsupported). */
PDFLIB_API const char * PDFLIB_CALL
PDF_encoding_get_glyphname(PDF *p, const char *encoding, int slot);

/* Request a glyph unicode value from a custom encoding (unsupported). */
PDFLIB_API int PDFLIB_CALL
PDF_encoding_get_unicode(PDF *p, const char *encoding, int slot);

/* Add a glyph name to a custom encoding. */
PDFLIB_API void PDFLIB_CALL
PDF_encoding_set_char(PDF *p, const char *encoding, int slot,
    const char *glyphname, int uv);
/* }}} */


/* {{{ p_font.c: text and font handling
 * ----------------------------------------------------------------------
 */

/* Search a font, and prepare it for later use. PDF_load_font() is
 * recommended. */
PDFLIB_API int PDFLIB_CALL
PDF_findfont(PDF *p, const char *fontname, const char *encoding, int embed);

/* Request a glyph ID value from a font (unsupported). */
PDFLIB_API int PDFLIB_CALL
PDF_get_glyphid(PDF *p, int font, int code);

/* Open and search a font, and prepare it for later use. */
PDFLIB_API int PDFLIB_CALL
PDF_load_font(PDF *p, const char *fontname, int len,
    const char *encoding, const char *optlist);

/* Set the current font in the given size, using a font handle returned by
 * PDF_load_font(). */
PDFLIB_API void PDFLIB_CALL
PDF_setfont(PDF *p, int font, float fontsize);
/* }}} */


/* {{{ p_gstate.c: graphics state
 * ----------------------------------------------------------------------
 */

/* Maximum length of dash arrays */
#define MAX_DASH_LENGTH	8

/* Concatenate a matrix to the current transformation matrix. */
PDFLIB_API void PDFLIB_CALL
PDF_concat(PDF *p, float a, float b, float c, float d, float e, float f);

/* Reset all color and graphics state parameters to their defaults. */
PDFLIB_API void PDFLIB_CALL
PDF_initgraphics(PDF *p);

/* Restore the most recently saved graphics state. */
PDFLIB_API void PDFLIB_CALL
PDF_restore(PDF *p);

/* Rotate the coordinate system by phi degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_rotate(PDF *p, float phi);

/* Save the current graphics state. */
PDFLIB_API void PDFLIB_CALL
PDF_save(PDF *p);

/* Scale the coordinate system. */
PDFLIB_API void PDFLIB_CALL
PDF_scale(PDF *p, float sx, float sy);

/* Set the current dash pattern to b black and w white units. */
PDFLIB_API void PDFLIB_CALL
PDF_setdash(PDF *p, float b, float w);

/* Set a more complicated dash pattern defined by an optlist. */
PDFLIB_API void PDFLIB_CALL
PDF_setdashpattern(PDF *p, const char *optlist);

/* Set the flatness to a value between 0 and 100 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setflat(PDF *p, float flatness);

/* Set the linecap parameter to a value between 0 and 2 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinecap(PDF *p, int linecap);

/* Set the linejoin parameter to a value between 0 and 2 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinejoin(PDF *p, int linejoin);

/* Set the current linewidth to width. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinewidth(PDF *p, float width);

/* Explicitly set the current transformation matrix. */
PDFLIB_API void PDFLIB_CALL
PDF_setmatrix(PDF *p, float a, float b, float c, float d, float e, float f);

/* Set the miter limit to a value greater than or equal to 1. */
PDFLIB_API void PDFLIB_CALL
PDF_setmiterlimit(PDF *p, float miter);

/* Deprecated, use PDF_setdashpattern() instead. */
PDFLIB_API void PDFLIB_CALL
PDF_setpolydash(PDF *p, float *dasharray, int length);

/* Skew the coordinate system in x and y direction by alpha and beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_skew(PDF *p, float alpha, float beta);

/* Translate the origin of the coordinate system. */
PDFLIB_API void PDFLIB_CALL
PDF_translate(PDF *p, float tx, float ty);
/* }}} */


/* {{{ p_hyper.c: bookmarks and document info fields
 * ----------------------------------------------------------------------
 */

/* Add a nested bookmark under parent, or a new top-level bookmark if
 * parent = 0. Returns a bookmark descriptor which may be
 * used as parent for subsequent nested bookmarks. If open = 1, child
 * bookmarks will be folded out, and invisible if open = 0. */
PDFLIB_API int PDFLIB_CALL
PDF_add_bookmark(PDF *p, const char *text, int parent, int open);

PDFLIB_API int PDFLIB_CALL
PDF_add_bookmark2(PDF *p, const char *text, int len, int parent, int open);

/* Create a named destination on an arbitrary page in the current document. */
PDFLIB_API void PDFLIB_CALL
PDF_add_nameddest(PDF *p, const char *name, int reserved, const char *optlist);

/* Fill document information field key with value. key is one of "Subject",
 * "Title", "Creator", "Author", "Keywords", or a user-defined key. */
PDFLIB_API void PDFLIB_CALL
PDF_set_info(PDF *p, const char *key, const char *value);

PDFLIB_API void PDFLIB_CALL
PDF_set_info2(PDF *p, const char *key, const char *value, int len);
/* }}} */


/* {{{ p_icc.c: ICC profile handling
 * ----------------------------------------------------------------------
 */

/* Search an ICC profile, and prepare it for later use. */
PDFLIB_API int PDFLIB_CALL
PDF_load_iccprofile(PDF *p, const char *profilename, int reserved,
    const char *optlist);
/* }}} */


/* {{{ p_image.c: image handling
 * ----------------------------------------------------------------------
 */

/* Add an existing image as thumbnail for the current page. */
PDFLIB_API void PDFLIB_CALL
PDF_add_thumbnail(PDF *p, int image);

/* Close an image retrieved with PDF_load_image(). */
PDFLIB_API void PDFLIB_CALL
PDF_close_image(PDF *p, int image);

/* Place an image or template at (x, y) with various options. */
PDFLIB_API void PDFLIB_CALL
PDF_fit_image(PDF *p, int image, float x, float y, const char *optlist);

/* Open a (disk-based or virtual) image file with various options. */
PDFLIB_API int PDFLIB_CALL
PDF_load_image(PDF *p, const char *imagetype, const char *filename,
    int reserved, const char *optlist);

/* Deprecated, use PDF_load_image() instead. */
PDFLIB_API int PDFLIB_CALL
PDF_open_CCITT(PDF *p, const char *filename, int width, int height,
    int BitReverse, int K, int BlackIs1);

/* Deprecated, use PDF_load_image() with virtual files instead. */
PDFLIB_API int PDFLIB_CALL
PDF_open_image(PDF *p, const char *imagetype, const char *source,
    const char *data, long length, int width, int height, int components,
    int bpc, const char *params);

/* Deprecated, use PDF_load_image() instead. */
PDFLIB_API int PDFLIB_CALL
PDF_open_image_file(PDF *p, const char *imagetype, const char *filename,
    const char *stringparam, int intparam);

/* Deprecated, use PDF_fit_image() instead. */
PDFLIB_API void PDFLIB_CALL
PDF_place_image(PDF *p, int image, float x, float y, float scale);
/* }}} */


/* {{{ p_kerning.c: font kerning
 * ----------------------------------------------------------------------
 */

/* Request the amount of kerning between two characters in a specified font.
   (unsupported) */
PDFLIB_API float PDFLIB_CALL
PDF_get_kern_amount(PDF *p, int font, int firstchar, int secondchar);
/* }}} */


/* {{{ p_params.c: parameter handling
 * ----------------------------------------------------------------------
 */

/* Get the contents of some PDFlib parameter with string type. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_parameter(PDF *p, const char *key, float modifier);

/* Get the value of some PDFlib parameter with float type. */
PDFLIB_API float PDFLIB_CALL
PDF_get_value(PDF *p, const char *key, float modifier);

/* Set some PDFlib parameter with string type. */
PDFLIB_API void PDFLIB_CALL
PDF_set_parameter(PDF *p, const char *key, const char *value);

/* Set the value of some PDFlib parameter with float type. */
PDFLIB_API void PDFLIB_CALL
PDF_set_value(PDF *p, const char *key, float value);
/* }}} */


/* {{{ p_pattern.c: pattern definition
 * ----------------------------------------------------------------------
 */

/* Start a new pattern definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_pattern(PDF *p,
    float width, float height, float xstep, float ystep, int painttype);

/* Finish a pattern definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_pattern(PDF *p);
/* }}} */


/* {{{ p_pdi.c: PDF import (requires the PDI library)
 * ----------------------------------------------------------------------
 */

/* Close all open page handles, and close the input PDF document. */
PDFLIB_API void PDFLIB_CALL
PDF_close_pdi(PDF *p, int doc);

/* Close the page handle, and free all page-related resources. */
PDFLIB_API void PDFLIB_CALL
PDF_close_pdi_page(PDF *p, int page);

/* Place an imported PDF page with the lower left corner at (x, y) with
 * various options. */
PDFLIB_API void PDFLIB_CALL
PDF_fit_pdi_page(PDF *p, int page, float x, float y, const char *optlist);

/* Get the contents of some PDI document parameter with string type. */
PDFLIB_API const char *PDFLIB_CALL
PDF_get_pdi_parameter(PDF *p, const char *key, int doc, int page,
    int reserved, int *len);

/* Get the contents of some PDI document parameter with numerical type. */
PDFLIB_API float PDFLIB_CALL
PDF_get_pdi_value(PDF *p, const char *key, int doc, int page, int reserved);

/* Open a (disk-based or virtual) PDF document and prepare it for later use. */
PDFLIB_API int PDFLIB_CALL
PDF_open_pdi(PDF *p, const char *filename, const char *optlist, int reserved);

/* Open an existing PDF document with callback functions for file access. */
PDFLIB_API int PDFLIB_CALL
PDF_open_pdi_callback(PDF *p, void *opaque, size_t filesize,
    size_t (*readproc)(void *opaque, void *buffer, size_t size),
    int (*seekproc)(void *opaque, long offset),
    const char *optlist);

/* Prepare a page for later use with PDF_place_pdi_page(). */
PDFLIB_API int PDFLIB_CALL
PDF_open_pdi_page(PDF *p, int doc, int pagenumber, const char *optlist);

/* Deprecated, use PDF_fit_pdi_page( ) instead. */
PDFLIB_API void PDFLIB_CALL
PDF_place_pdi_page(PDF *p, int page, float x, float y, float sx, float sy);

/* Perform various actions on a PDI document. */
PDFLIB_API int PDFLIB_CALL
PDF_process_pdi(PDF *p, int doc, int page, const char *optlist);
/* }}} */


/* {{{ p_resource.c: resources and virtual file system handling
 * ----------------------------------------------------------------------
 */

/* Create a new virtual file. */
PDFLIB_API void PDFLIB_CALL
PDF_create_pvf(PDF *p, const char *filename, int reserved,
               const void *data, size_t size, const char *optlist);

/* Delete a virtual file. */
PDFLIB_API int PDFLIB_CALL
PDF_delete_pvf(PDF *p, const char *filename, int reserved);
/* }}} */


/* {{{ p_shading.c: shadings
 * ----------------------------------------------------------------------
 */

/* Define a color blend (smooth shading) from the current fill color to the
 * supplied color. */
PDFLIB_API int PDFLIB_CALL
PDF_shading(PDF *p,
    const char *shtype,
    float x_0, float y_0,
    float x_1, float y_1,
    float c_1, float c_2, float c_3, float c_4,
    const char *optlist);

/* Define a shading pattern using a shading object. */
PDFLIB_API int PDFLIB_CALL
PDF_shading_pattern(PDF *p, int shading, const char *optlist);

/* Fill an area with a shading, based on a shading object. */
PDFLIB_API void PDFLIB_CALL
PDF_shfill(PDF *p, int shading);
/* }}} */


/* {{{ p_template.c: template definition
 * ----------------------------------------------------------------------
 */

/* Start a new template definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_template(PDF *p, float width, float height);

/* Finish a template definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_template(PDF *p);
/* }}} */


/* {{{ p_text.c: text output
 * ----------------------------------------------------------------------
 */

/* Function duplicates with explicit string length for use with
strings containing null characters. These are for C and C++ clients only,
but are used internally for the other language bindings. */

/* Print text at the next line. The spacing between lines is determined by
 * the "leading" parameter. */
PDFLIB_API void PDFLIB_CALL
PDF_continue_text(PDF *p, const char *text);

/* Same as PDF_continue_text but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_continue_text2(PDF *p, const char *text, int len);

/* Place a single text line at (x, y) with various options. */
PDFLIB_API void PDFLIB_CALL
PDF_fit_textline(PDF *p, const char *text, int len, float x, float y,
    const char *optlist);

/* This function is unsupported, and not considered part of the PDFlib API! */
PDFLIB_API void PDFLIB_CALL
PDF_set_text_matrix(PDF *p,
    float a, float b, float c, float d, float e, float f);

/* Set the text output position. */
PDFLIB_API void PDFLIB_CALL
PDF_set_text_pos(PDF *p, float x, float y);

/* Print text in the current font and size at the current position. */
PDFLIB_API void PDFLIB_CALL
PDF_show(PDF *p, const char *text);

/* Same as PDF_show() but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_show2(PDF *p, const char *text, int len);

/* Format text in the current font and size into the supplied text box
 * according to the requested formatting mode, which must be one of
 * "left", "right", "center", "justify", or "fulljustify". If width and height
 * are 0, only a single line is placed at the point (left, top) in the
 * requested mode. */
PDFLIB_API int PDFLIB_CALL
PDF_show_boxed(PDF *p, const char *text, float left, float top,
    float width, float height, const char *hmode, const char *feature);

/* Same as PDF_show_boxed() but with explicit string length.
   (unsupported) */
PDFLIB_API int PDFLIB_CALL
PDF_show_boxed2(PDF *p, const char *text, int len, float left, float top,
    float width, float height, const char *hmode, const char *feature);

/* Print text in the current font at (x, y). */
PDFLIB_API void PDFLIB_CALL
PDF_show_xy(PDF *p, const char *text, float x, float y);

/* Same as PDF_show_xy() but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_show_xy2(PDF *p, const char *text, int len, float x, float y);

/* Return the width of text in an arbitrary font. */
PDFLIB_API float PDFLIB_CALL
PDF_stringwidth(PDF *p, const char *text, int font, float fontsize);

/* Same as PDF_stringwidth but with explicit string length. */
PDFLIB_API float PDFLIB_CALL
PDF_stringwidth2(PDF *p, const char *text, int len, int font, float fontsize);
/* }}} */


/* {{{ p_type3.c: Type 3 (user-defined) fonts
 * ----------------------------------------------------------------------
 */

/* Start a type 3 font definition. */
PDFLIB_API void PDFLIB_CALL
PDF_begin_font(PDF *p, const char *fontname, int reserved,
    float a, float b, float c, float d, float e, float f, const char *optlist);

/* Start a type 3 glyph definition. */
PDFLIB_API void PDFLIB_CALL
PDF_begin_glyph(PDF *p, const char *glyphname, float wx,
    float llx, float lly, float urx, float ury);

/* Terminate a type 3 font definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_font(PDF *p);

/* Terminate a type 3 glyph definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_glyph(PDF *p);
/* }}} */


/* {{{ p_xgstate.c: explicit graphic states
 * ----------------------------------------------------------------------
 */

/* Create a gstate object definition. */
PDFLIB_API int PDFLIB_CALL
PDF_create_gstate(PDF *p, const char *optlist);

/* Activate a gstate object. */
PDFLIB_API void PDFLIB_CALL
PDF_set_gstate(PDF *p, int gstate);
/* }}} */


/*
 * ----------------------------------------------------------------------
 * PDFlib API structure with function pointers to all API functions
 * ----------------------------------------------------------------------
 */

/* The API structure with pointers to all PDFlib API functions */
struct PDFlib_api_s {
    /* {{{ p_annots.c: file attachments, notes, and links */
    void (PDFLIB_CALL * PDF_add_launchlink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, const char *filename);
    void (PDFLIB_CALL * PDF_add_locallink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, int page, const char *optlist);
    void (PDFLIB_CALL * PDF_add_note)(PDF *p, float llx, float lly,
    		float urx, float ury, const char *contents, const char *title,
		const char *icon, int open);
    void (PDFLIB_CALL * PDF_add_note2) (PDF *p, float llx, float lly,
		float urx, float ury, const char *contents, int len_cont,
		const char *title, int len_title, const char *icon, int open);
    void (PDFLIB_CALL * PDF_add_pdflink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, const char *filename, int page, const char *optlist);
    void (PDFLIB_CALL * PDF_add_weblink)(PDF *p,
    		float llx, float lly, float urx, float ury, const char *url);
    void (PDFLIB_CALL * PDF_attach_file)(PDF *p, float llx, float lly,
    		float urx, float ury, const char *filename,
		const char *description,
		const char *author, const char *mimetype, const char *icon);
    void (PDFLIB_CALL * PDF_attach_file2) (PDF *p, float llx, float lly,
                float urx, float ury, const char *filename, int reserved,
		const char *description, int len_descr, const char *author,
		int len_auth, const char *mimetype, const char *icon);
    void (PDFLIB_CALL * PDF_set_border_color)(PDF *p,
				    float red, float green, float blue);
    void (PDFLIB_CALL * PDF_set_border_dash)(PDF *p, float b, float w);

    void (PDFLIB_CALL * PDF_set_border_style)(PDF *p,
				    const char *style, float width);
    /* }}} */

    /* {{{ p_basic.c: general functions */
    void (PDFLIB_CALL * PDF_begin_page)(PDF *p, float width,
    				float height);
    void (PDFLIB_CALL * PDF_boot)(void);
    void (PDFLIB_CALL * PDF_close)(PDF *p);
    void (PDFLIB_CALL * PDF_delete)(PDF *);
    void (PDFLIB_CALL * PDF_end_page)(PDF *p);
    const PDFlib_api * (PDFLIB_CALL * PDF_get_api)(void);
    const char * (PDFLIB_CALL * PDF_get_apiname) (PDF *p);
    const char * (PDFLIB_CALL * PDF_get_buffer)(PDF *p, long *size);
    const char * (PDFLIB_CALL * PDF_get_errmsg) (PDF *p);
    int (PDFLIB_CALL * PDF_get_errnum) (PDF *p);
    int  (PDFLIB_CALL * PDF_get_majorversion)(void);
    int  (PDFLIB_CALL * PDF_get_minorversion)(void);
    void * (PDFLIB_CALL * PDF_get_opaque)(PDF *p);
    PDF* (PDFLIB_CALL * PDF_new)(void);
    PDF* (PDFLIB_CALL * PDF_new2)(errorproc_t errorhandler,
    				allocproc_t allocproc,
				reallocproc_t reallocproc,
				freeproc_t freeproc, void *opaque);
    int  (PDFLIB_CALL * PDF_open_file)(PDF *p, const char *filename);
    int  (PDFLIB_CALL * PDF_open_fp)(PDF *p, FILE *fp);
    void (PDFLIB_CALL * PDF_open_mem)(PDF *p, writeproc_t writeproc);
    void (PDFLIB_CALL * PDF_shutdown)(void);
    /* }}} */

    /* {{{ p_block.c: Variable Data Processing with blocks */
    int (PDFLIB_CALL * PDF_fill_imageblock) (PDF *p, int page,
		const char *blockname, int image, const char *optlist);
    int (PDFLIB_CALL * PDF_fill_pdfblock) (PDF *p, int page,
	    const char *blockname, int contents, const char *optlist);
    int (PDFLIB_CALL * PDF_fill_textblock) (PDF *p, int page,
	    const char *blockname, const char *text, int len,
	    const char *optlist);
    /* }}} */

    /* {{{ p_color.c: color handling */
    int  (PDFLIB_CALL * PDF_makespotcolor)(PDF *p, const char *spotname,
                        int reserved);
    void (PDFLIB_CALL * PDF_setcolor)(PDF *p,
			const char *fstype, const char *colorspace,
			float c1, float c2, float c3, float c4);
    void (PDFLIB_CALL * PDF_setgray)(PDF *p, float gray);
    void (PDFLIB_CALL * PDF_setgray_stroke)(PDF *p, float gray);
    void (PDFLIB_CALL * PDF_setgray_fill)(PDF *p, float gray);
    void (PDFLIB_CALL * PDF_setrgbcolor)(PDF *p, float red, float green,
    			float blue);
    void (PDFLIB_CALL * PDF_setrgbcolor_fill)(PDF *p,
			float red, float green, float blue);
    void (PDFLIB_CALL * PDF_setrgbcolor_stroke)(PDF *p,
			float red, float green, float blue);
    /* }}} */

    /* {{{ p_draw.c: path construction, painting, and clipping */
    void (PDFLIB_CALL * PDF_arc)(PDF *p, float x, float y,
				    float r, float alpha, float beta);
    void (PDFLIB_CALL * PDF_arcn)(PDF *p, float x, float y,
				    float r, float alpha, float beta);
    void (PDFLIB_CALL * PDF_circle)(PDF *p, float x, float y, float r);
    void (PDFLIB_CALL * PDF_clip)(PDF *p);
    void (PDFLIB_CALL * PDF_closepath)(PDF *p);
    void (PDFLIB_CALL * PDF_closepath_fill_stroke)(PDF *p);
    void (PDFLIB_CALL * PDF_closepath_stroke)(PDF *p);
    void (PDFLIB_CALL * PDF_curveto)(PDF *p, float x_1, float y_1,
				float x_2, float y_2, float x_3, float y_3);
    void (PDFLIB_CALL * PDF_endpath)(PDF *p);
    void (PDFLIB_CALL * PDF_fill)(PDF *p);
    void (PDFLIB_CALL * PDF_fill_stroke)(PDF *p);
    void (PDFLIB_CALL * PDF_lineto)(PDF *p, float x, float y);
    void (PDFLIB_CALL * PDF_moveto)(PDF *p, float x, float y);
    void (PDFLIB_CALL * PDF_rect)(PDF *p, float x, float y,
				    float width, float height);
    void (PDFLIB_CALL * PDF_stroke)(PDF *p);
    /* }}} */

    /*  {{{ p_encoding.c: encoding handling */
    void (PDFLIB_CALL * PDF_encoding_set_char) (PDF *p, const char *encoding,
			    int slot, const char *glyphname, int uv);
    /* }}} */

    /* {{{ p_font.c: text and font handling */
    int  (PDFLIB_CALL * PDF_findfont)(PDF *p, const char *fontname,
			    const char *encoding, int embed);
    int (PDFLIB_CALL * PDF_load_font)(PDF *p, const char *fontname,
		int len, const char *encoding, const char *optlist);
    void (PDFLIB_CALL * PDF_setfont)(PDF *p, int font, float fontsize);
    /* }}} */

    /* {{{ p_gstate.c graphics state */
    void (PDFLIB_CALL * PDF_concat)(PDF *p, float a, float b,
				    float c, float d, float e, float f);
    void (PDFLIB_CALL * PDF_initgraphics)(PDF *p);
    void (PDFLIB_CALL * PDF_restore)(PDF *p);
    void (PDFLIB_CALL * PDF_rotate)(PDF *p, float phi);
    void (PDFLIB_CALL * PDF_save)(PDF *p);
    void (PDFLIB_CALL * PDF_scale)(PDF *p, float sx, float sy);
    void (PDFLIB_CALL * PDF_setdash)(PDF *p, float b, float w);
    void (PDFLIB_CALL * PDF_setdashpattern) (PDF *p, const char *optlist);
    void (PDFLIB_CALL * PDF_setflat)(PDF *p, float flatness);
    void (PDFLIB_CALL * PDF_setlinecap)(PDF *p, int linecap);
    void (PDFLIB_CALL * PDF_setlinejoin)(PDF *p, int linejoin);
    void (PDFLIB_CALL * PDF_setlinewidth)(PDF *p, float width);
    void (PDFLIB_CALL * PDF_setmatrix)(PDF *p, float a, float b,
				    float c, float d, float e, float f);
    void (PDFLIB_CALL * PDF_setmiterlimit)(PDF *p, float miter);
    void (PDFLIB_CALL * PDF_setpolydash)(PDF *p, float *dasharray,
				    int length);
    void (PDFLIB_CALL * PDF_skew)(PDF *p, float alpha, float beta);
    void (PDFLIB_CALL * PDF_translate)(PDF *p, float tx, float ty);
    /* }}} */

    /* {{{ p_hyper.c: bookmarks and document info fields */
    int (PDFLIB_CALL * PDF_add_bookmark)(PDF *p, const char *text,
	    int parent, int open);
    int (PDFLIB_CALL * PDF_add_bookmark2) (PDF *p, const char *text, int len,
	    int parent, int open);
    void (PDFLIB_CALL * PDF_add_nameddest) (PDF *p, const char *name,
	    int reserved, const char *optlist);
    void (PDFLIB_CALL * PDF_set_info)(PDF *p, const char *key,
	    const char *value);
    void (PDFLIB_CALL * PDF_set_info2) (PDF *p, const char *key,
	    const char *value, int len);
    /* }}} */

    /* {{{ p_icc.c: ICC profile handling */
    int  (PDFLIB_CALL * PDF_load_iccprofile)(PDF *p, const char *profilename,
                        int reserved, const char *optlist);
    /* }}} */

    /* {{{ p_image.c: image handling */
    void (PDFLIB_CALL * PDF_add_thumbnail)(PDF *p, int image);
    void (PDFLIB_CALL * PDF_close_image)(PDF *p, int image);
    void (PDFLIB_CALL * PDF_fit_image) (PDF *p, int image, float x, float y,
	    const char *optlist);
    int (PDFLIB_CALL * PDF_load_image) (PDF *p, const char *imagetype,
	    const char *filename, int reserved, const char *optlist);
    int (PDFLIB_CALL * PDF_open_CCITT)(PDF *p, const char *filename,
    			int width, int height,
			int BitReverse, int K, int BlackIs1);
    int (PDFLIB_CALL * PDF_open_image)(PDF *p, const char *imagetype,
		const char *source, const char *data, long length, int width,
		int height, int components, int bpc, const char *params);
    int (PDFLIB_CALL * PDF_open_image_file)(PDF *p, const char *imagetype,
		const char *filename, const char *stringparam, int intparam);
    void (PDFLIB_CALL * PDF_place_image)(PDF *p, int image,
					float x, float y, float scale);
    /* }}} */

    /* {{{ p_kerning.c: font kerning */
    /* }}} */

    /* {{{ p_params.c: parameter handling */
    const char* (PDFLIB_CALL * PDF_get_parameter)(PDF *p,
				const char *key, float modifier);
    float (PDFLIB_CALL * PDF_get_value)(PDF *p, const char *key,
    				float modifier);
    void (PDFLIB_CALL * PDF_set_parameter)(PDF *p,
				const char *key, const char *value);
    void (PDFLIB_CALL * PDF_set_value)(PDF *p, const char *key,
    				float value);
    /* }}} */

    /* {{{ p_pattern.c: pattern definition */
    int  (PDFLIB_CALL * PDF_begin_pattern)(PDF *p,
    			float width, float height,
			float xstep, float ystep, int painttype);
    void (PDFLIB_CALL * PDF_end_pattern)(PDF *p);
    /* }}} */

    /* {{{ p_pdi.c: PDF import (requires the PDI library) */
    void (PDFLIB_CALL * PDF_close_pdi)(PDF *p, int doc);
    void (PDFLIB_CALL * PDF_close_pdi_page)(PDF *p, int page);
    void (PDFLIB_CALL * PDF_fit_pdi_page) (PDF *p, int page, float x,
	    float y, const char *optlist);
    const char * (PDFLIB_CALL * PDF_get_pdi_parameter)(PDF *p,
		    const char *key, int doc, int page, int reserved, int *len);
    float (PDFLIB_CALL * PDF_get_pdi_value)(PDF *p, const char *key,
					    int doc, int page, int reserved);
    int  (PDFLIB_CALL * PDF_open_pdi)(PDF *p, const char *filename,
				const char *optlist, int reserved);
    int (PDFLIB_CALL * PDF_open_pdi_callback) (PDF *p, void *opaque,
	    size_t filesize, size_t (*readproc)(void *opaque, void *buffer,
	    size_t size), int (*seekproc)(void *opaque, long offset),
	    const char *optlist);
    int  (PDFLIB_CALL * PDF_open_pdi_page)(PDF *p,
				int doc, int pagenumber, const char *optlist);
    void (PDFLIB_CALL * PDF_place_pdi_page)(PDF *p, int page,
				float x, float y, float sx, float sy);
    int (PDFLIB_CALL * PDF_process_pdi)(PDF *p,
				int doc, int page, const char *optlist);
    /* }}} */

    /* {{{ p_resource.c: resources and virtual file system handling */
    void (PDFLIB_CALL * PDF_create_pvf)(PDF *p, const char *filename,
                            int reserved, const void *data, size_t size,
                            const char *options);
    int (PDFLIB_CALL * PDF_delete_pvf)(PDF *p, const char *filename,
                            int reserved);
    /* }}} */

    /* {{{ p_shading.c: shadings */
    int (PDFLIB_CALL * PDF_shading) (PDF *p, const char *shtype, float x_0,
	    float y_0, float x_1, float y_1, float c_1, float c_2, float c_3,
	    float c_4, const char *optlist);
    int (PDFLIB_CALL * PDF_shading_pattern) (PDF *p, int shading,
	    const char *optlist);
    void (PDFLIB_CALL * PDF_shfill) (PDF *p, int shading);
    /* }}} */

    /* {{{ p_template.c: template definition */
    int  (PDFLIB_CALL * PDF_begin_template)(PDF *p,
    			float width, float height);
    void (PDFLIB_CALL * PDF_end_template)(PDF *p);
    /* }}} */

    /* {{{ p_text.c: text output */
    void (PDFLIB_CALL * PDF_continue_text)(PDF *p, const char *text);
    void (PDFLIB_CALL * PDF_continue_text2)(PDF *p, const char *text,
					    int len);
    void (PDFLIB_CALL * PDF_fit_textline)(PDF *p, const char *text,
                         int len, float x, float y, const char *optlist);
    void (PDFLIB_CALL * PDF_set_text_pos)(PDF *p, float x, float y);
    void (PDFLIB_CALL * PDF_show)(PDF *p, const char *text);
    void (PDFLIB_CALL * PDF_show2)(PDF *p, const char *text, int len);
    int  (PDFLIB_CALL * PDF_show_boxed)(PDF *p, const char *text,
                        float left, float top, float width, float height,
                        const char *hmode, const char *feature);
    void (PDFLIB_CALL * PDF_show_xy)(PDF *p, const char *text, float x,
    			float y);
    void (PDFLIB_CALL * PDF_show_xy2)(PDF *p, const char *text,
					    int len, float x, float y);
    float (PDFLIB_CALL * PDF_stringwidth)(PDF *p,
                                const char *text, int font, float fontsize);
    float (PDFLIB_CALL * PDF_stringwidth2)(PDF *p, const char *text,
                                            int len, int font, float fontsize);
    /* }}} */

    /* {{{ p_type3.c: Type 3 (user-defined) fonts */
    void (PDFLIB_CALL * PDF_begin_font)(PDF *p, const char *fontname,
        int reserved, float a, float b, float c, float d, float e, float f,
        const char *optlist);
    void (PDFLIB_CALL * PDF_begin_glyph)(PDF *p, const char *glyphname,
	float wx, float llx, float lly, float urx, float ury);
    void (PDFLIB_CALL * PDF_end_font)(PDF *p);
    void (PDFLIB_CALL * PDF_end_glyph)(PDF *p);
    /* }}} */

    /* {{{ p_xgstate.c: explicit graphic states */
    int (PDFLIB_CALL * PDF_create_gstate) (PDF *p, const char *optlist);
    void (PDFLIB_CALL * PDF_set_gstate) (PDF *p, int gstate);
    /* }}} */

    /* this is considered private */
    void *handle;
};

/*
 * ----------------------------------------------------------------------
 * page size formats
 * ----------------------------------------------------------------------
 */

/* The page sizes are only available to the C and C++ bindings */
#define a0_width	 (float) 2380.0
#define a0_height	 (float) 3368.0
#define a1_width	 (float) 1684.0
#define a1_height	 (float) 2380.0
#define a2_width	 (float) 1190.0
#define a2_height	 (float) 1684.0
#define a3_width	 (float) 842.0
#define a3_height	 (float) 1190.0
#define a4_width	 (float) 595.0
#define a4_height	 (float) 842.0
#define a5_width	 (float) 421.0
#define a5_height	 (float) 595.0
#define a6_width	 (float) 297.0
#define a6_height	 (float) 421.0
#define b5_width	 (float) 501.0
#define b5_height	 (float) 709.0
#define letter_width	 (float) 612.0
#define letter_height	 (float) 792.0
#define legal_width 	 (float) 612.0
#define legal_height 	 (float) 1008.0
#define ledger_width	 (float) 1224.0
#define ledger_height	 (float) 792.0
#define p11x17_width	 (float) 792.0
#define p11x17_height	 (float) 1224.0


/*
 * ----------------------------------------------------------------------
 * exception handling with try/catch implementation
 * ----------------------------------------------------------------------
 */

/* Set up an exception handling frame; must always be paired with PDF_CATCH().*/

#define PDF_TRY(p)		if (p) { if (setjmp(pdf_jbuf(p)->jbuf) == 0)

/* Inform the exception machinery that a PDF_TRY() will be left without
   entering the corresponding PDF_CATCH( ) clause. */
#define PDF_EXIT_TRY(p)		pdf_exit_try(p)

/* Catch an exception; must always be paired with PDF_TRY(). */
#define PDF_CATCH(p)		} if (pdf_catch(p))

/* Re-throw an exception to another handler. */
#define PDF_RETHROW(p)		pdf_rethrow(p)

/*
 * ----------------------------------------------------------------------
 * End of public declarations
 * ----------------------------------------------------------------------
 */


/*
 * ----------------------------------------------------------------------
 * Private stuff, do not use explicitly but only via the above macros!
 * ----------------------------------------------------------------------
 */

#include <setjmp.h>

typedef struct
{
    jmp_buf	jbuf;
} pdf_jmpbuf;

PDFLIB_API pdf_jmpbuf * PDFLIB_CALL
pdf_jbuf(PDF *p);

PDFLIB_API void PDFLIB_CALL
pdf_exit_try(PDF *p);

PDFLIB_API int PDFLIB_CALL
pdf_catch(PDF *p);

PDFLIB_API void PDFLIB_CALL
pdf_rethrow(PDF *p);

PDFLIB_API void PDFLIB_CALL
pdf_throw(PDF *p, const char *binding, const char *apiname, const char *errmsg);


/*
 * ----------------------------------------------------------------------
 * End of useful stuff
 * ----------------------------------------------------------------------
 */


#ifdef __cplusplus
}	/* extern "C" */
#endif

#if defined(__MWERKS__) && defined(PDFLIB_EXPORTS)
#pragma export off
#endif

#endif	/* PDFLIB_H */

/*
 * vim600: sw=4 fdm=marker
 */
