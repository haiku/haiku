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

/* $Id: p_intern.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib internal definitions
 *
 */

#ifndef P_INTERN_H
#define P_INTERN_H

#include "pdflib.h"

#include "pc_util.h"
#include "pc_geom.h"
#include "pc_file.h"
#include "pc_sbuf.h"
#include "pc_font.h"
#include "pc_optparse.h"

#include "p_color.h"
#include "p_keyconn.h"

/* PDFlib error numbers.
*/
enum
{
#define         pdf_genNames    1
#include        "p_generr.h"

    PDF_E_dummy
};

/* ------------------------ PDFlib feature configuration  ------------------- */

/* changing the following is not recommended, and not supported */

/* BMP image support */
#define PDF_BMP_SUPPORTED

/* GIF image support */
#define PDF_GIF_SUPPORTED

/* JPEG image support */
#define PDF_JPEG_SUPPORTED

/* PNG image support, requires HAVE_LIBZ */
#define HAVE_LIBPNG

/* TIFF image support */
#define HAVE_LIBTIFF

/* TrueType font support */
#define PDF_TRUETYPE_SUPPORTED

/* support proportional widths for the standard CJK fonts */
#define PDF_CJKFONTWIDTHS_SUPPORTED


/* ----------------------------------- macros ------------------------------- */

#define PDFLIBSERIAL            "PDFLIBSERIAL"          /* name of env var. */

/*
 * Allocation chunk sizes. These don't affect the generated documents
 * in any way. In order to save initial memory, however, you can lower
 * the values. Increasing the values will bring some performance gain
 * for large documents, but will waste memory for small ones.
 */
#define PAGES_CHUNKSIZE         512             /* pages */
#define PNODES_CHUNKSIZE        64              /* page tree nodes */
#define CONTENTS_CHUNKSIZE      64              /* page content streams */
#define FONTS_CHUNKSIZE         16              /* document fonts */
#define ENCODINGS_CHUNKSIZE     8               /* document encodings */
#define XOBJECTS_CHUNKSIZE      128             /* document xobjects */
#define IMAGES_CHUNKSIZE        128             /* document images */
#define OUTLINE_CHUNKSIZE       256             /* document outlines */
#define NAMES_CHUNKSIZE         256             /* names */
#define PDI_CHUNKSIZE           16              /* PDI instances */
#define COLORSPACES_CHUNKSIZE   16              /* color spaces */
#define PATTERN_CHUNKSIZE       4               /* pattern */
#define SHADINGS_CHUNKSIZE	4               /* shadings */
#define EXTGSTATE_CHUNKSIZE     4               /* external graphic states */
#define T3GLYPHS_CHUNKSIZE      256             /* type 3 font glyph table */
#define PRIVGLYPHS_CHUNKSIZE    256             /* private glyph table */
#define ICCPROFILE_CHUNKSIZE    4               /* ICC profiles */

/* The following shouldn't require any changes */
#define PDF_MAX_SAVE_LEVEL      12              /* max number of save levels */
#define PDF_FILENAMELEN         1024            /* maximum file name length */
#define PDF_MAX_PARAMSTRING     256             /* image parameter string */

/* Acrobat limit for page dimensions */
#define PDF_ACRO4_MINPAGE       ((float) 3)     /* 1/24 inch = 0.106 cm */
#define PDF_ACRO4_MAXPAGE       ((float) 14400) /* 200  inch = 508 cm */

/* --------------------- typedefs, enums and structs ------------------------ */

typedef enum {pdf_fill_winding, pdf_fill_evenodd} pdf_fillrule;
typedef enum { c_none, c_page, c_path, c_text } pdf_content_type;

/* Transition types for page transition effects */
typedef enum {
    trans_none, trans_split, trans_blinds, trans_box,
    trans_wipe, trans_dissolve, trans_glitter, trans_replace
} pdf_transition;

/* Document open modes */
typedef enum {
    open_auto, open_none, open_bookmarks, open_thumbnails, open_fullscreen
} pdf_openmode;

/* Destination uses */
typedef enum {
    pdf_openaction,
    pdf_bookmark,
    pdf_remotelink,
    pdf_locallink,
    pdf_nameddest
} pdf_destuse;

/* Destination types for internal and external links */
typedef enum {
    fixed,
    fitwindow,
    fitwidth,
    fitheight,
    fitrect,
    fitvisible,
    fitvisiblewidth,
    fitvisibleheight,
    nameddest,
    filedest
} pdf_desttype;

/* Border styles for links */
typedef enum {
    border_solid, border_dashed, border_beveled,
    border_inset, border_underlined
} pdf_border_style;

/* configurable flush points */
typedef enum {
    pdf_flush_none = 0,                         /* end of document */
    pdf_flush_page = 1<<0,                      /* after page */
    pdf_flush_content = 1<<1,                   /* font, xobj, annots */
    pdf_flush_reserved1 = 1<<2,                 /* reserved */
    pdf_flush_reserved2 = 1<<3,                 /* reserved */
    pdf_flush_heavy = 1<<4                      /* before realloc attempt */
} pdf_flush_state;

/*
 * Internal PDFlib states for error checking.
 * This enum must be kept in sync with the scope_names array in
 * p_basic.c::pdf_current_scope.
 */
typedef enum {
    pdf_state_object = 1<<0,            /* outside any document */
    pdf_state_document = 1<<1,          /* document */
    pdf_state_page = 1<<2,              /* page description in a document */
    pdf_state_pattern = 1<<3,           /* pattern in a document */
    pdf_state_template = 1<<4,          /* template in a document */
    pdf_state_path = 1<<5,              /* path in a page description */

    pdf_state_font = 1<<7,              /* font definition */
    pdf_state_glyph = 1<<8,             /* glyph description in a font */
    pdf_state_error = 1<<9              /* in error cleanup */
} pdf_state;

#define pdf_state_content       \
    (pdf_state) (pdf_state_page | pdf_state_pattern | \
                 pdf_state_template | pdf_state_glyph)

#define pdf_state_all							\
    (pdf_state) (pdf_state_object | pdf_state_document | pdf_state_page |  \
                 pdf_state_pattern | pdf_state_template | pdf_state_path | \
                 pdf_state_font | pdf_state_glyph)

#define PDF_STATE_STACK_SIZE    4

#define PDF_GET_STATE(p)                                                \
        ((p)->state_stack[(p)->state_sp])

#define PDF_SET_STATE(p, s)                                         \
        ((p)->state_stack[(p)->state_sp] = (s))

#define PDF_CHECK_STATE(p, s)                                       \
        if ((((p)->state_stack[(p)->state_sp] & (s)) == 0))             \
	    pdc_error(p->pdc, PDF_E_DOC_SCOPE, pdf_current_scope(p), 0, 0, 0)

#define PDF_PUSH_STATE(p, fn, s)                                        \
        if ((p)->state_sp == PDF_STATE_STACK_SIZE - 1)                  \
            pdc_error(p->pdc, PDF_E_INT_SSTACK_OVER, fn, 0, 0, 0);          \
        else                                                            \
            (p)->state_stack[++(p)->state_sp] = (s)

#define PDF_POP_STATE(p, fn)                                            \
        if ((p)->state_sp == 0)                                         \
            pdc_error(p->pdc, PDF_E_INT_SSTACK_UNDER, fn, 0, 0, 0);         \
        else                                                            \
            --(p)->state_sp

#define PDF_GET_EXTHANDLE(p, val) \
        (p->hastobepos ? val + 1 : val)

#define PDF_INPUT_HANDLE(p, val) \
        if (p->hastobepos) val--;

#define PDF_RETURN_HANDLE(p, val) \
        if (p->hastobepos) val++; \
        pdc_trace(p->pdc, "[%d]\n", val); \
        return val;

#define PDF_RETURN_BOOLEAN(p, val)			\
        return pdc_trace(p->pdc, "[%d]\n",		\
	    (val == -1 && p->hastobepos) ? 0 : val),	\
	    (val == -1 && p->hastobepos) ? 0 : val

#ifndef PDI_DEFINED
#define PDI_DEFINED
typedef struct PDI_s PDI;	/* The opaque PDI type */
#endif

typedef struct
{
    pdc_bool	pdfx_ok;
    PDI *	pi;
} pdf_pdi;

typedef enum {
    HideToolbar			= (1<<0),
    HideMenubar			= (1<<1),
    HideWindowUI		= (1<<2),
    FitWindow			= (1<<3),
    CenterWindow		= (1<<4),
    DisplayDocTitle		= (1<<5),
    NonFullScreenPageModeOutlines= (1<<6),
    NonFullScreenPageModeThumbs	= (1<<7),
    DirectionR2L		= (1<<8)
} pdf_prefs_e;

typedef struct {
    int flags;
    pdc_usebox ViewArea;
    pdc_usebox ViewClip;
    pdc_usebox PrintArea;
    pdc_usebox PrintClip;
} pdf_prefs;

/* Destination structure for bookmarks and links */
typedef struct {
    pdf_desttype type;
    char        *filename;      /* name of a file to be launched */
    pdc_bool	remote;		/* local or remote target file */
    int         page;           /* target page number */
    char        *name;          /* destination name, only for type=nameddest */
    int		len;		/* length of the name string */
    float       zoom;           /* magnification */
    float       left;
    float       right;
    float       bottom;
    float       top;
    float       color[3];       /* rgb color of bookmark text */
    pdc_fontstyle fontstyle;    /* font style of bookmark text */
} pdf_dest;

typedef struct {
    pdc_encodingvector *ev;     /* encoding vector */
    pdc_id      id;             /* encoding object id */
    pdc_id      tounicode_id;   /* tounicode object ids */
} pdf_encoding;

/* Opaque types which are detailed in the respective modules */
typedef struct pdf_xobject_s pdf_xobject;
typedef struct pdf_res_s pdf_res;
typedef struct pdf_category_s pdf_category;
typedef struct pdf_virtfile_s pdf_virtfile;
typedef struct pdf_annot_s pdf_annot;
typedef struct pdf_name_s pdf_name;
typedef struct pdf_info_s pdf_info;
typedef struct pdf_outline_s pdf_outline;
typedef struct pdf_image_s pdf_image;
typedef struct pdf_pattern_s pdf_pattern;
typedef struct pdf_shading_s pdf_shading;
typedef struct pdf_extgstateresource_s pdf_extgstateresource;

/* -------------------- special graphics state -------------------- */
typedef struct {
    pdc_matrix  ctm;            /* current transformation matrix */
    float       x;              /* current x coordinate */
    float       y;              /* current y coordinate */

    float       startx;         /* starting x point of the subpath */
    float       starty;         /* starting y point of the subpath */

    float       lwidth;         /* line width */
    int         lcap;           /* line cap style */
    int         ljoin;          /* line join style */
    float       miter;          /* line join style */
    float       flatness;       /* line join style */
    pdc_bool    dashed;         /* line dashing in effect */

    /* LATER: rendering intent and flatness */
} pdf_gstate;

/* ------------------------ text state ---------------------------- */
typedef struct {
    float       c;              /* character spacing */
    float       w;              /* word spacing */
    float       h;              /* horizontal scaling */
    float       l;              /* leading */
    int         f;              /* slot number of the current font */
    float       fs;             /* font size */
    pdc_matrix  m;              /* text matrix */
    float       me;             /* last text matrix component e */
    int         mode;           /* text rendering mode */
    float       rise;           /* text rise */
    pdc_matrix  lm;             /* text line matrix */
    pdc_bool    potm;           /* text matrix must be output at begin text */
} pdf_tstate;

/* Force graphics or color operator output, avoiding the optimization
 * which checks whether the new value might be the same as the old.
 * This is especially required for Type 3 glyph descriptions which
 * inherit the surrounding page description's gstate parameters,
 * and therefore even must write default values.
 */
#define PDF_FORCE_OUTPUT() (PDF_GET_STATE(p) == pdf_state_glyph)

/*
 * *************************************************************************
 * The core PDF document descriptor
 * *************************************************************************
 */

struct PDF_s {
    /* -------------------------- general stuff ------------------------ */
    unsigned long       magic;          /* poor man's integrity check */
    void	(*freeproc)(PDF *p, void *mem);
    pdc_core    *pdc;                   /* core context */
    int         compatibility;          /* PDF version number * 10 */

    char        *binding;               /* name of the language binding */
    pdc_bool    hastobepos;             /* return value has to be positiv */

    pdf_state   state_stack[PDF_STATE_STACK_SIZE];
    int         state_sp;               /* state stack pointer */

    /* ------------------- PDF Info dictionary entries ----------------- */
    char        time_str[PDC_TIME_SBUF_SIZE];   /* time string */
    char        *Keywords;
    char        *Subject;
    char        *Title;
    char        *Creator;
    char        *Author;
    pdf_info    *userinfo;              /* list of user-defined entries */

    /* -------------- I/O, error handling and memory management ------------- */
    size_t	(*writeproc)(PDF *p, void *data, size_t size);
    void        (*errorhandler)(PDF *p, int level, const char* msg);
    void        *opaque;                /* user-specific, opaque data */

    /* ------------------------- PDF import ---------------------------- */
    pdf_pdi     *pdi;                   /* PDI context array */
    int         pdi_capacity;           /* currently allocated size */
    pdc_usebox  pdi_usebox;
    pdc_bool	pdi_strict;		/* strict PDF parser mode */
    pdc_sbuf *  pdi_sbuf;               /* string buffer for pdi parms */

    /* ------------------------ resource stuff ------------------------- */
    pdf_category *resources;            /* anchor for the resource list */
    pdc_bool     resfilepending;        /* to read resource file is pending */
    char         *resourcefilename;     /* name of the resource file */
    char         *prefix;               /* prefix for resource file names */

    /* ---------------- virtual file system stuff ----------------------- */
    pdf_virtfile *filesystem;           /* anchor for the virtual file system */

    /* ------------------- hypertext bookkeeping ------------------- */
    pdf_openmode open_mode;             /* document open mode */
    pdf_dest	open_action;            /* open action/first page display */
    pdf_dest 	bookmark_dest;          /* destination for bookmarks */
    char        *base;                  /* document base URI */

    /* ------------------- PDF output bookkeeping ------------------- */
    pdc_output  *out;                   /* output manager */
    pdc_id      length_id;              /* id of current stream's length*/
    pdf_flush_state flush;              /* flush state */

    /* ------------------- page bookkeeping ------------------- */
    pdc_id     *pages;                 /* page ids */
    int         pages_capacity;
    int         current_page;           /* current page number (1-based) */

    pdc_id     *pnodes;                /* page tree node ids */
    int         pnodes_capacity;        /* current # of entries in pnodes */
    int         current_pnode;          /* current node number (0-based) */
    int         current_pnode_kids;     /* current # of kids in current node */


    /* ------------------- document resources ------------------- */
    pdc_font    *fonts;                 /* all fonts in document */
    int         fonts_capacity;         /* currently allocated size */
    int         fonts_number;           /* next available font number */

    pdf_xobject *xobjects;              /* all xobjects in document */
    int         xobjects_capacity;      /* currently allocated size */
    int         xobjects_number;        /* next available xobject slot */

    pdf_colorspace *colorspaces;        /* all color space resources */
    int         colorspaces_capacity;   /* currently allocated size */
    int         colorspaces_number;     /* next available color space number */


    pdf_pattern *pattern;               /* all pattern resources */
    int         pattern_capacity;       /* currently allocated size */
    int         pattern_number;         /* next available pattern number */

    pdf_shading *shadings;               /* all shading resources */
    int         shadings_capacity;       /* currently allocated size */
    int         shadings_number;         /* next available shading number */

    pdf_extgstateresource *extgstates;  /* all ext. graphic state resources */
    int         extgstates_capacity;    /* currently allocated size */
    int         extgstates_number;      /* next available extgstate number */

    pdf_image  *images;                 /* all images in document */
    int         images_capacity;        /* currently allocated size */

    /* ------------------- encodings ------------------- */
    pdf_encoding *encodings;            /* all encodings in document */
    int         encodings_capacity;     /* currently allocated size */
    int         encodings_number;       /* next available encoding slot */

    /* ------------------- document outline tree ------------------- */
    int         outline_capacity;       /* currently allocated size */
    int         outline_count;          /* total number of outlines */
    pdf_outline *outlines;              /* dynamic array of outlines */

    /* ------------------- name tree ------------------- */
    pdf_name   *names;                  /* page ids */
    int         names_capacity;
    int         names_number;      	/* next available names number */

    /* ------------------- page specific stuff ------------------- */
    pdc_id      res_id;                 /* id of this page's res dict */
    pdc_id     *contents_ids;           /* content sections' chain */
    int         contents_ids_capacity;  /* # of content sections */
    pdc_id      next_content;           /* # of current content section */
    pdf_content_type    contents;       /* type of current content section */
    pdf_transition      transition;     /* type of page transition */
    float       duration;               /* duration of page transition */
    pdf_prefs	ViewerPreferences;	/* bit mask with preferences */

    pdc_t3font *t3font;                 /* type 3 font info */

    pdf_annot   *annots;                /* annotation chain */

    float       width;                  /* MediaBox: current page's width */
    float       height;                 /* MediaBox: current page's height */
    pdc_rectangle       CropBox;
    pdc_rectangle       BleedBox;
    pdc_rectangle       TrimBox;
    pdc_rectangle       ArtBox;
    pdc_id      thumb_id;               /* id of page's thumb, or PDC_BAD_ID */

    float       ydirection;             /* direction of y axis of default */
                                        /* system rel. to viewport (1 or -1) */

    pdc_bool    usercoordinates;        /* interprete rectangle coordinates */
                                        /* of hypertext funcs. in user space */

    int         sl;                             /* current save level */
    pdf_gstate  gstate[PDF_MAX_SAVE_LEVEL];     /* graphics state */
    pdf_tstate  tstate[PDF_MAX_SAVE_LEVEL];     /* text state */
    pdf_cstate  cstate[PDF_MAX_SAVE_LEVEL];     /* color state */


    pdf_renderingintent rendintent;     /* RenderingIntent */

    pdc_bool    preserveoldpantonenames;/* preserve old PANTONE names */
    pdc_bool    spotcolorlookup;        /* use internal look-up table for
                                         * color values */


    /* ------------------------ template stuff ----------------------- */
    int         templ;                  /* current template if in templ. state*/

    /* ------------ other text and graphics-related stuff ------------ */
    /* leading, word, and character spacing are treated as a group */
    pdc_text_format     textformat;     /* text storage format */
    pdc_bool    textparams_done;        /* text parameters already set */
    pdc_bool    underline;              /* underline text */
    pdc_bool    overline;               /* overline text */
    pdc_bool    strikeout;              /* strikeout text */
    pdc_bool    inheritgs;              /* inherit gstate in templates */
    pdf_fillrule        fillrule;       /* nonzero or evenodd fill rule */
    pdc_byte    *currtext;              /* current allocated text string */

    /* -------------- auxiliary API function parameters -------------- */
    /* PDF_add_launchlink() */
    char		*launchlink_parameters;
    char		*launchlink_operation;
    char		*launchlink_defaultdir;

    /* ------------ hypertext encoding and storage format ------------ */
    pdc_encoding        hypertextencoding;
    pdc_text_format     hypertextformat;

    /* -------------- annotation border style and color -------------- */
    pdf_border_style    border_style;
    float               border_width;
    float               border_red;
    float               border_green;
    float               border_blue;
    float               border_dash1;
    float               border_dash2;

    /* ------------------------ miscellaneous ------------------------ */
    char        debug[256];             /* debug flags */

    /* -------------------- private glyph tables ---------------------- */
    pdc_glyph_tab *unicode2name;        /* private unicode to glyphname table */
    pdc_glyph_tab *name2unicode;        /* private glyphname to unicode table */
    int           glyph_tab_capacity;   /* currently allocated size */
    int           glyph_tab_size;       /* size of glyph tables */
    pdc_ushort    next_unicode;         /* next available unicode number */

};

/* Data source for images, compression, ASCII encoding, fonts, etc. */
typedef struct PDF_data_source_s PDF_data_source;
struct PDF_data_source_s {
    pdc_byte            *next_byte;
    size_t              bytes_available;
    void                (*init)(PDF *, PDF_data_source *src);
    int                 (*fill)(PDF *, PDF_data_source *src);
    void                (*terminate)(PDF *, PDF_data_source *src);

    pdc_byte            *buffer_start;
    size_t              buffer_length;
    void                *private_data;
    long                offset;         /* start of data to read */
    long                length;         /* length of data to read */
    long                total;          /* total bytes read so far */
};

/* ------ Private functions for library-internal use only --------- */

/* p_basic.c */
void		pdf_begin_contents_section(PDF *p);
void		pdf_end_contents_section(PDF *p);
void		pdf_error(PDF *, int level, const char *fmt, ...);
pdc_bool	pdf_enter_api(PDF *p, const char *funame, pdf_state s,
		    const char *fmt, ...);
const char *	pdf_current_scope(PDF *p);
void		pdf_grow_pages(PDF *p);
void            pdf_check_handle(PDF *p, int value, pdc_opttype type);

/* p_text.c */
void    pdf_init_tstate(PDF *p);
void    pdf_reset_tstate(PDF *p);
void    pdf_end_text(PDF *p);
void    pdf_set_leading(PDF *p, float leading);
void    pdf_set_text_rise(PDF *p, float rise);
void    pdf_set_horiz_scaling(PDF *p, float scale);
void    pdf_set_text_rendering(PDF *p, int mode);
void    pdf_set_char_spacing(PDF *p, float spacing);
void    pdf_set_word_spacing(PDF *p, float spacing);
float   pdf_get_horiz_scaling(PDF *p);
float   pdf_get_fontsize(PDF *p);
void    pdf_output_kern_text(PDF *p, pdc_byte *text, int len, int charlen);
void    pdf_put_textstring(PDF *p, pdc_byte *text, int len, int charlen);
void    pdf_show_text(PDF *p, const char *text, int len,
                      const float *x_p, const float *y_p, pdc_bool cont);
float   pdf__stringwidth(PDF *p, const char *text, int len,
                         int font, float size);
void    pdf__setfont(PDF *p, int font, float fontsize);
void    pdf__fit_textline(PDF *p, const char *text, int len, float x, float y,
                          const char *optlist);

/* p_gstate.c */
void	pdf__save(PDF *p);
void	pdf__restore(PDF *p);
void    pdf_init_gstate(PDF *p);
void    pdf_concat_raw(PDF *p, pdc_matrix *m);
void    pdf_concat_raw_ob(PDF *p, pdc_matrix *m, pdc_bool blind);
void    pdf_reset_gstate(PDF *p);
void    pdf_set_topdownsystem(PDF *p, float height);
void	pdf__setmatrix(PDF *p, pdc_matrix *n);

/* p_extgstate.c */
void    pdf_init_extgstate(PDF *p);
void    pdf_write_page_extgstates(PDF *p);
void    pdf_write_doc_extgstates(PDF *p);
void    pdf_cleanup_extgstates(PDF *p);



/* p_filter.c */
void    pdf_ASCII85Encode(PDF *p, PDF_data_source *src);
void    pdf_ASCIIHexEncode(PDF *p, PDF_data_source *src);
int     pdf_data_source_buf_fill(PDF *p, PDF_data_source *src);

void    pdf_data_source_file_init(PDF *p, PDF_data_source *src);
int     pdf_data_source_file_fill(PDF *p, PDF_data_source *src);
void    pdf_data_source_file_terminate(PDF *p, PDF_data_source *src);

void    pdf_copy_stream(PDF *p, PDF_data_source *src, pdc_bool compress);

/* p_annots.c */
void    pdf_init_annots(PDF *p);
void    pdf_write_annots_root(PDF *p);
void    pdf_init_page_annots(PDF *p);
void    pdf_write_page_annots(PDF *p);
void    pdf_cleanup_page_annots(PDF *p);
void	pdf_cleanup_annots(PDF *p);

/* p_draw.c */
void    pdf_begin_path(PDF *p);
void    pdf_end_path(PDF *p);
void    pdf__moveto(PDF *p, float x, float y);
void    pdf__rmoveto(PDF *p, float x, float y);
void    pdf__lineto(PDF *p, float x, float y);
void    pdf__rlineto(PDF *p, float x, float y);
void    pdf__curveto(PDF *p, float x_1, float y_1,
                             float x_2, float y_2, float x_3, float y_3);
void    pdf__rcurveto(PDF *p, float x_1, float y_1,
                              float x_2, float y_2, float x_3, float y_3);
void    pdf__rrcurveto(PDF *p, float x_1, float y_1,
                               float x_2, float y_2, float x_3, float y_3);
void    pdf__hvcurveto(PDF *p, float x_1, float x_2, float y_2, float y_3);
void    pdf__vhcurveto(PDF *p, float y_1, float x_2, float y_2, float x_3);
void    pdf__rect(PDF *p, float x, float y, float width, float height);
void    pdf__arc(PDF *p, float x, float y, float r, float alpha, float beta);
void    pdf__arcn(PDF *p, float x, float y, float r, float alpha, float beta);
void    pdf__circle(PDF *p, float x, float y, float r);
void    pdf__closepath(PDF *p);
void    pdf__endpath(PDF *p);
void    pdf__stroke(PDF *p);
void    pdf__closepath_stroke(PDF *p);
void    pdf__fill(PDF *p);
void    pdf__fill_stroke(PDF *p);
void    pdf__closepath_fill_stroke(PDF *p);
void    pdf__clip(PDF *p);

/* p_gstate.c */
void	pdf__setdash(PDF *p, float b, float w);
void	pdf__setflat(PDF *p, float flat);
void	pdf__setlinejoin(PDF *p, int join);
void	pdf__setlinecap(PDF *p, int cap);
void    pdf__setlinewidth(PDF *p, float width);
void	pdf__setmiterlimit(PDF *p, float miter);
void    pdf__initgraphics(PDF *p);

/* p_hyper.c */
void	pdf_init_destination(PDF *p, pdf_dest *dest);
void	pdf_cleanup_destination(PDF *p, pdf_dest *dest);
void	pdf_parse_destination_optlist(PDF *p, const char *optlist,
            pdf_dest *dest, int page, pdf_destuse destuse);
void	pdf_write_destination(PDF *p, pdf_dest *dest);

void    pdf_init_outlines(PDF *p);
void    pdf_write_outlines(PDF *p);
void    pdf_write_outline_root(PDF *p);
void    pdf_cleanup_outlines(PDF *p);

void    pdf_init_transition(PDF *p);
void    pdf_write_page_transition(PDF *p);

void	pdf_feed_digest_info(PDF *p);
void    pdf_init_info(PDF *p);
pdc_id	pdf_write_info(PDF *p);
void    pdf_cleanup_info(PDF *p);

void	pdf_cleanup_names(PDF *p);
pdc_id	pdf_write_names(PDF *p);

void    pdf_set_transition(PDF *p, const char *type);
void    pdf_set_duration(PDF *p, float t);

char   *pdf_convert_hypertext(PDF *p, const char *text, int len);


/* p_resource.c */
void    pdf_add_resource(PDF *p, const char *category, const char *resource);
void    pdf_cleanup_resources(PDF *p);
void    pdf__create_pvf(PDF *p, const char *filename, int reserved,
                        const void *data, size_t size, const char *options);
int     pdf__delete_pvf(PDF *p, const char *filename, int reserved);
char   *pdf_find_resource(PDF *p, const char *category, const char *name);
void    pdf_lock_pvf(PDF *p, const char *filename);
void    pdf_unlock_pvf(PDF *p, const char *filename);
void    pdf_cleanup_filesystem(PDF *p);
pdc_file *pdf_fopen(PDF *p, const char *filename, const char *qualifier,
                    int flags);

/* p_encoding.c */
void          pdf__encoding_set_char(PDF *p, const char *encoding, int slot,
                                     const char *glyphname, int uv);
pdc_encodingvector *pdf_generate_encoding(PDF *p, const char *encoding);
pdc_encodingvector *pdf_read_encoding(PDF *p, const char *encoding,
                                      const char *filename);
pdc_ushort    pdf_glyphname2unicode(PDF *p, const char *glyphname);
const char   *pdf_unicode2glyphname(PDF *p, pdc_ushort uv);
pdc_ushort    pdf_insert_glyphname(PDF *p, const char *glyphname);
const char   *pdf_insert_unicode(PDF *p, pdc_ushort uv);
pdc_ushort    pdf_register_glyphname(PDF *p, const char *glyphname,
                                     pdc_ushort uv);
const char   *pdf_get_encoding_name(PDF *p, pdc_encoding enc);
pdc_encoding  pdf_insert_encoding(PDF *p, const char *encoding);
pdc_encoding  pdf_find_encoding(PDF *p, const char *encoding);
void          pdf_set_encoding_glyphnames(PDF *p, pdc_encoding enc);
pdc_bool      pdf_get_encoding_isstdflag(PDF *p, pdc_encoding enc);
void          pdf_init_encoding_ids(PDF *p);
void          pdf_init_encodings(PDF *p);
void          pdf_grow_encodings(PDF *p);
void          pdf_cleanup_encodings(PDF *p);

/* p_pattern.c */
void    pdf_init_pattern(PDF *p);
void    pdf_write_page_pattern(PDF *p);
void    pdf_cleanup_pattern(PDF *p);

/* p_shading.c */
void    pdf_init_shadings(PDF *p);
void    pdf_write_page_shadings(PDF *p);
void    pdf_cleanup_shadings(PDF *p);

/* p_xgstate.c */
pdc_id	pdf_get_gstate_id(PDF *p, int gstate);
#endif  /* P_INTERN_H */
