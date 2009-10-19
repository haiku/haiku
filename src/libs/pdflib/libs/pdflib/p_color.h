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

/* $Id: p_color.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib color definitions
 *
 */

#ifndef P_COLOR_H
#define P_COLOR_H

typedef enum {
    NoColor = -1, DeviceGray = 0, DeviceRGB, DeviceCMYK,
    CalGray, CalRGB, Lab, ICCBased,
    Indexed, PatternCS, Separation, DeviceN
} pdf_colorspacetype;

/*
 * These are treated specially in the global colorspace list, and are not
 * written as /ColorSpace resource since we always specify them directly.
 * Pattern colorspace with base == pdc_undef means PaintType == 1.
 */
#define PDF_SIMPLE_COLORSPACE(cs)		\
	((cs)->type == DeviceGray ||		\
	 (cs)->type == DeviceRGB ||		\
	 (cs)->type == DeviceCMYK ||		\
	 ((cs)->type == PatternCS && cs->val.pattern.base == pdc_undef))

/* Rendering Intents */
typedef enum {
    AutoIntent = 0,
    AbsoluteColorimetric,
    RelativeColorimetric,
    Saturation,
    Perceptual
} pdf_renderingintent;

/* BlendModes */
typedef enum {
    BM_None	 	= 0,
    BM_Normal	 	= 1<<0,
    BM_Multiply	 	= 1<<1,
    BM_Screen	 	= 1<<2,
    BM_Overlay	 	= 1<<3,
    BM_Darken	 	= 1<<4,
    BM_Lighten	 	= 1<<5,
    BM_ColorDodge	= 1<<6,
    BM_ColorBurn	= 1<<7,
    BM_HardLight	= 1<<8,
    BM_SoftLight	= 1<<9,
    BM_Difference	= 1<<10,
    BM_Exclusion	= 1<<11,
    BM_Hue	 	= 1<<12,
    BM_Saturation	= 1<<13,
    BM_Color	 	= 1<<14,
    BM_Luminosity	= 1<<15
} pdf_blendmode;


struct pdf_pattern_s {
    pdc_id	obj_id;			/* object id of this pattern */
    int		painttype;		/* colored (1) or uncolored (2) */
    pdc_bool	used_on_current_page;	/* this pattern used on current page */
};

typedef enum {
    pdf_none = 0,
    pdf_fill,
    pdf_stroke,
    pdf_fillstroke
} pdf_drawmode;

typedef pdc_byte pdf_colormap[256][3];

typedef struct pdf_colorspace_s pdf_colorspace;

typedef struct {
    int      		cs;     	/* slot of underlying color space */

    union {
        float           gray;           /* DeviceGray */
        int             pattern;        /* Pattern */
        int             idx;        	/* Indexed */
        struct {                        /* DeviceRGB */
            float       r;
            float       g;
            float       b;
        } rgb;
        struct {                        /* DeviceCMYK */
            float       c;
            float       m;
            float       y;
            float       k;
        } cmyk;
    } val;
} pdf_color;

struct pdf_colorspace_s {
    pdf_colorspacetype type;            /* color space type */

    union {
	struct {			/* Indexed */
	    int   	base;		/* base color space */
	    pdf_colormap *colormap;	/* pointer to colormap */
	    pdc_bool	colormap_done;	/* colormap already written to output */
	    int		palette_size;	/* # of palette entries (not bytes!) */
	    pdc_id	colormap_id;	/* object id of colormap */
	} indexed;

	struct {			/* Pattern */
	    int   	base;		/* base color space for PaintType 2 */
	} pattern;

    } val;

    pdc_id      obj_id;                 /* object id of this colorspace */
    pdc_bool    used_on_current_page;   /* this resource used on current page */
};

typedef struct {
    pdf_color   fill;
    pdf_color   stroke;
} pdf_cstate;

/* p_color.c */
void    pdf_init_cstate(PDF *p);
void    pdf_init_colorspaces(PDF *p);
void    pdf_write_page_colorspaces(PDF *p);
void	pdf_write_function_dict(PDF *p, pdf_color *c0, pdf_color *c1, float N);
void    pdf_write_doc_colorspaces(PDF *p);
void	pdf_write_colorspace(PDF *p, int slot, pdc_bool direct);
void	pdf_write_color_values(PDF *p, pdf_color *color, pdf_drawmode mode);
void    pdf_set_color_values(PDF *p, pdf_color *c, pdf_drawmode drawmode);
int	pdf_add_colorspace(PDF *p, pdf_colorspace *cs, pdc_bool inuse);
void    pdf_cleanup_colorspaces(PDF *p);
void	pdf_write_colormap(PDF *p, int slot);
int	pdf_color_components(PDF *p, int slot);
int	pdf__makespotcolor(PDF *p, const char *spotname);
void    pdf__setcolor(PDF *p, const char *fstype, const char *colorspace,
                      float c1, float c2, float c3, float c4);


/* p_pattern.c */
void	pdf_grow_pattern(PDF *p);
#endif  /* P_COLOR_H */
