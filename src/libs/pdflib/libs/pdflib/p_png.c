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

/* $Id: p_png.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PNG processing for PDFlib
 *
 */

#include "p_intern.h"
#include "p_image.h"

/* SPNG - Simple PNG
**
** The items below, prefixed with spng_, or SPNG_, establish a replacement
** for LIBPNG that works very fast, but processes simple PNG images only:
**	- bit_depth <= 8 (no 16-bit)
**	- interlace_type 0 (no interlacing)
**	- color_type 0, 2, or 3 (no alpha-channel); color type 3 requires
**        libpng for palette processing
*/
#define SPNG_SIGNATURE	"\211\120\116\107\015\012\032\012"

#define SPNG_CHUNK_IHDR	0x49484452
#define SPNG_CHUNK_PLTE	0x504C5445
#define SPNG_CHUNK_tRNS	0x74524E53
#define SPNG_CHUNK_IDAT	0x49444154
#define SPNG_CHUNK_IEND	0x49454E44

/* spng_init() return codes
*/
#define SPNG_ERR_OK	0	/* no error */
#define SPNG_ERR_NOPNG	1	/* bad PNG signature */
#define SPNG_ERR_FMT	2	/* bad PNG file format */

typedef struct
{
    /* from IHDR:
    */
    int		width;
    int		height;
    pdc_byte	bit_depth;
    pdc_byte	color_type;
    pdc_byte	compr_type;
    pdc_byte	filter_type;
    pdc_byte	interlace_type;
} spng_info;

static int
spng_getint(pdc_file *fp)
{
    unsigned char buf[4];

    if (!PDC_OK_FREAD(fp, buf, 4))
	return -1;

    return (int) pdc_get_be_long(buf);
} /* spng_getint */

static int
spng_init(PDF *p, pdf_image *image, spng_info *spi)
{
    pdc_file *fp = image->fp;
    char buf[8];

    (void) p;

    /* check signature
    */
    if (!PDC_OK_FREAD(fp, buf, 8) ||
        strncmp(buf, SPNG_SIGNATURE, 8) != 0)
	return SPNG_ERR_NOPNG;

    /* read IHDR
    */
    if (spng_getint(fp) != 13 ||
        spng_getint(fp) != SPNG_CHUNK_IHDR)
	return SPNG_ERR_FMT;

    spi->width = spng_getint(fp);
    spi->height = spng_getint(fp);

    if (!PDC_OK_FREAD(fp, buf, 5))
	return SPNG_ERR_FMT;

    spi->bit_depth	= (pdc_byte) buf[0];
    spi->color_type	= (pdc_byte) buf[1];
    spi->compr_type	= (pdc_byte) buf[2];
    spi->filter_type	= (pdc_byte) buf[3];
    spi->interlace_type	= (pdc_byte) buf[4];

    (void) spng_getint(fp);     /* CRC */

    /* decide whether this image is "simple".
    */
#ifdef HAVE_LIBPNG
    if (spi->bit_depth > 8 || spi->color_type > 3 || spi->interlace_type != 0)
#else
    if (spi->bit_depth > 8 || spi->color_type > 2 || spi->interlace_type != 0)
#endif	/* !HAVE_LIBPNG */
    {
	image->use_raw = pdc_false;
	return SPNG_ERR_OK;
    }
    else
	image->use_raw = pdc_true;

    /* read (or skip) all chunks up to the first IDAT.
    */
    for (/* */ ; /* */ ; /* */)
    {
        int len = spng_getint(fp);
        int type = spng_getint(fp);

	switch (type)
	{
	    case SPNG_CHUNK_IDAT:		/* prepare data xfer	*/
		image->info.png.nbytes = (size_t) len;
		return SPNG_ERR_OK;

	    case -1:
		return SPNG_ERR_FMT;

	    /* if we decide to live without LIBPNG,
	    ** we should handle these cases, too.
	    */
	    case SPNG_CHUNK_tRNS:		/* transparency chunk	*/
	    case SPNG_CHUNK_PLTE:		/* read in palette	*/

	    default:
                pdc_fseek(fp, len + 4, SEEK_CUR);
                                                /* skip data & CRC      */
		break;
	} /* switch */
    }

    return SPNG_ERR_OK;
} /* spng_init */

static void
pdf_png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
    pdc_file *fp = (pdc_file *) png_ptr->io_ptr;
    char *filename = (char *) pdc_file_name(fp);

    if (!PDC_OK_FREAD(fp, data, length))
    {
	pdc_core *pdc = pdc_file_getpdc(fp);

        pdc_error(pdc, PDF_E_IMAGE_CORRUPT, "PNG", filename, 0, 0);
    }
}

#define PDF_PNG_BUFFERSIZE	8192

static void
pdf_data_source_PNG_init(PDF *p, PDF_data_source *src)
{
    static const char fn[] = "pdf_data_source_PNG_init";
    pdf_image	*image = (pdf_image *) src->private_data;

#ifdef HAVE_LIBPNG
    if (image->use_raw)
    {
#endif
	src->buffer_length = PDF_PNG_BUFFERSIZE;
	src->buffer_start = (pdc_byte *)
	    pdc_malloc(p->pdc, src->buffer_length, fn);
	src->bytes_available = 0;
	src->next_byte = src->buffer_start;

#ifdef HAVE_LIBPNG
    }
    else
    {
	image->info.png.cur_line = 0;
	src->buffer_length = image->info.png.rowbytes;
    }
#endif
}

#undef min
#define min(a, b) (((a) < (b)) ? (a) : (b))

static void
spng_error(PDF *p, PDF_data_source *src)
{
    pdf_image	*image = (pdf_image *) src->private_data;

    pdc_error(p->pdc, PDF_E_IMAGE_CORRUPT, "PNG", image->filename, 0, 0);
} /* spng_error */

static pdc_bool
pdf_data_source_PNG_fill(PDF *p, PDF_data_source *src)
{
    pdf_image	*image = (pdf_image *) src->private_data;

#ifdef HAVE_LIBPNG
    if (image->use_raw)
    {
#endif
	pdc_file *	fp = image->fp;
	size_t	buf_avail = src->buffer_length;
	pdc_byte *scan = src->buffer_start;

	src->bytes_available = 0;

	if (image->info.png.nbytes == 0)
	    return pdc_false;

	do
	{
	    size_t nbytes = min(image->info.png.nbytes, buf_avail);

            if (!PDC_OK_FREAD(fp, scan, nbytes))
		spng_error(p, src);

	    src->bytes_available += nbytes;
	    scan += nbytes;
	    buf_avail -= nbytes;

	    if ((image->info.png.nbytes -= nbytes) == 0)
	    {
		/* proceed to next IDAT chunk
		*/
                (void) spng_getint(fp);                    /* CRC */
                image->info.png.nbytes =
                     (size_t) spng_getint(fp);             /* length */

                if (spng_getint(fp) != SPNG_CHUNK_IDAT)
		{
		    image->info.png.nbytes = 0;
		    return pdc_true;
		}
	    }
	} while (buf_avail);

#ifdef HAVE_LIBPNG
    }
    else
    {
	if (image->info.png.cur_line == image->height)
	    return pdc_false;

	src->next_byte = image->info.png.raster +
	    image->info.png.cur_line * image->info.png.rowbytes;

	src->bytes_available = src->buffer_length;

	image->info.png.cur_line++;
    }
#endif	/* HAVE_LIBPNG */

    return pdc_true;
}

static void
pdf_data_source_PNG_terminate(PDF *p, PDF_data_source *src)
{
    pdf_image	*image = (pdf_image *) src->private_data;

#ifdef HAVE_LIBPNG
    if (image->use_raw)
    {
#endif
	pdc_free(p->pdc, (void *) src->buffer_start);

#ifdef HAVE_LIBPNG
    }
#endif
}

#ifdef HAVE_LIBPNG
/*
 * We suppress libpng's warning message by supplying
 * our own error and warning handlers
*/
static void
pdf_libpng_warning_handler(png_structp png_ptr, png_const_charp message)
{
    (void) png_ptr;	/* avoid compiler warning "unreferenced parameter" */
    (void) message;	/* avoid compiler warning "unreferenced parameter" */

    /* do nothing */
    return;
}

/*
 * use the libpng portability aid in an attempt to overcome version differences
 */
#ifndef png_jmpbuf
#define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

static void
pdf_libpng_error_handler(png_structp png_ptr, png_const_charp message)
{
    (void) message;	/* avoid compiler warning "unreferenced parameter" */

    longjmp(png_jmpbuf(png_ptr), 1);
}

static void *
pdf_libpng_malloc(png_structp png_ptr, size_t size)
{
    PDF *p = (PDF *)png_ptr->mem_ptr;

    return pdc_malloc(p->pdc, size, "libpng");
}

static void
pdf_libpng_free(png_structp png_ptr, void *mem)
{
    PDF *p = (PDF *)png_ptr->mem_ptr;

    pdc_free(p->pdc, mem);
}

pdc_bool
pdf_is_PNG_file(PDF *p, pdc_file *fp)
{
    pdc_byte sig[8];

    (void) p;

    if (!PDC_OK_FREAD(fp, sig, 8) || !png_check_sig(sig, 8)) {
        pdc_fseek(fp, 0L, SEEK_SET);
        return pdc_false;
    }
    return pdc_true;
}

int
pdf_process_PNG_data(
    PDF *p,
    int imageslot)
{
    static const char *fn = "pdf_process_PNG_data";
    pdc_file *save_fp;
    spng_info s_info;

    png_uint_32 width, height, ui;
    png_bytep *row_pointers, trans;
    png_color_8p sig_bit;
    png_color_16p trans_values;
    int bit_depth, color_type, i, num_trans;
    float dpi_x, dpi_y;
    pdf_image *image;
    volatile int errcode = 0;
    pdf_colorspace cs;
    pdf_colormap *colormap;
    volatile int slot;

    image = &p->images[imageslot];

    /*
     * We can install our own memory handlers in libpng since
     * our PNG library is specially extended to support this.
     * A custom version of libpng without support for
     * png_create_read_struct_2() is no longer supported.
     */

    image->info.png.png_ptr =
	png_create_read_struct_2(PNG_LIBPNG_VER_STRING, (png_voidp) NULL,
	pdf_libpng_error_handler, pdf_libpng_warning_handler,
	p, (png_malloc_ptr) pdf_libpng_malloc,
	(png_free_ptr) pdf_libpng_free);

    if (!image->info.png.png_ptr) {
        pdc_error(p->pdc, PDC_E_MEM_OUT, fn, 0, 0, 0);
    }

    image->info.png.info_ptr = png_create_info_struct(image->info.png.png_ptr);

    if (image->info.png.info_ptr == NULL) {
	png_destroy_read_struct(&image->info.png.png_ptr,
	    (png_infopp) NULL, (png_infopp) NULL);
        pdc_error(p->pdc, PDC_E_MEM_OUT, fn, 0, 0, 0);
    }

    if (setjmp(png_jmpbuf(image->info.png.png_ptr))) {
        errcode = PDF_E_IMAGE_CORRUPT;
        goto PDF_PNG_ERROR;
    }

    if (pdf_is_PNG_file(p, image->fp) == pdc_false) {
        errcode = PDC_E_IO_BADFORMAT;
        goto PDF_PNG_ERROR;
    }

    /* from file or from file or memory */
    png_set_read_fn(image->info.png.png_ptr, image->fp, pdf_png_read_data);

    png_set_sig_bytes(image->info.png.png_ptr, 8);
    png_read_info(image->info.png.png_ptr, image->info.png.info_ptr);
    png_get_IHDR(image->info.png.png_ptr, image->info.png.info_ptr,
	    &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    image->width	= (float) width;
    image->height	= (float) height;

    /* reduce 16-bit images to 8 bit since PDF stops at 8 bit */
    if (bit_depth == 16) {
	png_set_strip_16(image->info.png.png_ptr);
	bit_depth = 8;
    }

    image->bpc		= bit_depth;

    /*
     * Since PDF doesn't support a real alpha channel but only binary
     * tranparency ("poor man's alpha"), we do our best and treat
     * alpha values of up to 50% as transparent, and values above 50%
     * as opaque.
     *
     * Since this behaviour is not exactly what the image author had in mind,
     * it should probably be made user-configurable.
     *
     */
#define ALPHA_THRESHOLD 128

    switch (color_type) {
	case PNG_COLOR_TYPE_GRAY_ALPHA:
	    /* LATER: construct mask from alpha channel */
	    /*
	    png_set_IHDR(image->info.png.png_ptr, image->info.png.info_ptr,
	    	width, height, bit_depth, PNG_COLOR_MASK_ALPHA,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);
	    */
	    png_set_strip_alpha(image->info.png.png_ptr);
	    /* fall through */

	case PNG_COLOR_TYPE_GRAY:
	    if (png_get_sBIT(image->info.png.png_ptr,
	    		image->info.png.info_ptr, &sig_bit)) {
		png_set_shift(image->info.png.png_ptr, sig_bit);
	    }

		image->colorspace	= DeviceGray;

	    image->components	= 1;
	    break;

	case PNG_COLOR_TYPE_RGB_ALPHA:
	    /* LATER: construct mask from alpha channel */
	    png_set_strip_alpha(image->info.png.png_ptr);
	    /* fall through */

	case PNG_COLOR_TYPE_RGB:
	    if (image->colorspace == pdc_undef)
		image->colorspace	= DeviceRGB;
	    image->components	= 3;

	    break;

	case PNG_COLOR_TYPE_PALETTE:
	    png_get_PLTE(image->info.png.png_ptr, image->info.png.info_ptr,
		(png_colorp*) &colormap, &cs.val.indexed.palette_size);

	    image->components = 1;

	    /* This case should arguably be prohibited. However, we allow
	     * it and take the palette indices 0 and 1 as the mask,
	     * disregarding any color values in the palette.
	     */
	    if (image->imagemask) {
		image->colorspace = DeviceGray;
		break;
	    }

		cs.type = Indexed;
		cs.val.indexed.base = DeviceRGB;
		cs.val.indexed.colormap = colormap;
		cs.val.indexed.colormap_id = PDC_BAD_ID;
		slot = pdf_add_colorspace(p, &cs, pdc_false);

		image->colorspace = (pdf_colorspacetype) slot;


	    break;
    }

    if (image->imagemask)
    {
	if (image->components != 1) {
	    errcode = PDF_E_IMAGE_BADMASK;
	    goto PDF_PNG_ERROR;
	}

	if (p->compatibility <= PDC_1_3) {
	    if (image->components != 1 || image->bpc != 1) {
		errcode = PDF_E_IMAGE_MASK1BIT13;
		goto PDF_PNG_ERROR;
	    }
	} else if (image->bpc > 1) {
	    /* images with more than one bit will be written as /SMask,
	     * and don't require an /ImageMask entry.
	     */
	    image->imagemask = pdc_false;
	}
	image->colorspace = DeviceGray;
    }

    /* we invert this flag later */
    if (image->ignoremask)
        image->transparent = pdc_true;

    /* let libpng expand interlaced images */
    (void) png_set_interlace_handling(image->info.png.png_ptr);

    /* read the physical dimensions chunk to find the resolution values */
    dpi_x = (float) png_get_x_pixels_per_meter(image->info.png.png_ptr,
    		image->info.png.info_ptr);
    dpi_y = (float) png_get_y_pixels_per_meter(image->info.png.png_ptr,
    		image->info.png.info_ptr);

    if (dpi_x != 0 && dpi_y != 0) {	/* absolute values */
        image->dpi_x = dpi_x * (float) PDC_INCH2METER;
        image->dpi_y = dpi_y * (float) PDC_INCH2METER;

    } else {				/* aspect ratio */
	image->dpi_y = -png_get_pixel_aspect_ratio(image->info.png.png_ptr,
			    image->info.png.info_ptr);

	if (image->dpi_y == (float) 0)	/* unknown */
	    image->dpi_x = (float) 0;
	else
	    image->dpi_x = (float) -1.0;
    }

    /* read the transparency chunk */
    if (png_get_valid(image->info.png.png_ptr, image->info.png.info_ptr,
    	PNG_INFO_tRNS)) {
	png_get_tRNS(image->info.png.png_ptr, image->info.png.info_ptr,
	    &trans, &num_trans, &trans_values);
	if (num_trans > 0) {
	    if (color_type == PNG_COLOR_TYPE_GRAY) {
		image->transparent = !image->transparent;
		/* LATER: scale down 16-bit transparency values ? */
		image->transval[0] = (pdc_byte) trans_values[0].gray;

	    } else if (color_type == PNG_COLOR_TYPE_RGB) {
		image->transparent = !image->transparent;
		/* LATER: scale down 16-bit transparency values ? */
		image->transval[0] = (pdc_byte) trans_values[0].red;
		image->transval[1] = (pdc_byte) trans_values[0].green;
		image->transval[2] = (pdc_byte) trans_values[0].blue;

	    } else if (color_type == PNG_COLOR_TYPE_PALETTE) {
		/* we use the first transparent entry in the tRNS palette */
		for (i = 0; i < num_trans; i++) {
		    if ((pdc_byte) trans[i] < ALPHA_THRESHOLD) {
			image->transparent = !image->transparent;
			image->transval[0] = (pdc_byte) i;
			break;
		    }
		}
	    }
	}
    }



    png_read_update_info(image->info.png.png_ptr, image->info.png.info_ptr);

    image->info.png.rowbytes =
	png_get_rowbytes(image->info.png.png_ptr, image->info.png.info_ptr);

    image->info.png.raster = (pdc_byte *)
	pdc_calloc(p->pdc,image->info.png.rowbytes * height, fn);

    row_pointers = (png_bytep *)
	pdc_malloc(p->pdc, height * sizeof(png_bytep), fn);

    for (ui = 0; ui < height; ui++) {
	row_pointers[ui] =
	    image->info.png.raster + ui * image->info.png.rowbytes;
    }

    /* try the simple way:
    */
    save_fp = image->fp;

    image->src.init		= pdf_data_source_PNG_init;
    image->src.fill		= pdf_data_source_PNG_fill;
    image->src.terminate	= pdf_data_source_PNG_terminate;
    image->src.private_data	= (void *) image;


    image->fp = pdf_fopen(p, image->filename, NULL, PDC_FILE_BINARY);
    if (image->fp != NULL &&
        spng_init(p, image, &s_info) == SPNG_ERR_OK && image->use_raw)
    {
        pdc_fclose(save_fp);
	image->predictor	= pred_png;
	image->compression	= flate;
    }
    else
    {
	if (image->fp != (pdc_file *) 0)
            pdc_fclose(image->fp);

	image->fp = save_fp;

	/* fetch the actual image data */
	png_read_image(image->info.png.png_ptr, row_pointers);
    }

    image->in_use		= pdc_true;		/* mark slot as used */

    pdf_put_image(p, imageslot, pdc_true);

    pdc_free(p->pdc, image->info.png.raster);
    pdc_free(p->pdc, row_pointers);

    png_destroy_read_struct(&image->info.png.png_ptr,
	    &image->info.png.info_ptr, NULL);

    return imageslot;

    PDF_PNG_ERROR:
    png_destroy_read_struct(&image->info.png.png_ptr,
            &image->info.png.info_ptr, NULL);
    {
        const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
        switch (errcode)
        {
            case PDF_E_IMAGE_ICC:
            case PDF_E_IMAGE_ICC2:
            case PDF_E_IMAGE_MASK1BIT13:
            case PDF_E_IMAGE_BADMASK:
		pdc_set_errmsg(p->pdc, errcode, stemp, 0, 0, 0);
		break;

            case PDC_E_IO_BADFORMAT:
		pdc_set_errmsg(p->pdc, errcode, stemp, "PNG", 0, 0);
		break;

            case PDF_E_IMAGE_CORRUPT:
		pdc_set_errmsg(p->pdc, errcode, "PNG", stemp, 0, 0);
		break;

	    case 0: 		/* error code and message already set */
		break;
        }
    }

    if (image->verbose)
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    return -1;
}

#else	/* !HAVE_LIBPNG */

pdc_bool
pdf_is_PNG_file(PDF *p, pdc_file *fp)
{
    return pdc_false;
}

/* simple built-in PNG reader without libpng */

int
pdf_process_PNG_data(
    PDF *p,
    int imageslot)
{
    static const char fn[] = "pdf_process_PNG_data";
    spng_info s_info;
    pdf_image *image;

    image = &p->images[imageslot];

    image->src.init		= pdf_data_source_PNG_init;
    image->src.fill		= pdf_data_source_PNG_fill;
    image->src.terminate	= pdf_data_source_PNG_terminate;
    image->src.private_data	= (void *) image;

    if (spng_init(p, image, &s_info) == SPNG_ERR_OK && image->use_raw)
    {
	image->predictor	= pred_png;
	image->compression	= flate;

	image->width		= (float) s_info.width;
	image->height		= (float) s_info.height;
	image->bpc		= s_info.bit_depth;

	image->components	= 3;

	/* other types are rejected in spng_init() */
	image->colorspace	=
			(s_info.color_type == 0 ? DeviceGray : DeviceRGB);

    }
    else
    {
	pdc_set_errmsg(p->pdc, PDF_E_UNSUPP_IMAGE, "PNG", 0, 0, 0);

        if (image->verbose)
	    pdc_warning(p->pdc, -1, 0, 0, 0, 0);

	return -1;
    }

    image->in_use           = pdc_true;             /* mark slot as used */

    pdf_put_image(p, imageslot, pdc_true);

    return imageslot;

}

#endif	/* !HAVE_LIBPNG */
