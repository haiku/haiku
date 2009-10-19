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

/* $Id: pc_sbuf.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Dynamically growing string buffers.
 *
 */


#include "pc_util.h"
#include "pc_sbuf.h"


/* TODO: initial size parameter for pdc_sb_new().	*/
#undef	INIT_SIZE
#define	INIT_SIZE	1000

pdc_sbuf *
pdc_sb_new(pdc_core *pdc)
{
    static const char fn[] = "pdc_sb_new";

    pdc_sbuf *sb = (pdc_sbuf *)pdc_malloc(pdc, sizeof (pdc_sbuf), fn);

    sb->pdc = pdc;
    sb->buf = (char *)pdc_malloc(pdc, INIT_SIZE, fn); /* TODO: potential leak */
    sb->scan = sb->buf;
    sb->limit = sb->buf + INIT_SIZE;

    return sb;
} /* pdc_sb_new */


void
pdc_sb_delete(pdc_sbuf *sb)
{
    pdc_free(sb->pdc, sb->buf);
    pdc_free(sb->pdc, sb);
} /* pdc_sb_delete */


void
pdc_sb_copy(pdc_sbuf *dst, const pdc_sbuf *src)
{
    static const char fn[] = "pdc_sb_copy";

    int s_size = src->scan - src->buf;
    int d_cap = dst->limit - dst->buf;

    if (d_cap < s_size)
    {
	do
	{
	    d_cap *= 2;
	} while (d_cap < s_size);

	pdc_free(dst->pdc, dst->buf);
	dst->buf = (char *)pdc_malloc(dst->pdc, (size_t) d_cap, fn);
	dst->limit = dst->buf + d_cap;
    }

    memcpy(dst->buf, src->buf, (size_t) s_size);
    dst->scan = dst->buf + s_size;
} /* pdc_sb_copy */


void
pdc_sb_put_c(pdc_sbuf *sb, int ch)
{
    static const char fn[] = "pdc_sb_put_c";

    int size = sb->limit - sb->buf;

    sb->buf = (char *)pdc_realloc(sb->pdc, sb->buf, (size_t) (2 * size), fn);
    sb->scan = sb->buf + size;
    sb->limit = sb->buf + 2 * size;

    *(sb->scan++) = (char) ch;
} /* pdc_sb_put_c */


void
pdc_sb_write(pdc_sbuf *dst, const char *src, int len)
{
    int i;

    if (len == -1)
	len = (int) strlen(src);

    /* TODO: optimize. */
    for (i = 0; i < len; ++i)
	pdc_sb_putc(dst, src[i]);
} /* pdc_sb_write */
