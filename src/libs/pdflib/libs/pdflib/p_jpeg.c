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

/* $Id: p_jpeg.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * JPEG processing for PDFlib
 *
 */

#include "p_intern.h"
#include "p_image.h"

#ifndef PDF_JPEG_SUPPORTED

pdc_bool
pdf_is_JPEG_file(PDF *p, pdc_file *fp, pdf_jpeg_info *jpeg)
{
    (void) p;
    (void) fp;

    return pdc_false;
}

int
pdf_process_JPEG_data(
    PDF *p,
    int imageslot)
{
    (void) imageslot;

    pdc_warning(p->pdc, PDF_E_UNSUPP_IMAGE, "JPEG", 0, 0, 0);
    return -1;
}

#else

/*
 * The following enum is stolen from the IJG JPEG library
 * Comments added by tm.
 * This table contains far too many names since PDFlib
 * is rather simple-minded about markers.
 */

typedef enum {		/* JPEG marker codes			*/
  M_SOF0  = 0xc0,	/* baseline DCT				*/
  M_SOF1  = 0xc1,	/* extended sequential DCT		*/
  M_SOF2  = 0xc2,	/* progressive DCT			*/
  M_SOF3  = 0xc3,	/* lossless (sequential)		*/

  M_SOF5  = 0xc5,	/* differential sequential DCT		*/
  M_SOF6  = 0xc6,	/* differential progressive DCT		*/
  M_SOF7  = 0xc7,	/* differential lossless		*/

  M_JPG   = 0xc8,	/* JPEG extensions			*/
  M_SOF9  = 0xc9,	/* extended sequential DCT		*/
  M_SOF10 = 0xca,	/* progressive DCT			*/
  M_SOF11 = 0xcb,	/* lossless (sequential)		*/

  M_SOF13 = 0xcd,	/* differential sequential DCT		*/
  M_SOF14 = 0xce,	/* differential progressive DCT		*/
  M_SOF15 = 0xcf,	/* differential lossless		*/

  M_DHT   = 0xc4,	/* define Huffman tables		*/

  M_DAC   = 0xcc,	/* define arithmetic conditioning table	*/

  M_RST0  = 0xd0,	/* restart				*/
  M_RST1  = 0xd1,	/* restart				*/
  M_RST2  = 0xd2,	/* restart				*/
  M_RST3  = 0xd3,	/* restart				*/
  M_RST4  = 0xd4,	/* restart				*/
  M_RST5  = 0xd5,	/* restart				*/
  M_RST6  = 0xd6,	/* restart				*/
  M_RST7  = 0xd7,	/* restart				*/

  M_SOI   = 0xd8,	/* start of image			*/
  M_EOI   = 0xd9,	/* end of image				*/
  M_SOS   = 0xda,	/* start of scan			*/
  M_DQT   = 0xdb,	/* define quantization tables		*/
  M_DNL   = 0xdc,	/* define number of lines		*/
  M_DRI   = 0xdd,	/* define restart interval		*/
  M_DHP   = 0xde,	/* define hierarchical progression	*/
  M_EXP   = 0xdf,	/* expand reference image(s)		*/

  M_APP0  = 0xe0,	/* application marker, used for JFIF	*/
  M_APP1  = 0xe1,	/* application marker, used for Exif	*/
  M_APP2  = 0xe2,	/* application marker, used for FlashPix*
                         * and ICC Profiles                     */
  M_APP3  = 0xe3,	/* application marker			*/
  M_APP4  = 0xe4,	/* application marker			*/
  M_APP5  = 0xe5,	/* application marker			*/
  M_APP6  = 0xe6,	/* application marker			*/
  M_APP7  = 0xe7,	/* application marker			*/
  M_APP8  = 0xe8,	/* application marker, used for SPIFF	*/
  M_APP9  = 0xe9,	/* application marker			*/
  M_APP10 = 0xea,	/* application marker			*/
  M_APP11 = 0xeb,	/* application marker			*/
  M_APP12 = 0xec,	/* application marker			*/
  M_APP13 = 0xed,	/* application marker, used by Photoshop*/
  M_APP14 = 0xee,	/* application marker, used by Adobe	*/
  M_APP15 = 0xef,	/* application marker			*/

  M_JPG0  = 0xf0,	/* reserved for JPEG extensions		*/
  M_JPG13 = 0xfd,	/* reserved for JPEG extensions		*/
  M_COM   = 0xfe,	/* comment				*/

  M_TEM   = 0x01,	/* temporary use			*/

  M_ERROR = 0x100,      /* dummy marker, internal use only      */
  M_CONT  = 0x101       /* dummy marker, internal use only      */
} JPEG_MARKER;

#define JPEG_BUFSIZE	1024

static void
pdf_data_source_JPEG_init(PDF *p, PDF_data_source *src)
{
  pdf_image	*image;

  image = (pdf_image *) src->private_data;

  src->buffer_start = (pdc_byte *)
  	pdc_malloc(p->pdc, JPEG_BUFSIZE, "PDF_data_source_JPEG_init");
  src->buffer_length = JPEG_BUFSIZE;

  pdc_fseek(image->fp, image->info.jpeg.startpos, SEEK_SET);
}

static pdc_bool
pdf_data_source_JPEG_fill(PDF *p, PDF_data_source *src)
{
  pdf_image	*image;

  (void) p;	/* avoid compiler warning "unreferenced parameter" */

  image = (pdf_image *) src->private_data;

  src->next_byte = src->buffer_start;
  src->bytes_available =
      pdc_fread(src->buffer_start, 1, JPEG_BUFSIZE, image->fp);

  if (src->bytes_available == 0)
    return pdc_false;
  else
    return pdc_true;
}

static void
pdf_data_source_JPEG_terminate(PDF *p, PDF_data_source *src)
{
  pdc_free(p->pdc, (void *) src->buffer_start);
}

/*
 * The following routine used to be a macro in its first incarnation:
 *
 * #define get_2bytes(fp) ((unsigned int) (getc(fp) << 8) + getc(fp))
 *
 * However, this is bad programming since C doesn't guarantee
 * the evaluation order of the getc() calls! As suggested by
 * Murphy's law, there are indeed compilers which produce the wrong
 * order of the getc() calls, e.g. the Metrowerks C compiler for BeOS.
 * Since there are only a few calls we don't care about the performance
 * penalty and use a simplistic C function which does the right thing.
 */

/* read two byte parameter, MSB first */
static unsigned int
get_2bytes(pdc_file *fp)
{
    unsigned int val;
    val = (unsigned int) (pdc_fgetc(fp) << 8);
    val += (unsigned int) pdc_fgetc(fp);
    return val;
}

static int
pdf_next_jpeg_marker(pdc_file *fp)
{ /* look for next JPEG Marker  */
  int c;

  do {
    do {                            /* skip to FF 		  */
      if (pdc_feof(fp))
	return M_ERROR;             /* dummy marker               */
      c = pdc_fgetc(fp);
    } while (c != 0xFF);

    do {                            /* skip repeated FFs  	  */
      if (pdc_feof(fp))
	return M_ERROR;             /* dummy marker               */
      c = pdc_fgetc(fp);
    } while (c == 0xFF);
  } while (c == 0);                 /* repeat if FF/00 	      	  */

  return c;
}

pdc_bool
pdf_is_JPEG_file(PDF *p, pdc_file *fp, pdf_jpeg_info *jpeg)
{
    int c;

    (void) p;

#if defined(MVS) && defined(I370)
    jpeg->startpos = 0L;
#else
    /* Tommy's special trick for Macintosh JPEGs: simply skip some  */
    /* hundred bytes at the beginning of the file!                  */
    do {
        do {                            /* skip if not FF           */
            c = pdc_fgetc(fp);
        } while (!pdc_feof(fp) && c != 0xFF);

        if (pdc_feof(fp)) {
            pdc_fseek(fp, 0L, SEEK_SET);
            return pdc_false;
        }

        do {                            /* skip repeated FFs        */
            c = pdc_fgetc(fp);
        } while (c == 0xFF);

        /* remember start position */
        if ((jpeg->startpos = pdc_ftell(fp)) < 0L) {
            pdc_fseek(fp, 0L, SEEK_SET);
            return pdc_false;
        }

        jpeg->startpos -= 2;     /* subtract marker length     */

        if (c == M_SOI) {
            pdc_fseek(fp, jpeg->startpos, SEEK_SET);
            break;
        }
    } while (!pdc_feof(fp));
#endif  /* !MVS || !I370 */

#define BOGUS_LENGTH    768
    /* Heuristics: if we are that far from the start chances are
     * it is a TIFF file with embedded JPEG data which we cannot
     * handle - regard as hopeless...
     */
    if (pdc_feof(fp) || jpeg->startpos > BOGUS_LENGTH) {
        pdc_fseek(fp, 0L, SEEK_SET);
        return pdc_false;
    }

    return pdc_true;
}

/* open JPEG image and analyze marker */
int
pdf_process_JPEG_data(
    PDF *p,
    int imageslot)
{
    static const char fn[] = "pdf_process_JPEG_data";
    int b, c, unit;
    unsigned long i, length;
#define APP_MAX 255
    unsigned char appstring[APP_MAX];
    pdc_byte *app13;
    pdc_byte *s;
    pdf_image *image;
    pdc_bool adobeflag = pdc_false;
    pdc_bool markers_done = pdc_false;
    int errint = 0;
    int errcode = 0;

    image = &p->images[imageslot];
    image->compression = dct;
    image->use_raw = pdc_true;

    /* jpeg file not available */
    if (image->reference != pdf_ref_direct)
    {

        image->in_use = pdc_true;                   /* mark slot as used */
        pdf_put_image(p, imageslot, pdc_true);
        return imageslot;
    }

    if (pdf_is_JPEG_file(p, image->fp, &image->info.jpeg) == pdc_false) {
        errcode = PDF_E_IMAGE_CORRUPT;
        goto PDF_JPEG_ERROR;
    }

    image->src.init		= pdf_data_source_JPEG_init;
    image->src.fill		= pdf_data_source_JPEG_fill;
    image->src.terminate	= pdf_data_source_JPEG_terminate;
    image->src.private_data	= (void *) image;

  /* process JPEG markers */
  while (!markers_done && (c = pdf_next_jpeg_marker(image->fp)) != M_EOI) {


    switch (c) {
      case M_ERROR:
      /* The following are not supported in PDF 1.3 and above */
      case M_SOF3:
      case M_SOF5:
      case M_SOF6:
      case M_SOF7:
      case M_SOF9:
      case M_SOF11:
      case M_SOF13:
      case M_SOF14:
      case M_SOF15:
          errint = c;
          errcode = PDF_E_JPEG_COMPRESSION;
          goto PDF_JPEG_ERROR;


      /* SOF2 and SOF10 are progressive DCT */
      case M_SOF2:
      case M_SOF10:
	/* fallthrough */

      case M_SOF0:
      case M_SOF1:
        (void) get_2bytes(image->fp);    /* read segment length  */

        image->bpc               = pdc_fgetc(image->fp);
        image->height            = (float) get_2bytes(image->fp);
        image->width             = (float) get_2bytes(image->fp);
        image->components        = pdc_fgetc(image->fp);

	/*
	 * No need to read more markers since multiscan detection
	 * not required for single-component images.
	 */
	if (image->components == 1)
	    markers_done = pdc_true;

	break;

      case M_APP0:		/* check for JFIF marker with resolution */
        length = get_2bytes(image->fp);

	for (i = 0; i < length-2; i++) {	/* get contents of marker */
          b = pdc_fgetc(image->fp);
	  if (i < APP_MAX)			/* store marker in appstring */
	    appstring[i] = (unsigned char) b;
	}

	/* Check for JFIF application marker and read density values
	 * per JFIF spec version 1.02.
	 */

#define JFIF_ASPECT_RATIO	0	/* JFIF unit byte: aspect ratio only */
#define JFIF_DOTS_PER_INCH	1	/* JFIF unit byte: dots per inch     */
#define JFIF_DOTS_PER_CM	2	/* JFIF unit byte: dots per cm       */

#define PDF_STRING_JFIF	"\112\106\111\106"

	if (length >= 14 && !strncmp(PDF_STRING_JFIF, (char *) appstring, 4)) {
	  unit = appstring[7];		        /* resolution unit */
	  					/* resolution value */
	  image->dpi_x = (float) ((appstring[8]<<8) + appstring[9]);
	  image->dpi_y = (float) ((appstring[10]<<8) + appstring[11]);

	  if (image->dpi_x <= (float) 0.0 || image->dpi_y <= (float) 0.0) {
	    image->dpi_x = (float) 0.0;
	    image->dpi_y = (float) 0.0;
	    break;
	  }

	  switch (unit) {
	    case JFIF_DOTS_PER_INCH:
	      break;

	    case JFIF_DOTS_PER_CM:
	      image->dpi_x *= (float) 2.54;
	      image->dpi_y *= (float) 2.54;
	      break;

	    case JFIF_ASPECT_RATIO:
	      image->dpi_x *= -1;
	      image->dpi_y *= -1;
	      break;

	    default:				/* unknown ==> ignore */
		/* */ ;
	  }
	}

        break;


#ifdef NYI
        /* LATER: read SPIFF marker */
      case M_APP8:				/* check for SPIFF marker */

        break;
#endif

      case M_APP13:				/* check for Photoshop marker */
        length = get_2bytes(image->fp);

	/* get marker contents */
	length -= 2;			/* account for two length bytes */
	app13 = (pdc_byte*)pdc_malloc(p->pdc, length, fn);

        if (!PDC_OK_FREAD(image->fp, app13, length)) {
	    pdc_free(p->pdc, app13);
            errcode = PDF_E_IMAGE_CORRUPT;
            goto PDF_JPEG_ERROR;
	}

#define PS_HEADER_LEN		14
#define PDF_STRING_Photoshop	"\120\150\157\164\157\163\150\157\160"
#define PDF_STRING_8BIM		"\070\102\111\115"
#define RESOLUTION_INFO_ID	0x03ED	/* resolution info resource block */
#define PS_FIXED_TO_FLOAT(h, l)	((float) (h) + ((float) (l)/(1<<16)))

	/* Not a valid Photoshop marker */
	if (length < 9 || strncmp(PDF_STRING_Photoshop, (char *) app13, 9)) {
	    pdc_free(p->pdc, app13);
	    break;
	}

	/* walk all image resource blocks and look for ResolutionInfo */
	for (s = app13 + PS_HEADER_LEN; s < app13 + length; /* */) {
	    long len;
	    unsigned int type;

	    if (strncmp((char *) s, PDF_STRING_8BIM, 4))
		break;	/* out of sync */
	    s += 4;	/* identifying string */

	    type = (unsigned int) ((s[0]<<8) + s[1]);
	    s += 2;	/* resource type */

	    s += *s + ((*s & 1) ? 1 : 2);	/* resource name */

	    len = (((((s[0]<<8) + s[1])<<8) + s[2])<<8) + s[3];
	    s += 4;	/* Size */

	    if (type == RESOLUTION_INFO_ID && len >= 16) {
		  image->dpi_x =
			PS_FIXED_TO_FLOAT((s[0]<<8) + s[1], (s[2]<<8) + s[3]);
		  image->dpi_y =
			PS_FIXED_TO_FLOAT((s[8]<<8) + s[9], (s[10]<<8) + s[11]);
		  break;
	    }

	    s += len + ((len & 1) ? 1 : 0); 	/* Data */
	}

	pdc_free(p->pdc, app13);
        break;

      case M_APP14:				/* check for Adobe marker */
        length = get_2bytes(image->fp);

	for (i = 0; i < length-2; i++) {	/* get contents of marker */
          b = pdc_fgetc(image->fp);
	  if (i < APP_MAX)			/* store marker in appstring */
	    appstring[i] = (unsigned char) b;
	  else
	    break;
	}

	/*
	 * Check for Adobe application marker. It is known (per Adobe's TN5116)
	 * to contain the string "Adobe" at the start of the APP14 marker.
	 */
#define PDF_STRING_Adobe	"\101\144\157\142\145"

	if (length >= 12 && !strncmp(PDF_STRING_Adobe, (char *) appstring, 5))
	  adobeflag = pdc_true;		/* set Adobe flag */

	break;

      case M_SOS:
	markers_done = pdc_true;
        length = get_2bytes(image->fp);

	/*
	 * If the scan doesn't contain all components it must be
	 * a multiscan image, which doesn't work in Acrobat.
	 */
        if (pdc_fgetc(image->fp) < image->components) {
            errcode = PDF_E_JPEG_MULTISCAN;
            goto PDF_JPEG_ERROR;
	}

	/* account for the "number of components in scan" byte just read */
	for (length -= 3; length > 0; length--) {
          if (pdc_feof(image->fp)) {
              errcode = PDF_E_IMAGE_CORRUPT;
              goto PDF_JPEG_ERROR;
	  }
          (void) pdc_fgetc(image->fp);
	}
	break;

      case M_SOI:		/* ignore markers without parameters */
      case M_EOI:
      case M_TEM:
      case M_RST0:
      case M_RST1:
      case M_RST2:
      case M_RST3:
      case M_RST4:
      case M_RST5:
      case M_RST6:
      case M_RST7:
	break;

      default:			/* skip variable length markers */
        length = get_2bytes(image->fp);
	for (length -= 2; length > 0; length--) {
          if (pdc_feof(image->fp)) {
              errcode = PDF_E_IMAGE_CORRUPT;
              goto PDF_JPEG_ERROR;
	  }
          (void) pdc_fgetc(image->fp);
	}
	break;
    }
  }

    /* do some sanity checks with the parameters */
    if (image->height <= 0 || image->width <= 0 || image->components <= 0) {
        errcode = PDF_E_IMAGE_CORRUPT;
        goto PDF_JPEG_ERROR;
    }

    if (image->bpc != 8) {
        errint = image->bpc;
        errcode = PDF_E_IMAGE_BADDEPTH;
        goto PDF_JPEG_ERROR;
    }


    {
        switch (image->components) {
            case 1:
		/* spot color may have been applied */
                if (image->colorspace == pdc_undef)
                    image->colorspace = DeviceGray;
                break;

            case 3:
		image->colorspace = DeviceRGB;
                break;

            case 4:
                image->colorspace = DeviceCMYK;
                break;

            default:
                errint = image->components;
                errcode = PDF_E_IMAGE_BADCOMP;
                goto PDF_JPEG_ERROR;
        }
    }


    if (image->imagemask)
    {
	if (image->components != 1) {
	    errcode = PDF_E_IMAGE_BADMASK;
	    goto PDF_JPEG_ERROR;
	}

	if (p->compatibility <= PDC_1_3) {
	    errcode = PDF_E_IMAGE_MASK1BIT13;
	    goto PDF_JPEG_ERROR;
	} else {
	    /* images with more than one bit will be written as /SMask,
	     * and don't require an /ImageMask entry.
	     */
	    image->imagemask = pdc_false;
	}
    }

    /* special handling of Photoshop-generated CMYK JPEG files */
    if (adobeflag && p->colorspaces[image->colorspace].type == DeviceCMYK) {
	image->invert = !image->invert;
    }

    image->in_use	= pdc_true;		/* mark slot as used */

    if (image->doinline)
        pdf_put_inline_image(p, imageslot);
    else
        pdf_put_image(p, imageslot, pdc_true);

    return imageslot;

    PDF_JPEG_ERROR:
    {
        const char *stemp = pdc_errprintf(p->pdc, "%s", image->filename);
        switch (errcode)
        {
            case PDF_E_IMAGE_ICC:
            case PDF_E_IMAGE_ICC2:
            case PDF_E_IMAGE_COLORIZE:
            case PDF_E_JPEG_MULTISCAN:
            case PDF_E_IMAGE_BADMASK:
		pdc_set_errmsg(p->pdc, errcode, stemp, 0, 0, 0);
		break;

            case PDC_E_IO_BADFORMAT:
		pdc_set_errmsg(p->pdc, errcode, stemp, "JPEG", 0, 0);
		break;

            case PDF_E_IMAGE_CORRUPT:
		pdc_set_errmsg(p->pdc, errcode, "JPEG", stemp, 0, 0);
		break;

            case PDF_E_JPEG_COMPRESSION:
            case PDF_E_IMAGE_BADDEPTH:
            case PDF_E_IMAGE_BADCOMP:
		pdc_set_errmsg(p->pdc, errcode,
		    pdc_errprintf(p->pdc, "%d", errint), stemp, 0, 0);
		break;

	    case 0: 		/* error code and message already set */
		break;
        }
    }

    if (image->verbose)
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    return -1;
}

#endif	/* PDF_JPEG_SUPPORTED */
