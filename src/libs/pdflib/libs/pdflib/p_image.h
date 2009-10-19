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

/* $Id: p_image.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Header file for the PDFlib image subsystem
 *
 */

#ifndef P_IMAGE_H
#define P_IMAGE_H

#ifdef HAVE_LIBTIFF
#include "tiffio.h"
#endif

#ifdef HAVE_LIBPNG
#include "png.h"
#endif

/* Type of image reference */
typedef enum
{
    pdf_img_auto,
    pdf_img_bmp,
    pdf_img_ccitt,
    pdf_img_gif,
    pdf_img_jpeg,
    pdf_img_png,
    pdf_img_raw,
    pdf_img_tiff
}
pdf_image_type;

#ifdef P_IMAGE_C
const pdc_keyconn pdf_image_keylist[] =
{
    {"AUTO",  pdf_img_auto},
    {"BMP",   pdf_img_bmp},
    {"CCITT", pdf_img_ccitt},
    {"GIF",   pdf_img_gif},
    {"JPEG",  pdf_img_jpeg},
    {"PNG",   pdf_img_png},
    {"RAW",   pdf_img_raw},
    {"TIFF",  pdf_img_tiff},
    {NULL, 0}
};
#endif /* P_IMAGE_C */

/* BMP specific image information */
typedef struct pdf_bmp_info_t {
    pdc_ulong           compression;    /* BMP compression */
    size_t              rowbytes;       /* length of row data */
    size_t              rowbytes_pad;   /* padded length of row data */
    size_t              rowbytes_pdf;   /* length of row data for PDF */
    size_t              skiprows;       /* number of rows to be skipped */
    pdc_byte           *bitmap;         /* bitmap buffer */
    pdc_byte           *end;            /* first byte above bitmap buffer */
    pdc_byte           *pos;            /* current position in bitmap buffer */
} pdf_bmp_info;

/* JPEG specific image information */
typedef struct pdf_jpeg_info_t {
    long                startpos;       /* start of image data in file */
} pdf_jpeg_info;

/* GIF specific image information */
typedef struct pdf_gif_info_t {
    int			c_size;		/* current code size (9..12)	*/
    int			t_size;		/* current table size (257...)	*/
    int			i_buff;		/* input buffer			*/
    int			i_bits;		/* # of bits in i_buff		*/
    int			o_buff;		/* output buffer		*/
    int			o_bits;		/* # of bits in o_buff		*/
} pdf_gif_info;


/* PNG specific image information */
typedef struct pdf_png_info_t {
    size_t		nbytes;		/* number of bytes left		*/
    					/* in current IDAT chunk	*/
#ifdef HAVE_LIBPNG
    png_structp		png_ptr;
    png_infop		info_ptr;
    png_uint_32		rowbytes;
    pdc_byte		*raster;
    int			cur_line;
#endif	/* HAVE_LIBPNG */
} pdf_png_info;


/* TIFF specific image information */
typedef struct pdf_tiff_info_t {
#ifdef HAVE_LIBTIFF
    TIFF		*tif;		/* pointer to TIFF data structure */
    uint32		*raster;	/* frame buffer */
#endif	/* HAVE_LIBTIFF */

    int			cur_line;	/* current image row or strip */
} pdf_tiff_info;

/* CCITT specific image information */
typedef struct pdf_ccitt_info_t {
    int			BitReverse;	/* reverse all bits prior to use */
} pdf_ccitt_info;

/* Type of image reference */
typedef enum {pdf_ref_direct, pdf_ref_file, pdf_ref_url} pdf_ref_type;

/* must be kept in sync with pdf_filter_names[] in p_image.c */
typedef enum { none, lzw, runlength, ccitt, dct, flate, jbig2 } pdf_compression;

typedef enum { pred_default = 1, pred_tiff = 2, pred_png = 15 } pdf_predictor;

/* The image descriptor */
struct pdf_image_s {
    pdc_file 		*fp;		/* image file pointer */
    char		*filename;	/* image file name or url */
    /* width and height in pixels, or in points for PDF pages and templates */
    float		width;		/* image width */
    float		height;		/* image height */
    pdf_compression	compression;	/* image compression type */
    int	                colorspace;	/* image color space */

    /*************************** option variables *****************************/
    pdc_bool            verbose;        /* put out warning/error messages */
    pdc_bool            bitreverse;     /* bitwise reversal of all bytes */
    int                 bpc;            /* bits per color component */
    int                 components;     /* number of color components */
    int                 height_pixel;   /* image height in pixel */
    pdc_bool            ignoremask;     /* ignore any transparency information*/
    pdc_bool            doinline;       /* inline image */
    pdc_bool            interpolate;    /* interpolate image   */
    pdc_bool		invert;		/* reverse black and white */
    int                 K;              /* encoding type of CCITT */
    pdc_bool            imagemask;     	/* create a mask from a 1-bit image */
    int                 mask;           /* image number of image mask */
    pdf_renderingintent ri;             /* rendering intent of image */
    int                 page;           /* page number of TIFF image */
    pdf_ref_type        reference;      /* kind of image data reference */
    int                 width_pixel;    /* image width in pixel */
    /**************************************************************************/

    pdc_bool		transparent;	/* image is transparent */
    pdc_byte		transval[4];	/* transparent color values */
    pdf_predictor	predictor;	/* predictor for lzw and flate */

    float		dpi_x;		/* horiz. resolution in dots per inch */
    float		dpi_y;		/* vert. resolution in dots per inch */
    					/* dpi is 0 if unknown */

    pdc_bool		in_use;		/* image slot currently in use */

    char		*params;	/* for TIFF */
    int			strips;		/* number of strips in image */
    int			rowsperstrip;	/* number of rows per strip */
    int                 pagehandle;     /* PDI page handle */
    int			dochandle;	/* PDI document handle */
    pdc_usebox		usebox;
    pdc_bool		use_raw;	/* use raw (compressed) image data */

    /* image format specific information */
    union {
        pdf_bmp_info    bmp;
	pdf_jpeg_info	jpeg;
	pdf_gif_info	gif;
	pdf_png_info	png;
	pdf_tiff_info	tiff;
	pdf_ccitt_info	ccitt;
    } info;

    int			no;		/* PDF image number */
    PDF_data_source	src;
};

/* xobject types */
typedef enum {
    image_xobject = 1 << 0,
    form_xobject = 1 << 1,
    pdi_xobject = 1 << 2
} pdf_xobj_type;

typedef enum {
    xobj_flag_used = 1 << 0,		/* in use */
    xobj_flag_write = 1 << 1		/* write at end of page */
} pdf_xobj_flags;

/* A PDF xobject */
struct pdf_xobject_s {
    pdc_id		obj_id;		/* object id of this xobject */
    int			flags;		/* bit mask of pdf_xobj_flags */
    pdf_xobj_type	type;		/* type of this xobject */
};

/* p_bmp.c */
int      pdf_process_BMP_data(PDF *p, int imageslot);
pdc_bool pdf_is_BMP_file(PDF *p, pdc_file *fp);

/* p_ccitt.c */
int      pdf_process_CCITT_data(PDF *p, int imageslot);
int      pdf_process_RAW_data(PDF *p, int imageslot);

/* p_gif.c */
int      pdf_process_GIF_data(PDF *p, int imageslot);
pdc_bool pdf_is_GIF_file(PDF *p, pdc_file *fp);

/* p_jpeg.c */
int      pdf_process_JPEG_data(PDF *p, int imageslot);
pdc_bool pdf_is_JPEG_file(PDF *p, pdc_file *fp, pdf_jpeg_info *jpeg);

/* p_png.c */
int	 pdf_process_PNG_data(PDF *p, int imageslot);
pdc_bool pdf_is_PNG_file(PDF *p, pdc_file *fp);

/* p_tiff.c */
int      pdf_process_TIFF_data(PDF *p, int imageslot);
pdc_bool pdf_is_TIFF_file(PDF *p, pdc_file *fp, pdf_tiff_info *tiff,
                          pdc_bool check);


/* p_image.c */
int	pdf_new_xobject(PDF *p, pdf_xobj_type type, pdc_id obj_id);
void	pdf_grow_images(PDF *p);
void	pdf_put_image(PDF *p, int im, pdc_bool firststrip);
void    pdf_put_inline_image(PDF *p, int im);

void	pdf_init_images(PDF *p);
void	pdf_cleanup_images(PDF *p);
void	pdf_cleanup_image(PDF *p, int im);
void	pdf_init_xobjects(PDF *p);
void	pdf_write_xobjects(PDF *p);
void	pdf_cleanup_xobjects(PDF *p);
void    pdf_place_xobject(PDF *p, int im, float x, float y,
			const char *optlist);
void	pdf__fit_image(PDF *p, int im, float x, float y, const char *optlist);
int	pdf__load_image(PDF *p, const char *type, const char *filename,
			const char *optlist);


#endif /* P_IMAGE_H */
