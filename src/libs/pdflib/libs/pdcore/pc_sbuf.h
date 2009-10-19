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

/* $Id: pc_sbuf.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Dynamically growing string buffers.
 *
 */

#ifndef PC_SBUF_H
#define PC_SBUF_H

#include "pc_core.h"

typedef struct	pdc_sbuf_s	pdc_sbuf;

/* TODO: init_len parameter */
pdc_sbuf *	pdc_sb_new(pdc_core *pdc);
void		pdc_sb_delete(pdc_sbuf *sb);

void		pdc_sb_copy(pdc_sbuf *dst, const pdc_sbuf *src);
void		pdc_sb_write(pdc_sbuf *dst, const char *src, int len);

/* public macros.
*/
#define pdc_sb_putc(b, c)			\
	(((b)->scan < (b)->limit)		\
	? (void) (*(b)->scan++ = (char) (c))	\
	: pdc_sb_put_c((b), (c)))

#define pdc_sb_get_cptr(b)			\
	((b)->buf)

#define pdc_sb_get_size(b)			\
	((b)->scan - (b)->buf)

#define pdc_sb_rewrite(b)			\
	((void) ((b)->scan = (b)->buf))


/* the declarations below are strictly private to pc_sbuf.c!
** use the above macros only!
*/
struct pdc_sbuf_s
{
    pdc_core *	pdc;

    char *	buf;
    char *	scan;
    char *	limit;
};

void	pdc_sb_put_c(pdc_sbuf *sb, int ch);

#endif	/* PC_SBUF_H */
