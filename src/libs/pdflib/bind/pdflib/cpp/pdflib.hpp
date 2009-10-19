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

// $Id: pdflib.hpp 14574 2005-10-29 16:27:43Z bonefish $
//
// in sync with pdflib.h 1.151.2.22
//
// C++ wrapper for PDFlib
//
//

#ifndef PDFLIB_HPP
#define PDFLIB_HPP

#include <string>

	using namespace std;

// We use PDF as a C++ class name, therefore hide the actual C struct
// name for PDFlib usage with C++.

#define PDF_THROWS_CPP_EXCEPTIONS

#include "pdflib.h"


#ifdef	PDF_THROWS_CPP_EXCEPTIONS
#define PDF_THROWS(x)		throw (x)
#define PDF_THROWS_NOTHING	throw ()
#else
#define PDFlib PDF
#define PDF_THROWS(x)
#define PDF_THROWS_NOTHING
#endif


// The C++ class wrapper for PDFlib

class PDFlib {
public:
#ifdef	PDF_THROWS_CPP_EXCEPTIONS
    class Exception
    {
    public:
	Exception(string errmsg, int errnum, string apiname, void *opaque);
	string get_message();
	string get_errmsg();
	int get_errnum();
	string get_apiname();
	const void *get_opaque();
    private:
	string m_errmsg;
	int m_errnum;
	string m_apiname;
	void * m_opaque;
    }; // Exception

    PDFlib(errorproc_t errorproc = NULL,
	allocproc_t allocproc = NULL,
	reallocproc_t reallocproc = NULL,
	freeproc_t freeproc = NULL,
	void *opaque = NULL) PDF_THROWS(Exception);

    PDFlib(allocproc_t allocproc,
	reallocproc_t reallocproc,
	freeproc_t freeproc,
	void *opaque = NULL) PDF_THROWS(Exception);

#else // PDF_THROWS_CPP_EXCEPTIONS

    PDFlib(errorproc_t errorproc = NULL,
	allocproc_t allocproc = NULL,
	reallocproc_t reallocproc = NULL,
	freeproc_t freeproc = NULL,
	void *opaque = NULL);

#endif // PDF_THROWS_CPP_EXCEPTIONS

    ~PDFlib() PDF_THROWS_NOTHING;

    /* p_annots.c */
    void add_launchlink(float llx, float lly, float urx, float ury,
    	string filename) PDF_THROWS(PDFlib::Exception);
    void add_locallink(float llx, float lly, float urx, float ury,
    	int page, string optlist) PDF_THROWS(PDFlib::Exception);
    void add_note(float llx, float lly, float urx, float ury,
    	string contents, string title, string icon, bool open)
	PDF_THROWS(PDFlib::Exception);
    void add_pdflink(float llx, float lly, float urx, float ury,
    	string filename, int page,
	string optlist) PDF_THROWS(PDFlib::Exception);
    void add_weblink(float llx, float lly, float urx, float ury, string url)
	PDF_THROWS(PDFlib::Exception);
    void attach_file(float llx, float lly, float urx, float ury,
    	string filename, string description, string author,
	string mimetype, string icon) PDF_THROWS(PDFlib::Exception);
    void set_border_color(float red, float green, float blue)
	PDF_THROWS(PDFlib::Exception);
    void set_border_dash(float b, float w) PDF_THROWS(PDFlib::Exception);
    void set_border_style(string style, float width)
    	PDF_THROWS(PDFlib::Exception);

    /* p_basic.c */
    void begin_page(float width, float height) PDF_THROWS(PDFlib::Exception);
    void close() PDF_THROWS(PDFlib::Exception);
    void end_page() PDF_THROWS(PDFlib::Exception);
    string get_apiname() PDF_THROWS(PDFlib::Exception);
    const char *get_buffer(long *size) PDF_THROWS(PDFlib::Exception);
    string get_errmsg() PDF_THROWS(PDFlib::Exception);
    int get_errnum() PDF_THROWS(PDFlib::Exception);
    int get_majorversion() PDF_THROWS_NOTHING;
    int get_minorversion() PDF_THROWS_NOTHING;
    void * get_opaque() PDF_THROWS(PDFlib::Exception);
    // Overloaded generic open and close methods
    int  open(string filename) PDF_THROWS(PDFlib::Exception);
    int  open(FILE *fp) PDF_THROWS(PDFlib::Exception);
    void open(writeproc_t writeproc) PDF_THROWS(PDFlib::Exception);
    int open_fp(FILE *fp) PDF_THROWS(PDFlib::Exception);
    int open_file(string filename) PDF_THROWS(PDFlib::Exception);
    void open_mem(writeproc_t writeproc) PDF_THROWS(PDFlib::Exception);

    /* p_block.c */
    int  fill_imageblock(int page, string blockname, int image, string optlist)
	PDF_THROWS(PDFlib::Exception);
    int  fill_pdfblock(int page, string blockname, int contents, string optlist)
	PDF_THROWS(PDFlib::Exception);
    int  fill_textblock(int page, string blockname, string text, string optlist)
	PDF_THROWS(PDFlib::Exception);

    /* p_color.c */
    int makespotcolor(string spotname, int reserved)
    	PDF_THROWS(PDFlib::Exception);
    void setcolor(string fstype, string colorspace,
	float c1, float c2, float c3, float c4) PDF_THROWS(PDFlib::Exception);
    void setgray(float g) PDF_THROWS(PDFlib::Exception);
    void setgray_fill(float g) PDF_THROWS(PDFlib::Exception);
    void setgray_stroke(float g) PDF_THROWS(PDFlib::Exception);
    void setrgbcolor(float red, float green, float blue)
    	PDF_THROWS(PDFlib::Exception);
    void setrgbcolor_fill(float red, float green, float blue)
	PDF_THROWS(PDFlib::Exception);
    void setrgbcolor_stroke(float red, float green, float blue)
	PDF_THROWS(PDFlib::Exception);

    /* p_draw.c */
    void arc(float x, float y, float r, float alpha, float beta)
	PDF_THROWS(PDFlib::Exception);
    void arcn(float x, float y, float r, float alpha, float beta)
	PDF_THROWS(PDFlib::Exception);
    void circle(float x, float y, float r) PDF_THROWS(PDFlib::Exception);
    void clip() PDF_THROWS(PDFlib::Exception);
    void closepath() PDF_THROWS(PDFlib::Exception);
    void closepath_fill_stroke() PDF_THROWS(PDFlib::Exception);
    void closepath_stroke() PDF_THROWS(PDFlib::Exception);
    void curveto(float x1, float y1, float x2, float y2, float x3, float y3)
	PDF_THROWS(PDFlib::Exception);
    void endpath() PDF_THROWS(PDFlib::Exception);
    void fill() PDF_THROWS(PDFlib::Exception);
    void fill_stroke() PDF_THROWS(PDFlib::Exception);
    void lineto(float x, float y) PDF_THROWS(PDFlib::Exception);
    void moveto(float x, float y) PDF_THROWS(PDFlib::Exception);
    void rect(float x, float y, float width, float height)
	PDF_THROWS(PDFlib::Exception);
    void stroke() PDF_THROWS(PDFlib::Exception);

    /* p_encoding.c */
    void encoding_set_char(string encoding, int slot, string glyphname,
	int uv) PDF_THROWS(PDFlib::Exception);

    /* p_font.c */
    int  findfont(string fontname, string encoding, int embed)
	PDF_THROWS(PDFlib::Exception);
    int  load_font(string fontname, string encoding, string optlist)
	PDF_THROWS(PDFlib::Exception);
    void setfont(int font, float fontsize) PDF_THROWS(PDFlib::Exception);

    /* p_gstate.c */
    void concat(float a, float b, float c, float d, float e, float f)
	PDF_THROWS(PDFlib::Exception);
    void initgraphics() PDF_THROWS(PDFlib::Exception);
    void restore() PDF_THROWS(PDFlib::Exception);
    void rotate(float phi) PDF_THROWS(PDFlib::Exception);
    void save() PDF_THROWS(PDFlib::Exception);
    void scale(float sx, float sy) PDF_THROWS(PDFlib::Exception);
    void setdash(float b, float w) PDF_THROWS(PDFlib::Exception);
    void setdashpattern(string optlist) PDF_THROWS(PDFlib::Exception);
    void setflat(float flatness) PDF_THROWS(PDFlib::Exception);
    void setlinecap(int linecap) PDF_THROWS(PDFlib::Exception);
    void setlinejoin(int linejoin) PDF_THROWS(PDFlib::Exception);
    void setlinewidth(float width) PDF_THROWS(PDFlib::Exception);
    void setmatrix(float a, float b, float c, float d, float e, float f)
	PDF_THROWS(PDFlib::Exception);
    void setmiterlimit(float miter) PDF_THROWS(PDFlib::Exception);
    void setpolydash(float *darray, int length) PDF_THROWS(PDFlib::Exception);
    void skew(float alpha, float beta) PDF_THROWS(PDFlib::Exception);
    void translate(float tx, float ty) PDF_THROWS(PDFlib::Exception);

    /* p_hyper.c */
    int add_bookmark(string text, int parent, bool open)
    	PDF_THROWS(PDFlib::Exception);
    void add_nameddest (string name, string optlist) PDF_THROWS(PDFlib::Exception);
    void set_info(string key, string value) PDF_THROWS(PDFlib::Exception);

    /* p_icc.c */
    int  load_iccprofile(string profilename, string optlist)
	PDF_THROWS(PDFlib::Exception);

    /* p_image.c */
    void add_thumbnail(int image) PDF_THROWS(PDFlib::Exception);
    void close_image(int image) PDF_THROWS(PDFlib::Exception);
    void fit_image (int image, float x, float y, string optlist)
	PDF_THROWS(PDFlib::Exception);
    int load_image (string imagetype, string filename, string optlist)
	PDF_THROWS(PDFlib::Exception);
    int open_CCITT(string filename, int width, int height,
    	bool BitReverse, int K, bool BlackIs1) PDF_THROWS(PDFlib::Exception);
    int open_image(string imagetype, string source, const char *data,
	long len, int width, int height, int components, int bpc,
	string params) PDF_THROWS(PDFlib::Exception);
    int open_image_file(string imagetype, string filename,
    	string stringparam, int intparam) PDF_THROWS(PDFlib::Exception);
    void place_image(int image, float x, float y, float scale)
	PDF_THROWS(PDFlib::Exception);

    /* p_params.c */
    string get_parameter(string key, float modifier)
    	PDF_THROWS(PDFlib::Exception);
    float get_value(string key, float modifier) PDF_THROWS(PDFlib::Exception);
    void set_parameter(string key, string value) PDF_THROWS(PDFlib::Exception);
    void set_value(string key, float value) PDF_THROWS(PDFlib::Exception);

    /* p_pattern.c */
    int begin_pattern(float width, float height, float xstep, float ystep,
	int painttype) PDF_THROWS(PDFlib::Exception);
    void end_pattern() PDF_THROWS(PDFlib::Exception);

    /* p_pdi.c */
    void close_pdi(int doc) PDF_THROWS(PDFlib::Exception);
    void close_pdi_page(int page) PDF_THROWS(PDFlib::Exception);
    void fit_pdi_page (int page, float x, float y, string optlist)
	PDF_THROWS(PDFlib::Exception);
    string get_pdi_parameter(string key, int doc, int page, int reserved,
	int *len) PDF_THROWS(PDFlib::Exception);
    float get_pdi_value(string key, int doc, int page, int reserved)
	PDF_THROWS(PDFlib::Exception);
    int open_pdi(string filename, string stringparam, int reserved)
	PDF_THROWS(PDFlib::Exception);
    int open_pdi_page(int doc, int pagenumber, string optlist)
	PDF_THROWS(PDFlib::Exception);
    void place_pdi_page(int page, float x, float y, float sx, float sy)
	PDF_THROWS(PDFlib::Exception);
    int process_pdi(int doc, int page, string optlist)
	PDF_THROWS(PDFlib::Exception);

    /* p_resource.c */
    void create_pvf(string filename, void *data, size_t size, string options)
	PDF_THROWS(PDFlib::Exception);
    int delete_pvf(string filename) PDF_THROWS(PDFlib::Exception);

    /* p_shading.c */
    int shading (string shtype, float x0, float y0, float x1, float y1,
    	float c1, float c2, float c3, float c4, string optlist)
	PDF_THROWS(PDFlib::Exception);
    int shading_pattern (int shading, string optlist)
    	PDF_THROWS(PDFlib::Exception);
    void shfill (int shading) PDF_THROWS(PDFlib::Exception);

    /* p_template.c */
    int begin_template(float width, float height) PDF_THROWS(PDFlib::Exception);
    void end_template() PDF_THROWS(PDFlib::Exception);

    /* p_text.c */
    void continue_text(string text) PDF_THROWS(PDFlib::Exception);
    void fit_textline(string text, float x, float y, string optlist)
	PDF_THROWS(PDFlib::Exception);
    void set_text_pos(float x, float y) PDF_THROWS(PDFlib::Exception);
    void show(string text) PDF_THROWS(PDFlib::Exception);
    int show_boxed(string text, float left, float top,
	float width, float height, string hmode, string feature)
	PDF_THROWS(PDFlib::Exception);
    void show_xy(string text, float x, float y) PDF_THROWS(PDFlib::Exception);
    float stringwidth(string text, int font, float fontsize)
	PDF_THROWS(PDFlib::Exception);

    /* p_type3.c */
    void begin_font(string fontname, float a, float b, float c, float d,
	float e, float f, string optlist) PDF_THROWS(PDFlib::Exception);
    void begin_glyph(string glyphname, float wx, float llx, float lly,
	float urx, float ury) PDF_THROWS(PDFlib::Exception);
    void end_font() PDF_THROWS(PDFlib::Exception);
    void end_glyph() PDF_THROWS(PDFlib::Exception);

    /* p_xgstate.c */
    int create_gstate (string optlist) PDF_THROWS(PDFlib::Exception);
    void set_gstate(int gstate) PDF_THROWS(PDFlib::Exception);


private:
    PDF *p;
    const PDFlib_api *m_PDFlib_api;
};

#endif	// PDFLIB_HPP
