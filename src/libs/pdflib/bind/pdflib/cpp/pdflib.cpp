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

// $Id: pdflib.cpp 14574 2005-10-29 16:27:43Z bonefish $
//
// in sync with pdflib.h 1.151.2.22
//
// Implementation of C++ wrapper for PDFlib
//
//

#include "pdflib.hpp"

#define CHAR(s)	(s).c_str()
#define LEN(s)	(s).size()


#ifdef	PDF_THROWS_CPP_EXCEPTIONS

PDFlib::Exception::Exception(string errmsg, int errnum, string apiname, void *opaque)
: m_errmsg(errmsg),
  m_errnum(errnum),
  m_apiname(apiname),
  m_opaque(opaque)
{ }

string PDFlib::Exception::get_errmsg() { return m_errmsg; }
string PDFlib::Exception::get_message() { return m_errmsg; }
int PDFlib::Exception::get_errnum() { return m_errnum; }
string PDFlib::Exception::get_apiname() { return m_apiname; }
const void * PDFlib::Exception::get_opaque() { return m_opaque; }

#define PDFCPP_TRY	PDF_TRY(p)
#define PDFCPP_CATCH  \
PDF_CATCH(p) {\
    throw Exception(PDF_get_errmsg(p), PDF_get_errnum(p),\
			    PDF_get_apiname(p), PDF_get_opaque(p));\
}
#else

#define PDFCPP_TRY
#define PDFCPP_CATCH

#endif // PDF_THROWS_CPP_EXCEPTIONS


#ifdef	PDF_THROWS_CPP_EXCEPTIONS
PDFlib::PDFlib(
    errorproc_t errorproc,
    allocproc_t allocproc,
    reallocproc_t reallocproc,
    freeproc_t freeproc,
    void *opaque) PDF_THROWS(PDFlib::Exception)
{
    m_PDFlib_api = ::PDF_get_api();

    if (m_PDFlib_api->sizeof_PDFlib_api != sizeof(PDFlib_api) ||
	m_PDFlib_api->major != PDFLIB_MAJORVERSION ||
	m_PDFlib_api->minor != PDFLIB_MINORVERSION) {
	throw Exception("loaded wrong version of PDFlib library", 0,
		"pdflib.cpp", opaque);
    }

    m_PDFlib_api->PDF_boot();

    p = m_PDFlib_api->PDF_new2(NULL, allocproc, reallocproc, freeproc, opaque);

/*
 * errorproc is ignored here to be compatible with old applications
 * that were not compiled with PDF_THROWS_CPP_EXCEPTIONS
 */
    (void) errorproc;

    if (p == (PDF *)0) {
	throw Exception("No memory for PDFlib object", 0, "pdflib.cpp", opaque);
    }

    PDFCPP_TRY
    {
	PDF_set_parameter(p, "binding", "C++");
    }
    PDFCPP_CATCH;
}

PDFlib::PDFlib(
    allocproc_t allocproc,
    reallocproc_t reallocproc,
    freeproc_t freeproc,
    void *opaque) PDF_THROWS(PDFlib::Exception)
{
    m_PDFlib_api = ::PDF_get_api();

    if (m_PDFlib_api->sizeof_PDFlib_api != sizeof(PDFlib_api) ||
	m_PDFlib_api->major != PDFLIB_MAJORVERSION ||
	m_PDFlib_api->minor != PDFLIB_MINORVERSION) {
	throw Exception("loaded wrong version of PDFlib library", 0,
		"pdflib.cpp", opaque);
    }

    m_PDFlib_api->PDF_boot();

    p = m_PDFlib_api->PDF_new2(NULL, allocproc, reallocproc, freeproc, opaque);

    if (p == (PDF *)0) {
	throw Exception("No memory for PDFlib object", 0, "pdflib.cpp", opaque);
    }

    PDFCPP_TRY
    {
	PDF_set_parameter(p, "binding", "C++");
    }
    PDFCPP_CATCH;
}

#else // ! PDF_THROWS_CPP_EXCEPTIONS

PDFlib::PDFlib(
    errorproc_t errorproc,
    allocproc_t allocproc,
    reallocproc_t reallocproc,
    freeproc_t freeproc,
    void *opaque)
{
    PDF_boot();
    m_PDFlib_api = ::PDF_get_api();

    p = m_PDFlib_api->PDF_new2(errorproc, allocproc, reallocproc, freeproc, opaque);

    PDF_set_parameter(p, "binding", "C++");
}

#endif // ! PDF_THROWS_CPP_EXCEPTIONS


PDFlib::~PDFlib() PDF_THROWS_NOTHING
{
    m_PDFlib_api->PDF_delete(p);
    m_PDFlib_api->PDF_shutdown();
}

/* p_annots.c */
void
PDFlib::add_launchlink(float llx, float lly, float urx, float ury,
    string filename) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_launchlink(p, llx, lly, urx, ury, CHAR(filename));
    PDFCPP_CATCH;
}

void
PDFlib::add_locallink(float llx, float lly, float urx, float ury, int page,
    string optlist) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_locallink(p, llx, lly, urx, ury, page, CHAR(optlist));
    PDFCPP_CATCH;
}

void
PDFlib::add_note(float llx, float lly, float urx, float ury, string contents,
    string title, string icon, bool p_open) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_note(p, llx, lly, urx, ury, CHAR(contents),
    	CHAR(title), CHAR(icon), p_open);
    PDFCPP_CATCH;
}

void
PDFlib::add_pdflink(float llx, float lly, float urx, float ury,
    string filename, int page, string optlist) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
    {
	m_PDFlib_api->PDF_add_pdflink(p, llx, lly, urx, ury, CHAR(filename),
			    page, CHAR(optlist));
    }
    PDFCPP_CATCH;
}

void
PDFlib::add_weblink(float llx, float lly, float urx, float ury,
    string url) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_weblink(p, llx, lly, urx, ury, CHAR(url));
    PDFCPP_CATCH;
}

void
PDFlib::attach_file(float llx, float lly, float urx, float ury,
    string filename, string description, string author,
    string mimetype, string icon) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
    {
	m_PDFlib_api->PDF_attach_file2(p, llx, lly, urx, ury, CHAR(filename), 0,
	    CHAR(description), (int) LEN(description), CHAR(author),
	    (int) LEN(author), CHAR(mimetype), CHAR(icon));
    }
    PDFCPP_CATCH;
}

void
PDFlib::set_border_color(float red, float green, float blue)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_border_color(p, red, green, blue);
    PDFCPP_CATCH;
}

void
PDFlib::set_border_dash(float b, float w) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_border_dash(p, b, w);
    PDFCPP_CATCH;
}

void
PDFlib::set_border_style(string style, float width) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_border_style(p, CHAR(style), width);
    PDFCPP_CATCH;
}

/* p_basic.c */

void
PDFlib::begin_page(float width, float height) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_begin_page(p, width, height);
    PDFCPP_CATCH;
}

void
PDFlib::close() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_close(p);
    PDFCPP_CATCH;
}

void
PDFlib::end_page() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_end_page(p);
    PDFCPP_CATCH;
}

string
PDFlib::get_apiname() PDF_THROWS(PDFlib::Exception)
{
    const char *retval = NULL;

    PDFCPP_TRY 
    {
	retval = m_PDFlib_api->PDF_get_apiname(p);
    }
    PDFCPP_CATCH;

    if (retval)
	return retval;
    else
	return "";
}

const char *
PDFlib::get_buffer(long *size) PDF_THROWS(PDFlib::Exception)
{
    const char * retval = NULL;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_get_buffer(p, size);
    PDFCPP_CATCH;

    return retval;
}

string
PDFlib::get_errmsg() PDF_THROWS(PDFlib::Exception)
{
    const char *retval = NULL;

    PDFCPP_TRY 
    {
	retval = m_PDFlib_api->PDF_get_errmsg(p);
    }
    PDFCPP_CATCH;

    if (retval)
	return retval;
    else
	return "";
}

int
PDFlib::get_errnum() PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY retval = m_PDFlib_api->PDF_get_errnum(p);
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::get_majorversion() PDF_THROWS_NOTHING
{
    return (int) m_PDFlib_api->PDF_get_value(NULL, "major", 0);
}

int
PDFlib::get_minorversion() PDF_THROWS_NOTHING
{
    return (int) m_PDFlib_api->PDF_get_value(NULL, "minor", 0);
}

void *
PDFlib::get_opaque() PDF_THROWS(PDFlib::Exception)
{
    void * retval = NULL;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_get_opaque(p);
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open(string filename) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_open_file(p, CHAR(filename));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open(FILE *fp) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_open_fp(p, fp);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::open(writeproc_t writeproc) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_open_mem(p, writeproc);
    PDFCPP_CATCH;
}

int
PDFlib::open_file(string filename) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_open_file(p, CHAR(filename));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_fp(FILE *fp) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_open_fp(p, fp);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::open_mem(writeproc_t writeproc) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_open_mem(p, writeproc);
    PDFCPP_CATCH;
}

/* p_block.c */
int
PDFlib::fill_imageblock(int page, string blockname, int image, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY retval = m_PDFlib_api->PDF_fill_imageblock(p, page, CHAR(blockname),
				image, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::fill_pdfblock(int page, string blockname, int contents, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_fill_pdfblock(p, page, CHAR(blockname),
				contents, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::fill_textblock(int page, string blockname, string text, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_fill_textblock(p, page, CHAR(blockname),
			    CHAR(text), (int) LEN(text), CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

/* p_color.c */

int
PDFlib::makespotcolor(string spotname, int reserved) PDF_THROWS(PDFlib::Exception)
{
	(void) reserved;
	
    int retval = 0;

    PDFCPP_TRY  retval = m_PDFlib_api->PDF_makespotcolor(p, CHAR(spotname), (int) LEN(spotname));
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::setcolor(string fstype, string colorspace,
    float c1, float c2, float c3, float c4) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
	m_PDFlib_api->PDF_setcolor(p, CHAR(fstype), CHAR(colorspace), c1, c2, c3, c4);
    PDFCPP_CATCH;
}

void
PDFlib::setgray(float g) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "fillstroke", "gray", g, 0, 0, 0);
    PDFCPP_CATCH;
}

void
PDFlib::setgray_fill(float g) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "fill", "gray", g, 0, 0, 0);
    PDFCPP_CATCH;
}

void
PDFlib::setgray_stroke(float g) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "stroke", "gray", g, 0, 0, 0);
    PDFCPP_CATCH;
}

void
PDFlib::setrgbcolor(float red, float green, float blue)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "fillstroke", "rgb", red, green, blue, 0);
    PDFCPP_CATCH;
}

void
PDFlib::setrgbcolor_fill(float red, float green, float blue)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "fill", "rgb", red, green, blue, 0);
    PDFCPP_CATCH;
}

void
PDFlib::setrgbcolor_stroke(float red, float green, float blue)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setcolor(p, "stroke", "rgb", red, green, blue, 0);
    PDFCPP_CATCH;
}

/* p_draw.c */

void
PDFlib::arc(float x, float y, float r, float alpha, float beta)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_arc(p, x, y, r, alpha, beta);
    PDFCPP_CATCH;
}

void
PDFlib::arcn(float x, float y, float r, float alpha, float beta)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_arcn(p, x, y, r, alpha, beta);
    PDFCPP_CATCH;
}

void
PDFlib::circle(float x, float y, float r) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_circle(p, x, y, r);
    PDFCPP_CATCH;
}

void
PDFlib::clip() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_clip(p);
    PDFCPP_CATCH;
}

void
PDFlib::closepath() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_closepath(p);
    PDFCPP_CATCH;
}

void
PDFlib::closepath_fill_stroke() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_closepath_fill_stroke(p);
    PDFCPP_CATCH;
}

void
PDFlib::closepath_stroke() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_closepath_stroke(p);
    PDFCPP_CATCH;
}

void
PDFlib::curveto(float x1, float y1, float x2, float y2, float x3, float y3)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_curveto(p, x1, y1, x2, y2, x3, y3);
    PDFCPP_CATCH;
}

void
PDFlib::endpath() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_endpath(p);
    PDFCPP_CATCH;
}

void
PDFlib::fill() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_fill(p);
    PDFCPP_CATCH;
}

void
PDFlib::fill_stroke() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_fill_stroke(p);
    PDFCPP_CATCH;
}

void
PDFlib::lineto(float x, float y) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_lineto(p, x, y);
    PDFCPP_CATCH;
}

void
PDFlib::moveto(float x, float y) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_moveto(p, x, y);
    PDFCPP_CATCH;
}

void
PDFlib::rect(float x, float y, float width, float height)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_rect(p, x, y, width, height);
    PDFCPP_CATCH;
}

void
PDFlib::stroke() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_stroke(p);
    PDFCPP_CATCH;
}

/* p_encoding.c */

void
PDFlib::encoding_set_char(string encoding, int slot, string glyphname, int uv)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_encoding_set_char(p, CHAR(encoding), slot,
	CHAR(glyphname), uv);
    PDFCPP_CATCH;
}

/* p_font.c */

int
PDFlib::findfont(string fontname, string encoding, int embed)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;
    PDFCPP_TRY

	retval = m_PDFlib_api->PDF_findfont(p, CHAR(fontname),
		CHAR(encoding), embed);
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::load_font(string fontname, string encoding, string optlist)
        PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;
    PDFCPP_TRY
       retval = m_PDFlib_api->PDF_load_font(p, CHAR(fontname), 0, CHAR(encoding),
		    CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::setfont(int font, float fontsize) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setfont(p, font, fontsize);
    PDFCPP_CATCH;
}

/* p_gstate.c */

void
PDFlib::concat(float a, float b, float c, float d, float e, float f)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_concat(p, a, b, c, d, e, f);
    PDFCPP_CATCH;
}

void
PDFlib::initgraphics() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_initgraphics(p);
    PDFCPP_CATCH;
}

void
PDFlib::restore() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_restore(p);
    PDFCPP_CATCH;
}

void
PDFlib::rotate(float phi) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_rotate(p, phi);
    PDFCPP_CATCH;
}

void
PDFlib::save() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_save(p);
    PDFCPP_CATCH;
}

void
PDFlib::scale(float sx, float sy) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_scale(p, sx, sy);
    PDFCPP_CATCH;
}

void
PDFlib::setdash(float b, float w) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setdash(p, b, w);
    PDFCPP_CATCH;
}

void
PDFlib::setdashpattern(string optlist) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setdashpattern(p, CHAR(optlist));
    PDFCPP_CATCH;
}

void
PDFlib::setflat(float flatness) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setflat(p, flatness);
    PDFCPP_CATCH;
}

void
PDFlib::setlinecap(int linecap) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setlinecap(p, linecap);
    PDFCPP_CATCH;
}

void
PDFlib::setlinejoin(int linejoin) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setlinejoin(p, linejoin);
    PDFCPP_CATCH;
}

void
PDFlib::setlinewidth(float width) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setlinewidth(p, width);
    PDFCPP_CATCH;
}

void
PDFlib::setmatrix( float a, float b, float c, float d, float e, float f)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setmatrix(p, a, b, c, d, e, f);
    PDFCPP_CATCH;
}

void
PDFlib::setmiterlimit(float miter) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setmiterlimit(p, miter);
    PDFCPP_CATCH;
}

void
PDFlib::setpolydash(float *darray, int length) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_setpolydash(p, darray, length);
    PDFCPP_CATCH;
}

void
PDFlib::skew(float alpha, float beta) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_skew(p, alpha, beta);
    PDFCPP_CATCH;
}

void
PDFlib::translate(float tx, float ty) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_translate(p, tx, ty);
    PDFCPP_CATCH;
}

/* p_hyper.c */

int
PDFlib::add_bookmark(string text, int parent, bool p_open)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;
    PDFCPP_TRY
      retval = m_PDFlib_api->PDF_add_bookmark2(p, CHAR(text), (int) LEN(text),
		    parent, p_open);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::add_nameddest(string name, string optlist) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_nameddest(p, CHAR(name), 0, CHAR(optlist));
    PDFCPP_CATCH;
}

void
PDFlib::set_info(string key, string value) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
	m_PDFlib_api->PDF_set_info2(p, CHAR(key), CHAR(value),
		(int) LEN(value));
    PDFCPP_CATCH;
}

/* p_icc.c */

int
PDFlib::load_iccprofile(string profilename, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_load_iccprofile(p, CHAR(profilename), 0,
		CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

/* p_image.c */

void
PDFlib::add_thumbnail(int image) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_add_thumbnail(p, image);
    PDFCPP_CATCH;
}

void
PDFlib::close_image(int image) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_close_image(p, image);
    PDFCPP_CATCH;
}

void
PDFlib::fit_image (int image, float x, float y, string optlist)
        PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_fit_image(p, image, x, y, CHAR(optlist));
    PDFCPP_CATCH;
}

int
PDFlib::load_image (string imagetype, string filename, string optlist)
        PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_load_image(p, CHAR(imagetype),
		CHAR(filename),0, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_CCITT(string filename, int width, int height, bool BitReverse,
    int K, bool BlackIs1) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
    {
	retval = m_PDFlib_api->PDF_open_CCITT(p, CHAR(filename), width, height,
				    BitReverse, K, BlackIs1);
    }
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_image(string imagetype, string source, const char *data, long len,
    int width, int height, int components, int bpc, string params)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
    {
	retval = m_PDFlib_api->PDF_open_image(p, CHAR(imagetype), CHAR(source),
			data, len, width, height, components, bpc, CHAR(params));
    }
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_image_file(string imagetype, string filename,
    string stringparam, int intparam) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
    {
	retval = m_PDFlib_api->PDF_open_image_file(p, CHAR(imagetype),
		    CHAR(filename), CHAR(stringparam), intparam);
    }
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::place_image(int image, float x, float y, float p_scale)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_place_image(p, image, x, y, p_scale);
    PDFCPP_CATCH;
}

/* p_params.c */

string
PDFlib::get_parameter(string key, float modifier) PDF_THROWS(PDFlib::Exception)
{
    const char *retval = NULL;

    PDFCPP_TRY
    {
        retval = m_PDFlib_api->PDF_get_parameter(p, CHAR(key), modifier);
    }
    PDFCPP_CATCH;

    if (retval)
	return retval;
    else
	return "";
}

float
PDFlib::get_value(string key, float modifier) PDF_THROWS(PDFlib::Exception)
{
    float retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_get_value(p, CHAR(key), modifier);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::set_parameter(string key, string value) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_parameter(p, CHAR(key), CHAR(value));
    PDFCPP_CATCH;
}

void
PDFlib::set_value(string key, float value) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_value(p, CHAR(key), value);
    PDFCPP_CATCH;
}

/* p_pattern.c */

int
PDFlib::begin_pattern(float width, float height, float xstep, float ystep,
    int painttype) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY

	retval = m_PDFlib_api->PDF_begin_pattern(p, width, height,
		xstep, ystep, painttype);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::end_pattern() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_end_pattern(p);
    PDFCPP_CATCH;
}

/* p_pdi.c */

void
PDFlib::close_pdi(int doc) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_close_pdi(p, doc);
    PDFCPP_CATCH;
}

void
PDFlib::close_pdi_page(int page) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_close_pdi_page(p, page);
    PDFCPP_CATCH;
}

void
PDFlib::fit_pdi_page (int page, float x, float y, string optlist)
        PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_fit_pdi_page(p, page, x, y, CHAR(optlist));
    PDFCPP_CATCH;
}

string
PDFlib::get_pdi_parameter(string key, int doc, int page, int reserved, int *len)
    PDF_THROWS(PDFlib::Exception)
{
    const char *retval = NULL;

    PDFCPP_TRY
        retval = m_PDFlib_api->PDF_get_pdi_parameter(p, CHAR(key),
		    doc, page, reserved, len);
    PDFCPP_CATCH;

    if (retval)
	return retval;
    else
	return "";
}

float
PDFlib::get_pdi_value(string key, int doc, int page, int reserved)
    PDF_THROWS(PDFlib::Exception)
{
    float retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_get_pdi_value(p, CHAR(key), doc, page,reserved);
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_pdi(string filename, string optlist, int reserved)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_open_pdi(p, CHAR(filename),
		CHAR(optlist), reserved);
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::open_pdi_page(int doc, int pagenumber, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;
    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_open_pdi_page(p, doc, pagenumber,
		    CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::place_pdi_page(int page, float x, float y, float sx, float sy)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_place_pdi_page(p, page, x, y, sx, sy);
    PDFCPP_CATCH;
}

int
PDFlib::process_pdi(int doc, int page, string optlist)
        PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_process_pdi(p, doc, page, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

/* p_resource.c */

void
PDFlib::create_pvf(string filename, void *data, size_t size, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
	m_PDFlib_api->PDF_create_pvf(p, CHAR(filename), 0, data, size, CHAR(optlist));
    PDFCPP_CATCH;
}

int
PDFlib::delete_pvf(string filename) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_delete_pvf(p, CHAR(filename), 0);
    PDFCPP_CATCH;

    return retval;
}

/* p_shading.c */
int
PDFlib::shading (string shtype, float x0, float y0, float x1, float y1,
    float c1, float c2, float c3, float c4, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_shading(p, CHAR(shtype), x0, y0, x1, y1,
		    c1, c2, c3, c4, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

int
PDFlib::shading_pattern (int shade, string optlist) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_shading_pattern(p, shade, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::shfill (int shade) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
	m_PDFlib_api->PDF_shfill(p, shade);
    PDFCPP_CATCH;
}

/* p_template.c */

int
PDFlib::begin_template(float width, float height)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY	retval = m_PDFlib_api->PDF_begin_template(p, width, height);
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::end_template() PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_end_template(p);
    PDFCPP_CATCH;
}

/* p_text.c */

void
PDFlib::continue_text(string text) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_continue_text2(p, CHAR(text), (int) LEN(text));
    PDFCPP_CATCH;
}

void
PDFlib::fit_textline(string text, float x, float y, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY
        m_PDFlib_api->PDF_fit_textline(p, CHAR(text), (int) LEN(text), x, y, CHAR(optlist));
    PDFCPP_CATCH;
}

void
PDFlib::set_text_pos(float x, float y) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_text_pos(p, x, y);
    PDFCPP_CATCH;
}

void
PDFlib::show(string text) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_show2(p, CHAR(text), (int) LEN(text));
    PDFCPP_CATCH;
}

int
PDFlib::show_boxed(string text, float left, float top,
    float width, float height, string hmode, string feature)
    PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
    {
	retval = m_PDFlib_api->PDF_show_boxed(p, CHAR(text), left, top, width,
		    height, CHAR(hmode), CHAR(feature));
    }
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::show_xy(string text, float x, float y) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_show_xy2(p, CHAR(text), (int) LEN(text), x, y);
    PDFCPP_CATCH;
}

float
PDFlib::stringwidth(string text, int font, float fontsize)
    PDF_THROWS(PDFlib::Exception)
{
    float retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_stringwidth2(p, CHAR(text),
		    (int) LEN(text), font, fontsize);
    PDFCPP_CATCH;

    return retval;
}

/* p_type3.c */

void
PDFlib::begin_font(string fontname, float a, float b,
    float c, float d, float e, float f, string optlist)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_begin_font(p, CHAR(fontname), 0,
	a, b, c, d, e, f, CHAR(optlist));
    PDFCPP_CATCH;
}

void
PDFlib::begin_glyph(string glyphname, float wx, float llx, float lly,
    float urx, float ury) PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_begin_glyph(p, CHAR(glyphname), wx, llx, lly, urx, ury);
    PDFCPP_CATCH;
}

void
PDFlib::end_font()
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_end_font(p);
    PDFCPP_CATCH;
}

void
PDFlib::end_glyph()
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_end_glyph(p);
    PDFCPP_CATCH;
}

/* p_xgstate.c */

int
PDFlib::create_gstate (string optlist) PDF_THROWS(PDFlib::Exception)
{
    int retval = 0;

    PDFCPP_TRY
	retval = m_PDFlib_api->PDF_create_gstate(p, CHAR(optlist));
    PDFCPP_CATCH;

    return retval;
}

void
PDFlib::set_gstate(int gstate)
    PDF_THROWS(PDFlib::Exception)
{
    PDFCPP_TRY	m_PDFlib_api->PDF_set_gstate(p, gstate);
    PDFCPP_CATCH;
}
