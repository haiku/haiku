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

/* $Id: pdflib.java 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib Java class
 */

package com.pdflib;

import java.io.*;

/** PDFlib -- a library for generating PDF on the fly.

    Note that this is only a syntax summary. It covers several products:
    PDFlib Lite, PDFlib, PDFlib+PDI, and PDFlib Personalization Server (PPS).
    Not all features are available in all products, although dummies for
    all missing API functions are provided. A comparison which
    details function availability in different products is available at
    http://www.pdflib.com.

    For complete information please refer to the PDFlib API reference
    manual which is available in the PDF file PDFlib-manual.pdf in the
    PDFlib distribution.

    @author Thomas Merz
    @version 5.0.3
*/

public final class pdflib {

    // The initialization code for loading the PDFlib shared library.
    // The library name will be transformed into something platform-
    // specific by the VM, e.g. libpdf_java.so or pdf_java.dll.

    static {
	try {
	    System.loadLibrary("pdf_java");
	} catch (UnsatisfiedLinkError e) {
	    System.err.println(
	"Cannot load the PDFlib shared library/DLL for Java.\n" +
	"Make sure to properly install the native PDFlib library.\n\n" +
	"For your information, the current value of java.library.path is:\n" +
	 System.getProperty("java.library.path") + "\n");

	    throw e;
	}
	PDF_boot();
    }

    // ------------------------------------------------------------------------
    // public functions

    /* p_annots.c: */

    /** Add a launch annotation (to a target of arbitrary file type).
	@exception com.pdflib.PDFlibException
	PDF output cannot be finished after an exception.
    */
    public final void add_launchlink(
    float llx, float lly, float urx, float ury, String filename)
    throws PDFlibException
    {
	PDF_add_launchlink(p, llx, lly, urx, ury, filename);
    }

    /** Add a link annotation to a target within the current PDF file.
	@exception com.pdflib.PDFlibException
	PDF output cannot be finished after an exception.
    */
    public final void add_locallink(
    float llx, float lly, float urx, float ury, int page, String optlist)
    throws PDFlibException
    {
	PDF_add_locallink(p, llx, lly, urx, ury, page, optlist);
    }

    /**  Add a note annotation. icon is one of of "comment", "insert", "note",
        "paragraph", "newparagraph", "key", or "help".
	@exception com.pdflib.PDFlibException
	PDF output cannot be finished after an exception.
     */
    public final void add_note(
    float llx, float lly, float urx, float ury,
    String contents, String title, String icon, int open)
    throws PDFlibException
    {
	PDF_add_note(p, llx, lly, urx, ury, contents, title, icon, open);
    }

    /** Add a file link annotation (to a PDF target).
	@exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void add_pdflink(
    float llx, float lly, float urx, float ury,
    String filename, int page, String optlist)
    throws PDFlibException
    {
	PDF_add_pdflink(p, llx, lly, urx, ury, filename, page, optlist);
    }

    /** Add a weblink annotation to a target URL on the Web.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void add_weblink(
    float llx, float lly, float urx, float ury, String url)
    throws PDFlibException
    {
	PDF_add_weblink(p, llx, lly, urx, ury, url);
    }

    /** Add a file attachment annotation. icon is one of "graph", "paperclip",
       "pushpin", or "tag".
	@exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void attach_file(
    float llx, float lly, float urx, float ury, String filename,
    String description, String author, String mimetype, String icon)
    throws PDFlibException
    {
	PDF_attach_file(p, llx, lly, urx, ury, filename,
	    description, author, mimetype, icon);
    }

    /** Set the border color for all kinds of annotations.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_border_color(float red, float green, float blue)
    throws PDFlibException
    {
	PDF_set_border_color(p, red, green, blue);
    }

    /** Set the border dash style for all kinds of annotations.
        See PDF_setdash().
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void set_border_dash(float b, float w)
    throws PDFlibException
    {
	PDF_set_border_dash(p, b, w);
    }

    /** Set the border style for all kinds of annotations. style is "solid" or
        "dashed".
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void set_border_style(String style, float width)
    throws PDFlibException
    {
	PDF_set_border_style(p, style, width);
    }

    /* p_basic.c */

    /** Add a new page to the document.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void begin_page(float width, float height)
    throws PDFlibException
    {
	PDF_begin_page(p, width, height);
    }

    /** Close the generated PDF file, and release all document-related
	resources.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void close()
    throws PDFlibException
    {
	PDF_close(p);
    }

    /** Delete the PDFlib object, and free all internal resources.
	Never throws any PDFlib exception.
     */
    public final void delete()
    {
	PDF_delete(p);
	p = (long) 0;
    }

    /** Finish the page.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void end_page()
    throws PDFlibException
    {
	PDF_end_page(p);
    }

    /** Get the name of the API function which failed.
       @return PDFlib API function name
     */
    public final String get_apiname()
    throws PDFlibException
    {
	return PDF_get_apiname(p);
    }

    /** Get the contents of the PDF output buffer. The result must be used by
        the client before calling any other PDFlib function.
        @return A byte array containing the PDF output generated so far.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final byte[] get_buffer()
    throws PDFlibException
    {
	return PDF_get_buffer(p);
    }

    /** Get the descriptive text of a failed function call.
       @return PDFlib error message
     */
    public final String get_errmsg()
    throws PDFlibException
    {
	return PDF_get_errmsg(p);
    }

    /** Get the the reason code of a failed function call.
	@return PDFlib error number
     */
    public final int get_errnum()
    throws PDFlibException
    {
	return PDF_get_errnum(p);
    }

    /** Create a new PDFlib object.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public pdflib()
    throws PDFlibException
    {
	p = PDF_new();
    }

    /** Create a new PDF file using the supplied file name.
        @return -1 on error, 1 on success.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int open_file(String filename)
    throws PDFlibException
    {
	return PDF_open_file(p, filename);
    }

    /* p_block.c */

    /** Process an image block according to its properties.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int fill_imageblock(
    int page, String blockname, int image, String optlist)
    throws PDFlibException
    {
	return PDF_fill_imageblock(p, page, blockname, image, optlist);
    }

    /** Process a PDF block according to its properties.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int fill_pdfblock(
    int page, String blockname, int contents, String optlist)
    throws PDFlibException
    {
	return PDF_fill_pdfblock(p, page, blockname, contents, optlist);
    }

    /** Process a text block according to its properties.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int fill_textblock(
    int page, String blockname, String text, String optlist)
    throws PDFlibException
    {
	return PDF_fill_textblock(p, page, blockname, text, optlist);
    }

    /* p_color.c */

    /** Make a named spot color from the current color.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int makespotcolor(String spotname)
    throws PDFlibException
    {
	return PDF_makespotcolor(p, spotname);
    }
 
    /** Set the current color space and color.
        fstype is "fill", "stroke", or "fillstroke".
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void setcolor(
    String fstype, String colorspace, float c1, float c2, float c3, float c4)
    throws PDFlibException
    {
	PDF_setcolor(p, fstype, colorspace, c1, c2, c3, c4);
    }
 
    /** Set the current fill and stroke color.
	@deprecated Use setcolor("fillstroke", "gray", g, 0, 0, 0) instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setgray(float g)
    throws PDFlibException
    {
	PDF_setcolor(p, "fillstroke", "gray", g, 0, 0, 0);
    }

    /** Set the current fill color to a gray value between 0 and 1 inclusive.
	@deprecated Use setcolor("fill", "gray", g, 0, 0, 0) instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setgray_fill(float g)
    throws PDFlibException
    {
	PDF_setcolor(p, "fill", "gray", g, 0, 0, 0);
    }

    /** Set the current fill color to the supplied RGB values.
	@deprecated Use setcolor("fill", "rgb", red, green, blue, 0) instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setrgbcolor_fill(float red, float green, float blue)
    throws PDFlibException
    {
	PDF_setcolor(p, "fill", "rgb", red, green, blue, 0);
    }

    /** Set the current fill and stroke color to the supplied RGB values.
	@deprecated Use setcolor("fillstroke", "rgb", red, green, blue, 0)
	instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setrgbcolor(float red, float green, float blue)
    throws PDFlibException
    {
	PDF_setcolor(p, "fillstroke", "rgb", red, green, blue, 0);
    }

    /** Set the current stroke color to the supplied RGB values.
	@deprecated Use setcolor("stroke", "rgb", red, green, blue, 0) instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setrgbcolor_stroke(float red, float green, float blue)
    throws PDFlibException
    {
	PDF_setcolor(p, "stroke", "rgb", red, green, blue, 0);
    }

    /** Set the current stroke color to a gray value between 0 and 1
        inclusive.
	@deprecated Use setcolor("stroke", "gray", g, 0, 0, 0) instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setgray_stroke(float g)
    throws PDFlibException
    {
	PDF_setcolor(p, "stroke", "gray", g, 0, 0, 0);
    }

    /* p_draw.c */

    /** Draw a circular arc with center (x, y) and radius r from alpha to beta.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void arc(float x, float y, float r, float alpha, float beta)
    throws PDFlibException
    {
	PDF_arc(p, x, y, r, alpha, beta);
    }

    /** Draw a clockwise circular arc from alpha to beta degrees.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void arcn(float x, float y, float r, float alpha, float beta)
    throws PDFlibException
    {
	PDF_arcn(p, x, y, r, alpha, beta);
    }
 
    /** Draw a circle with center (x, y) and radius r.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void circle(float x, float y, float r)
    throws PDFlibException
    {
	PDF_circle(p, x, y, r);
    }

    /** Use the current path as clipping path.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void clip()
    throws PDFlibException
    {
	PDF_clip(p);
    }

    /** Close the current path.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void closepath()
    throws PDFlibException
    {
	PDF_closepath(p);
    }

    /** Close the path, fill, and stroke it.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void closepath_fill_stroke()
    throws PDFlibException
    {
	PDF_closepath_fill_stroke(p);
    }

    /** Close the path, and stroke it.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void closepath_stroke()
    throws PDFlibException
    {
	PDF_closepath_stroke(p);
    }

    /** Draw a Bezier curve from the current point, using 3 more control
        points.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void curveto(
    float x1, float y1, float x2, float y2, float x3, float y3)
    throws PDFlibException
    {
	PDF_curveto(p, x1, y1, x2, y2, x3, y3);
    }

    /** End the current path without filling or stroking it.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void endpath()
    throws PDFlibException
    {
	PDF_endpath(p);
    }

    /** Fill the interior of the path with the current fill color.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void fill()
    throws PDFlibException
    {
	PDF_fill(p);
    }

    /** Fill and stroke the path with the current fill and stroke color.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void fill_stroke()
    throws PDFlibException
    {
	PDF_fill_stroke(p);
    }

    /** Draw a line from the current point to (x, y).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void lineto(float x, float y)
    throws PDFlibException
    {
	PDF_lineto(p, x, y);
    }

    /** Set the current point to (x, y).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void moveto(float x, float y)
    throws PDFlibException
    {
	PDF_moveto(p, x, y);
    }

    /** Draw a rectangle at lower left (x, y) with width and height.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void rect(float x, float y, float width, float height)
    throws PDFlibException
    {
	PDF_rect(p, x, y, width, height);
    }

    /** Stroke the path with the current color and line width,and clear it.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void stroke()
    throws PDFlibException
    {
	PDF_stroke(p);
    }

    /* p_encoding.c */

    /** Add a glyph name to a custom encoding.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void encoding_set_char(
    String encoding, int slot, String glyphname, int uv)
    throws PDFlibException
    {
	PDF_encoding_set_char(p, encoding, slot, glyphname, uv);
    }

    /* p_font.c */

    /** Search a font, and prepare it for later use. PDF_load_font() is
        recommended.
	@return A valid font handle.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int findfont(String fontname, String encoding, int options)
    throws PDFlibException
    {
	return PDF_findfont(p, fontname, encoding, options);
    }

    /** Search a font, and prepare it for later use.
	@return A valid font handle.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int load_font(String fontname, String encoding, String optlist)
    throws PDFlibException
    {
	return PDF_load_font(p, fontname, encoding, optlist);
    }

    /** Set the current font in the given size,
        using a font handle returned by PDF_load_font().
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void setfont(int font, float fontsize)
    throws PDFlibException
    {
	PDF_setfont(p, font, fontsize);
    }

    /* p_gstate.c */

    /** Concatenate a matrix to the current transformation matrix.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void concat(
    float a, float b, float c, float d, float e, float f)
    throws PDFlibException
    {
	PDF_concat(p, a, b, c, d, e, f);
    }

    /** Reset all implicit color and graphics state parameters to their
        defaults.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void initgraphics()
    throws PDFlibException
    {
	PDF_initgraphics(p);
    }

    /** Restore the most recently saved graphics state.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void restore()
    throws PDFlibException
    {
	PDF_restore(p);
    }

    /** Rotate the coordinate system by phi degrees.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void rotate(float phi)
    throws PDFlibException
    {
	PDF_rotate(p, phi);
    }

    /** Save the current graphics state.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void save()
    throws PDFlibException
    {
	PDF_save(p);
    }

    /** Scale the coordinate system by (sx, sy).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void scale(float sx, float sy)
    throws PDFlibException
    {
	PDF_scale(p, sx, sy);
    }

    /** Set the current dash pattern to b black and w white units.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setdash(float b, float w)
    throws PDFlibException
    {
	PDF_setdash(p, b, w);
    }

    /** Set a more complicated dash pattern defined by an optlist.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setdashpattern(String optlist)
    throws PDFlibException
    {
	PDF_setdashpattern(p, optlist);
    }

    /** Set the flatness to a value between 0 and 100 inclusive.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setflat(float flatness)
    throws PDFlibException
    {
	PDF_setflat(p, flatness);
    }

    /** Set the linecap parameter to a value between 0 and 2 inclusive.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setlinecap(int linecap)
    throws PDFlibException
    {
	PDF_setlinecap(p, linecap);
    }

    /** Set the line join parameter to a value between 0 and 2 inclusive.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setlinejoin(int linejoin)
    throws PDFlibException
    {
	PDF_setlinejoin(p, linejoin);
    }

    /** Set the current linewidth to width.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setlinewidth(float width)
    throws PDFlibException
    {
	PDF_setlinewidth(p, width);
    }

    /** Explicitly set the current transformation matrix.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setmatrix(
    float a, float b, float c, float d, float e, float f)
    throws PDFlibException
    {
	PDF_setmatrix(p, a, b, c, d, e, f);
    }
 
    /** Set the miter limit to a value greater than or equal to 1.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setmiterlimit(float miter)
    throws PDFlibException
    {
	PDF_setmiterlimit(p, miter);
    }

    /** Set a dash pattern.
	@deprecated Use PDF_setdashpattern() instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void setpolydash(float[] dasharray)
    throws PDFlibException
    {
	PDF_setpolydash(p, dasharray);
    }

    /** Skew the coordinate system in x and y direction by alpha and beta
        degrees.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final void skew(float alpha, float beta)
    throws PDFlibException
    {
	PDF_skew(p, alpha, beta);
    }

    /** Translate the origin of the coordinate system to (tx, ty).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void translate(float tx, float ty)
    throws PDFlibException
    {
	PDF_translate(p, tx, ty);
    }

    /* p_hyper.c */

    /** Add a nested bookmark under parent, or a new top-level bookmark if
        parent = 0. If open = 1, child bookmarks will
	be folded out, and invisible if open = 0.
	@return A bookmark descriptor which may be used as parent for
	subsequent nested bookmarks.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int add_bookmark(String text, int parent, int open)
    throws PDFlibException
    {
	return PDF_add_bookmark(p, text, parent, open);
    }

    /** Create a named destination on an arbitrary page in the current
        document.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void add_nameddest(String name, String optlist)
    throws PDFlibException
    {
	PDF_add_nameddest(p, name, optlist);
    }

    /** Fill document information field key with value. key is one of "Subject",
        "Title", "Creator", "Author", "Keywords", or a user-defined key.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_info(String key, String value)
    throws PDFlibException
    {
	PDF_set_info(p, key, value);
    }

    /* p_icc.c */

    /** Search an ICC profile, and prepare it for later use.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int load_iccprofile(String profilename, String optlist)
    throws PDFlibException
    {
	return PDF_load_iccprofile(p, profilename, optlist);
    }

    /* p_image.c */

    /** Add an existing image as thumbnail for the current page.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void add_thumbnail(int image)
    throws PDFlibException
    {
	PDF_add_thumbnail(p, image);
    }

    /** Close an image retrieved with one of the PDF_open_image*() functions.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void close_image(int image)
    throws PDFlibException
    {
	PDF_close_image(p, image);
    }

    /** Place an image or template at (x, y) with various options.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void fit_image(int image, float x, float y, String optlist)
    throws PDFlibException
    {
	PDF_fit_image(p, image, x, y, optlist);
    }

    /** Open a (disk-based or virtual) image file with various options.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int load_image(
    String imagetype, String filename, String optlist)
    throws PDFlibException
    {
	return PDF_load_image(p, imagetype, filename, optlist);
    }

    /** Load a CCITT image file.
	@deprecated Use PDF_load_image() instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int open_CCITT(
    String filename, int width, int height, int BitReverse, int K, int BlackIs1)
    throws PDFlibException
    {
	return PDF_open_CCITT(p, filename, width, height, BitReverse, K,
		BlackIs1);
    }

    /** Open an image from memory.
	@deprecated Use PDF_load_image() with virtual files instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */ 
    public final int open_image(
    String imagetype, String source, byte[] data, long length, int width,
    int height, int components, int bpc, String params)
    throws PDFlibException
    {
	return PDF_open_image(p, imagetype, source, data, length, width,
	    height, components, bpc, params);
    }

    /** Open an image from file.
	@deprecated Use PDF_load_image() with virtual files instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int open_image_file(
    String imagetype, String filename, String stringparam, int intparam)
    throws PDFlibException
    {
	return PDF_open_image_file(p, imagetype, filename, stringparam,
		intparam);
    }

    /** Place an image on the page.
	@deprecated Use PDF_fit_image() with virtual files instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void place_image(int image, float x, float y, float scale)
    throws PDFlibException
    {
	PDF_place_image(p, image, x, y, scale);
    }

    /* p_params.c */

    /** Get some PDFlib parameters with string type
        @return A string containing the requested parameter.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final String get_parameter(String key, float modifier)
    throws PDFlibException
    {
	return PDF_get_parameter(p, key, modifier);
    }

    /** Get the value of some PDFlib parameters with float type.
        @return The requested parameter value.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final float get_value(String key, float modifier)
    throws PDFlibException
    {
	return PDF_get_value(p, key, modifier);
    }

    /** Set some PDFlib parameters with string type.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_parameter(String key, String value)
    throws PDFlibException
    {
	PDF_set_parameter(p, key, value);
    }

    /** Set some PDFlib parameters with float type.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_value(String key, float value)
    throws PDFlibException
    {
	PDF_set_value(p, key, value);
    }

    /* p_pattern.c */

    /** Start a new pattern definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int begin_pattern(
    float width, float height, float xstep, float ystep, int painttype)
    throws PDFlibException
    {
	return PDF_begin_pattern(p, width, height, xstep, ystep, painttype);
    }
 
    /** Finish a pattern definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void end_pattern()
    throws PDFlibException
    {
	PDF_end_pattern(p);
    }
 
    /* p_pdi.c */

    /** Close all open page handles, and close the input PDF document.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void close_pdi(int doc)
    throws PDFlibException
    {
	PDF_close_pdi(p, doc);
    }

    /** Close the page handle, and free all page-related resources.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void close_pdi_page(int page)
    throws PDFlibException
    {
	PDF_close_pdi_page(p, page);
    }
 
    /** Place an imported PDF page with the lower left corner at (x, y) with
        various options.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void fit_pdi_page(int page, float x, float y, String optlist)
    throws PDFlibException
    {
	PDF_fit_pdi_page(p, page, x, y, optlist);
    }
 
    /** Get the contents of some PDI document parameter with string type.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final String get_pdi_parameter(
    String key, int doc, int page, int reserved)
    throws PDFlibException
    {
	return PDF_get_pdi_parameter(p, key, doc, page, reserved);
    }

    /** Get the contents of some PDI document parameter with numerical type.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final float get_pdi_value(
    String key, int doc, int page, int reserved)
    throws PDFlibException
    {
	return PDF_get_pdi_value(p, key, doc, page, reserved);
    }

    /** Open an existing PDF document and prepare it for later use.
            @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int open_pdi(String filename, String optlist, int reserved)
    throws PDFlibException
    {
	return PDF_open_pdi(p, filename, optlist, reserved);
    }
 
    /** Open a (disk-based or virtual) PDF document and prepare it
        for later use.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int open_pdi_page(int doc, int pagenumber, String optlist)
    throws PDFlibException
    {
	return PDF_open_pdi_page(p, doc, pagenumber, optlist);
    }
 
    /** Perform various actions on a PDI document.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final float process_pdi(int doc, int page, String optlist)
    throws PDFlibException
    {
	return PDF_process_pdi(p, doc, page, optlist);
    }

    /** Place an imported page on the output page.
	@deprecated Use PDF_fit_pdi_page() instead.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void place_pdi_page(
    int page, float x, float y, float sx, float sy)
    throws PDFlibException
    {
	PDF_place_pdi_page(p, page, x, y, sx, sy);
    }

    /* p_resource.c */

    /** Create a file in the PDFlib virtual file system (PVF).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void create_pvf(String filename, byte[] data, String optlist)
    throws PDFlibException
    {
	PDF_create_pvf(p, filename, data, optlist);
    }

    /** Delete a virtual file.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int delete_pvf(String filename)
    throws PDFlibException
    {
	return PDF_delete_pvf(p, filename);
    }
 
    /* p_shading.c */

    /** Define a color blend (smooth shading) from the current fill color
        to the supplied color.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int shading(
    String shtype, float x0, float y0, float x1, float y1,
    float c1, float c2, float c3, float c4, String optlist)
    throws PDFlibException
    {
	return PDF_shading(p, shtype, x0, y0, x1, y1, c1, c2, c3, c4, optlist);
    }

    /** Define a shading pattern using a shading object.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int shading_pattern(int shading, String optlist)
    throws PDFlibException
    {
	return PDF_shading_pattern(p, shading, optlist);
    }

    /** Fill an area with a shading, based on a shading object.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void shfill(int shading)
    throws PDFlibException
    {
	PDF_shfill(p, shading);
    }

    /* p_template.c */

    /** Start a new template definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int begin_template(float width, float height)
    throws PDFlibException
    {
	return PDF_begin_template(p, width, height);
    }

    /** Finish a template definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void end_template()
    throws PDFlibException
    {
	PDF_end_template(p);
    }
 
    /* p_text.c */

    /** Print text at the next line. The spacing between lines is determined
        by the "leading" parameter.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void continue_text(String text)
    throws PDFlibException
    {
	PDF_continue_text(p, text);
    }

    /** Print text in the current font placed and fitted at (x, y).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void fit_textline(
    String text, float x, float y, String optlist)
    throws PDFlibException
    {
	PDF_fit_textline(p, text, x, y, optlist);
    }

    /** Set the text output position.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_text_pos(float x, float y)
    throws PDFlibException
    {
	PDF_set_text_pos(p, x, y);
    }

    /** Print text in the current font and size at the current position.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void show(String text)
    throws PDFlibException
    {
	PDF_show(p, text);
    }

    /** Format text in the current font and size into the supplied text box
        according to the requested formatting mode, which must be one of
        "left", "right", "center", "justify", or "fulljustify". If width
	and height are 0, only a single line is placed at the point
	(left, top) in the requested mode.
        @return Number of characters which didn't fit in the box.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final int show_boxed(String text, float left, float bottom,
    	float width, float height, String hmode, String feature)
    throws PDFlibException
    {
	return PDF_show_boxed(p, text, left, bottom, width, height,
	    	hmode, feature);
    }

    /** Print text in the current font at (x, y).
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void show_xy(String text, float x, float y)
    throws PDFlibException
    {
	PDF_show_xy(p, text, x, y);
    }

    /** Return the width of text in an arbitrary font.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
     */
    public final float stringwidth(String text, int font, float fontsize)
    throws PDFlibException
    {
	return PDF_stringwidth(p, text, font, fontsize);
    }

    /* p_type3.c */

    /** Start a type3 font definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void begin_font(
    String name, float a, float b, float c, float d, float e, float f,
	String optlist)
    throws PDFlibException
    {
	PDF_begin_font(p, name, a, b, c, d, e, f, optlist);
    }

    /** Start a type 3 glyph definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void begin_glyph(
    String glyphname, float wx, float llx, float lly, float urx, float ury)
    throws PDFlibException
    {
	PDF_begin_glyph(p, glyphname, wx, llx, lly, urx, ury);
    }

    /** Terminate a type 3 font definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void end_font()
    throws PDFlibException
    {
	PDF_end_font(p);
    }

    /** Terminate a type 3 glyph definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void end_glyph()
    throws PDFlibException
    {
	PDF_end_glyph(p);
    }

    /* p_xgstate.c */

    /** Create a gstate object definition.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final int create_gstate(String optlist)
    throws PDFlibException
    {
	return PDF_create_gstate(p, optlist);
    }

    /** Activate a gstate object.
        @exception com.pdflib.PDFlibException
        PDF output cannot be finished after an exception.
    */
    public final void set_gstate(int gstate)
    throws PDFlibException
    {
	PDF_set_gstate(p, gstate);
    }


    // ------------------------------------------------------------------------
    // private functions

    private long p;

    protected final void finalize()
    throws PDFlibException
    {
	PDF_delete(p);
	p = (long) 0;
    }

    private final static void classFinalize() {
	PDF_shutdown();
    }

    /* p_annots.c */

    private final static native void PDF_add_launchlink(long jp, float jarg1, float jarg2, float jarg3, float jarg4, String jarg5) throws PDFlibException;

    private final static native void PDF_add_locallink(long jp, float jarg1, float jarg2, float jarg3, float jarg4, int jarg5, String jarg6) throws PDFlibException;

    private final static native void PDF_add_note(long jp, float jarg1, float jarg2, float jarg3, float jarg4, String jarg5, String jarg6, String jarg7, int jarg8) throws PDFlibException;

    private final static native void PDF_add_pdflink(long jp, float jarg1, float jarg2, float jarg3, float jarg4, String jarg5, int jarg6, String jarg7) throws PDFlibException;

    private final static native void PDF_add_weblink(long jp, float jarg1, float jarg2, float jarg3, float jarg4, String jarg5) throws PDFlibException;

    private final static native void PDF_attach_file(long jp, float jarg1, float jarg2, float jarg3, float jarg4, String jarg5, String jarg6, String jarg7, String jarg8, String jarg9) throws PDFlibException;

    private final static native void PDF_set_border_color(long jp, float jarg1, float jarg2, float jarg3) throws PDFlibException;

    private final static native void PDF_set_border_dash(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_set_border_style(long jp, String jarg1, float jarg2) throws PDFlibException;

    /* p_basic.c */

    private final static native void PDF_begin_page(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_boot();

    private final static native void PDF_close(long jp) throws PDFlibException;

    private final static native synchronized void PDF_delete(long jp);

    private final static native void PDF_end_page(long jp) throws PDFlibException;

    private final static native String PDF_get_apiname(long jp) throws PDFlibException;

    private final static native byte[] PDF_get_buffer(long jp) throws PDFlibException;

    private final static native String PDF_get_errmsg(long jp) throws PDFlibException;

    private final static native int PDF_get_errnum(long jp) throws PDFlibException;

    private final static native synchronized long PDF_new() throws PDFlibException;

    private final static native int PDF_open_file(long jp, String jarg1) throws PDFlibException;

    private final static native void PDF_shutdown();


    /* p_block.c */

    private final static native int PDF_fill_imageblock(long jp, int jpage, String jblockname, int jimage, String joptlist) throws PDFlibException;

    private final static native int PDF_fill_pdfblock(long jp, int jpage, String jblockname, int jcontents, String joptlist) throws PDFlibException;
    
    private final static native int PDF_fill_textblock(long jp, int jpage, String jblockname, String jtext, String joptlist) throws PDFlibException;

    /* p_color.c */

    private final static native int PDF_makespotcolor(long jp, String jarg1) throws PDFlibException;

    private final static native void PDF_setcolor(long jp, String jarg1, String jarg2, float jarg3, float jarg4, float jarg5, float jarg6) throws PDFlibException;

    /* p_draw.c */

    private final static native void PDF_arc(long jp, float jarg1, float jarg2, float jarg3, float jarg4, float jarg5) throws PDFlibException;

    private final static native void PDF_arcn(long jp, float jarg1, float jarg2, float jarg3, float jarg4, float jarg5) throws PDFlibException;

    private final static native void PDF_circle(long jp, float jarg1, float jarg2, float jarg3) throws PDFlibException;

    private final static native void PDF_clip(long jp) throws PDFlibException;

    private final static native void PDF_closepath(long jp) throws PDFlibException;

    private final static native void PDF_closepath_fill_stroke(long jp) throws PDFlibException;

    private final static native void PDF_closepath_stroke(long jp) throws PDFlibException;

    private final static native void PDF_curveto(long jp, float jarg1, float jarg2, float jarg3, float jarg4, float jarg5, float jarg6) throws PDFlibException;

    private final static native void PDF_endpath(long jp) throws PDFlibException;

    private final static native void PDF_fill(long jp) throws PDFlibException;

    private final static native void PDF_fill_stroke(long jp) throws PDFlibException;

    private final static native void PDF_lineto(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_moveto(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_rect(long jp, float jarg1, float jarg2, float jarg3, float jarg4) throws PDFlibException;

    private final static native void PDF_stroke(long jp) throws PDFlibException;

    /* p_encoding.c */

    private final static native void PDF_encoding_set_char(long jp, String jencoding, int jslot, String jglyphname, int juv) throws PDFlibException;

    /* p_font.c */

    private final static native int PDF_findfont(long jp, String jarg1, String jarg2, int jarg3) throws PDFlibException;

    private final static native int PDF_load_font(long jp, String jfontname, String jencoding, String joptlist) throws PDFlibException;

    private final static native void PDF_setfont(long jp, int jarg1, float jarg2) throws PDFlibException;

    /* p_gstate.c */

    private final static native void PDF_concat(long jp, float a, float b, float c, float d, float e, float f) throws PDFlibException;

    private final static native void PDF_initgraphics(long jp) throws PDFlibException;

    private final static native void PDF_restore(long jp) throws PDFlibException;

    private final static native void PDF_rotate(long jp, float jarg1) throws PDFlibException;

    private final static native void PDF_save(long jp) throws PDFlibException;

    private final static native void PDF_scale(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_setdash(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_setdashpattern(long jp, String joptlist) throws PDFlibException;

    private final static native void PDF_setflat(long jp, float jarg1) throws PDFlibException;

    private final static native void PDF_setlinecap(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_setlinejoin(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_setlinewidth(long jp, float jarg1) throws PDFlibException;

    private final static native void PDF_setmatrix(long jp, float jarg1, float jarg2, float jarg3, float jarg4, float jarg5, float jarg6) throws PDFlibException;

    private final static native void PDF_setmiterlimit(long jp, float jarg1) throws PDFlibException;

    private final static native void PDF_setpolydash(long jp, float[] jarg1) throws PDFlibException;

    private final static native void PDF_skew(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_translate(long jp, float jarg1, float jarg2) throws PDFlibException;

    /* p_hyper.c */

    private final static native int PDF_add_bookmark(long jp, String jarg1, int jarg2, int jarg3) throws PDFlibException;

    private final static native void PDF_add_nameddest(long jp, String jname, String joptlist) throws PDFlibException;

    private final static native void PDF_set_info(long jp, String jarg1, String jarg2) throws PDFlibException;

    /* p_icc.c */

    private final static native int PDF_load_iccprofile(long jp, String jprofilename, String joptlist) throws PDFlibException;

    /* p_image.c */

    private final static native void PDF_add_thumbnail(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_close_image(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_fit_image(long jp, int jimage, float jx, float jy, String joptlist) throws PDFlibException;

    private final static native int PDF_load_image(long jp, String jimagetype, String jfilename, String joptlist) throws PDFlibException;

    private final static native int PDF_open_CCITT(long jp, String jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6) throws PDFlibException;

    private final static native int PDF_open_image(long jp, String jarg1, String jarg2, byte[] jarg3, long jarg4, int jarg5, int jarg6, int jarg7, int jarg8, String jarg9) throws PDFlibException;

    private final static native int PDF_open_image_file(long jp, String jarg1, String jarg2, String jarg3, int jarg4) throws PDFlibException;

    private final static native void PDF_place_image(long jp, int jarg1, float jarg2, float jarg3, float jarg4) throws PDFlibException;

    /* p_params.c */

    private final static native String PDF_get_parameter(long jp, String key, float mod) throws PDFlibException;

    private final static native float PDF_get_value(long jp, String key, float mod) throws PDFlibException;

    private final static native void PDF_set_parameter(long jp, String jarg1, String jarg2) throws PDFlibException;

    private final static native void PDF_set_value(long jp, String jarg1, float jarg2) throws PDFlibException;

    /* p_pattern.c */

    private final static native int PDF_begin_pattern(long jp, float jarg1, float jarg2, float jarg3, float jarg4, int jarg5) throws PDFlibException;

    private final static native void PDF_end_pattern(long jp) throws PDFlibException;

    /* p_pdi.c */

    private final static native void PDF_close_pdi(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_close_pdi_page(long jp, int jarg1) throws PDFlibException;

    private final static native void PDF_fit_pdi_page(long jp, int jpage, float jx, float jy, String joptlist) throws PDFlibException;

    private final static native String PDF_get_pdi_parameter(long jp, String jarg2, int jarg3, int jarg4, int jarg5) throws PDFlibException;

    private final static native float PDF_get_pdi_value(long jp, String jarg1, int jarg2, int jarg3, int jarg4) throws PDFlibException;

    private final static native int PDF_open_pdi(long jp, String jarg1, String jarg2, int jarg3) throws PDFlibException;

    private final static native int PDF_open_pdi_page(long jp, int jarg1, int jarg2, String jarg3) throws PDFlibException;

    private final static native void PDF_place_pdi_page(long jp, int jarg1, float jarg2, float jarg3, float jarg4, float jarg5) throws PDFlibException;

    private final static native int PDF_process_pdi(long jp, int jdoc, int jpage, String joptlist) throws PDFlibException;

    /* p_resource.c */

    private final static native void PDF_create_pvf(long jp, String jfilename, byte[] jdata, String joptlist) throws PDFlibException;

    private final static native int PDF_delete_pvf(long jp, String jfilename) throws PDFlibException;

    /* p_shading.c */

    private final static native int PDF_shading(long jp, String jshtype, float jx0, float jy0, float jx1, float jy1, float jc1, float jc2, float jc3, float jc4, String joptlist) throws PDFlibException;

    private final static native int PDF_shading_pattern(long jp, int jshading, String joptlist) throws PDFlibException;

    private final static native void PDF_shfill(long jp, int jshading) throws PDFlibException;

    /* p_template.c */

    private final static native int PDF_begin_template(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_end_template(long jp) throws PDFlibException;

    /* p_text.c */

    private final static native void PDF_continue_text(long jp, String jarg1) throws PDFlibException;

    private final static native void PDF_fit_textline(long jp, String jtext, float jx, float jy, String joptlist) throws PDFlibException;

    private final static native void PDF_set_text_pos(long jp, float jarg1, float jarg2) throws PDFlibException;

    private final static native void PDF_show(long jp, String jarg1) throws PDFlibException;

    private final static native int PDF_show_boxed(long jp, String jarg1, float jarg2, float jarg3, float jarg4, float jarg5, String jarg6, String jarg7) throws PDFlibException;

    private final static native void PDF_show_xy(long jp, String jarg1, float jarg2, float jarg3) throws PDFlibException;

    private final static native float PDF_stringwidth(long jp, String jarg1, int jarg2, float jarg3) throws PDFlibException;

    /* p_type3.c */

    private final static native void PDF_begin_font(long jp, String jname, float ja, float jb, float jc, float jd, float je, float jf, String optlist) throws PDFlibException;

    private final static native void PDF_begin_glyph(long jp, String jname, float jwx, float jllx, float jlly, float jurx, float jury) throws PDFlibException;

    private final static native void PDF_end_font(long jp) throws PDFlibException;

    private final static native void PDF_end_glyph(long jp) throws PDFlibException;

    /* p_xgstate.c */

    private final static native int PDF_create_gstate(long jp, String joptlist) throws PDFlibException;

    private final static native void PDF_set_gstate(long jp, int jhandle) throws PDFlibException;
}
