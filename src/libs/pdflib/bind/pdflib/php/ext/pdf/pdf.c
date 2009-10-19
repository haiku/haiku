/*
   +----------------------------------------------------------------------+
   | PHP version 4                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2003 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.02 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_02.txt.                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Uwe Steinmann <Uwe.Steinmann@fernuni-hagen.de>              |
   |          Rainer Schaaf <rjs@pdflib.com>                              |
   +----------------------------------------------------------------------+
*/

/* $Id: pdf.c 14574 2005-10-29 16:27:43Z bonefish $ */

/* Bootstrap of PDFlib Feature setup */
#define PDF_FEATURE_SERIAL


/* derived from:
    Id: pdf.c,v 1.112.2.5 2003/04/30 21:54:02 iliaa Exp

    with some exeptions:
    - pdf_get_major/minorversion not included, as pdf_get_value supports this
      without a PDF-object
    - #if ZEND_MODULE_API_NO >= 20010901 for new ZEND_MODULE support,
      so that it compiles with older PHP Versions too
    - TSRMLS fixes included only with ZEND_MODULE_API_NO >= 20010901
      would break older builds otherwise
	- new PHP streams #if ZEND_MODULE_API_NO >= 20020429 && HAVE_PHP_STREAM
	  TODO: CVS (1.107/108/109/112)
	- added better support to see if it comes as binary from PDFlib GmbH.
	- change from emalloc to safe_emalloc (112.2.4 -> 112.2.5) not included
	  (would not be backwardcompatible with older php builds)
	- TODO: merge back to PHP-CVS
   */

/* pdflib 2.02 ... 4.0x is subject to the ALADDIN FREE PUBLIC LICENSE.
   Copyright (C) 1997-1999 Thomas Merz. 2000-2002 PDFlib GmbH */

/* Note that there is no code from the pdflib package in this file */

/* {{{ includes
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "php_globals.h"
#include "zend_list.h"
#include "ext/standard/head.h"
#include "ext/standard/info.h"
#include "ext/standard/file.h"

#if HAVE_LIBGD13
#include "ext/gd/php_gd.h"
#if HAVE_GD_BUNDLED 
#include "ext/gd/libgd/gd.h" 
#else 
#include "gd.h"
#endif
static int le_gd;
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef PHP_WIN32
# include <io.h>
# include <fcntl.h>
#endif
/* }}} */

#if HAVE_PDFLIB

#include "php_pdf.h"

#if ((100*PDFLIB_MAJORVERSION+10*PDFLIB_MINORVERSION+PDFLIB_REVISION) >= 500)
	/* This wrapper code will work only with PDFlib V5 or greater,
	 * because the special handling for returning 0 instead of -1
	 * for PHP is now done in the PDFlib kernel
	 */
#else

#error "PDFlib version does not match PHP-wrapper code"

#endif /* PDFlib >= V5 */

#undef VIRTUAL_DIR

static int le_pdf;

/* {{{ pdf_functions[]
 */
function_entry pdf_functions[] = {
	/* p_font.c */
	PHP_FE(pdf_add_launchlink, NULL)
	PHP_FE(pdf_add_locallink, NULL)
	PHP_FE(pdf_add_note, NULL)
	PHP_FE(pdf_add_pdflink, NULL)
	PHP_FE(pdf_add_weblink, NULL)
	PHP_FE(pdf_attach_file, NULL)
	PHP_FE(pdf_set_border_color, NULL)
	PHP_FE(pdf_set_border_dash, NULL)
	PHP_FE(pdf_set_border_style, NULL)

	/* p_basic.c */
	PHP_FE(pdf_begin_page, NULL)
	PHP_FE(pdf_close, NULL)
	PHP_FE(pdf_delete, NULL)
	PHP_FE(pdf_end_page, NULL)
	PHP_FE(pdf_get_buffer, NULL)
	PHP_FE(pdf_new, NULL)
	PHP_FE(pdf_open_file, NULL)

	/* p_block.c */
	PHP_FE(pdf_fill_imageblock, NULL)
	PHP_FE(pdf_fill_pdfblock, NULL)
	PHP_FE(pdf_fill_textblock, NULL)

	/* p_color.c */
	PHP_FE(pdf_makespotcolor, NULL)
	PHP_FE(pdf_setcolor, NULL)
	PHP_FE(pdf_setgray_fill, NULL)		/* deprecated (since 4.0) */
	PHP_FE(pdf_setgray_stroke, NULL)	/* deprecated (since 4.0) */
	PHP_FE(pdf_setgray, NULL)			/* deprecated (since 4.0) */
	PHP_FE(pdf_setrgbcolor_fill, NULL)	/* deprecated (since 4.0) */
	PHP_FE(pdf_setrgbcolor_stroke, NULL)/* deprecated (since 4.0) */
	PHP_FE(pdf_setrgbcolor, NULL)		/* deprecated (since 4.0) */

	/* p_draw.c */
	PHP_FE(pdf_arc, NULL)
	PHP_FE(pdf_arcn, NULL)
	PHP_FE(pdf_circle, NULL)
	PHP_FE(pdf_clip, NULL)
	PHP_FE(pdf_closepath, NULL)
	PHP_FE(pdf_closepath_fill_stroke, NULL)
	PHP_FE(pdf_closepath_stroke, NULL)
	PHP_FE(pdf_curveto, NULL)
	PHP_FE(pdf_endpath, NULL)
	PHP_FE(pdf_fill, NULL)
	PHP_FE(pdf_fill_stroke, NULL)
	PHP_FE(pdf_lineto, NULL)
	PHP_FE(pdf_moveto, NULL)
	PHP_FE(pdf_rect, NULL)
	PHP_FE(pdf_stroke, NULL)

	/* p_encoding.c */
	PHP_FE(pdf_encoding_set_char, NULL)

	/* p_font.c */
	PHP_FE(pdf_findfont, NULL)
	PHP_FE(pdf_load_font, NULL)
	PHP_FE(pdf_setfont, NULL)

	/* p_gstate.c */
	PHP_FE(pdf_concat, NULL)
	PHP_FE(pdf_initgraphics, NULL)
	PHP_FE(pdf_restore, NULL)
	PHP_FE(pdf_rotate, NULL)
	PHP_FE(pdf_save, NULL)
	PHP_FE(pdf_scale, NULL)
	PHP_FE(pdf_setdash, NULL)
	PHP_FE(pdf_setdashpattern, NULL)
	PHP_FE(pdf_setflat, NULL)
	PHP_FE(pdf_setlinecap, NULL)
	PHP_FE(pdf_setlinejoin, NULL)
	PHP_FE(pdf_setlinewidth, NULL)
	PHP_FE(pdf_setmatrix, NULL)
	PHP_FE(pdf_setmiterlimit, NULL)
	PHP_FE(pdf_setpolydash, NULL)		/* deprecated since V5.0 */
	PHP_FE(pdf_skew, NULL)
	PHP_FE(pdf_translate, NULL)

	/* p_hyper.c */
	PHP_FE(pdf_add_bookmark, NULL)
	PHP_FE(pdf_add_nameddest, NULL)
	PHP_FE(pdf_set_info, NULL)

	/* p_icc.c */
	PHP_FE(pdf_load_iccprofile, NULL)

	/* p_image.c */
	PHP_FE(pdf_add_thumbnail, NULL)
	PHP_FE(pdf_close_image, NULL)
	PHP_FE(pdf_fit_image, NULL)
	PHP_FE(pdf_load_image, NULL)
	PHP_FE(pdf_open_ccitt, NULL)		/* deprecated since V5.0 */
	PHP_FE(pdf_open_image, NULL)		/* deprecated since V5.0 */
	PHP_FE(pdf_open_image_file, NULL)	/* deprecated since V5.0 */
	PHP_FE(pdf_place_image, NULL)		/* deprecated since V5.0 */

	/* p_params.c */
	PHP_FE(pdf_get_parameter, NULL)
	PHP_FE(pdf_get_value, NULL)
	PHP_FE(pdf_set_parameter, NULL)
	PHP_FE(pdf_set_value, NULL)

	/* p_pattern.c */
	PHP_FE(pdf_begin_pattern, NULL)
	PHP_FE(pdf_end_pattern, NULL)

	/* p_pdi.c */
	PHP_FE(pdf_close_pdi, NULL)
	PHP_FE(pdf_close_pdi_page, NULL)
	PHP_FE(pdf_fit_pdi_page, NULL)
	PHP_FE(pdf_get_pdi_parameter, NULL)
	PHP_FE(pdf_get_pdi_value, NULL)
	PHP_FE(pdf_open_pdi, NULL)
	PHP_FE(pdf_open_pdi_page, NULL)
	PHP_FE(pdf_place_pdi_page, NULL)	/* deprecated since V5.0 */
	PHP_FE(pdf_process_pdi, NULL)

	/* p_resource.c */
	PHP_FE(pdf_create_pvf, NULL)
	PHP_FE(pdf_delete_pvf, NULL)

	/* p_shading.c */
	PHP_FE(pdf_shading, NULL)
	PHP_FE(pdf_shading_pattern, NULL)
	PHP_FE(pdf_shfill, NULL)

	/* p_template.c */
	PHP_FE(pdf_begin_template, NULL)
	PHP_FE(pdf_end_template, NULL)

	/* p_text.c */
	PHP_FE(pdf_continue_text, NULL)
	PHP_FE(pdf_fit_textline, NULL)
	PHP_FE(pdf_set_text_pos, NULL)
	PHP_FE(pdf_show, NULL)
	PHP_FE(pdf_show_boxed, NULL)
	PHP_FE(pdf_show_xy, NULL)
	PHP_FE(pdf_stringwidth, NULL)

	/* p_type3.c */
	PHP_FE(pdf_begin_font, NULL)
	PHP_FE(pdf_begin_glyph, NULL)
	PHP_FE(pdf_end_font, NULL)
	PHP_FE(pdf_end_glyph, NULL)

	/* p_xgstate.c */
	PHP_FE(pdf_create_gstate, NULL)
	PHP_FE(pdf_set_gstate, NULL)

	/* exception handling */
	PHP_FE(pdf_get_errnum, NULL)
	PHP_FE(pdf_get_errmsg, NULL)
	PHP_FE(pdf_get_apiname, NULL)

	/* End of the official PDFLIB V3.x/V4.x/V5.x API */

#if HAVE_LIBGD13
	/* not supported by PDFlib GmbH */
	PHP_FE(pdf_open_memory_image, NULL)
#endif

	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ pdf_module_entry
 */
zend_module_entry pdf_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
	"pdf", 
	pdf_functions, 
	PHP_MINIT(pdf), 
	PHP_MSHUTDOWN(pdf), 
	NULL, 
	NULL, 
	PHP_MINFO(pdf), 
#if ZEND_MODULE_API_NO >= 20010901
    NO_VERSION_YET,
#endif
	STANDARD_MODULE_PROPERTIES 
};
/* }}} */

#ifdef COMPILE_DL_PDF
ZEND_GET_MODULE(pdf)
#endif

/* PHP/PDFlib internal functions */
/* {{{ _free_pdf_doc
 */
static void _free_pdf_doc(zend_rsrc_list_entry *rsrc)
{
	PDF *pdf = (PDF *)rsrc->ptr;
	PDF_delete(pdf);
}
/* }}} */

/* {{{ custom_errorhandler
 */
static void custom_errorhandler(PDF *p, int errnum, const char *shortmsg)
{
    if (errnum == PDF_NonfatalError)
    {
		/*
		 * PDFlib warnings should be visible to the user.
		 * If he decides to live with PDFlib warnings
		 * he may use the PDFlib function
		 * pdf_set_parameter($p, "warning" 0) to switch off
		 * the warnings inside PDFlib.
		 */
		php_error(E_WARNING, "PDFlib warning %s", shortmsg);
    }
    else
    {
        /* give up in all other cases */
		php_error(E_ERROR, "PDFlib error %s", shortmsg);
    }
}
/* }}} */

/* {{{ pdf_emalloc
 */
static void *pdf_emalloc(PDF *p, size_t size, const char *caller)
{
	return(emalloc(size));
}
/* }}} */

/* {{{ pdf_realloc
 */
static void *pdf_realloc(PDF *p, void *mem, size_t size, const char *caller)
{
	return(erealloc(mem, size));
}
/* }}} */

/* {{{ pdf_efree
 */
static void pdf_efree(PDF *p, void *mem)
{
	efree(mem);
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(pdf)
{
	char tmp[32];

	snprintf(tmp, 31, "%d.%02d", PDF_get_majorversion(), PDF_get_minorversion() );
	tmp[31]=0;

	php_info_print_table_start();
	php_info_print_table_row(2, "PDF Support", "enabled" );
	php_info_print_table_row(2, "PDFlib GmbH Version", PDFLIB_VERSIONSTRING );
	php_info_print_table_row(2, "Revision", "$Revision: 1.1 $" );
	php_info_print_table_end();

}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(pdf)
{
	if ((PDF_get_majorversion() != PDFLIB_MAJORVERSION) ||
			(PDF_get_minorversion() != PDFLIB_MINORVERSION)) {
		php_error(E_ERROR,"PDFlib error: Version mismatch in wrapper code");
	}
	le_pdf = zend_register_list_destructors_ex(_free_pdf_doc, NULL, "pdf object", module_number);

	/* this does something like setlocale("C", ...) in PDFlib 3.x */
	PDF_boot();
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(pdf)
{
	PDF_shutdown();
	return SUCCESS;
}
/* }}} */


/* p_annots.c */

/* {{{ proto void pdf_add_launchlink(int pdfdoc, float llx, float lly, float urx, float ury, string filename)
 * Add a launch annotation (to a target of arbitrary file type). */
PHP_FUNCTION(pdf_add_launchlink)
{
	zval **p, **llx, **lly, **urx, **ury, **filename;
	PDF *pdf;
	const char * vfilename;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &p, &llx, &lly, &urx, &ury, &filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_double_ex(llx);
	convert_to_double_ex(lly);
	convert_to_double_ex(urx);
	convert_to_double_ex(ury);
	convert_to_string_ex(filename);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
	vfilename = Z_STRVAL_PP(filename);
#endif  

	PDF_add_launchlink(pdf,
		(float) Z_DVAL_PP(llx),
		(float) Z_DVAL_PP(lly),
		(float) Z_DVAL_PP(urx),
		(float) Z_DVAL_PP(ury),
		vfilename);

	RETURN_TRUE;
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_add_locallink(int pdfdoc, float llx, float lly, float urx, float ury, int page, string optlist)
 * Add a link annotation to a target within the current PDF file. */
PHP_FUNCTION(pdf_add_locallink)
{
	zval **p, **llx, **lly, **urx, **ury, **page, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &p, &llx, &lly, &urx, &ury, &page, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_double_ex(llx);
	convert_to_double_ex(lly);
	convert_to_double_ex(urx);
	convert_to_double_ex(ury);
	convert_to_long_ex(page);
	convert_to_string_ex(optlist);

	PDF_add_locallink(pdf,
		(float) Z_DVAL_PP(llx),
		(float) Z_DVAL_PP(lly),
		(float) Z_DVAL_PP(urx),
		(float) Z_DVAL_PP(ury),
		Z_LVAL_PP(page),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_add_note(int pdfdoc, float llx, float lly, float urx, float ury, string contents, string title, string icon, int open)
 * Add a note annotation. icon is one of of "comment", "insert", "note", "paragraph", "newparagraph", "key", or "help". */
PHP_FUNCTION(pdf_add_note)
{
	zval **p, **llx, **lly, **urx, **ury, **contents, **title, **icon, **open;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 9 || zend_get_parameters_ex(9, &p, &llx, &lly, &urx, &ury, &contents, &title, &icon, &open) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_double_ex(llx);
	convert_to_double_ex(lly);
	convert_to_double_ex(urx);
	convert_to_double_ex(ury);
	convert_to_string_ex(contents);
	convert_to_string_ex(title);
	convert_to_string_ex(icon);
	convert_to_long_ex(open);

	PDF_add_note2(pdf,
		 (float) Z_DVAL_PP(llx),
		 (float) Z_DVAL_PP(lly),
		 (float) Z_DVAL_PP(urx),
		 (float) Z_DVAL_PP(ury),
		 Z_STRVAL_PP(contents),
		 Z_STRLEN_PP(contents),
		 Z_STRVAL_PP(title),
		 Z_STRLEN_PP(title),
		 Z_STRVAL_PP(icon),
		 Z_LVAL_PP(open));

	RETURN_TRUE;
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_add_pdflink(int pdfdoc, float llx, float lly, float urx, float ury, string filename, int page, string optlist)
 * Add a file link annotation (to a PDF target). */
PHP_FUNCTION(pdf_add_pdflink)
{
	zval **p, **llx, **lly, **urx, **ury, **filename, **page, **optlist;
	PDF *pdf;
	const char * vfilename;

	if (ZEND_NUM_ARGS() != 8 || zend_get_parameters_ex(8, &p, &llx, &lly, &urx, &ury, &filename, &page, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_double_ex(llx);
	convert_to_double_ex(lly);
	convert_to_double_ex(urx);
	convert_to_double_ex(ury);
	convert_to_string_ex(filename);
	convert_to_long_ex(page);
	convert_to_string_ex(optlist);
#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
	vfilename = Z_STRVAL_PP(filename);
#endif  

	PDF_add_pdflink(pdf, (float) Z_DVAL_PP(llx), 
						 (float) Z_DVAL_PP(lly), 
						 (float) Z_DVAL_PP(urx), 
						 (float) Z_DVAL_PP(ury),
						 vfilename, 
						 Z_LVAL_PP(page),
						 Z_STRVAL_PP(optlist));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_add_weblink(int pdfdoc, float llx, float lly, float urx, float ury, string url)
 * Add a weblink annotation to a target URL on the Web. */
PHP_FUNCTION(pdf_add_weblink)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_string_ex(arg6);
	PDF_add_weblink(pdf, (float) Z_DVAL_PP(arg2), 
						 (float) Z_DVAL_PP(arg3), 
						 (float) Z_DVAL_PP(arg4), 
						 (float) Z_DVAL_PP(arg5), 
						 Z_STRVAL_PP(arg6));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_attach_file(int pdfdoc, float lly, float lly, float urx, float ury, string filename, string description, string author, string mimetype, string icon)
 * Add a file attachment annotation. icon is one of "graph", "paperclip", "pushpin", or "tag". */
PHP_FUNCTION(pdf_attach_file)
{
	zval **p, **llx, **lly, **urx, **ury, **filename, **description, **author, **mimetype, **icon;
	PDF *pdf;
	const char * vfilename;

	if (ZEND_NUM_ARGS() != 10 || zend_get_parameters_ex(10, &p, &llx, &lly, &urx, &ury, &filename, &description, &author, &mimetype, &icon) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_double_ex(llx);
	convert_to_double_ex(lly);
	convert_to_double_ex(urx);
	convert_to_double_ex(ury);
	convert_to_string_ex(filename);
	convert_to_string_ex(description);
	convert_to_string_ex(author);
	convert_to_string_ex(mimetype);
	convert_to_string_ex(icon);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
	vfilename = Z_STRVAL_PP(filename);
#endif  


	PDF_attach_file2(pdf,
		(float) Z_DVAL_PP(llx),
		(float) Z_DVAL_PP(lly),
		(float) Z_DVAL_PP(urx),
		(float) Z_DVAL_PP(ury),
		vfilename,
		0,
		Z_STRVAL_PP(description),
		Z_STRLEN_PP(description),
		Z_STRVAL_PP(author),
		Z_STRLEN_PP(author),
		Z_STRVAL_PP(mimetype),
		Z_STRVAL_PP(icon));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_set_border_color(int pdfdoc, float red, float green, float blue)
 * Set the border color for all kinds of annotations. */
PHP_FUNCTION(pdf_set_border_color)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	PDF_set_border_color(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_set_border_dash(int pdfdoc, float b, float w)
 * Set the border dash style for all kinds of annotations. See PDF_setdash(). */
PHP_FUNCTION(pdf_set_border_dash)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_set_border_dash(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_set_border_style(int pdfdoc, string style, float width)
 * Set the border style for all kinds of annotations. style is "solid" or "dashed". */
PHP_FUNCTION(pdf_set_border_style)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_set_border_style(pdf, Z_STRVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* p_basic.c */

/* {{{ proto void pdf_begin_page(int pdfdoc, float width, float height)
 * Add a new page to the document. */
PHP_FUNCTION(pdf_begin_page) 
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_begin_page(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_close(int pdfdoc)
 * Close the generated PDF file, and release all document-related resources. */
PHP_FUNCTION(pdf_close) 
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_close(pdf);

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto bool pdf_delete(int pdfdoc)
 * Delete the PDF object, and free all internal resources. */
PHP_FUNCTION(pdf_delete)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

#ifndef Z_RESVAL        /* for php 4.0.3pl1 */
#define Z_RESVAL(zval)            (zval).value.lval
#define Z_RESVAL_PP(zval_pp)      Z_RESVAL(**zval_pp)
#endif
	zend_list_delete(Z_RESVAL_PP(arg1));

	RETURN_TRUE;
}

/* }}} */

/* {{{ proto void pdf_end_page(int pdfdoc)
 * Finish the page. */
PHP_FUNCTION(pdf_end_page) 
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_end_page(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int pdf_get_apiname(int pdfdoc);
 * Get the name of the API function which threw the last exception or failed. */
PHP_FUNCTION(pdf_get_apiname)
{
        zval **p;
        PDF *pdf;
		char *buffer;

        if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &p) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        buffer = PDF_get_apiname(pdf);

		RETURN_STRING(buffer, 1);
}
/* }}} */

/* {{{ proto int pdf_get_buffer(int pdfdoc)
 * Get the contents of the PDF output buffer. The result must be used by the client before calling any other PDFlib function. */
PHP_FUNCTION(pdf_get_buffer)
{
	zval **arg1;
	long size;
	PDF *pdf;
	const char *buffer;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	buffer = PDF_get_buffer(pdf, &size);

	RETURN_STRINGL((char *)buffer, size, 1);
}

/* }}} */

/* {{{ proto int pdf_get_errmsg(int pdfdoc);
 * Get the contents of the PDF output buffer. The result must be used by
 * the client before calling any other PDFlib function. */
PHP_FUNCTION(pdf_get_errmsg)
{
        zval **p;
        PDF *pdf;
		char *buffer;

        if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &p) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        buffer = PDF_get_errmsg(pdf);

		RETURN_STRING(buffer, 1);
}
/* }}} */

/* {{{ proto int pdf_get_errnum(int pdfdoc);
 * Get the descriptive text of the last thrown exception, or the reason of
 * a failed function call.*/
PHP_FUNCTION(pdf_get_errnum)
{
        zval **p;
        PDF *pdf;
		int retval;

        if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &p) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        retval = PDF_get_errnum(pdf);

		RETURN_LONG(retval);
}
/* }}} */

/* {{{ proto int pdf_new()
 * Creates a new PDF object */
PHP_FUNCTION(pdf_new)
{
	PDF *pdf;

	pdf = PDF_new2(custom_errorhandler, pdf_emalloc, pdf_realloc, pdf_efree, NULL);
	if (pdf != NULL) {
		PDF_set_parameter(pdf, "imagewarning", "true");

		/* Trigger special handling of PDFlib-handles for PHP */
		PDF_set_parameter(pdf, "hastobepos", "true");
		PDF_set_parameter(pdf, "binding", "PHP");
		ZEND_REGISTER_RESOURCE(return_value, pdf, le_pdf);
	} else {
		php_error(E_ERROR, "PDF_new: internal error");
	}

}

/* }}} */

/* {{{ proto int pdf_open_file(int pdfdoc [, char filename])
 * Create a new PDF file using the supplied file name. */
PHP_FUNCTION(pdf_open_file)
{
	zval **p, **filename;
	int pdf_file;
	const char *vfilename;
	int argc;
	PDF *pdf;

	if((argc = ZEND_NUM_ARGS()) > 2)
		WRONG_PARAM_COUNT;

	if (argc == 1) {
		if (zend_get_parameters_ex(1, &p) == FAILURE)
			WRONG_PARAM_COUNT;
	} else {
		if (zend_get_parameters_ex(2, &p, &filename) == FAILURE)
			WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	if (argc == 2) {
		convert_to_string_ex(filename);
#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
		virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
		virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
		vfilename = Z_STRVAL_PP(filename);
#endif  

		pdf_file = PDF_open_file(pdf, vfilename);
	} else {
		/* open in memory */
		pdf_file = PDF_open_file(pdf, "");
	}

	RETURN_LONG(pdf_file); /* change return from -1 to 0 handled by PDFlib */
}

/* }}} */

/* p_block.c */

/* TODO [optlist] */
/* {{{ proto int pdf_fill_imageblock(int pdfdoc, int page, string spotname, int image, string optlist);
 * Process an image block according to its properties. */
PHP_FUNCTION(pdf_fill_imageblock)
{
	zval **p, **page, **blockname, **image, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &page, &blockname, &image, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(page);
	convert_to_string_ex(blockname);
	convert_to_long_ex(image);
	convert_to_string_ex(optlist);

	retval = PDF_fill_imageblock(pdf,
		Z_LVAL_PP(page),
		Z_STRVAL_PP(blockname),
		Z_LVAL_PP(image),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* change return from -1 to 0 handled by PDFlib */
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_fill_pdfblock(int pdfdoc, int page, string spotname, int contents, string optlist);
 * Process a PDF block according to its properties. */
PHP_FUNCTION(pdf_fill_pdfblock)
{
	zval **p, **page, **blockname, **contents, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &page, &blockname, &contents, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(page);
	convert_to_string_ex(blockname);
	convert_to_long_ex(contents);
	convert_to_string_ex(optlist);

	retval = PDF_fill_pdfblock(pdf,
		Z_LVAL_PP(page),
		Z_STRVAL_PP(blockname),
		Z_LVAL_PP(contents),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* change return from -1 to 0 handled by PDFlib */
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_fill_textblock(int pdfdoc, int page, string spotname, string text, string optlist);
 * Process a text block according to its properties. */
PHP_FUNCTION(pdf_fill_textblock)
{
	zval **p, **page, **blockname, **text, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &page, &blockname, &text, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(page);
	convert_to_string_ex(blockname);
	convert_to_string_ex(text);
	convert_to_string_ex(optlist);

	retval = PDF_fill_textblock(pdf,
		Z_LVAL_PP(page),
		Z_STRVAL_PP(blockname),
		Z_STRVAL_PP(text),
		Z_STRLEN_PP(text),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* change return from -1 to 0 handled by PDFlib */
}
/* }}} */

/* p_color.c */

/* {{{ proto int pdf_makespotcolor(int pdfdoc, string spotname);
 * Make a named spot color from the current color. */
PHP_FUNCTION(pdf_makespotcolor)
{
	zval **arg1, **arg2;
	PDF *pdf;
	int spotcolor;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);

	spotcolor = PDF_makespotcolor(pdf,
		Z_STRVAL_PP(arg2),
		Z_STRLEN_PP(arg2));

	RETURN_LONG(spotcolor); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_setcolor(int pdfdoc, string fstype, string colorspace, float c1 [, float c2 [, float c3 [, float c4]]]);
 * Set the current color space and color. fstype is "fill", "stroke", or "both". */
PHP_FUNCTION(pdf_setcolor)
{
	zval **p, **fstype, **colorspace, **c1, **c2, **c3, **c4;
	PDF *pdf;
	int argc = ZEND_NUM_ARGS();

	if(argc < 4 || argc > 7) {
		WRONG_PARAM_COUNT;
	}
	switch(argc) {
		case 4:
			if(zend_get_parameters_ex(4, &p, &fstype, &colorspace, &c1) == FAILURE) {
				WRONG_PARAM_COUNT;
			}
			break;
		case 5:
			if(zend_get_parameters_ex(5, &p, &fstype, &colorspace, &c1, &c2) == FAILURE) {
				WRONG_PARAM_COUNT;
			}
			break;
		case 6:
			if(zend_get_parameters_ex(6, &p, &fstype, &colorspace, &c1, &c2, &c3) == FAILURE) {
				WRONG_PARAM_COUNT;
			}
			break;
		case 7:
			if(zend_get_parameters_ex(7, &p, &fstype, &colorspace, &c1, &c2, &c3, &c4) == FAILURE) {
				WRONG_PARAM_COUNT;
			}
			break;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(fstype);
	convert_to_string_ex(colorspace);
	convert_to_double_ex(c1);
	if(argc > 4) convert_to_double_ex(c2);
	if(argc > 5) convert_to_double_ex(c3);
	if(argc > 6) convert_to_double_ex(c4);


	PDF_setcolor(pdf,
		Z_STRVAL_PP(fstype),
		Z_STRVAL_PP(colorspace),
	    (float) Z_DVAL_PP(c1),
		(float) ((argc>4) ? Z_DVAL_PP(c2):0),
		(float) ((argc>5) ? Z_DVAL_PP(c3):0),
		(float) ((argc>6) ? Z_DVAL_PP(c4):0));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setgray(int pdfdoc, float value)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setgray)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	PDF_setcolor(pdf, "both", "gray", (float) Z_DVAL_PP(arg2), 0, 0, 0);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setgray_fill(int pdfdoc, float value)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setgray_fill)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	PDF_setcolor(pdf, "fill", "gray", (float) Z_DVAL_PP(arg2), 0, 0, 0);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setgray_stroke(int pdfdoc, float value)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setgray_stroke) 
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	PDF_setcolor(pdf, "stroke", "gray", (float) Z_DVAL_PP(arg2), 0, 0, 0);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setrgbcolor(int pdfdoc, float red, float green, float blue)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setrgbcolor)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	PDF_setcolor(pdf, "both", "rgb", (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4), 0);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setrgbcolor_fill(int pdfdoc, float red, float green, float blue)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setrgbcolor_fill)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	PDF_setcolor(pdf, "fill", "rgb", (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4), 0);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setrgbcolor_stroke(int pdfdoc, float red, float green, float blue)
 * Depricated user pdf_setcolor instead */
PHP_FUNCTION(pdf_setrgbcolor_stroke)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	PDF_setcolor(pdf, "stroke", "rgb", (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4), 0);
	RETURN_TRUE;
}
/* }}} */

/* p_draw.c */

/* {{{ proto void pdf_arc(int pdfdoc, float x, float y, float r, float alpha, float beta)
 * Draw a counterclockwise circular arc from alpha to beta degrees. */
PHP_FUNCTION(pdf_arc)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_double_ex(arg6);

	PDF_arc(pdf, (float) Z_DVAL_PP(arg2),
				 (float) Z_DVAL_PP(arg3),
				 (float) Z_DVAL_PP(arg4),
				 (float) Z_DVAL_PP(arg5),
				 (float) Z_DVAL_PP(arg6));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_arcn(int pdfdoc, float x, float y, float r, float alpha, float beta);
 * Draw a clockwise circular arc from alpha to beta degrees. */
PHP_FUNCTION(pdf_arcn)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_double_ex(arg6);

	PDF_arcn(pdf,
		(float) Z_DVAL_PP(arg2),
		(float) Z_DVAL_PP(arg3),
		(float) Z_DVAL_PP(arg4),
		(float) Z_DVAL_PP(arg5),
		(float) Z_DVAL_PP(arg6));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_circle(int pdfdoc, float x, float y, float r)
 * Draw a circle with center (x, y) and radius r. */
PHP_FUNCTION(pdf_circle)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	PDF_circle(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_clip(int pdfdoc)
 * Use the current path as clipping path. */
PHP_FUNCTION(pdf_clip)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_clip(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_closepath(int pdfdoc)
 * Close the current path. */
PHP_FUNCTION(pdf_closepath)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_closepath(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_closepath_fill_stroke(int pdfdoc)
 * Close the path, fill, and stroke it. */
PHP_FUNCTION(pdf_closepath_fill_stroke)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_closepath_fill_stroke(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_closepath_stroke(int pdfdoc)
 * Close the path, and stroke it. */
PHP_FUNCTION(pdf_closepath_stroke)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_closepath_stroke(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_curveto(int pdfdoc, float x1, float y1, float x2, float y2, float x3, float y3)
 * Draw a Bezier curve from the current point, using 3 more control points. */
PHP_FUNCTION(pdf_curveto)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6, **arg7;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_double_ex(arg6);
	convert_to_double_ex(arg7);

	PDF_curveto(pdf, (float) Z_DVAL_PP(arg2),
					 (float) Z_DVAL_PP(arg3),
					 (float) Z_DVAL_PP(arg4),
					 (float) Z_DVAL_PP(arg5),
					 (float) Z_DVAL_PP(arg6),
					 (float) Z_DVAL_PP(arg7));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_endpath(int pdfdoc)
 *  End the current path without filling or stroking it. */
PHP_FUNCTION(pdf_endpath) 
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_endpath(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_fill(int pdfdoc)
 * Fill the interior of the path with the current fill color. */
PHP_FUNCTION(pdf_fill) 
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_fill(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_fill_stroke(int pdfdoc)
 * Fill and stroke the path with the current fill and stroke color. */
PHP_FUNCTION(pdf_fill_stroke)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_fill_stroke(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_lineto(int pdfdoc, float x, float y)
 * Draw a line from the current point to (x, y). */
PHP_FUNCTION(pdf_lineto)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_lineto(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_moveto(int pdfdoc, float x, float y)
 * Set the current point. */
PHP_FUNCTION(pdf_moveto)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_moveto(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_rect(int pdfdoc, float x, float y, float width, float height)
 * Draw a rectangle at lower left (x, y) with width and height. */
PHP_FUNCTION(pdf_rect)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &arg1, &arg2, &arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);

	PDF_rect(pdf, (float) Z_DVAL_PP(arg2),
				  (float) Z_DVAL_PP(arg3),
				  (float) Z_DVAL_PP(arg4),
				  (float) Z_DVAL_PP(arg5));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_stroke(int pdfdoc)
 * Stroke the path with the current color and line width, and clear it. */
PHP_FUNCTION(pdf_stroke)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_stroke(pdf);
	RETURN_TRUE;
}
/* }}} */

/* p_encoding.c */

/* {{{ proto void pdf_encoding_set_char(int pdfdoc, string encoding, int slot, string glyphname, int uv);
 * Add a glyph name to a custom encoding. */
PHP_FUNCTION(pdf_encoding_set_char)
{
        zval **p, **encoding, **slot, **glyphname, **uv;
        PDF *pdf;

        if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &encoding, &slot, &glyphname, &uv) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        convert_to_string_ex(encoding);
        convert_to_long_ex(slot);
        convert_to_string_ex(glyphname);
        convert_to_long_ex(uv);

        PDF_encoding_set_char(pdf,
                Z_STRVAL_PP(encoding),
                Z_LVAL_PP(slot),
                Z_STRVAL_PP(glyphname),
                Z_LVAL_PP(uv));

        RETURN_TRUE;
}
/* }}} */


/* p_font.c */

/* {{{ proto int pdf_findfont(int pdfdoc, string fontname, string encoding [, int embed])
 * Search a font, and prepare it for later use. PDF_load_font() is recommended. */
PHP_FUNCTION(pdf_findfont)
{
	zval **arg1, **arg2, **arg3, **arg4;
	int embed, font;
	const char *fontname, *encoding;
	PDF *pdf;

	switch (ZEND_NUM_ARGS()) {
	case 3:
		if (zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		embed = 0;
		break;
	case 4:
		if (zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		convert_to_long_ex(arg4);
		embed = Z_LVAL_PP(arg4);
		break;
	default:
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	fontname = Z_STRVAL_PP(arg2);

	convert_to_string_ex(arg3);
	encoding = Z_STRVAL_PP(arg3);

	font = PDF_findfont(pdf, fontname, encoding, embed);

	RETURN_LONG(font); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_load_font(int pdfdoc, string fontname, string encoding, string optlist);
 * Open and search a font, and prepare it for later use.*/
PHP_FUNCTION(pdf_load_font)
{
	zval **p, **fontname, **encoding, **optlist;
	PDF *pdf;
	int reserved = 0;
	int retval;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &fontname, &encoding, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(fontname);
	convert_to_string_ex(encoding);
	convert_to_string_ex(optlist);

	retval = PDF_load_font(pdf,
		Z_STRVAL_PP(fontname),
		reserved,
		Z_STRVAL_PP(encoding),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_setfont(int pdfdoc, int font, float fontsize)
 * Set the current font in the given size, using a font handle returned by PDF_load_font(). */
PHP_FUNCTION(pdf_setfont)
{
	zval **arg1, **arg2, **arg3;
	int font;
	float fontsize;
	PDF *pdf;

	if(ZEND_NUM_ARGS() != 3)
		WRONG_PARAM_COUNT;
	if (zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE)
		WRONG_PARAM_COUNT;

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);
	font = Z_LVAL_PP(arg2);

	convert_to_double_ex(arg3);
	fontsize = (float)Z_DVAL_PP(arg3);

	PDF_setfont(pdf, font, fontsize);

	RETURN_TRUE;
}
/* }}} */

/* p_gstate.c */

/* {{{ proto void pdf_concat(int pdfdoc, float a, float b, float c, float d, float e, float f)
 * Concatenate a matrix to the current transformation matrix. */
PHP_FUNCTION(pdf_concat)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6, **arg7;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_double_ex(arg6);
	convert_to_double_ex(arg7);

	PDF_concat(pdf,
	    (float) Z_DVAL_PP(arg2),
	    (float) Z_DVAL_PP(arg3),
	    (float) Z_DVAL_PP(arg4),
	    (float) Z_DVAL_PP(arg5),
	    (float) Z_DVAL_PP(arg6),
	    (float) Z_DVAL_PP(arg7));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_initgraphics(int pdfdoc);
 * Reset all color and graphics state parameters to their defaults. */
PHP_FUNCTION(pdf_initgraphics)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_initgraphics(pdf);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_restore(int pdfdoc)
 * Restore the most recently saved graphics state. */
PHP_FUNCTION(pdf_restore)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_restore(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_rotate(int pdfdoc, float angle)
 * Rotate the coordinate system by phi degrees. */
PHP_FUNCTION(pdf_rotate)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	PDF_rotate(pdf, (float) Z_DVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_save(int pdfdoc)
 * Save the current graphics state. */
PHP_FUNCTION(pdf_save)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_save(pdf);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_scale(int pdfdoc, float x_scale, float y_scale)
 * Scale the coordinate system. */
PHP_FUNCTION(pdf_scale)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_scale(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setdash(int pdfdoc, float black, float white)
 * Set the current dash pattern to b black and w white units. */
PHP_FUNCTION(pdf_setdash)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_setdash(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setdashpattern(int pdfdoc, string optlist)
 * Set a more complicated dash pattern defined by an optlist. */
PHP_FUNCTION(pdf_setdashpattern) 
{
	zval **p, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(optlist);

	PDF_setdashpattern(pdf, Z_STRVAL_PP(optlist));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setflat(int pdfdoc, float flatness)
 * Set the flatness to a value between 0 and 100 inclusive. */
PHP_FUNCTION(pdf_setflat) 
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);

	PDF_setflat(pdf, (float) Z_DVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setlinecap(int pdfdoc, int linecap)
 * Set the linecap parameter to a value between 0 and 2 inclusive. */
PHP_FUNCTION(pdf_setlinecap) 
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);

	PDF_setlinecap(pdf, Z_LVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setlinejoin(int pdfdoc, int linejoin)
 * Set the line join parameter to a value between 0 and 2 inclusive. */
PHP_FUNCTION(pdf_setlinejoin) 
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);

	PDF_setlinejoin(pdf, Z_LVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setlinewidth(int pdfdoc, float width)
 * Set the current linewidth to width. */
PHP_FUNCTION(pdf_setlinewidth)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	PDF_setlinewidth(pdf, (float) Z_DVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setmatrix(int pdfdoc, float a, float b, float c, float d, float e, float f)
 * Explicitly set the current transformation matrix. */
PHP_FUNCTION(pdf_setmatrix)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6, **arg7;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_double_ex(arg6);
	convert_to_double_ex(arg7);

	PDF_setmatrix(pdf,
	    (float) Z_DVAL_PP(arg2),
	    (float) Z_DVAL_PP(arg3),
	    (float) Z_DVAL_PP(arg4),
	    (float) Z_DVAL_PP(arg5),
	    (float) Z_DVAL_PP(arg6),
	    (float) Z_DVAL_PP(arg7));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setmiterlimit(int pdfdoc, float value)
 * Set the miter limit to a value greater than or equal to 1. */
PHP_FUNCTION(pdf_setmiterlimit)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);

	PDF_setmiterlimit(pdf, (float) Z_DVAL_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_setpolydash(int pdfdoc, float darray)
 * Deprecated, use PDF_setdashpattern() instead. */ 
PHP_FUNCTION(pdf_setpolydash)
{
	zval **arg1, **arg2;
	HashTable *array;
	int len, i;
	float *darray;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_array_ex(arg2);
	array = Z_ARRVAL_PP(arg2);
	len = zend_hash_num_elements(array);

	/* TODO: ohne Malloc: maximum ist 8 ... */
	if (NULL == (darray = emalloc(len * sizeof(double)))) {
	    RETURN_FALSE;
	}
	zend_hash_internal_pointer_reset(array);
	for (i=0; i<len; i++) {
	    zval *keydata, **keydataptr;

	    zend_hash_get_current_data(array, (void **) &keydataptr);
	    keydata = *keydataptr;
	    if (Z_TYPE_P(keydata) == IS_DOUBLE) {
		darray[i] = (float) Z_DVAL_P(keydata);
	    } else if (Z_TYPE_P(keydata) == IS_LONG) {
		darray[i] = (float) Z_LVAL_P(keydata);
	    } else {
		php_error(E_WARNING,"PDFlib set_polydash: illegal darray value");
	    }
	    zend_hash_move_forward(array);
	}

	PDF_setpolydash(pdf, darray, len);

	efree(darray);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_skew(int pdfdoc, float xangle, float yangle)
 * Skew the coordinate system in x and y direction by alpha and beta degrees. */
PHP_FUNCTION(pdf_skew)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_skew(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_translate(int pdfdoc, float x, float y)
 * Translate the origin of the coordinate system. */
PHP_FUNCTION(pdf_translate) 
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_translate(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* p_hyper.c */

/* {{{ proto int pdf_add_bookmark(int pdfdoc, string text [, int parent, int open]) 
 * Add a nested bookmark under parent, or a new top-level bookmark if parent = 0. Returns a bookmark descriptor which may be used as parent for subsequent nested bookmarks. If open = 1, child bookmarks will be folded out, and invisible if open = 0. */
PHP_FUNCTION(pdf_add_bookmark)
{
	zval **p, **text, **parent, **open;
	int parentid, p_open, id;
	PDF *pdf;

	switch (ZEND_NUM_ARGS()) {
	case 2:
		if (zend_get_parameters_ex(2, &p, &text) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 3:
		if (zend_get_parameters_ex(3, &p, &text, &parent) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
		break;
	case 4:
		if (zend_get_parameters_ex(4, &p, &text, &parent, &open) == FAILURE) {
			WRONG_PARAM_COUNT;
		}
	break;
	default:
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(text);

	if (ZEND_NUM_ARGS() > 2) {
		convert_to_long_ex(parent);
		parentid = Z_LVAL_PP(parent);

		if (ZEND_NUM_ARGS() > 3) {
			convert_to_long_ex(open);
			p_open = Z_LVAL_PP(open);
		} else {
			p_open = 0;
		}
	} else {
		parentid = 0;
		p_open = 0;
	}

	/* will never return 0 */
	id = PDF_add_bookmark2(pdf,
			Z_STRVAL_PP(text),
			Z_STRLEN_PP(text),
			parentid,
			p_open);

	RETURN_LONG(id);
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_add_nameddest(int pdfdoc, string name, string optlist)
 * Set a more complicated dash pattern defined by an optlist. */
PHP_FUNCTION(pdf_add_nameddest) 
{
	zval **p, **name, **optlist;
	PDF *pdf;
	int reserved = 0;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &p, &name, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(name);
	convert_to_string_ex(optlist);

	PDF_add_nameddest(pdf,
		Z_STRVAL_PP(name),
		reserved,
		Z_STRVAL_PP(optlist));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool pdf_set_info(int pdfdoc, string key, string value)
 * Fill document information field key with value. key is one of "Subject", "Title", "Creator", "Author", "Keywords", or a user-defined key. */
PHP_FUNCTION(pdf_set_info) 
{
	zval **p, **key, **value;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &p, &key, &value) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(key);
	convert_to_string_ex(value);

	PDF_set_info2(pdf,
		Z_STRVAL_PP(key),
		Z_STRVAL_PP(value),
		Z_STRLEN_PP(value));

	RETURN_TRUE;
}
/* }}} */

/* p_icc.c */

/* TODO [optlist] */
/* {{{ proto int pdf_load_iccprofile(int pdfdoc, string profilename, string optlist);
 * Search an ICC profile, and prepare it for later use. */
PHP_FUNCTION(pdf_load_iccprofile)
{
	zval **p, **profilename, **optlist;
	PDF *pdf;
	int reserved = 0;
	char * vprofilename;
	int retval;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &p, &profilename, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(profilename);
	convert_to_string_ex(optlist);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(profilename), &vprofilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(profilename), &vprofilename);
#    endif
#else
	vprofilename = Z_STRVAL_PP(profilename);
#endif  


	retval = PDF_load_iccprofile(pdf,
		vprofilename,
		reserved,
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* p_image.c */

/* {{{ proto void pdf_add_thumbnail(int pdfdoc, int image);
 * Add an existing image as thumbnail for the current page. */
PHP_FUNCTION(pdf_add_thumbnail)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);

	PDF_add_thumbnail(pdf,
		Z_LVAL_PP(arg2));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_close_image(int pdfdoc, int image)
 * Close an image retrieved with PDF_load_image(). */
PHP_FUNCTION(pdf_close_image)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);
	convert_to_long_ex(arg2);

	PDF_close_image(pdf, Z_LVAL_PP(arg2));
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_fit_image(int pdfdoc, int image, float x, float y, string optlist);
 * Place an image or template at (x, y) with various options. */
PHP_FUNCTION(pdf_fit_image)
{
	zval **p, **image, **x, **y, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &image, &x, &y, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(image);
	convert_to_double_ex(x);
	convert_to_double_ex(y);
	convert_to_string_ex(optlist);

	PDF_fit_image(pdf,
		Z_LVAL_PP(image),
		(float)Z_DVAL_PP(x),
		(float)Z_DVAL_PP(y),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_load_image(int pdfdoc, string imagetype, string filename, string optlist);
 * Open a (disk-based or virtual) image file with various options.*/
PHP_FUNCTION(pdf_load_image)
{
	zval **p, **imagetype, **filename, **optlist;
	PDF *pdf;
	const char *vfilename;
	int reserved = 0;
	int retval;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &imagetype, &filename, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(imagetype);
	convert_to_string_ex(filename);
	convert_to_string_ex(optlist);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
	vfilename = Z_STRVAL_PP(filename);
#endif  

	retval = PDF_load_image(pdf,
		Z_STRVAL_PP(imagetype),
		vfilename,
		reserved,
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto int pdf_open_ccitt(int pdfdoc, string filename, int width, int height, int bitreverse, int k, int blackls1)
 * Deprecated, use PDF_load_image() instead. */
PHP_FUNCTION(pdf_open_ccitt)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6, **arg7;
	PDF *pdf;
	int pdf_image;
	char *image;

	if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(arg2), &image TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(arg2), &image);
#    endif
#else
	image = Z_STRVAL_PP(arg2);
#endif  

	convert_to_long_ex(arg3);
	convert_to_long_ex(arg4);
	convert_to_long_ex(arg5);
	convert_to_long_ex(arg6);
	convert_to_long_ex(arg7);

	pdf_image = PDF_open_CCITT(pdf,
	    image,
	    Z_LVAL_PP(arg3),
	    Z_LVAL_PP(arg4),
	    Z_LVAL_PP(arg5),
	    Z_LVAL_PP(arg6),
	    Z_LVAL_PP(arg7));

	RETURN_LONG(pdf_image); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto int pdf_open_image(int pdfdoc, string imagetype, string source, string data, long length, int width, int height, int components, int bpc, string params)
 * Deprecated, use PDF_load_image() with virtual files instead. */
PHP_FUNCTION(pdf_open_image)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6, **arg7, **arg8, **arg9, **arg10;
	PDF *pdf;
	int pdf_image;

	if (ZEND_NUM_ARGS() != 10 || zend_get_parameters_ex(10, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6, &arg7, &arg8, &arg9, &arg10) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_string_ex(arg3);
	convert_to_string_ex(arg4);
	convert_to_long_ex(arg5);
	convert_to_long_ex(arg6);
	convert_to_long_ex(arg7);
	convert_to_long_ex(arg8);
	convert_to_long_ex(arg9);
	convert_to_string_ex(arg10);

	pdf_image = PDF_open_image(pdf,
		Z_STRVAL_PP(arg2),
		Z_STRVAL_PP(arg3),
		Z_STRVAL_PP(arg4),
		Z_LVAL_PP(arg5),
		Z_LVAL_PP(arg6),
		Z_LVAL_PP(arg7),
		Z_LVAL_PP(arg8),
		Z_LVAL_PP(arg9),
		Z_STRVAL_PP(arg10));

	RETURN_LONG(pdf_image); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto int pdf_open_image_file(int pdfdoc, string imagetype, string filename, string stringparam, int intparam)
 * Deprecated, use PDF_load_image() instead. */
PHP_FUNCTION(pdf_open_image_file)
{
	zval **p, **imagetype, **filename, **stringparam, **intparam;
	PDF *pdf;
	int pdf_image, argc;
	const char *vfilename;

	switch ((argc = ZEND_NUM_ARGS())) {
	case 3:
		if (zend_get_parameters_ex(3, &p, &imagetype, &filename) == FAILURE)
			WRONG_PARAM_COUNT;
		break;
	case 5:
		if (zend_get_parameters_ex(5, &p, &imagetype, &filename, &stringparam, &intparam) == FAILURE)
			WRONG_PARAM_COUNT;
		break;
	default:
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(imagetype);
	convert_to_string_ex(filename);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(filename), &vfilename);
#    endif
#else
	vfilename = Z_STRVAL_PP(filename);
#endif  

	if (argc == 3) {
		pdf_image = PDF_open_image_file(pdf, Z_STRVAL_PP(imagetype), vfilename, "", 0);
	} else {
	    convert_to_string_ex(stringparam);
	    convert_to_long_ex(intparam);

	    pdf_image = PDF_open_image_file(pdf,
				Z_STRVAL_PP(imagetype),
				vfilename,
				Z_STRVAL_PP(stringparam),
				Z_LVAL_PP(intparam));
	}

	RETURN_LONG(pdf_image); /* offset handled in PDFlib Kernel */

}
/* }}} */

/* {{{ proto void pdf_place_image(int pdfdoc, int image, float x, float y, float scale)
 * Deprecated, use PDF_fit_image() instead. */
PHP_FUNCTION(pdf_place_image)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &arg1, &arg2, &arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);

	PDF_place_image(pdf, Z_LVAL_PP(arg2), (float) Z_DVAL_PP(arg3), (float) Z_DVAL_PP(arg4), (float) Z_DVAL_PP(arg5));
	RETURN_TRUE;
}
/* }}} */

/* p_params.c */

/* {{{ proto string pdf_get_parameter(int pdfdoc, string key, float modifier)
 * Get the contents of some PDFlib parameter with string type. */
PHP_FUNCTION(pdf_get_parameter)
{
	zval **argv[3];
	int argc = ZEND_NUM_ARGS();
	PDF *pdf;
	char *value;

	if(((argc < 2) || (argc > 3)) || zend_get_parameters_array_ex(argc, argv) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	convert_to_string_ex(argv[1]);

	if(0 == (strcmp(Z_STRVAL_PP(argv[1]), "version"))) {
	    value = (char *) PDF_get_parameter(0, Z_STRVAL_PP(argv[1]), 0.0);
	    RETURN_STRING(value, 1);
	} else if(0 == (strcmp(Z_STRVAL_PP(argv[1]), "pdi"))) {
	    value = (char *) PDF_get_parameter(0, Z_STRVAL_PP(argv[1]), 0.0);
	    RETURN_STRING(value, 1);
	} else {
	    ZEND_FETCH_RESOURCE(pdf, PDF *, argv[0], -1, "pdf object", le_pdf);
	}

	if(argc == 3) {
		convert_to_double_ex(argv[2]);
		value = (char *) PDF_get_parameter(pdf, Z_STRVAL_PP(argv[1]), (float) Z_DVAL_PP(argv[2]));
	} else {
		value = (char *) PDF_get_parameter(pdf, Z_STRVAL_PP(argv[1]), 0.0);
	}

	RETURN_STRING(value, 1);
}
/* }}} */

/* {{{ proto float pdf_get_value(int pdfdoc, string key, float modifier)
 * Get the value of some PDFlib parameter with float type. */
PHP_FUNCTION(pdf_get_value)
{
	zval **argv[3];
	int argc = ZEND_NUM_ARGS();
	PDF *pdf;
	double value;

	if(((argc < 2) || (argc > 3)) || zend_get_parameters_array_ex(argc, argv) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	convert_to_string_ex(argv[1]);
	if(argc == 3)
	    convert_to_double_ex(argv[2]);

	if(0 == (strcmp(Z_STRVAL_PP(argv[1]), "major"))) {
		value = PDF_get_value(0, Z_STRVAL_PP(argv[1]), 0);
		RETURN_DOUBLE(value);
	} else if(0 == (strcmp(Z_STRVAL_PP(argv[1]), "minor"))) {
		value = PDF_get_value(0, Z_STRVAL_PP(argv[1]), 0);
		RETURN_DOUBLE(value);
	} else if(0 == (strcmp(Z_STRVAL_PP(argv[1]), "revision"))) {
		value = PDF_get_value(0, Z_STRVAL_PP(argv[1]), 0);
		RETURN_DOUBLE(value);
	} else {
	    ZEND_FETCH_RESOURCE(pdf, PDF *, argv[0], -1, "pdf object", le_pdf);
	}

	if(argc < 3) {
		value = PDF_get_value(pdf,
					Z_STRVAL_PP(argv[1]),
					0.0);
	} else {
		value = PDF_get_value(pdf,
					Z_STRVAL_PP(argv[1]),
					(float)Z_DVAL_PP(argv[2]));
	}

	RETURN_DOUBLE(value);
}
/* }}} */

/* {{{ proto void pdf_set_parameter(int pdfdoc, string key, string value)
 *  Set some PDFlib parameter with string type. */
PHP_FUNCTION(pdf_set_parameter)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_string_ex(arg3);

	PDF_set_parameter(pdf, Z_STRVAL_PP(arg2), Z_STRVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_set_value(int pdfdoc, string key, float value)
 * Set the value of some PDFlib parameter with float type. */
PHP_FUNCTION(pdf_set_value)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_set_value(pdf, Z_STRVAL_PP(arg2), (float)Z_DVAL_PP(arg3));

	RETURN_TRUE;
}
/* }}} */

/* p_pattern.c */

/* {{{ proto int pdf_begin_pattern(int pdfdoc, float width, float height, float xstep, float ystep, int painttype);
 * Start a new pattern definition. */
PHP_FUNCTION(pdf_begin_pattern)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5, **arg6;
	PDF *pdf;
	int pattern_image;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &arg1, &arg2, &arg3, &arg4, &arg5, &arg6) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	convert_to_double_ex(arg4);
	convert_to_double_ex(arg5);
	convert_to_long_ex(arg6);

	pattern_image = PDF_begin_pattern(pdf,
		(float) Z_DVAL_PP(arg2),
		(float) Z_DVAL_PP(arg3),
		(float) Z_DVAL_PP(arg4),
		(float) Z_DVAL_PP(arg5),
		Z_LVAL_PP(arg6));

	RETURN_LONG(pattern_image); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_end_pattern(int pdfdoc);
 * Finish the pattern definition. */
PHP_FUNCTION(pdf_end_pattern)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	PDF_end_pattern(pdf);

	RETURN_TRUE;
}
/* }}} */

/* p_pdi.c */

/* {{{ proto void pdf_close_pdi(int pdfdoc, int doc);
 * Close all open page handles, and close the input PDF document. */
PHP_FUNCTION(pdf_close_pdi)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);

	PDF_close_pdi(pdf,
		Z_LVAL_PP(arg2));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_close_pdi_page(int pdfdoc, int page);
 * Close the page handle, and free all page-related resources. */
PHP_FUNCTION(pdf_close_pdi_page)
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_long_ex(arg2);

	PDF_close_pdi_page(pdf,
		Z_LVAL_PP(arg2));

	RETURN_TRUE;
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_fit_pdi_page(int pdfdoc, int page, float x, float y, string optlist);
 * Place an imported PDF page with the lower left corner at (x, y) with
 * various options.*/
PHP_FUNCTION(pdf_fit_pdi_page)
{
	zval **p, **page, **x, **y, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &page, &x, &y, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(page);
	convert_to_double_ex(x);
	convert_to_double_ex(y);
	convert_to_string_ex(optlist);

	PDF_fit_pdi_page(pdf,
		Z_LVAL_PP(page),
		(float)Z_DVAL_PP(x),
		(float)Z_DVAL_PP(y),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE
}
/* }}} */

/* TODO [reserved] */
/* {{{ proto string pdf_get_pdi_parameter(int pdfdoc, string key, int doc, int page, int reserved);
 * Get the contents of some PDI document parameter with string type. */
PHP_FUNCTION(pdf_get_pdi_parameter)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5;
	PDF *pdf;
	const char *buffer;
	int size;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &arg1, &arg2, &arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_long_ex(arg3);
	convert_to_long_ex(arg4);
	convert_to_long_ex(arg5);

	buffer = PDF_get_pdi_parameter(pdf,
		Z_STRVAL_PP(arg2),
		Z_LVAL_PP(arg3),
		Z_LVAL_PP(arg4),
		Z_LVAL_PP(arg5),
		&size);

	RETURN_STRINGL((char *)buffer, size, 1);
}
/* }}} */

/* TODO [reserved] */
/* {{{ proto float pdf_get_pdi_value(int pdfdoc, string key, int doc, int page, int reserved);
 * Get the contents of some PDI document parameter with numerical type. */
PHP_FUNCTION(pdf_get_pdi_value)
{
	zval **arg1, **arg2, **arg3, **arg4, **arg5;
	PDF *pdf;
	double value;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &arg1, &arg2, &arg3, &arg4, &arg5) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_long_ex(arg3);
	convert_to_long_ex(arg4);
	convert_to_long_ex(arg5);

	value = (double)PDF_get_pdi_value(pdf,
		Z_STRVAL_PP(arg2),
		Z_LVAL_PP(arg3),
		Z_LVAL_PP(arg4),
		Z_LVAL_PP(arg5));

	RETURN_DOUBLE(value);
}
/* }}} */

/* TODO [optlist, reserved] */
/* {{{ proto int pdf_open_pdi(int pdfdoc, string filename, string optlist, int reserved);
 * Open a (disk-based or virtual) PDF document and prepare it for later use. */
PHP_FUNCTION(pdf_open_pdi)
{
	zval **arg1, **arg2, **arg3, **arg4;
	PDF *pdf;
	int pdi_handle;
	char *file;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &arg1, &arg2, &arg3, &arg4) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	convert_to_string_ex(arg3);
	convert_to_long_ex(arg4);

#ifdef VIRTUAL_DIR
#    if ZEND_MODULE_API_NO >= 20010901
	virtual_filepath(Z_STRVAL_PP(arg2), &file TSRMLS_CC);
#    else
	virtual_filepath(Z_STRVAL_PP(arg2), &file);
#    endif
#else
	file = Z_STRVAL_PP(arg2);
#endif  

	pdi_handle = PDF_open_pdi(pdf,
		file,
		Z_STRVAL_PP(arg3),
		Z_LVAL_PP(arg4));

	RETURN_LONG(pdi_handle); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_open_pdi_page(int pdfdoc, int doc, int pagenumber, string optlist);
 * Prepare a page for later use with PDF_place_pdi_page(). */
PHP_FUNCTION(pdf_open_pdi_page)
{
	zval **p, **doc, **pagenumber, **optlist;
	PDF *pdf;
	int pdi_image;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &doc, &pagenumber, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(doc);
	convert_to_long_ex(pagenumber);
	convert_to_string_ex(optlist);

	pdi_image = PDF_open_pdi_page(pdf,
		Z_LVAL_PP(doc),
		Z_LVAL_PP(pagenumber),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(pdi_image); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_place_pdi_page(int pdfdoc, int page, float x, float y, float sx, float sy)
 * Deprecated, use PDF_fit_pdi_page( ) instead. */
PHP_FUNCTION(pdf_place_pdi_page)
{
	zval **p, **page, **x, **y, **sx, **sy;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 6 || zend_get_parameters_ex(6, &p, &page, &x, &y, &sx, &sy) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(page);
	convert_to_double_ex(x);
	convert_to_double_ex(y);
	convert_to_double_ex(sx);
	convert_to_double_ex(sy);

	PDF_place_pdi_page(pdf,
		Z_LVAL_PP(page),
		(float) Z_DVAL_PP(x),
		(float) Z_DVAL_PP(y),
		(float) Z_DVAL_PP(sx),
		(float) Z_DVAL_PP(sy));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int pdf_process_pdi(int pdfdoc, int doc, int page, string optlist);
 * Perform various actions on a PDI document. */
PHP_FUNCTION(pdf_process_pdi)
{
	zval **p, **doc, **page, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &doc, &page, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(doc);
	convert_to_long_ex(page);
	convert_to_string_ex(optlist);

	retval = PDF_process_pdi(pdf,
		Z_LVAL_PP(doc),
		Z_LVAL_PP(page),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* p_resource.c */

/* TODO [optlist] */
/* {{{ proto void pdf_create_pvf(int pdfdoc, string filename, string data, string optlist);
 * Create a new virtual file. */
PHP_FUNCTION(pdf_create_pvf)
{
	zval **p, **filename, **data, **optlist;
	PDF *pdf;
	int reserved = 0;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &filename, &data, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(filename);
	convert_to_string_ex(data);
	convert_to_string_ex(optlist);

	PDF_create_pvf(pdf,
		Z_STRVAL_PP(filename),
		reserved,
		Z_STRVAL_PP(data),
		Z_STRLEN_PP(data),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE
}
/* }}} */

/* {{{ proto int pdf_delete_pvf(int pdfdoc, string filname);
 * Delete a virtual file. */
PHP_FUNCTION(pdf_delete_pvf)
{
	zval **p, **filename;
	PDF *pdf;
	int retval;
	int reserved = 0;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &filename) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(filename);

	retval = PDF_delete_pvf(pdf,
		Z_STRVAL_PP(filename),
		reserved);

	RETURN_LONG(retval); /* change return from -1 to 0 handled by PDFlib */
}
/* }}} */

/* p_shading.c */

/* TODO [optlist] */
/* {{{ proto int pdf_shading(int pdfdoc, string type, float x0, float y0, float x1, float y1, float c1, float c2, float c3, float c4, string optlist);
 * Define a color blend (smooth shading) from the current fill color to the supplied color. */
PHP_FUNCTION(pdf_shading)
{
	zval **p, **type, **x0, **y0, **x1, **y1, **c1, **c2, **c3, **c4, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 11 || zend_get_parameters_ex(11, &p, &type, &x0, &y0, &x1, &y1, &c1, &c2, &c3, &c4, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(type);
	convert_to_double_ex(x0);
	convert_to_double_ex(y0);
	convert_to_double_ex(x1);
	convert_to_double_ex(y1);
	convert_to_double_ex(c1);
	convert_to_double_ex(c2);
	convert_to_double_ex(c3);
	convert_to_double_ex(c4);
	convert_to_string_ex(optlist);

	retval = PDF_shading(pdf,
		Z_STRVAL_PP(type),
		(float) Z_DVAL_PP(x0),
		(float) Z_DVAL_PP(y0),
		(float) Z_DVAL_PP(x1),
		(float) Z_DVAL_PP(y1),
		(float) Z_DVAL_PP(c1),
		(float) Z_DVAL_PP(c2),
		(float) Z_DVAL_PP(c3),
		(float) Z_DVAL_PP(c4),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto int pdf_shading_pattern(int pdfdoc, int shading, string optlist);
 * Define a shading pattern using a shading object. */
PHP_FUNCTION(pdf_shading_pattern)
{
	zval **p, **shading, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &p, &shading, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(shading);
	convert_to_string_ex(optlist);

	retval = PDF_shading_pattern(pdf,
		Z_LVAL_PP(shading),
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_shfill(int pdfdoc, int shading);
 * Fill an area with a shading, based on a shading object. */
PHP_FUNCTION(pdf_shfill)
{
	zval **p, **shading;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &shading) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_long_ex(shading);

	PDF_shfill(pdf,
		Z_LVAL_PP(shading));

	RETURN_TRUE;
}
/* }}} */

/* p_template.c */

/* {{{ proto int pdf_begin_template(int pdfdoc, float width, float height);
 * Start a new template definition. */
PHP_FUNCTION(pdf_begin_template)
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;
	int tmpl_image;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);

	tmpl_image = PDF_begin_template(pdf,
		(float) Z_DVAL_PP(arg2),
		(float) Z_DVAL_PP(arg3));

	RETURN_LONG(tmpl_image); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_end_template(int pdfdoc);
 * Finish a template definition. */
PHP_FUNCTION(pdf_end_template)
{
	zval **arg1;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &arg1) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);


	PDF_end_template(pdf);

	RETURN_TRUE;
}
/* }}} */

/* p_text.c */

/* {{{ proto void pdf_continue_text(int pdfdoc, string text)
 * Print text at the next line. The spacing between lines is determined by the "leading" parameter. */
PHP_FUNCTION(pdf_continue_text)
{
	zval **p, **text;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &text) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(text);

	PDF_continue_text2(pdf,
		Z_STRVAL_PP(text),
		Z_STRLEN_PP(text));
	RETURN_TRUE;
}
/* }}} */

/* TODO [optlist] */
/* {{{ proto void pdf_fit_textline(int pdfdoc, int text, float x, float y, string optlist);
 * Place an imported PDF page with the lower left corner at (x, y) with
 * various options.*/
PHP_FUNCTION(pdf_fit_textline)
{
	zval **p, **text, **x, **y, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 5 || zend_get_parameters_ex(5, &p, &text, &x, &y, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(text);
	convert_to_double_ex(x);
	convert_to_double_ex(y);
	convert_to_string_ex(optlist);

	PDF_fit_textline(pdf,
		Z_STRVAL_PP(text),
		Z_STRLEN_PP(text),
		(float)Z_DVAL_PP(x),
		(float)Z_DVAL_PP(y),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE
}
/* }}} */

/* {{{ proto void pdf_set_text_pos(int pdfdoc, float x, float y)
 * Set the text output position. */
PHP_FUNCTION(pdf_set_text_pos) 
{
	zval **arg1, **arg2, **arg3;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 3 || zend_get_parameters_ex(3, &arg1, &arg2, &arg3) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_double_ex(arg2);
	convert_to_double_ex(arg3);
	PDF_set_text_pos(pdf, (float) Z_DVAL_PP(arg2), (float) Z_DVAL_PP(arg3));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_show(int pdfdoc, string text)
 * Print text in the current font and size at the current position. */
PHP_FUNCTION(pdf_show) 
{
	zval **arg1, **arg2;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);

	convert_to_string_ex(arg2);
	PDF_show2(pdf, Z_STRVAL_PP(arg2), Z_STRLEN_PP(arg2));
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int pdf_show_boxed(int pdfdoc, string text, float x, float y, float width, float height, string hmode [, string feature])
 * Format text in the current font and size into the supplied text box according to the requested formatting mode, which must be one of "left", "right", "center", "justify", or "fulljustify". If width and height are 0, only a single line is placed at the point (left, top) in the requested mode. */
PHP_FUNCTION(pdf_show_boxed) 
{
	zval **argv[8];
	int argc = ZEND_NUM_ARGS();
	int nr;
	char *feature;
	PDF *pdf;

	if (((argc < 7) || (argc > 8)) || zend_get_parameters_array_ex(argc, argv) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, argv[0], -1, "pdf object", le_pdf);

	convert_to_string_ex(argv[1]);
	convert_to_double_ex(argv[2]);
	convert_to_double_ex(argv[3]);
	convert_to_double_ex(argv[4]);
	convert_to_double_ex(argv[5]);
	convert_to_string_ex(argv[6]);

	if(argc == 8) {
		convert_to_string_ex(argv[7]);
		feature = Z_STRVAL_PP(argv[7]);
	} else {
		feature = NULL;
	}

	nr = PDF_show_boxed(pdf, Z_STRVAL_PP(argv[1]),
							(float) Z_DVAL_PP(argv[2]),
							(float) Z_DVAL_PP(argv[3]),
							(float) Z_DVAL_PP(argv[4]),
							(float) Z_DVAL_PP(argv[5]),
							Z_STRVAL_PP(argv[6]),
							feature);

	RETURN_LONG(nr);
}
/* }}} */

/* {{{ proto void pdf_show_xy(int pdfdoc, string text, float x, float y)
 * Print text in the current font at (x, y). */
PHP_FUNCTION(pdf_show_xy) 
{
	zval **p, **text, **x, **y;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 4 || zend_get_parameters_ex(4, &p, &text, &x, &y) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(text);
	convert_to_double_ex(x);
	convert_to_double_ex(y);

	PDF_show_xy2(pdf,
		Z_STRVAL_PP(text),
		Z_STRLEN_PP(text),
		(float) Z_DVAL_PP(x),
		(float) Z_DVAL_PP(y));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto float pdf_stringwidth(int pdfdoc, string text [, int font, float fontsize])
 * Return the width of text in an arbitrary font. */
PHP_FUNCTION(pdf_stringwidth)
{
	zval **p, **text, **font, **fontsize;
	int p_font;
	double width, p_fontsize;
	PDF *pdf;

	switch (ZEND_NUM_ARGS()) {
	case 2:
		if (zend_get_parameters_ex(2, &p, &text) == FAILURE)
			WRONG_PARAM_COUNT;
		break;
	case 4:
		if (zend_get_parameters_ex(4, &p, &text, &font, &fontsize) == FAILURE)
			WRONG_PARAM_COUNT;
		convert_to_long_ex(font);
		break;
	default:
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(text);
	if (ZEND_NUM_ARGS() == 2) {
	    p_font = (int)PDF_get_value(pdf, "font", 0);
	    p_fontsize = PDF_get_value(pdf, "fontsize", 0);
	} else {
	    convert_to_long_ex(font);
	    p_font = Z_LVAL_PP(font);
	    convert_to_double_ex(fontsize);
	    p_fontsize = Z_DVAL_PP(fontsize);
	}
	width = (double) PDF_stringwidth2(pdf,
				Z_STRVAL_PP(text),
				Z_STRLEN_PP(text),
				p_font,
				(float)p_fontsize);

	RETURN_DOUBLE(width);
}
/* }}} */

/* p_type3.c */

/* TODO [optlist] */
/* {{{ proto void pdf_begin_font(string fontname, int subtype, float a, float b,
    float c, float d, float e, float f, string optlist);
 * Start a type 3 font definition. */
PHP_FUNCTION(pdf_begin_font)
{
	zval **p, **fontname, **a, **b, **c, **d, **e, **f, **optlist;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 9 || zend_get_parameters_ex(9, &p, &fontname, &a, &b, &c, &d, &e, &f, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(fontname);
	convert_to_double_ex(a);
	convert_to_double_ex(b);
	convert_to_double_ex(c);
	convert_to_double_ex(d);
	convert_to_double_ex(e);
	convert_to_double_ex(f);
	convert_to_string_ex(optlist);

	PDF_begin_font(pdf,
		Z_STRVAL_PP(fontname),
		0,
		(float) Z_DVAL_PP(a),
		(float) Z_DVAL_PP(b),
		(float) Z_DVAL_PP(c),
		(float) Z_DVAL_PP(d),
		(float) Z_DVAL_PP(e),
		(float) Z_DVAL_PP(f),
		Z_STRVAL_PP(optlist));

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_begin_glyph(int pdfdoc, string glyphname, float wx, float llx, float lly, float urx, float ury)
 * Start a type 3 glyph definition. */
PHP_FUNCTION(pdf_begin_glyph)
{
        zval **p, **glyphname, **wx, **llx, **lly, **urx, **ury;
        PDF *pdf;

        if (ZEND_NUM_ARGS() != 7 || zend_get_parameters_ex(7, &p, &glyphname, &wx, &llx, &lly, &urx, &ury) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        convert_to_string_ex(glyphname);
        convert_to_double_ex(wx);
        convert_to_double_ex(llx);
        convert_to_double_ex(lly);
        convert_to_double_ex(urx);
        convert_to_double_ex(ury);

        PDF_begin_glyph(pdf,
                Z_STRVAL_PP(glyphname),
                (float) Z_DVAL_PP(wx),
                (float) Z_DVAL_PP(llx),
                (float) Z_DVAL_PP(lly),
                (float) Z_DVAL_PP(urx),
                (float) Z_DVAL_PP(ury));

        RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_end_font(int pdfdoc);
 * Terminate a type 3 font definition. */
PHP_FUNCTION(pdf_end_font)
{
        zval **p;
        PDF *pdf;

        if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &p) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);


        PDF_end_font(pdf);

        RETURN_TRUE;
}
/* }}} */

/* {{{ proto void pdf_end_glyph(int pdfdoc);
 * Terminate a type 3 glyph definition. */
PHP_FUNCTION(pdf_end_glyph)
{
        zval **p;
        PDF *pdf;

        if (ZEND_NUM_ARGS() != 1 || zend_get_parameters_ex(1, &p) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);


        PDF_end_glyph(pdf);

        RETURN_TRUE;
}
/* }}} */

/* p_xgstate.c */

/* {{{ proto int pdf_create_gstate(int pdfdoc, string optlist);
 * Create a gstate object definition. */
PHP_FUNCTION(pdf_create_gstate)
{
	zval **p, **optlist;
	PDF *pdf;
	int retval;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &optlist) == FAILURE) {
		WRONG_PARAM_COUNT;
	}

	ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

	convert_to_string_ex(optlist);

	retval = PDF_create_gstate(pdf,
		Z_STRVAL_PP(optlist));

	RETURN_LONG(retval); /* offset handled in PDFlib Kernel */
}
/* }}} */

/* {{{ proto void pdf_set_gstate(int pdfdoc, int gstate);
 * Activate a gstate object. */
PHP_FUNCTION(pdf_set_gstate)
{
        zval **p, **gstate;
        PDF *pdf;

        if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &p, &gstate) == FAILURE) {
                WRONG_PARAM_COUNT;
        }

        ZEND_FETCH_RESOURCE(pdf, PDF *, p, -1, "pdf object", le_pdf);

        convert_to_long_ex(gstate);

        PDF_set_gstate(pdf,
                Z_LVAL_PP(gstate));

        RETURN_TRUE;
}
/* }}} */

/* End of the official PDFlib GmbH V3.x/V4.x/V5.x API */

#if HAVE_LIBGD13
/* {{{ proto int pdf_open_memory_image(int pdfdoc, int image)
   Takes an GD image and returns an image for placement in a PDF document */
PHP_FUNCTION(pdf_open_memory_image)
{
	zval **arg1, **arg2;
	int i, j, color, count;
	int pdf_image;
	gdImagePtr im;
	unsigned char *buffer, *ptr;
	PDF *pdf;

	if (ZEND_NUM_ARGS() != 2 || zend_get_parameters_ex(2, &arg1, &arg2) == FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(pdf, PDF *, arg1, -1, "pdf object", le_pdf);
	ZEND_GET_RESOURCE_TYPE_ID(le_gd,"gd");
	if(!le_gd)
	{
		php_error(E_ERROR, "Unable to find handle for GD image stream. Please check the GD extension is loaded.");
	}
	ZEND_FETCH_RESOURCE(im, gdImagePtr, arg2, -1, "Image", le_gd);

	count = 3 * im->sx * im->sy;
	buffer = (unsigned char *) emalloc(count);

	ptr = buffer;
	for(i=0; i<im->sy; i++) {
		for(j=0; j<im->sx; j++) {
#if HAVE_LIBGD20
			if(gdImageTrueColor(im)) {
				if (im->tpixels && gdImageBoundsSafe(im, j, i)) {
					color = gdImageTrueColorPixel(im, j, i);
                    *ptr++ = (color >> 16) & 0xFF;
                    *ptr++ = (color >> 8) & 0xFF;
                    *ptr++ = color & 0xFF;
				}
			} else {
#endif
				if (im->pixels && gdImageBoundsSafe(im, j, i)) {
					color = im->pixels[i][j];
					*ptr++ = im->red[color];
					*ptr++ = im->green[color];
					*ptr++ = im->blue[color];
				}
#if HAVE_LIBGD20
			}
#endif
		}
	}

	pdf_image = PDF_open_image(pdf, "raw", "memory", buffer, im->sx*im->sy*3, im->sx, im->sy, 3, 8, NULL);
	efree(buffer);

	if (pdf_image == 0) {
		efree(buffer);
		RETURN_FALSE;
	}

	RETURN_LONG(pdf_image); /* offset handled in PDFlib Kernel */
}
/* }}} */
#endif /* HAVE_LIBGD13 */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
