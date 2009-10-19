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

/* $Id: pc_output.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib output routines
 *
 */

#ifndef PC_OUTPUT_H
#define PC_OUTPUT_H

/* --------------------------- General --------------------------- */

/* Acrobat viewers change absolute values < 1/65536 to zero */
#define PDF_SMALLREAL   (0.000015)

/* maximum capacity of a dictionary, in entries */
#define PDF_MAXDICTSIZE   (4095)

/* maximum capacity of aa array, in elements */
#define PDF_MAXARRAYSIZE  (8191)

/* some ASCII characters and strings, deliberately defined as hex/oct codes */

#define PDF_NEWLINE		((char) 0x0a)		/* ASCII '\n' */
#define PDF_RETURN		((char) 0x0d)		/* ASCII '\r' */
#define PDF_SPACE		((char) 0x20)		/* ASCII ' '  */
#define PDF_HASH                ((char) 0x23)           /* ASCII '#'  */
#define PDF_PARENLEFT           ((char) 0x28)           /* ASCII '('  */
#define PDF_PARENRIGHT          ((char) 0x29)           /* ASCII ')'  */
#define PDF_PLUS                ((char) 0x2b)           /* ASCII '+'  */
#define PDF_SLASH               ((char) 0x2f)           /* ASCII '/'  */
#define PDF_BACKSLASH           ((char) 0x5c)           /* ASCII '\\' */
#define PDF_A                   ((char) 0x41)           /* ASCII 'A'  */
#define PDF_n                   ((char) 0x6e)           /* ASCII 'n'  */
#define PDF_r                   ((char) 0x72)           /* ASCII 'r'  */

#define PDF_STRING_0123456789ABCDEF	\
	"\060\061\062\063\064\065\066\067\070\071\101\102\103\104\105\106"

typedef struct pdc_output_s pdc_output;

/* --------------------------- Setup --------------------------- */
void	*pdc_boot_output(pdc_core *pdc);
pdc_bool pdc_init_output(void *opaque, pdc_output *out, const char *filename,
    FILE *fp, size_t (*writeproc)(pdc_output *out, void *data, size_t size),
    int compatibility);
void	pdc_cleanup_output(pdc_output *out);

void	*pdc_get_opaque(pdc_output *out);

/* --------------------------- Digest --------------------------- */

void	pdc_init_digest(pdc_output *out);
void	pdc_update_digest(pdc_output *out, unsigned char *input,
	    unsigned int len);
void	pdc_finish_digest(pdc_output *out);


/* --------------------------- Objects and ids --------------------------- */

pdc_id	pdc_alloc_id(pdc_output *out);
void	pdc_mark_free(pdc_output *out, pdc_id obj_id);

pdc_id	pdc_begin_obj(pdc_output *out, pdc_id obj_id);
#define pdc_end_obj(out)		pdc_puts(out, "endobj\n")

#define PDC_NEW_ID	0L
#define PDC_BAD_ID	-1L
#define PDC_FREE_ID	-2L


/* --------------------------- Strings --------------------------- */
/* output a string (including parentheses) and quote all required characters */
void	pdc_put_pdfstring(pdc_output *out, const char *text,
	    int len);

/* output a string (including parentheses) which may be Unicode string */
void	pdc_put_pdfunistring(pdc_output *out, const char *string);

/* --------------------------- Names --------------------------- */
/* output a PDF name (including leading slash) and quote all required chars */
void    pdc_put_pdfname(pdc_output *out, const char *text, size_t len);

/* return a quoted version of a string */
char   *pdc_make_quoted_pdfname(pdc_output *out,
	    const char *text, size_t len,char *buf);


/* --------------------------- Dictionaries --------------------------- */
#define pdc_begin_dict(out)	pdc_puts(out, "<<")
#define pdc_end_dict(out)	pdc_puts(out, ">>\n")


/* --------------------------- Streams --------------------------- */
void	pdc_begin_pdfstream(pdc_output *out);
void	pdc_end_pdfstream(pdc_output *out);
void	pdc_put_pdfstreamlength(pdc_output *out, pdc_id length_id);

int     pdc_get_compresslevel(pdc_output *out);
void    pdc_set_compresslevel(pdc_output *out, int compresslevel);


/* --------------------------- Document sections  --------------------------- */
void	pdc_write_xref_and_trailer(pdc_output *out,
				    pdc_id info_id, pdc_id root_id);


/* --------------------------- Low-level output --------------------------- */
void	pdc_flush_stream(pdc_output *out);
long	pdc_tell_out(pdc_output *out);
void	pdc_close_stream1(pdc_output *out);
void	pdc_close_output(pdc_output *out);
void    pdc_cleanup_stream(pdc_output *out);
const char *pdc_get_stream_contents(pdc_output *out, long *size);
int	pdc_stream_not_empty(pdc_output *out);

void    pdc_write(pdc_output *out, const void *data,size_t size);
void	pdc_puts(pdc_output *out, const char *s);
void	pdc_putc(pdc_output *out, const char c);


/* ------------------------- High-level output ------------------------- */
void	pdc_printf(pdc_output *out, const char *fmt, ...);

#endif	/* PC_OUTPUT_H */
