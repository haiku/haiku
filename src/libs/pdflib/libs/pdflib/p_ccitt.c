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

/* $Id: p_ccitt.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * CCITT (Fax G3 and G4) processing for PDFlib
 *
 */

#include "p_intern.h"
#include "p_image.h"

/*
 * Do a bit-reversal of all bytes in the buffer.
 * This is supported for some clients which provide
 * CCITT-compressed data in a byte-reversed format.
 */

static void
pdf_reverse_bit_order(unsigned char *buffer, size_t size)
{
    size_t i;

    /* table for fast byte reversal */
    static const pdc_byte reverse[256] = {
	    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
	    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
	    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
	    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
	    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
	    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
	    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
	    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
	    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
	    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
	    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
	    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
	    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
	    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
	    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
	    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
	    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
	    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
	    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
	    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
	    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
	    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
	    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
	    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
	    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
	    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
	    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
	    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
	    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
	    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
	    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
	    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
    };

    if (buffer != NULL) {
	for (i = 0; i < size; i++) {
	    buffer[i] = reverse[buffer[i]];
	}
    }
}

static void
pdf_data_source_ccitt_raw_init(PDF *p, PDF_data_source *src)
{
  (void) p;

  src->bytes_available = 0;
}

static pdc_bool
pdf_data_source_ccitt_raw_fill(PDF *p, PDF_data_source *src)
{
    pdf_image	*image;
    pdc_bool     ismem;

    (void) p;

    if (src->bytes_available)
       return pdc_false;

    image = (pdf_image *) src->private_data;

    src->buffer_start = (pdc_byte *)
        pdc_freadall(image->fp, (size_t *) &src->buffer_length, &ismem);

    if (src->buffer_length == 0)
       return pdc_false;

    src->bytes_available = src->buffer_length;
    src->next_byte = src->buffer_start;

    if (image->info.ccitt.BitReverse)
	pdf_reverse_bit_order(src->buffer_start, src->bytes_available);

    if (ismem)
        src->buffer_length = 0;

    return pdc_true;
}

static void
pdf_data_source_ccitt_raw_terminate(PDF *p, PDF_data_source *src)
{
    if (src->buffer_length)
        pdc_free(p->pdc, (void *) src->buffer_start);
}

static int
pdf_process_ccitt_raw_data(PDF *p, int imageslot)
{
    pdf_image *image = &p->images[imageslot];
    const char *stemp = NULL;

    /* check data length for raw uncompressed images */
    if (image->compression == none && image->fp)
    {
        size_t length = pdc_file_size(image->fp);
        if (length != (size_t)(image->height_pixel *
            ((image->width_pixel * image->components * image->bpc + 7) / 8)))
        {
            stemp = pdc_errprintf(p->pdc, "%s", image->filename);

	    pdc_set_errmsg(p->pdc, PDF_E_RAW_ILLSIZE,
		pdc_errprintf(p->pdc, "%d", length), stemp, 0, 0);

            if (image->verbose)
		pdc_error(p->pdc, -1, 0, 0, 0, 0);

            return -1;
        }
    }



    if (image->reference == pdf_ref_direct)
    {
        image->src.init = pdf_data_source_ccitt_raw_init;
        image->src.fill = pdf_data_source_ccitt_raw_fill;
        image->src.terminate = pdf_data_source_ccitt_raw_terminate;
        image->src.private_data = (void *) image;
    }

    image->in_use = pdc_true;             /* mark slot as used */

    if (image->doinline)
        pdf_put_inline_image(p, imageslot);
    else
        pdf_put_image(p, imageslot, pdc_true);

    return imageslot;
}

int
pdf_process_CCITT_data(PDF *p, int imageslot)
{
    pdf_image *image = &p->images[imageslot];

    /* CCITT specific information */
    image->info.ccitt.BitReverse = image->bitreverse;
    image->compression = ccitt;
    image->use_raw = pdc_true;

    return pdf_process_ccitt_raw_data(p, imageslot);
}

int
pdf_process_RAW_data(PDF *p, int imageslot)
{
    pdf_image *image = &p->images[imageslot];

    /* RAW specific information */
    image->info.ccitt.BitReverse = 0;
    image->compression = none;

    return pdf_process_ccitt_raw_data(p, imageslot);
}
