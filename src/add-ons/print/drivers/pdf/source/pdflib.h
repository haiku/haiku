/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2002 PDFlib GmbH and Thomas Merz. All rights reserved. |
 +---------------------------------------------------------------------------+
 |    This software is NOT in the public domain.  It can be used under two   |
 |    substantially different licensing terms:                               |
 |                                                                           |
 |    The commercial license is available for a fee, and allows you to       |
 |    - ship a commercial product based on PDFlib                            |
 |    - implement commercial Web services with PDFlib                        |
 |    - distribute (free or commercial) software when the source code is     |
 |      not made available                                                   |
 |    Details can be found in the file PDFlib-license.pdf.                   |
 |                                                                           |
 |    The "Aladdin Free Public License" doesn't require any license fee,     |
 |    and allows you to                                                      |
 |    - develop and distribute PDFlib-based software for which the complete  |
 |      source code is made available                                        |
 |    - redistribute PDFlib non-commercially under certain conditions        |
 |    - redistribute PDFlib on digital media for a fee if the complete       |
 |      contents of the media are freely redistributable                     |
 |    Details can be found in the file aladdin-license.pdf.                  |
 |                                                                           |
 |    These conditions extend to ports to other programming languages.       |
 |    PDFlib is distributed with no warranty of any kind. Commercial users,  |
 |    however, will receive warranty and support statements in writing.      |
 *---------------------------------------------------------------------------*/

/* $Id: pdflib.h,v 1.1 2002/07/09 12:24:37 ejakowatz Exp $
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

#define PDFLIB_CALL	__cdecl

#ifdef PDFLIB_EXPORTS
#define PDFLIB_API __declspec(dllexport) /* prepare a DLL (internal use only) */

#elif defined(PDFLIB_DLL)
#define PDFLIB_API __declspec(dllimport) /* PDFlib clients: import PDFlib DLL */

#else	/* !PDFLIB_DLL */
#define PDFLIB_API /* */	/* default: generate or use static library */

#endif	/* !PDFLIB_DLL */

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
typedef struct PDF_s PDF;

/* The API structure with function pointers. */
typedef struct PDFlib_api_s PDFlib_api;


/*
 * ----------------------------------------------------------------------
 * p_basic.c: general functions
 * ----------------------------------------------------------------------
 */

/*
 * The version defines below may be used to check the version of the
 * include file against the library.
 */

/* do not change this (version.sh will do it for you :) */
#define PDFLIB_MAJORVERSION	4	/* PDFlib major version number */
#define PDFLIB_MINORVERSION	0	/* PDFlib minor version number */
#define PDFLIB_REVISION		3	/* PDFlib revision number */
#define PDFLIB_VERSIONSTRING	"4.0.3"	/* The whole bunch */

/*
 * Allow for the external and internal float type to be easily redefined.
 * This is only for special applications which require improved accuracy,
 * and not supported in the general case due to platform and binary
 * compatibility issues.
*/
/* #define float double */

/* Retrieve a structure with PDFlib API function pointers (mainly for DLLs) */
PDFLIB_API PDFlib_api * PDFLIB_CALL
PDF_get_api(void);

/* Returns the PDFlib major version number. */
PDFLIB_API int PDFLIB_CALL
PDF_get_majorversion(void);

/* Returns the PDFlib minor version number. */
PDFLIB_API int PDFLIB_CALL
PDF_get_minorversion(void);

/* Boot PDFlib (recommended although currently not required). */
PDFLIB_API void PDFLIB_CALL
PDF_boot(void);

/* Shut down PDFlib (recommended although currently not required). */
PDFLIB_API void PDFLIB_CALL
PDF_shutdown(void);

/* Create a new PDF object with client-supplied error handling and memory
 allocation routines. */
typedef void  (*errorproc_t)(PDF *p1, int type, const char *msg);
typedef void* (*allocproc_t)(PDF *p2, size_t size, const char *caller);
typedef void* (*reallocproc_t)(PDF *p3,
		void *mem, size_t size, const char *caller);
typedef void  (*freeproc_t)(PDF *p4, void *mem);

PDFLIB_API PDF * PDFLIB_CALL
PDF_new2(errorproc_t errorhandler, allocproc_t allocproc,
	reallocproc_t reallocproc, freeproc_t freeproc, void *opaque);

/* Fetch opaque application pointer stored in PDFlib (for multi-threading). */
PDFLIB_API void * PDFLIB_CALL
PDF_get_opaque(PDF *p);

/* Create a new PDF object, using default error handling and memory
 management. */
PDFLIB_API PDF * PDFLIB_CALL
PDF_new(void);

/* Delete the PDF object, and free all internal resources. */
PDFLIB_API void PDFLIB_CALL
PDF_delete(PDF *p);

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

/* Close the generated PDF file, and release all document-related resources. */
PDFLIB_API void PDFLIB_CALL
PDF_close(PDF *p);

/* Add a new page to the document. */
PDFLIB_API void PDFLIB_CALL
PDF_begin_page(PDF *p, float width, float height);

/* Finish the page. */
PDFLIB_API void PDFLIB_CALL
PDF_end_page(PDF *p);

/* PDFlib exceptions which may be handled by a user-supplied error handler */
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


/*
 * ----------------------------------------------------------------------
 * p_params.c: parameter handling
 * ----------------------------------------------------------------------
 */

/* Set some PDFlib parameter with string type. */
PDFLIB_API void PDFLIB_CALL
PDF_set_parameter(PDF *p, const char *key, const char *value);

/* Set the value of some PDFlib parameter with float type. */
PDFLIB_API void PDFLIB_CALL
PDF_set_value(PDF *p, const char *key, float value);

/* Get the contents of some PDFlib parameter with string type. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_parameter(PDF *p, const char *key, float modifier);

/* Get the value of some PDFlib parameter with float type. */
PDFLIB_API float PDFLIB_CALL
PDF_get_value(PDF *p, const char *key, float modifier);


/*
 * ----------------------------------------------------------------------
 * p_font.c: text and font handling
 * ----------------------------------------------------------------------
 */

/* Search a font, and prepare it for later use.  The metrics will be
 loaded, and if embed is nonzero, the font file will be checked, but not
 yet used. Encoding is one of "builtin", "macroman", "winansi", "host", or
 a user-defined encoding name, or the name of a CMap. */
PDFLIB_API int PDFLIB_CALL
PDF_findfont(PDF *p, const char *fontname, const char *encoding, int embed);

/* Set the current font in the given size, using a font handle returned by
 PDF_findfont(). */
PDFLIB_API void PDFLIB_CALL
PDF_setfont(PDF *p, int font, float fontsize);

/* Request a glyph name from a custom encoding (unsupported). */
PDFLIB_API const char * PDFLIB_CALL
PDF_encoding_get_name(PDF *p, const char *encoding, int slot);

/*
 * ----------------------------------------------------------------------
 * p_text.c: text output
 * ----------------------------------------------------------------------
 */

/* Print text in the current font and size at the current position. */
PDFLIB_API void PDFLIB_CALL
PDF_show(PDF *p, const char *text);

/* Print text in the current font at (x, y). */
PDFLIB_API void PDFLIB_CALL
PDF_show_xy(PDF *p, const char *text, float x, float y);

/* Print text at the next line. The spacing between lines is determined by
 the "leading" parameter. */
PDFLIB_API void PDFLIB_CALL
PDF_continue_text(PDF *p, const char *text);

/* Format text in the current font and size into the supplied text box
 according to the requested formatting mode, which must be one of
 "left", "right", "center", "justify", or "fulljustify". If width and height
 are 0, only a single line is placed at the point (left, top) in the
 requested mode. */
PDFLIB_API int PDFLIB_CALL
PDF_show_boxed(PDF *p, const char *text, float left, float top,
    float width, float height, const char *hmode, const char *feature);

/* This function is unsupported, and not considered part of the PDFlib API! */
PDFLIB_API void PDFLIB_CALL
PDF_set_text_matrix(PDF *p,
    float a, float b, float c, float d, float e, float f);

/* Set the text output position. */
PDFLIB_API void PDFLIB_CALL
PDF_set_text_pos(PDF *p, float x, float y);

/* Return the width of text in an arbitrary font. */
PDFLIB_API float PDFLIB_CALL
PDF_stringwidth(PDF *p, const char *text, int font, float size);

/* Function duplicates with explicit string length for use with
strings containing null characters. These are for C and C++ clients only,
but are used internally for the other language bindings. */

/* Same as PDF_show() but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_show2(PDF *p, const char *text, int len);

/* Same as PDF_show_xy() but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_show_xy2(PDF *p, const char *text, int len, float x, float y);

/* Same as PDF_continue_text but with explicit string length. */
PDFLIB_API void PDFLIB_CALL
PDF_continue_text2(PDF *p, const char *text, int len);

/* Same as PDF_stringwidth but with explicit string length. */
PDFLIB_API float PDFLIB_CALL
PDF_stringwidth2(PDF *p, const char *text, int len, int font, float size);


/*
 * ----------------------------------------------------------------------
 * p_gstate.c: graphics state
 * ----------------------------------------------------------------------
 */

/* Maximum length of dash arrays */
#define MAX_DASH_LENGTH	8

/* Set the current dash pattern to b black and w white units. */
PDFLIB_API void PDFLIB_CALL
PDF_setdash(PDF *p, float b, float w);

/* Set a more complicated dash pattern defined by an array. */
PDFLIB_API void PDFLIB_CALL
PDF_setpolydash(PDF *p, float *dasharray, int length);

/* Set the flatness to a value between 0 and 100 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setflat(PDF *p, float flatness);

/* Set the line join parameter to a value between 0 and 2 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinejoin(PDF *p, int linejoin);

/* Set the linecap parameter to a value between 0 and 2 inclusive. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinecap(PDF *p, int linecap);

/* Set the miter limit to a value greater than or equal to 1. */
PDFLIB_API void PDFLIB_CALL
PDF_setmiterlimit(PDF *p, float miter);

/* Set the current linewidth to width. */
PDFLIB_API void PDFLIB_CALL
PDF_setlinewidth(PDF *p, float width);

/* Reset all color and graphics state parameters to their defaults. */
PDFLIB_API void PDFLIB_CALL
PDF_initgraphics(PDF *p);

/* Save the current graphics state. */
PDFLIB_API void PDFLIB_CALL
PDF_save(PDF *p);

/* Restore the most recently saved graphics state. */
PDFLIB_API void PDFLIB_CALL
PDF_restore(PDF *p);

/* Translate the origin of the coordinate system. */
PDFLIB_API void PDFLIB_CALL
PDF_translate(PDF *p, float tx, float ty);

/* Scale the coordinate system. */
PDFLIB_API void PDFLIB_CALL
PDF_scale(PDF *p, float sx, float sy);

/* Rotate the coordinate system by phi degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_rotate(PDF *p, float phi);

/* Skew the coordinate system in x and y direction by alpha and beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_skew(PDF *p, float alpha, float beta);

/* Concatenate a matrix to the current transformation matrix. */
PDFLIB_API void PDFLIB_CALL
PDF_concat(PDF *p, float a, float b, float c, float d, float e, float f);

/* Explicitly set the current transformation matrix. */
PDFLIB_API void PDFLIB_CALL
PDF_setmatrix(PDF *p, float a, float b, float c, float d, float e, float f);


/*
 * ----------------------------------------------------------------------
 * p_draw.c: path construction, painting, and clipping
 * ----------------------------------------------------------------------
 */

/* Set the current point. */
PDFLIB_API void PDFLIB_CALL
PDF_moveto(PDF *p, float x, float y);

/* Draw a line from the current point to (x, y). */
PDFLIB_API void PDFLIB_CALL
PDF_lineto(PDF *p, float x, float y);

/* Draw a Bezier curve from the current point, using 3 more control points. */
PDFLIB_API void PDFLIB_CALL
PDF_curveto(PDF *p, float x1, float y1, float x2, float y2, float x3, float y3);

/* Draw a circle with center (x, y) and radius r. */
PDFLIB_API void PDFLIB_CALL
PDF_circle(PDF *p, float x, float y, float r);

/* Draw a counterclockwise circular arc from alpha to beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_arc(PDF *p, float x, float y, float r, float alpha, float beta);

/* Draw a clockwise circular arc from alpha to beta degrees. */
PDFLIB_API void PDFLIB_CALL
PDF_arcn(PDF *p, float x, float y, float r, float alpha, float beta);

/* Draw a rectangle at lower left (x, y) with width and height. */
PDFLIB_API void PDFLIB_CALL
PDF_rect(PDF *p, float x, float y, float width, float height);

/* Close the current path. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath(PDF *p);

/* Stroke the path with the current color and line width, and clear it. */
PDFLIB_API void PDFLIB_CALL
PDF_stroke(PDF *p);

/* Close the path, and stroke it. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath_stroke(PDF *p);

/* Fill the interior of the path with the current fill color. */
PDFLIB_API void PDFLIB_CALL
PDF_fill(PDF *p);

/* Fill and stroke the path with the current fill and stroke color. */
PDFLIB_API void PDFLIB_CALL
PDF_fill_stroke(PDF *p);

/* Close the path, fill, and stroke it. */
PDFLIB_API void PDFLIB_CALL
PDF_closepath_fill_stroke(PDF *p);

/* End the current path without filling or stroking it. */
PDFLIB_API void PDFLIB_CALL
PDF_endpath(PDF *p);

/* Use the current path as clipping path. */
PDFLIB_API void PDFLIB_CALL
PDF_clip(PDF *p);


/*
 * ----------------------------------------------------------------------
 * p_color.c: color handling
 * ----------------------------------------------------------------------
 */

/* Deprecated, use PDF_setcolor(p, "fill", "gray", g, 0, 0, 0) instead. */
PDFLIB_API void PDFLIB_CALL
PDF_setgray_fill(PDF *p, float gray);

/* Deprecated, use PDF_setcolor(p, "stroke", "gray", g, 0, 0, 0) instead. */
PDFLIB_API void PDFLIB_CALL
PDF_setgray_stroke(PDF *p, float gray);

/* Deprecated, use PDF_setcolor(p, "both", "gray", g, 0, 0, 0) instead. */
PDFLIB_API void PDFLIB_CALL
PDF_setgray(PDF *p, float gray);

/* Deprecated, use PDF_setcolor(p, "fill", "rgb", red, green, blue, 0). */
PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_fill(PDF *p, float red, float green, float blue);

/* Deprecated, use PDF_setcolor(p, "stroke", "rgb", red, green, blue, 0). */
PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor_stroke(PDF *p, float red, float green, float blue);

/* Deprecated, use PDF_setcolor(p, "both", "rgb", red, green, blue, 0). */
PDFLIB_API void PDFLIB_CALL
PDF_setrgbcolor(PDF *p, float red, float green, float blue);

/* Make a named spot color from the current color. */
PDFLIB_API int PDFLIB_CALL
PDF_makespotcolor(PDF *p, const char *spotname, int len);

/* Set the current color space and color. type is "fill", "stroke", or "both".*/
PDFLIB_API void PDFLIB_CALL
PDF_setcolor(PDF *p, const char *fstype, const char *colorspace,
    float c1, float c2, float c3, float c4);


/*
 * ----------------------------------------------------------------------
 * p_pattern.c: pattern definition
 * ----------------------------------------------------------------------
 */

/* Start a new pattern definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_pattern(PDF *p,
    float width, float height, float xstep, float ystep, int painttype);

/* Finish a pattern definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_pattern(PDF *p);


/*
 * ----------------------------------------------------------------------
 * p_template.c: template definition
 * ----------------------------------------------------------------------
 */

/* Start a new template definition. */
PDFLIB_API int PDFLIB_CALL
PDF_begin_template(PDF *p, float width, float height);

/* Finish a template definition. */
PDFLIB_API void PDFLIB_CALL
PDF_end_template(PDF *p);


/*
 * ----------------------------------------------------------------------
 * p_image.c: image handling
 * ----------------------------------------------------------------------
 */

/* Place an image or template with the lower left corner at (x, y),
 and scale it. */
PDFLIB_API void PDFLIB_CALL
PDF_place_image(PDF *p, int image, float x, float y, float scale);

/* Use image data from a variety of data sources. Supported types are
 "jpeg", "ccitt", "raw". Supported sources are "memory", "fileref", "url".
 len is only used for type="raw", params is only used for type="ccitt". */
PDFLIB_API int PDFLIB_CALL
PDF_open_image(PDF *p, const char *imagetype, const char *source,
    const char *data, long length, int width, int height, int components,
    int bpc, const char *params);

/* Open an image file. Supported types are "jpeg", "tiff", "gif", and "png"
 stringparam is either "", "mask", "masked", or "page". intparam is either 0,
 the image number of the applied mask, or the page. */
PDFLIB_API int PDFLIB_CALL
PDF_open_image_file(PDF *p, const char *imagetype, const char *filename,
    const char *stringparam, int intparam);

/* Close an image retrieved with one of the PDF_open_image*() functions. */
PDFLIB_API void PDFLIB_CALL
PDF_close_image(PDF *p, int image);

/* Add an existing image as thumbnail for the current page. */
PDFLIB_API void PDFLIB_CALL
PDF_add_thumbnail(PDF *p, int image);


/*
 * ----------------------------------------------------------------------
 * p_ccitt.c: fax-compressed data processing
 * ----------------------------------------------------------------------
 */

/* Open a raw CCITT image. */
PDFLIB_API int PDFLIB_CALL
PDF_open_CCITT(PDF *p, const char *filename, int width, int height,
    int BitReverse, int K, int BlackIs1);


/*
 * ----------------------------------------------------------------------
 * p_hyper.c: bookmarks and document info fields
 * ----------------------------------------------------------------------
 */

/* Add a nested bookmark under parent, or a new top-level bookmark if
 parent = 0. Returns a bookmark descriptor which may be
 used as parent for subsequent nested bookmarks. If open = 1, child
 bookmarks will be folded out, and invisible if open = 0. */
PDFLIB_API int PDFLIB_CALL
PDF_add_bookmark(PDF *p, const char *text, int parent, int open);


/* Fill document information field key with value. key is one of "Subject",
 "Title", "Creator", "Author", "Keywords", or a user-defined key. */
PDFLIB_API void PDFLIB_CALL
PDF_set_info(PDF *p, const char *key, const char *value);


/*
 * ----------------------------------------------------------------------
 * p_annots.c: file attachments, notes, and links
 * ----------------------------------------------------------------------
 */

/* Add a file attachment annotation. icon is one of "graph", "paperclip",
 "pushpin", or "tag". */
PDFLIB_API void PDFLIB_CALL
PDF_attach_file(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename, const char *description, const char *author,
    const char *mimetype, const char *icon);

/* Add a note annotation. icon is one of of "comment", "insert", "note",
 "paragraph", "newparagraph", "key", or "help". */
PDFLIB_API void PDFLIB_CALL
PDF_add_note(PDF *p, float llx, float lly, float urx, float ury,
    const char *contents, const char *title, const char *icon, int open);

/* Add a file link annotation (to a PDF target). */
PDFLIB_API void PDFLIB_CALL
PDF_add_pdflink(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename, int page, const char *dest);

/* Add a launch annotation (to a target of arbitrary file type). */
PDFLIB_API void PDFLIB_CALL
PDF_add_launchlink(PDF *p, float llx, float lly, float urx, float ury,
    const char *filename);

/* Add a link annotation to a target within the current PDF file. */
PDFLIB_API void PDFLIB_CALL
PDF_add_locallink(PDF *p, float llx, float lly, float urx, float ury,
    int page, const char *dest);

/* Add a weblink annotation to a target URL on the Web. */
PDFLIB_API void PDFLIB_CALL
PDF_add_weblink(PDF *p, float llx, float lly, float urx, float ury,
    const char *url);

/* Set the border style for all kinds of annotations. style is "solid" or
 "dashed". */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_style(PDF *p, const char *style, float width);

/* Set the border color for all kinds of annotations. */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_color(PDF *p, float red, float green, float blue);

/* Set the border dash style for all kinds of annotations. See PDF_setdash(). */
PDFLIB_API void PDFLIB_CALL
PDF_set_border_dash(PDF *p, float b, float w);


/*
 * ----------------------------------------------------------------------
 * p_pdi.c: PDF import (requires the PDI library)
 * ----------------------------------------------------------------------
 */

/* Open an existing PDF document and prepare it for later use. */
PDFLIB_API int PDFLIB_CALL
PDF_open_pdi(PDF *p, const char *filename, const char *stringparam,
    int intparam);

/* Close all open page handles, and close the input PDF document. */
PDFLIB_API void PDFLIB_CALL
PDF_close_pdi(PDF *p, int doc);

/* Prepare a page for later use with PDF_place_pdi_page(). */
PDFLIB_API int PDFLIB_CALL
PDF_open_pdi_page(PDF *p, int doc, int page, const char *label);

/* Place a PDF page with the lower left corner at (x, y), and scale it. */
PDFLIB_API void PDFLIB_CALL
PDF_place_pdi_page(PDF *p, int page, float x, float y, float sx, float sy);

/* Close the page handle, and free all page-related resources. */
PDFLIB_API void PDFLIB_CALL
PDF_close_pdi_page(PDF *p, int page);

/* Get the contents of some PDI document parameter with string type. */
PDFLIB_API const char *PDFLIB_CALL
PDF_get_pdi_parameter(PDF *p, const char *key, int doc, int page,
    int index, int *len);

/* Get the contents of some PDI document parameter with numerical type. */
PDFLIB_API float PDFLIB_CALL
PDF_get_pdi_value(PDF *p, const char *key, int doc, int page, int index);


/*
 * ----------------------------------------------------------------------
 * p_stream.c: output stream handling
 * ----------------------------------------------------------------------
 */

/* Get the contents of the PDF output buffer. The result must be used by
 the client before calling any other PDFlib function. */
PDFLIB_API const char * PDFLIB_CALL
PDF_get_buffer(PDF *p, long *size);


/*
 * ----------------------------------------------------------------------
 * page size formats
 * ----------------------------------------------------------------------
 */

/*
Although PDF doesn´t impose any restrictions on the usable page size,
Acrobat implementations suffer from architectural limits concerning
the page size.
Although PDFlib will generate PDF documents with page sizes outside
these limits, the default error handler will issue a warning message.

Acrobat 3 minimum page size: 1" = 72 pt = 2.54 cm
Acrobat 3 maximum page size: 45" = 3240 pt = 114.3 cm
Acrobat 4 minimum page size: 0.25" = 18 pt = 0.635 cm
Acrobat 4 maximum page size: 200" = 14400 pt = 508 cm
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

/* The API structure with pointers to all PDFlib API functions */
struct PDFlib_api_s {
    /* general functions */
    PDFlib_api * (PDFLIB_CALL * const PDF_get_api)(void);
    int  (PDFLIB_CALL * const PDF_get_majorversion)(void);
    int  (PDFLIB_CALL * const PDF_get_minorversion)(void);
    void (PDFLIB_CALL * const PDF_boot)(void);
    void (PDFLIB_CALL * const PDF_shutdown)(void);
    PDF* (PDFLIB_CALL * const PDF_new2)(errorproc_t errorhandler,
    				allocproc_t allocproc,
				reallocproc_t reallocproc,
				freeproc_t freeproc, void *opaque);
    void * (PDFLIB_CALL * const PDF_get_opaque)(PDF *p);
    PDF* (PDFLIB_CALL * const PDF_new)(void);
    void (PDFLIB_CALL * const PDF_delete)(PDF *);
    int  (PDFLIB_CALL * const PDF_open_file)(PDF *p, const char *filename);
    int  (PDFLIB_CALL * const PDF_open_fp)(PDF *p, FILE *fp);
    void (PDFLIB_CALL * const PDF_open_mem)(PDF *p, writeproc_t writeproc);
    void (PDFLIB_CALL * const PDF_close)(PDF *p);
    void (PDFLIB_CALL * const PDF_begin_page)(PDF *p, float width,
    				float height);
    void (PDFLIB_CALL * const PDF_end_page)(PDF *p);

    /* parameter handling */
    void (PDFLIB_CALL * const PDF_set_parameter)(PDF *p,
				const char *key, const char *value);
    void (PDFLIB_CALL * const PDF_set_value)(PDF *p, const char *key,
    				float value);
    const char* (PDFLIB_CALL * const PDF_get_parameter)(PDF *p,
				const char *key, float modifier);
    float (PDFLIB_CALL * const PDF_get_value)(PDF *p, const char *key,
    				float modifier);

    /* text and font handling */
    int  (PDFLIB_CALL * const PDF_findfont)(PDF *p, const char *fontname,
			    const char *encoding, int embed);
    void (PDFLIB_CALL * const PDF_setfont)(PDF *p, int font, float fontsize);
    const char * (PDFLIB_CALL * const PDF_encoding_get_name)(PDF *p,
			    const char *encoding, int slot);

    /* text output */
    void (PDFLIB_CALL * const PDF_show)(PDF *p, const char *text);
    void (PDFLIB_CALL * const PDF_show_xy)(PDF *p, const char *text, float x,
    			float y);
    void (PDFLIB_CALL * const PDF_continue_text)(PDF *p, const char *text);
    int  (PDFLIB_CALL * const PDF_show_boxed)(PDF *p, const char *text,
			float left, float top, float width, float height,
			const char *hmode, const char *feature);
    void (PDFLIB_CALL * const PDF_set_text_matrix)(PDF *p, float a, float b,
					float c, float d, float e, float f);
    void (PDFLIB_CALL * const PDF_set_text_pos)(PDF *p, float x, float y);
    float (PDFLIB_CALL * const PDF_stringwidth)(PDF *p,
				const char *text, int font, float size);
    void (PDFLIB_CALL * const PDF_show2)(PDF *p, const char *text, int len);
    void (PDFLIB_CALL * const PDF_show_xy2)(PDF *p, const char *text,
					    int len, float x, float y);
    void (PDFLIB_CALL * const PDF_continue_text2)(PDF *p, const char *text,
					    int len);
    float (PDFLIB_CALL * const PDF_stringwidth2)(PDF *p, const char *text,
					    int len, int font, float size);
    /* graphics state */
    void (PDFLIB_CALL * const PDF_setdash)(PDF *p, float b, float w);
    void (PDFLIB_CALL * const PDF_setpolydash)(PDF *p, float *dasharray,
				    int length);
    void (PDFLIB_CALL * const PDF_setflat)(PDF *p, float flatness);
    void (PDFLIB_CALL * const PDF_setlinejoin)(PDF *p, int linejoin);
    void (PDFLIB_CALL * const PDF_setlinecap)(PDF *p, int linecap);
    void (PDFLIB_CALL * const PDF_setmiterlimit)(PDF *p, float miter);
    void (PDFLIB_CALL * const PDF_setlinewidth)(PDF *p, float width);
    void (PDFLIB_CALL * const PDF_initgraphics)(PDF *p);
    void (PDFLIB_CALL * const PDF_save)(PDF *p);
    void (PDFLIB_CALL * const PDF_restore)(PDF *p);
    void (PDFLIB_CALL * const PDF_translate)(PDF *p, float tx, float ty);
    void (PDFLIB_CALL * const PDF_scale)(PDF *p, float sx, float sy);
    void (PDFLIB_CALL * const PDF_rotate)(PDF *p, float phi);
    void (PDFLIB_CALL * const PDF_skew)(PDF *p, float alpha, float beta);
    void (PDFLIB_CALL * const PDF_concat)(PDF *p, float a, float b,
				    float c, float d, float e, float f);
    void (PDFLIB_CALL * const PDF_setmatrix)(PDF *p, float a, float b,
				    float c, float d, float e, float f);

    /* path construction, painting, and clipping */
    void (PDFLIB_CALL * const PDF_moveto)(PDF *p, float x, float y);
    void (PDFLIB_CALL * const PDF_lineto)(PDF *p, float x, float y);
    void (PDFLIB_CALL * const PDF_curveto)(PDF *p, float x1, float y1,
				float x2, float y2, float x3, float y3);
    void (PDFLIB_CALL * const PDF_circle)(PDF *p, float x, float y, float r);
    void (PDFLIB_CALL * const PDF_arc)(PDF *p, float x, float y,
				    float r, float alpha, float beta);
    void (PDFLIB_CALL * const PDF_arcn)(PDF *p, float x, float y,
				    float r, float alpha, float beta);
    void (PDFLIB_CALL * const PDF_rect)(PDF *p, float x, float y,
				    float width, float height);
    void (PDFLIB_CALL * const PDF_closepath)(PDF *p);
    void (PDFLIB_CALL * const PDF_stroke)(PDF *p);
    void (PDFLIB_CALL * const PDF_closepath_stroke)(PDF *p);
    void (PDFLIB_CALL * const PDF_fill)(PDF *p);
    void (PDFLIB_CALL * const PDF_fill_stroke)(PDF *p);
    void (PDFLIB_CALL * const PDF_closepath_fill_stroke)(PDF *p);
    void (PDFLIB_CALL * const PDF_endpath)(PDF *p);
    void (PDFLIB_CALL * const PDF_clip)(PDF *p);

    /* color handling */
    void (PDFLIB_CALL * const PDF_setgray_fill)(PDF *p, float gray);
    void (PDFLIB_CALL * const PDF_setgray_stroke)(PDF *p, float gray);
    void (PDFLIB_CALL * const PDF_setgray)(PDF *p, float gray);
    void (PDFLIB_CALL * const PDF_setrgbcolor_fill)(PDF *p,
			float red, float green, float blue);
    void (PDFLIB_CALL * const PDF_setrgbcolor_stroke)(PDF *p,
			float red, float green, float blue);
    void (PDFLIB_CALL * const PDF_setrgbcolor)(PDF *p, float red, float green,
    			float blue);
    int  (PDFLIB_CALL * const PDF_makespotcolor)(PDF *p, const char *spotname,
    			int len);
    void (PDFLIB_CALL * const PDF_setcolor)(PDF *p,
			const char *fstype, const char *colorspace,
			float c1, float c2, float c3, float c4);

    /* pattern definition */
    int  (PDFLIB_CALL * const PDF_begin_pattern)(PDF *p,
    			float width, float height,
			float xstep, float ystep, int painttype);
    void (PDFLIB_CALL * const PDF_end_pattern)(PDF *p);

    /* template definition */
    int  (PDFLIB_CALL * const PDF_begin_template)(PDF *p,
    			float width, float height);
    void (PDFLIB_CALL * const PDF_end_template)(PDF *p);

    /* image handling */
    void (PDFLIB_CALL * const PDF_place_image)(PDF *p, int image,
					float x, float y, float scale);
    int (PDFLIB_CALL * const PDF_open_image)(PDF *p, const char *imagetype,
		const char *source, const char *data, long length, int width,
		int height, int components, int bpc, const char *params);
    int (PDFLIB_CALL * const PDF_open_image_file)(PDF *p, const char *imagetype,
		const char *filename, const char *stringparam, int intparam);
    void (PDFLIB_CALL * const PDF_close_image)(PDF *p, int image);
    void (PDFLIB_CALL * const PDF_add_thumbnail)(PDF *p, int image);

    /* fax-compressed data processing */
    int (PDFLIB_CALL * const PDF_open_CCITT)(PDF *p, const char *filename,
    			int width, int height,
			int BitReverse, int K, int BlackIs1);

    /* bookmarks and document info fields */
    int (PDFLIB_CALL * const PDF_add_bookmark)(PDF *p,
			const char *text, int parent, int open);
    void (PDFLIB_CALL * const PDF_set_info)(PDF *p,
    			const char *key, const char *value);

    /* file attachments, notes, and links */
    void (PDFLIB_CALL * const PDF_attach_file)(PDF *p, float llx, float lly,
    		float urx, float ury, const char *filename,
		const char *description,
		const char *author, const char *mimetype, const char *icon);
    void (PDFLIB_CALL * const PDF_add_note)(PDF *p, float llx, float lly,
    		float urx, float ury, const char *contents, const char *title,
		const char *icon, int open);
    void (PDFLIB_CALL * const PDF_add_pdflink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, const char *filename, int page, const char *dest);
    void (PDFLIB_CALL * const PDF_add_launchlink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, const char *filename);
    void (PDFLIB_CALL * const PDF_add_locallink)(PDF *p,
    		float llx, float lly, float urx,
		float ury, int page, const char *dest);
    void (PDFLIB_CALL * const PDF_add_weblink)(PDF *p,
    		float llx, float lly, float urx, float ury, const char *url);
    void (PDFLIB_CALL * const PDF_set_border_style)(PDF *p,
				    const char *style, float width);
    void (PDFLIB_CALL * const PDF_set_border_color)(PDF *p,
				    float red, float green, float blue);
    void (PDFLIB_CALL * const PDF_set_border_dash)(PDF *p, float b, float w);

    /* PDF import (requires the PDI library) */
    int  (PDFLIB_CALL * const PDF_open_pdi)(PDF *p, const char *filename,
				const char *stringparam, int intparam);
    void (PDFLIB_CALL * const PDF_close_pdi)(PDF *p, int doc);
    int  (PDFLIB_CALL * const PDF_open_pdi_page)(PDF *p,
				int doc, int page, const char *label);
    void (PDFLIB_CALL * const PDF_place_pdi_page)(PDF *p, int page,
				float x, float y, float sx, float sy);
    void (PDFLIB_CALL * const PDF_close_pdi_page)(PDF *p, int page);
    const char * (PDFLIB_CALL * const PDF_get_pdi_parameter)(PDF *p,
		    const char *key, int doc, int page, int index, int *len);
    float (PDFLIB_CALL * const PDF_get_pdi_value)(PDF *p, const char *key,
					    int doc, int page, int index);

    /* output stream handling */
    const char * (PDFLIB_CALL * const PDF_get_buffer)(PDF *p, long *size);

    /* this is considered private */
    void *handle;
};

#ifdef __cplusplus
}	/* extern "C" */
#endif

#if defined(__MWERKS__) && defined(PDFLIB_EXPORTS)
#pragma export off
#endif

#endif	/* PDFLIB_H */
