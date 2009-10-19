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

/* $Id: p_filter.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * ASCII85 and Hex encoding for PDFlib
 *
 */

#include "p_intern.h"

void
pdf_ASCIIHexEncode(PDF *p, PDF_data_source *src)
{
    static const char BinToHex[] = PDF_STRING_0123456789ABCDEF;
    int CharsPerLine;
    size_t i;
    pdc_byte *data;

    CharsPerLine = 0;

    if (src->init)
	src->init(p, src);

    while (src->fill(p, src))
    {
	for (data=src->next_byte, i=src->bytes_available; i > 0; i--, data++)
	{
	  pdc_putc(p->out, BinToHex[*data>>4]);     /* first nibble  */
	  pdc_putc(p->out, BinToHex[*data & 0x0F]); /* second nibble */
	  if ((CharsPerLine += 2) >= 64) {
	    pdc_putc(p->out, PDF_NEWLINE);
	    CharsPerLine = 0;
	  }
	}
    }

    if (src->terminate)
	src->terminate(p, src);

    pdc_puts(p->out, ">\n");         /* EOD marker for PDF hex strings */
}

/* methods for constructing a data source from a file */

#define FILE_BUFSIZE 1024

void
pdf_data_source_file_init(PDF *p, PDF_data_source *src)
{
    pdc_file *fp;

    src->buffer_length = FILE_BUFSIZE;
    src->buffer_start = (pdc_byte *)
	pdc_malloc(p->pdc, src->buffer_length, "pdf_data_source_file_init");

    fp = pdf_fopen(p, (const char *) src->private_data, "embedded ",
                   PDC_FILE_BINARY);

    if (fp == NULL)
	pdc_error(p->pdc, -1, 0, 0, 0, 0);

    if (src->offset)
	pdc_fseek(fp, src->offset, SEEK_SET);

    src->private_data = (void *) fp;
    src->total = (long) 0;
}

pdc_bool
pdf_data_source_file_fill(PDF *p, PDF_data_source *src)
{
    size_t	bytes_needed;
    (void) p;	/* avoid compiler warning "unreferenced parameter" */

    if (src->length != (long) 0 && src->total + FILE_BUFSIZE > src->length)
	bytes_needed = (size_t) (src->length - src->total);
    else
	bytes_needed = FILE_BUFSIZE;

    src->next_byte = src->buffer_start;
    src->bytes_available = pdc_fread(src->buffer_start, 1,
                        bytes_needed, (pdc_file *) (src->private_data));

    src->total += (long) src->bytes_available;

    if (src->bytes_available == 0)
	return pdc_false;
    else
	return pdc_true;
}

void
pdf_data_source_file_terminate(PDF *p, PDF_data_source *src)
{
    pdc_free(p->pdc, (void *) src->buffer_start);
    pdc_fclose((pdc_file *) src->private_data);

    if (src->length != (long) 0 && src->total != src->length)
	pdc_error(p->pdc, PDC_E_IO_NODATA, "?", 0, 0, 0);
}

/* methods for constructing a data source from a memory buffer */

int
pdf_data_source_buf_fill(PDF *p, PDF_data_source *src)
{
    (void) p;	/* avoid compiler warning "unreferenced parameter" */

    if (src->next_byte == NULL) {
	src->next_byte		= src->buffer_start;
	src->bytes_available	= src->buffer_length;
	return pdc_true;
    }

    return pdc_false;
}

/* copy the complete contents of src to a stream */
void
pdf_copy_stream(PDF *p, PDF_data_source *src, pdc_bool compress)
{
    int oldcompresslevel = pdc_get_compresslevel(p->out);

    if (!compress)
	pdc_set_compresslevel(p->out, 0);

    if (src->init)
	src->init(p, src);

    pdc_begin_pdfstream(p->out);

    while (src->fill(p, src))
	pdc_write(p->out, src->next_byte, src->bytes_available);

    pdc_end_pdfstream(p->out);

    if (src->terminate)
	src->terminate(p, src);

    if (!compress)
	pdc_set_compresslevel(p->out, oldcompresslevel);
}
