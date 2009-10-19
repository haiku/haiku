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

/* $Id: p_gif.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * GIF processing for PDFlib
 *
 */

#include "p_intern.h"
#include "p_image.h"

#ifndef PDF_GIF_SUPPORTED

pdc_bool
pdf_is_GIF_file(PDF *p, pdc_file *fp)
{
    (void) p;
    (void) fp;

    return pdc_false;
}

int
pdf_process_GIF_data(
    PDF *p,
    int imageslot)
{
    (void) imageslot;

    pdc_warning(p->pdc, PDF_E_UNSUPP_IMAGE, "GIF", 0, 0, 0);
    return -1;
}

#else

#define LOCALCOLORMAP		0x80
#define BitSet(byteval, bitval)	(((byteval) & (bitval)) == (bitval))

static int ReadColorMap(pdc_core *pdc, pdc_file *fp,
                        int number, pdf_colormap *buffer);
static int DoExtension(PDF *p, pdf_image *image, int label);
static int GetDataBlock(PDF *p, pdf_image *image, unsigned char  *buf);

static void
pdf_data_source_GIF_init(PDF *p, PDF_data_source *src)
{
    pdf_image		*image = (pdf_image *) src->private_data;

    src->buffer_length	= 260;	/* max. GIF "data sub-block" length */

    src->buffer_start	= (pdc_byte*) pdc_malloc(p->pdc, src->buffer_length,
				"pdf_data_source_GIF_init");
    src->bytes_available= 0;
    src->next_byte	= src->buffer_start;

    /* init the LZW transformation vars */
    image->info.gif.c_size = 9;		/* initial code size	*/
    image->info.gif.t_size = 257;	/* initial "table" size	*/
    image->info.gif.i_buff = 0;		/* input buffer		*/
    image->info.gif.i_bits = 0;		/* input buffer empty	*/
    image->info.gif.o_bits = 0;		/* output buffer empty	*/
} /* pdf_data_source_GIF_init */

static pdc_bool
pdf_data_source_GIF_fill(PDF *p, PDF_data_source *src)
{
#define c_size	image->info.gif.c_size
#define t_size	image->info.gif.t_size
#define i_buff	image->info.gif.i_buff
#define i_bits	image->info.gif.i_bits
#define o_buff	image->info.gif.o_buff
#define o_bits	image->info.gif.o_bits

    pdf_image *		image = (pdf_image *) src->private_data;
    pdc_file *		fp = image->fp;
    int			n_bytes = pdc_fgetc(fp);
                                 /* # of bytes to read	*/
    unsigned char *	o_curr = src->buffer_start;
    int			c_mask = (1 << c_size) - 1;
    pdc_bool		flag13 = pdc_false;

    src->bytes_available = 0;

    if (n_bytes == EOF)
    {
        const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
	pdc_error(p->pdc, PDF_E_IMAGE_CORRUPT, "GIF", stemp, 0, 0);
    }

    if (n_bytes == 0)
	return pdc_false;

    for (/* */ ; /* */ ; /* */)
    {
	int w_bits = c_size;	/* number of bits to write */
	int code;

	/* get at least c_size bits into i_buff	*/
	while (i_bits < c_size)
	{
	    if (n_bytes-- == 0)
	    {
		src->bytes_available = (size_t) (o_curr - src->buffer_start);
		return pdc_true;
	    }
            /* EOF will be caught later */
            i_buff |= pdc_fgetc(fp) << i_bits;
	    i_bits += 8;
	}
	code = i_buff & c_mask;
	i_bits -= c_size;
	i_buff >>= c_size;

	if (flag13 && code != 256 && code != 257)
	{
            const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
            pdc_error(p->pdc, PDF_E_IMAGE_CORRUPT, "GIF", stemp, 0, 0);
	}

	if (o_bits > 0)
	{
	    o_buff |= code >> (c_size - 8 + o_bits);
	    w_bits -= 8 - o_bits;
	    *(o_curr++) = (unsigned char) o_buff;
	}
	if (w_bits >= 8)
	{
	    w_bits -= 8;
	    *(o_curr++) = (unsigned char) (code >> w_bits);
	}
	o_bits = w_bits;
	if (o_bits > 0)
	    o_buff = code << (8 - o_bits);

	++t_size;
	if (code == 256)	/* clear code */
	{
	    c_size = 9;
	    c_mask = (1 << c_size) - 1;
	    t_size = 257;
	    flag13 = pdc_false;
	}

	if (code == 257)	/* end code */
	{
	    src->bytes_available = (size_t) (o_curr - src->buffer_start);
	    return pdc_true;
	}

	if (t_size == (1 << c_size))
	{
	    if (++c_size > 12)
	    {
		--c_size;
		flag13 = pdc_true;
	    }
	    else
		c_mask = (1 << c_size) - 1;
	}
    } /* for (;;) */

#undef	c_size
#undef	t_size
#undef	i_buff
#undef	i_bits
#undef	o_buff
#undef	o_bits
} /* pdf_data_source_GIF_fill */

static void
pdf_data_source_GIF_terminate(PDF *p, PDF_data_source *src)
{
    pdc_free(p->pdc, (void *) src->buffer_start);
}

#define PDF_STRING_GIF  "\107\111\106"
#define PDF_STRING_87a  "\070\067\141"
#define PDF_STRING_89a  "\070\071\141"

pdc_bool
pdf_is_GIF_file(PDF *p, pdc_file *fp)
{
    unsigned char buf[3];

    (void) p;

    if (!PDC_OK_FREAD(fp, buf, 3) ||
        strncmp((const char *) buf, PDF_STRING_GIF, 3) != 0) {
        pdc_fseek(fp, 0L, SEEK_SET);
        return pdc_false;
    }
    return pdc_true;
}

int
pdf_process_GIF_data(
    PDF *p,
    int imageslot)
{
    static const char fn[] = "pdf_process_GIF_data";
    unsigned char	buf[16];
    char	c;
    int		imageCount = 0;
    char	version[4];
    int         errcode = 0;
    pdf_image	*image;
    pdf_colorspace cs;
    pdf_colormap colormap;
    int slot;

    image = &p->images[imageslot];


    /* we invert this flag later */
    if (image->ignoremask)
	image->transparent = pdc_true;

    if (image->page == pdc_undef)
        image->page = 1;

    /* Error reading magic number or not a GIF file */
    if (pdf_is_GIF_file(p, image->fp) == pdc_false) {
        errcode = PDC_E_IO_BADFORMAT;
        goto PDF_GIF_ERROR;
    }

    /* Version number */
    if (! PDC_OK_FREAD(image->fp, buf, 3)) {
        errcode = PDC_E_IO_BADFORMAT;
        goto PDF_GIF_ERROR;
    }
    strncpy(version, (const char *) buf, 3);
    version[3] = '\0';
    if ((strcmp(version, PDF_STRING_87a) != 0) &&
        (strcmp(version, PDF_STRING_89a) != 0)) {
        errcode = PDC_E_IO_BADFORMAT;
        goto PDF_GIF_ERROR;
    }

    /* Failed to read screen descriptor */
    if (! PDC_OK_FREAD(image->fp, buf, 7)) {
        errcode = PDC_E_IO_BADFORMAT;
        goto PDF_GIF_ERROR;
    }

    cs.type = Indexed;
    /* size of the global color table */
    cs.val.indexed.palette_size = 2 << (buf[4] & 0x07);
    cs.val.indexed.base = DeviceRGB;
    cs.val.indexed.colormap = &colormap;
    cs.val.indexed.colormap_id = PDC_BAD_ID;

    if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
        if (ReadColorMap(p->pdc, image->fp,
                         cs.val.indexed.palette_size, &colormap)) {
            errcode = PDF_E_IMAGE_COLORMAP;
            goto PDF_GIF_ERROR;
	}
    }

    /* translate the aspect ratio to PDFlib notation */
    if (buf[6] != 0) {
	image->dpi_x = -(buf[6] + ((float) 15.0)) / ((float) 64.0);
	image->dpi_y = (float) -1.0;
    }

    for (/* */ ; /* */ ; /* */) {
	/* EOF / read error in image data */
        if (!PDC_OK_FREAD(image->fp, &c, 1)) {
            errcode = PDC_E_IO_NODATA;
            goto PDF_GIF_ERROR;
	}

#define PDF_SEMICOLON		((char) 0x3b)		/* ASCII ';'  */

	if (c == PDF_SEMICOLON) {		/* GIF terminator */
	    /* Not enough images found in file */
	    if (imageCount < image->page) {
                if (!imageCount)
                    errcode = PDF_E_IMAGE_CORRUPT;
                else
                    errcode = PDF_E_IMAGE_NOPAGE;
                goto PDF_GIF_ERROR;
	    }
	    break;
	}

#define PDF_EXCLAM		((char) 0x21)		/* ASCII '!'  */

	if (c == PDF_EXCLAM) { 	/* Extension */
            if (!PDC_OK_FREAD(image->fp, &c, 1)) {
		/* EOF / read error on extension function code */
                errcode = PDC_E_IO_NODATA;
                goto PDF_GIF_ERROR;
	    }
	    DoExtension(p, image, (int) c);
	    continue;
	}

#define PDF_COMMA		((char) 0x2c)		/* ASCII ','  */

	if (c != PDF_COMMA) {		/* Not a valid start character */
	    /* Bogus character, ignoring */
	    continue;
	}

	++imageCount;

        if (! PDC_OK_FREAD(image->fp, buf, 9)) {
	    /* Couldn't read left/top/width/height */
            errcode = PDC_E_IO_NODATA;
            goto PDF_GIF_ERROR;
	}

	image->components	= 1;
	image->bpc		= 8;
        image->width            = (float) pdc_get_le_ushort(&buf[4]);
        image->height           = (float) pdc_get_le_ushort(&buf[6]);

	if (image->imagemask)
	{
	    if (p->compatibility <= PDC_1_3) {
		errcode = PDF_E_IMAGE_MASK1BIT13;
		goto PDF_GIF_ERROR;
	    } else {
		/* images with more than one bit will be written as /SMask,
		 * and don't require an /ImageMask entry.
		 */
		image->imagemask = pdc_false;
	    }
	    image->colorspace = DeviceGray;
	}

#define INTERLACE		0x40
	if (BitSet(buf[8], INTERLACE)) {
            errcode = PDF_E_GIF_INTERLACED;
            goto PDF_GIF_ERROR;
	}

	if (BitSet(buf[8], LOCALCOLORMAP)) {
            if (ReadColorMap(p->pdc, image->fp,
                             cs.val.indexed.palette_size, &colormap))
	    {
                errcode = PDF_E_IMAGE_COLORMAP;
                goto PDF_GIF_ERROR;
	    }
	}

	/* read the "LZW initial code size".
	*/
        if (!PDC_OK_FREAD(image->fp, buf, 1)) {
            errcode = PDC_E_IO_NODATA;
            goto PDF_GIF_ERROR;
	}
        if (buf[0] != 8) {
            if (imageCount > 1)
                errcode = PDF_E_IMAGE_NOPAGE;
            else
                errcode = PDF_E_GIF_LZWSIZE;
            goto PDF_GIF_ERROR;
	}

	if (imageCount == image->page)
	    break;
    }

    image->src.init		= pdf_data_source_GIF_init;
    image->src.fill		= pdf_data_source_GIF_fill;
    image->src.terminate	= pdf_data_source_GIF_terminate;
    image->src.private_data	= (void *) image;

    image->compression		= lzw;
    image->use_raw  		= pdc_true;

    image->params = (char *) pdc_malloc(p->pdc, PDF_MAX_PARAMSTRING, fn);
    strcpy(image->params, "/EarlyChange 0");

    image->in_use               = pdc_true;             /* mark slot as used */

	slot = pdf_add_colorspace(p, &cs, pdc_false);
	image->colorspace = (pdf_colorspacetype) slot;



    pdf_put_image(p, imageslot, pdc_true);

    return imageslot;

    PDF_GIF_ERROR:
    {
        const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
        switch (errcode)
        {
            case PDC_E_IO_NODATA:
            case PDF_E_IMAGE_COLORMAP:
            case PDF_E_GIF_INTERLACED:
            case PDF_E_GIF_LZWSIZE:
		pdc_set_errmsg(p->pdc, errcode, stemp, 0, 0, 0);
		break;

            case PDC_E_IO_BADFORMAT:
		pdc_set_errmsg(p->pdc, errcode, stemp, "GIF", 0, 0);
		break;

            case PDF_E_IMAGE_CORRUPT:
		pdc_set_errmsg(p->pdc, errcode, "GIF", stemp, 0, 0);
		break;

            case PDF_E_IMAGE_NOPAGE:
		pdc_set_errmsg(p->pdc, errcode,
		    pdc_errprintf(p->pdc, "%d", image->page), "GIF", stemp, 0);
		break;

	    case 0: 		/* error code and message already set */
		break;
        }
    }

    if (image->verbose)
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    return -1;
} /* pdf_open_GIF_data */

static int
ReadColorMap(pdc_core *pdc, pdc_file *fp, int number, pdf_colormap *buffer)
{
    int		i;
    unsigned char	rgb[3];

    (void) pdc;

    for (i = 0; i < number; ++i) {
        if (! PDC_OK_FREAD(fp, rgb, sizeof(rgb))) {
	    return pdc_true;		/* yk: true == error */
	}

	(*buffer)[i][0] = rgb[0] ;
	(*buffer)[i][1] = rgb[1] ;
	(*buffer)[i][2] = rgb[2] ;
    }
    return pdc_false;			/* yk: false == ok.  */
} /* ReadColorMap */

static int
DoExtension(PDF *p, pdf_image *image, int label)
{
    pdc_byte            buf[256];

    switch ((unsigned char) label) {
	case 0x01:		/* Plain Text Extension */
	    break;

	case 0xff:		/* Application Extension */
	    break;

	case 0xfe:		/* Comment Extension */
	    while (GetDataBlock(p, image, (unsigned char*) buf) != 0) {
		/* */
	    }
	    return pdc_false;

	case 0xf9:		/* Graphic Control Extension */
	    (void) GetDataBlock(p, image, (unsigned char*) buf);

	    if ((buf[0] & 0x1) != 0) {
		image->transparent = !image->transparent;
		image->transval[0] = buf[3];
	    }

	    while (GetDataBlock(p, image, (unsigned char*) buf) != 0) {
		    /* */ ;
	    }
	    return pdc_false;

	default:
	    break;
    }

    while (GetDataBlock(p, image, (unsigned char*) buf) != 0) {
	    /* */ ;
    }

    return pdc_false;
} /* DoExtension */

static int
GetDataBlock(PDF *p, pdf_image *image, unsigned char *buf)
{
    unsigned char	count;
    pdc_file *fp = image->fp;

    if ((!PDC_OK_FREAD(fp, &count, 1)) ||
        ((count != 0) && (!PDC_OK_FREAD(fp, buf, count))))
    {
        const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
        pdc_error(p->pdc, PDF_E_IMAGE_CORRUPT, "GIF", stemp, 0, 0);
    }

    return count;
} /* GetDataBlock */

#endif  /* PDF_GIF_SUPPORTED */
