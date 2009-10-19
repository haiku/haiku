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

/* $Id: p_font.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Header file for the PDFlib font subsystem
 *
 */

#ifndef P_FONT_H
#define P_FONT_H

#define PDF_DEFAULT_WIDTH       250     /* some reasonable default */
#define PDF_DEFAULT_CIDWIDTH    1000    /* for CID fonts */
#define PDF_DEFAULT_GLYPH       0x0020  /* space */

/* internal maximal length of fontnames */
#define PDF_MAX_FONTNAME        128

/* last text rendering mode number */
#define  PDF_LAST_TRMODE  7

/* p_truetype.c */
pdc_bool        pdf_get_metrics_tt(PDF *p, pdc_font *font,
                    const char *fontname, pdc_encoding enc,
                    const char *filename);
int             pdf_check_tt_font(PDF *p, const char *filename,
                    const char *fontname, pdc_font *font);
int             pdf_check_tt_hostfont(PDF *p, const char *hostname);

/* p_afm.c */
pdc_bool        pdf_process_metrics_data(PDF *p, pdc_font *font,
                    const char *fontname);
pdc_bool        pdf_get_metrics_afm(PDF *p, pdc_font *font,
                    const char *fontname, pdc_encoding enc,
                    const char *filename);
pdc_bool        pdf_get_core_metrics_afm(PDF *p, pdc_font *font,
                    pdc_core_metric *metric, const char *fontname,
                    const char *filename);

/* p_pfm.c */
pdc_bool        pdf_check_pfm_encoding(PDF *p, pdc_font *font,
                       const char *fontname, pdc_encoding enc);
pdc_bool        pdf_get_metrics_pfm(PDF *p, pdc_font *font,
                    const char *fontname, pdc_encoding enc,
                    const char *filename);

/* p_cid.c */
pdc_bool        pdf_get_metrics_cid(PDF *p, pdc_font *font,
                                    const char *fontname,
                                    const char *encoding);
int             pdf_handle_cidfont(PDF *p, const char *fontname,
                                   const char *encoding);
const char*     pdf_get_ordering_cid(PDF *p, pdc_font *font);
int             pdf_get_supplement_cid(PDF *p, pdc_font *font);
void            pdf_put_cidglyph_widths(PDF *p, pdc_font *font);


/* p_font.c */
void    pdf_init_fonts(PDF *p);
void    pdf_grow_fonts(PDF *p);
int     pdf_init_newfont(PDF *p);
void    pdf_write_page_fonts(PDF *p);
void    pdf_write_doc_fonts(PDF *p);
void    pdf_cleanup_fonts(PDF *p);
int     pdf__load_font(PDF *p, const char *fontname, int reserved,
                       const char *encoding, const char *optlist);
pdc_bool pdf_make_fontflag(PDF *p, pdc_font *font);
int      pdf_get_font(PDF *p);
int      pdf_get_monospace(PDF *p);
const char *pdf_get_fontname(PDF *p);
const char *pdf_get_fontstyle(PDF *p);
const char *pdf_get_fontencoding(PDF *p);


/* p_type1.c */
int pdf_t1check_fontfile(PDF *p, pdc_font *font, pdc_file *fp);
PDF_data_source *pdf_make_t1src(PDF *p, pdc_font *font,
                                PDF_data_source *t1src);
void     pdf_put_length_objs(PDF *p, PDF_data_source *t1src,
		     pdc_id length1_id, pdc_id length2_id, pdc_id length3_id);

/* p_type3.c */
void    pdf_init_type3(PDF *p);
int     pdf_get_t3colorized(PDF *p);
int     pdf_handle_t3font(PDF *p, const char *fontname, pdc_encoding enc,
                          int oldslot);
void    pdf__begin_font(PDF *p, const char *fontname,
                        float a, float b, float c, float d, float e, float f,
                        const char *optlist);
void    pdf__end_font(PDF *p);
void    pdf__begin_glyph(PDF *p, const char *glyphname, float wx,
                         float llx, float lly, float urx, float ury);
void    pdf__end_glyph(PDF *p);

#endif  /* P_FONT_H */
