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

/* $Id: pc_core.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib core services
 *
 */

#ifndef WINCE
#include <time.h>
#else
#include <winbase.h>
#endif

#include "pc_util.h"

#define PDF_NonfatalError 11
#define PDF_UnknownError  12


/* TODO: how to make this dynamic?
** exception during pdc_core_init():
**	- out of memory in pdc_sb_new()
** exception during exception:
**	- out of memory in pdc_sb_vprintf()
**	- format error in pdc_sb_vprintf()
*/
#define PDC_ERRPARM_SIZE	2048
#define PDC_ERRBUF_SIZE		(5 * PDC_ERRPARM_SIZE)
#define PDC_XSTACK_INISIZE	10

#define N_ERRTABS		(PDC_ET_LAST / 1000)

/* exception handling frame.
*/
typedef struct
{
    pdc_jmpbuf	jbuf;
    /* we could anchor a destructor list here. */
} pdc_xframe;

typedef struct
{
    pdc_error_info *	ei;
    int			n_entries;
} error_table;


/* ------------------------ the core core structure ---------------------- */

struct pdc_core_s {
    /* ------------ try/catch ------------ */
    pdc_xframe *	x_stack;
    int			x_ssize;
    int			x_sp;		/* exception stack pointer	*/

    /* ------------ error handling ------------ */
    pdc_bool		warnings_enabled;
    pdc_bool		in_error;
    char		errbuf[PDC_ERRBUF_SIZE];
    char		errparms[4][PDC_ERRPARM_SIZE];
    int			epcount;
    int			errnum;
    pdc_bool		x_thrown;	/* exception thrown and not caught */
    const char *	apiname;
    pdc_error_fp	errorhandler;	/* client error handler		*/
    void *		opaque;		/* client specific, opaque data */

    error_table		err_tables[N_ERRTABS];

    /* ------------------ tracing ---------------- */
    pdc_bool		trace;		/* trace feature enabled? */
    char *		tracefilename;	/* name of the trace file */
    int			floatdigits;	/* floating point output precision */

    /* ------------ memory management ------------ */
    pdc_alloc_fp	allocproc;
    pdc_realloc_fp	reallocproc;
    pdc_free_fp		freeproc;
};



/* ----------- default memory management & error handling ----------- */

static void *
default_malloc(void *opaque, size_t size, const char *caller)
{
    (void) opaque;
    (void) caller;

    return malloc(size);
}

static void *
default_realloc(void *opaque, void *mem, size_t size, const char *caller)
{
    (void) opaque;
    (void) caller;

    return realloc(mem, size);
}

static void
default_free(void *opaque, void *mem)
{
    (void) opaque;

    free(mem);
}

static void
default_errorhandler(void *opaque, int errnum, const char *msg)
{
    (void) opaque;

    if (errnum == PDF_NonfatalError)
    {
	fprintf(stderr, "PDFlib warning (ignored): %s\n", msg);
    }
    else
    {
	fprintf(stderr, "PDFlib exception (fatal): %s\n", msg);
	exit(99);
    }
}

pdc_bool
pdc_enter_api(pdc_core *pdc, const char *apiname)
{
    if (pdc->in_error)
	return pdc_false;

    pdc->errnum = 0;
    pdc->apiname = apiname;
    return pdc_true;
}

void
pdc_set_warnings(pdc_core *pdc, pdc_bool on)
{
    pdc->warnings_enabled = on;
}

pdc_bool
pdc_in_error(pdc_core *pdc)
{
    return pdc->in_error;
}


/* --------------------- error table management --------------------- */

static pdc_error_info	core_errors[] =
{
#define		pdc_genInfo	1
#include	"pc_generr.h"
};

#define N_CORE_ERRORS	(sizeof core_errors / sizeof (pdc_error_info))


static void
pdc_panic(pdc_core *pdc, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(pdc->errbuf, fmt, ap);
    va_end(ap);

    pdc->errnum = PDF_UnknownError;
    (*pdc->errorhandler)(pdc->opaque, pdc->errnum, pdc->errbuf);
} /* pdc_panic */


static void
check_parms(pdc_core *pdc, pdc_error_info *ei)
{
    const char *msg = ei->errmsg;
    const char *dollar;

    while ((dollar = strchr(msg, '$')) != (char *) 0)
    {
	if (isdigit(dollar[1]))
	{
	    int n = dollar[1] - '0';

	    if (ei->nparms < n || n < 1)
		pdc_panic(pdc, "illegal parameter '$%d' in error message %d",
				    n, ei->errnum);
	}
	else if (dollar[1] != '$')
	{
	    pdc_panic(pdc,
		"illegal '$' in error message %d", ei->errnum);
	}

	msg = dollar + 1;
    }
} /* check_parms */


void
pdc_register_errtab(pdc_core *pdc, int et, pdc_error_info *ei, int n_entries)
{
    int i;
    int n = (et / 1000) - 1;

    if (n < 0 || N_ERRTABS <= n || et % 1000 != 0)
	pdc_panic(pdc, "tried to register unknown error table %d", et);

    /* ignore multiple registrations of the same table.
    */
    if (pdc->err_tables[n].ei != (pdc_error_info *) 0)
	return;

    check_parms(pdc, &ei[0]);

    for (i = 1; i < n_entries; ++i)
    {
	if (ei[i].errnum <= ei[i-1].errnum)
	{
	    pdc_panic(pdc,
		"duplicate or misplaced error number %d", ei[i].errnum);
	}

	check_parms(pdc, &ei[i]);
    }

    pdc->err_tables[n].ei = ei;
    pdc->err_tables[n].n_entries = n_entries;
} /* pdc_register_errtab */


/* pdc_init_core() never throws exceptions.
** it returns NULL if there's not enough memory.
*/
pdc_core *
pdc_init_core(
    pdc_error_fp errorhandler,
    pdc_alloc_fp allocproc,
    pdc_realloc_fp reallocproc,
    pdc_free_fp freeproc,
    void *opaque)
{
    static const char fn[] = "pdc_init_core";

    pdc_core *pdc;
    int i;

    /* if allocproc is NULL, we use pdc's default memory handling.
    */
    if (allocproc == (pdc_alloc_fp) 0)
    {
	allocproc	= default_malloc;
	reallocproc	= default_realloc;
	freeproc	= default_free;
    }

    if (errorhandler == (pdc_error_fp) 0)
	errorhandler = default_errorhandler;

    pdc = (pdc_core *) (*allocproc)(opaque, sizeof (pdc_core), fn);

    if (pdc == (pdc_core *) 0)
	return (pdc_core *) 0;

    pdc->errorhandler	= errorhandler;
    pdc->allocproc	= allocproc;
    pdc->reallocproc	= reallocproc;
    pdc->freeproc	= freeproc;
    pdc->opaque		= opaque;

    pdc->trace		= pdc_false;
    pdc->tracefilename	= NULL;
    pdc->floatdigits	= 4;

    /* initialize error & exception handling.
    */
    pdc->warnings_enabled = pdc_true;
    pdc->in_error = pdc_false;
    pdc->x_thrown = pdc_false;
    pdc->epcount = 0;
    pdc->errnum = 0;
    pdc->apiname = "";
    pdc->x_sp = -1;
    pdc->x_ssize = PDC_XSTACK_INISIZE;
    pdc->x_stack = (pdc_xframe *)
	(*allocproc)(opaque, pdc->x_ssize * sizeof (pdc_xframe), fn);

    if (pdc->x_stack == (pdc_xframe *) 0)
    {
	(*freeproc)(opaque, pdc);
	return (pdc_core *) 0;
    }

    /* initialize error tables.
    */
    for (i = 0; i < N_ERRTABS; ++i)
	pdc->err_tables[i].ei = (pdc_error_info *) 0;

    pdc_register_errtab(pdc, PDC_ET_CORE, core_errors, N_CORE_ERRORS);

    return pdc;
}

void
pdc_delete_core(pdc_core *pdc)
{
    int i;

    if (pdc->tracefilename)
	pdc_free(pdc, pdc->tracefilename);

    pdc_free(pdc, pdc->x_stack);


    pdc_free(pdc, pdc);
}

/* --------------------------- memory management --------------------------- */

void *
pdc_malloc(pdc_core *pdc, size_t size, const char *caller)
{
    void *ret;

    /* the behavior of malloc(0) is undefined in ANSI C, and may
     * result in a NULL pointer return value which makes PDFlib bail out.
     */
    if (size == (size_t) 0 || (long) size < 0L) {
	size = (size_t) 1;
	pdc_warning(pdc, PDC_E_INT_ALLOC0, caller, 0, 0, 0);
    }

    if ((ret = (*pdc->allocproc)(pdc->opaque, size, caller)) == (void *) 0)
    {
	pdc_error(pdc, PDC_E_MEM_OUT, caller, 0, 0, 0);
    }

    return ret;
}

/* We cook up our own calloc routine, using the caller-supplied
 * malloc and memset.
 */
void *
pdc_calloc(pdc_core *pdc, size_t size, const char *caller)
{
    void *ret;

    if (size == (size_t) 0 || (long) size < 0L) {
	size = (size_t) 1;
	pdc_warning(pdc, PDC_E_INT_ALLOC0, caller, 0, 0, 0);
    }

    if ((ret = (*pdc->allocproc)(pdc->opaque, size, caller)) == (void *) 0)
    {
	pdc_error(pdc, PDC_E_MEM_OUT, caller, 0, 0, 0);
    }

    memset(ret, 0, size);
    return ret;
}

void *
pdc_realloc(pdc_core *pdc, void *mem, size_t size, const char *caller)
{
    void *ret;

    if (size == (size_t) 0 || (long) size < 0L) {
        size = (size_t) 1;
        pdc_warning(pdc, PDC_E_INT_ALLOC0, caller, 0, 0, 0);
    }

    if ((ret = (*pdc->reallocproc)(pdc->opaque, mem, size, caller))
         == (void *) 0)
	pdc_error(pdc, PDC_E_MEM_OUT, caller, 0, 0, 0);

    return ret;
}

void
pdc_free(pdc_core *pdc, void *mem)
{
    /* just in case the freeproc() isn't that ANSI compatible...
    */
    if (mem != NULL)
	(*pdc->freeproc)(pdc->opaque, mem);
}


/* --------------------------- exception handling --------------------------- */

const char *pdc_errprintf(pdc_core *pdc, const char *fmt, ...)
{
    va_list ap;

    if (pdc->epcount < 0 || pdc->epcount > 3)
        pdc->epcount = 0;

    va_start(ap, fmt);
    vsprintf(pdc->errparms[pdc->epcount], fmt, ap);
    va_end(ap);

    return pdc->errparms[pdc->epcount++];
}

static pdc_error_info *
get_error_info(pdc_core *pdc, int errnum)
{
    int n = (errnum / 1000) - 1;

    if (0 <= n && n < N_ERRTABS && pdc->err_tables[n].ei != 0)
    {
	error_table *etab = &pdc->err_tables[n];
	int i;

	/* LATER: binary search. */
	for (i = 0; i < etab->n_entries; ++i)
	{
	    if (etab->ei[i].errnum == errnum)
		return &etab->ei[i];
	}
    }

    pdc_panic(pdc, "Internal error: unknown error number %d", errnum);

    return (pdc_error_info *) 0;	/* for the compiler */
} /* get_error_info */


static void
make_errmsg(
    pdc_core *		pdc,
    pdc_error_info *	ei,
    const char *	parm1,
    const char *	parm2,
    const char *	parm3,
    const char *	parm4)
{
    const char *src = ei->ce_msg ? ei->ce_msg : ei->errmsg;
    char *	dst = pdc->errbuf;
    const char *dollar;

    pdc->epcount = 0;

    /* copy *src to *dst, replacing "$N" with *parmN.
    */
    while ((dollar = strchr(src, '$')) != (char *) 0)
    {
	const char *parm = (const char *) 0;

	memcpy(dst, src, (size_t) (dollar - src));
	dst += dollar - src;
	src = dollar + 1;

	switch (*src)
	{
	    case '1':	parm = (parm1 ? parm1 : "?");	break;
	    case '2':	parm = (parm2 ? parm2 : "?");	break;
	    case '3':	parm = (parm3 ? parm3 : "?");	break;
	    case '4':	parm = (parm4 ? parm4 : "?");	break;

	    case 0:	break;

	    default:	*(dst++) = *(src++);
			break;
	}

	if (parm != (const char *) 0)
	{
	    ++src;
	    strcpy(dst, parm);
	    dst += strlen(parm);
	}
    }

    strcpy(dst, src);
} /* make_errmsg */


void
pdc_set_errmsg(
    pdc_core *  pdc,
    int         errnum,
    const char *parm1,
    const char *parm2,
    const char *parm3,
    const char *parm4)
{
    pdc_error_info *ei = get_error_info(pdc, errnum);

    make_errmsg(pdc, ei, parm1, parm2, parm3, parm4);
    pdc->errnum = errnum;
} /* pdc_set_errmsg */


void
pdc_error(
    pdc_core *	pdc,
    int		errnum,
    const char *parm1,
    const char *parm2,
    const char *parm3,
    const char *parm4)
{
    if (pdc->in_error)		/* avoid recursive errors. */
	return;
    else
    {
	pdc->in_error = pdc_true;
	pdc->x_thrown = pdc_true;
    }

    if (errnum != -1)
    {
	pdc_error_info *ei = get_error_info(pdc, errnum);

	make_errmsg(pdc, ei, parm1, parm2, parm3, parm4);
	pdc->errnum = errnum;
    }

    pdc_trace(pdc, "\n[+++ exception %d in %s, %s +++]\n",
    	pdc->errnum, (pdc->errnum == 0) ? "" : pdc->apiname,
	(pdc->x_sp == -1 ?  "error handler active" : "try/catch active"));
    pdc_trace(pdc, "[\"%s\"]\n\n", pdc->errbuf);

    if (pdc->x_sp == -1) {
	char errbuf[PDC_ERRBUF_SIZE];

	sprintf(errbuf, "[%d] %s: %s",
	    pdc->errnum, pdc_get_apiname(pdc), pdc->errbuf);
	(*pdc->errorhandler)(pdc->opaque, PDF_UnknownError, errbuf);

	/*
	 * The error handler must never return. If it does, it is severely
	 * broken. We cannot remedy this, so we exit.
	 */
	 exit(99);

    } else {

	longjmp(pdc->x_stack[pdc->x_sp].jbuf.jbuf, 1);
    }

} /* pdc_error */


void
pdc_warning(
    pdc_core *	pdc,
    int		errnum,
    const char *parm1,
    const char *parm2,
    const char *parm3,
    const char *parm4)
{
    if (pdc->in_error || pdc->warnings_enabled == pdc_false)
	return;
    else
    {
	pdc->in_error = pdc_true;
	pdc->x_thrown = pdc_true;
    }

    if (errnum != -1)
    {
	pdc_error_info *ei = get_error_info(pdc, errnum);

	make_errmsg(pdc, ei, parm1, parm2, parm3, parm4);
	pdc->errnum = errnum;
    }

    pdc_trace(pdc, "\n[+++ warning %d in %s, %s +++]\n",
    	pdc->errnum, (pdc->errnum == 0) ? "" : pdc->apiname,
	(pdc->x_sp == -1 ?  "error handler active" : "try/catch active"));
    pdc_trace(pdc, "[\"%s\"]\n\n", pdc->errbuf);

    if (pdc->x_sp == -1) {
	char errbuf[PDC_ERRBUF_SIZE];

	sprintf(errbuf, "[%d] %s: %s",
	    pdc->errnum, pdc_get_apiname(pdc), pdc->errbuf);
	(*pdc->errorhandler)(pdc->opaque, PDF_NonfatalError, errbuf);

    } else {

	longjmp(pdc->x_stack[pdc->x_sp].jbuf.jbuf, 1);
    }

    /* a client-supplied error handler may return after a warning */
    pdc->in_error = pdc_false;

} /* pdc_warning */


pdc_jmpbuf *
pdc_jbuf(pdc_core *pdc)
{
    static const char fn[] = "pdc_jbuf";

    if (++pdc->x_sp == pdc->x_ssize)
    {
	/* TODO: test */
	pdc_xframe *aux = (pdc_xframe *) (*pdc->reallocproc)(
				pdc->opaque, pdc->x_stack,
				2 * pdc->x_ssize * sizeof (pdc_xframe), fn);

	if (aux == (pdc_xframe *) 0)
	{
	    --pdc->x_sp;
	    pdc->errnum = PDC_E_MEM_OUT;
	    pdc->x_thrown = pdc_true;
	    pdc->in_error = pdc_true;
	    strcpy(pdc->errbuf, "out of memory");
	    longjmp(pdc->x_stack[pdc->x_sp].jbuf.jbuf, 1);
	}

	pdc->x_stack = aux;
	pdc->x_ssize *= 2;
    }

    pdc->x_thrown = pdc_false;
    return &pdc->x_stack[pdc->x_sp].jbuf;
} /* pdc_jbuf */

void
pdc_exit_try(pdc_core *pdc)
{
    if (pdc->x_sp == -1)
    {
	strcpy(pdc->errbuf, "exception stack underflow");
	pdc->errnum = PDC_E_INT_XSTACK;
	(*pdc->errorhandler)(pdc->opaque, PDF_UnknownError, pdc->errbuf);
    }
    else
	--pdc->x_sp;
} /* pdc_exit_try */

int
pdc_catch_intern(pdc_core *pdc)
{
    pdc_bool result;

    if (pdc->x_sp == -1)
    {
	strcpy(pdc->errbuf, "exception stack underflow");
	pdc->errnum = PDC_E_INT_XSTACK;
	(*pdc->errorhandler)(pdc->opaque, PDF_UnknownError, pdc->errbuf);
    }
    else
	--pdc->x_sp;

    result = pdc->x_thrown;
    pdc->in_error = pdc_false;
    pdc->x_thrown = pdc_false;

    return result;
} /* pdc_catch_intern */

int
pdc_catch_extern(pdc_core *pdc)
{
    pdc_bool result;

    if (pdc->x_sp == -1)
    {
	strcpy(pdc->errbuf, "exception stack underflow");
	pdc->errnum = PDC_E_INT_XSTACK;
	(*pdc->errorhandler)(pdc->opaque, PDF_UnknownError, pdc->errbuf);
    }
    else
	--pdc->x_sp;

    result = pdc->x_thrown;
    pdc->x_thrown = pdc_false;

    return result;
} /* pdc_catch_extern */

void
pdc_rethrow(pdc_core *pdc)
{
    pdc_error(pdc, -1, 0, 0, 0, 0);
} /* pdc_rethrow */


int
pdc_get_errnum(pdc_core *pdc)
{
    return pdc->errnum;
}

const char *
pdc_get_errmsg(pdc_core *pdc)
{
    return (pdc->errnum == 0) ? "" : pdc->errbuf;
}

const char *
pdc_get_apiname(pdc_core *pdc)
{
    return (pdc->errnum == 0) ? "" : pdc->apiname;
}


/* --------------------------- debug trace  --------------------------- */

static const char digits[] = "0123456789ABCDEF";

static char *
pdc_ltoa(char *buf, long n, int width, char pad, int base)
{
    char	aux[20];
    int		k, i = sizeof aux;
    char *	dest = buf;
    unsigned long ul = (unsigned long) n;

    if (n == 0)
    {
	if (width == 0)
	    width = 1;

	for (k = 0; k < width; ++k)
	    *(dest++) = '0';

	return dest;
    }

    if (n < 0 && base == 10)
    {
	ul = (unsigned long) -ul;	/* safe against overflow,
					    while "n = -n" isn't! */
	*(dest++) = '-';
    }

    aux[--i] = digits[ul % base];
    n = (long) (ul / base);

    while (0 < n)
    {
	aux[--i] = digits[n % base];
	n = n / base;
    }

    width -= (int) (sizeof aux) - i;
    for (k = 0; k < width; ++k)
	*(dest++) = pad;

    memcpy(dest, &aux[i], sizeof aux - i);
    return dest + sizeof aux - i;
} /* pdc_ltoa */

/* Acrobat viewers have an upper limit on real and integer numbers */
#define PDF_BIGREAL		(32768.0)
#define PDF_BIGINT		(2147483647.0)

static char *
pdc_ftoa(pdc_core *pdc, char *buf, double x)
{
    static const long pow10[] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };

    char *	dest = buf;
    double	integ, fract;
    long	f;

    if (fabs(x) < PDF_SMALLREAL)
    {
	*dest = '0';
	return dest + 1;
    }

    if (x < 0)
    {
	x = -x;
	*(dest++) = '-';
    }

    if (x >= PDF_BIGREAL)
    {
	if (x > PDF_BIGINT)
	    pdc_error(pdc, PDC_E_INT_FLOATTOOLARGE, 0, 0, 0, 0);

	return pdc_ltoa(dest, (long) floor(x + 0.5), 0, ' ', 10);
    }

    fract = modf(x, &integ);
    f = (long) floor(fract * pow10[pdc->floatdigits] + 0.5);

    if (f == pow10[pdc->floatdigits])
    {
	integ += 1.0;
	f = 0;
    }

    if (integ == 0 && f == 0)	/* avoid "-0" */
	dest = buf;

    dest = pdc_ltoa(dest, (long) integ, 0, ' ', 10);

    if (f != 0)
    {
	char *	aux;
	int	i = pdc->floatdigits;
	long    rem;

	*(dest++) = '.';

        do      /* avoid trailing zeros */
	{
	    rem = f % 10;
	    f = f / 10;
	    --i;
	} while (rem == 0);

	aux = dest + i + 1;
	dest[i--] = digits[rem];

	for (; 0 <= i; --i)
	{
	    dest[i] = digits[f % 10];
	    f = f / 10;
	}

	return aux;
    }

    return dest;
} /* pdc_ftoa */

int
pdc_vsprintf(pdc_core *pdc, char *buf, const char *format, va_list args)
{
    char *dest = buf;

    for (/* */ ; /* */ ; /* */)
    {
	int	width = 0;
	char	pad = ' ';

	/* as long as there is no '%', just print.
	*/
	while (*format != 0 && *format != '%')
	    *(dest++) = *(format++);

	if (*format == 0)
	{
	    *dest = 0;
	    return dest - buf;
	}

	if (*(++format) == '0')
	{
	    pad = '0';
	    ++format;
	}

	while (isdigit(*format))
	    width = 10*width + *(format++) - '0';

	switch (*format)
	{
	    case 'X':
		dest = pdc_ltoa(dest, va_arg(args, int), width, pad, 16);
		break;

	    case 'c':
		*(dest++) = (char) va_arg(args, int);
		break;

	    case 'd':
		dest = pdc_ltoa(dest, va_arg(args, int), width, pad, 10);
		break;

	    case 'g':	/* for use in pdc_trace_api() */
	    case 'f':
		dest = pdc_ftoa(pdc, dest, va_arg(args, double));
		break;

	    case 'l':
	    {
		long n = va_arg(args, long);

		switch (*(++format))
		{
		    case 'X':
			dest = pdc_ltoa(dest, n, width, pad, 16);
			break;

		    case 'd':
			dest = pdc_ltoa(dest, n, width, pad, 10);
			break;

		    default:
			pdc_error(pdc, PDC_E_INT_BADFORMAT,
			    pdc_errprintf(pdc, "l%c",
				isprint(*format) ? *format : '?'),
			    pdc_errprintf(pdc, "0x%02X", *format),
			    0, 0);
		}

		break;
	    }

	    case 'p':
	    {
		void *ptr = va_arg(args, void *);
		dest += sprintf(dest, "%p", ptr);
		break;
	    }

	    case 's':
	    {
		char *	str = va_arg(args, char *);
		size_t	len;

		if (str == 0)
		    str = "(NULL)";

		if ((len = strlen(str)) != 0)
		{
		    memcpy(dest, str, len);
		    dest += len;
		}
		break;
	    }

	    case '%':
		*(dest++) = '%';
		break;

	    default:
		pdc_error(pdc, PDC_E_INT_BADFORMAT,
		    pdc_errprintf(pdc, "%c", isprint(*format) ? *format : '?'),
		    pdc_errprintf(pdc, "0x%02X", *format),
		    0, 0);
	} /* switch */

	++format;
    } /* loop */
} /* pdc_vsprintf */

void
pdc_set_floatdigits(pdc_core *pdc, int val)
{
    pdc->floatdigits = val;
}

void
pdc_set_tracefile(pdc_core *pdc, const char *filename)
{
    if (!filename || !*filename)
	return;

    if (pdc->tracefilename)
	pdc_free(pdc, pdc->tracefilename);

    pdc->tracefilename = pdc_strdup(pdc, filename);
}

/* Include informational stuff in [] brackets, and use
   %s/\[[^]]*\]//g 		and
   %s/)$/);/
 * to make it compilable :-)
 */

/* start or stop (client == NULL) a trace for the supplied client program */
void
pdc_set_trace(pdc_core *pdc, const char *client)
{
#ifndef WINCE
    time_t	timer;
    struct tm	ltime;

    time(&timer);
    ltime = *localtime(&timer);
#else
    SYSTEMTIME  st;

    GetLocalTime (&st);
#endif

    pdc->trace = client ? pdc_true : pdc_false;

    if (pdc->trace) {
	pdc_trace(pdc,
	"[ ------------------------------------------------------ ]\n");
	pdc_trace(pdc, "[ %s on %s ] ", client, PDF_PLATFORM);
	pdc_trace(pdc, "[%04d-%02d-%02d]\n",
#ifndef WINCE
	ltime.tm_year + 1900, ltime.tm_mon + 1, ltime.tm_mday);
#else
	st.wYear, st.wMonth, st.wDay);
#endif  /* !WINCE */

	pdc_trace(pdc,
	"[ ------------------------------------------------------ ]\n");
    }
}

/* trace function: version for pdf_enter_api() with
 * time stamp and function name output.
 */
void
pdc_trace_api(pdc_core *pdc, const char *funame, const char *fmt, va_list args)
{
    FILE *	fp;
    const char *filename;
    char	buf[2000];
#ifndef WINCE
    time_t	timer;
    struct tm	ltime;
#else
    SYSTEMTIME  st;
#endif

    if (!pdc || !pdc->trace)
	return;

    filename = pdc->tracefilename ? pdc->tracefilename : DEBUG_TRACE_FILE;

    if ((fp = fopen(filename, APPENDMODE)) == NULL)
	return;

#ifndef WINCE
    time(&timer);
    ltime = *localtime(&timer);
    fprintf(fp, "[%02d:%02d:%02d] ", ltime.tm_hour, ltime.tm_min, ltime.tm_sec);

#else

    GetLocalTime (&st);
    fprintf(fp, "[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
#endif	/* WINCE */

    fprintf(fp, "%s\t", funame);
    pdc_vsprintf(pdc, buf, fmt, args);
    fprintf(fp, "%s", buf);

    fclose(fp);
}

/* trace function: version without any decoration for return values etc.*/
void
pdc_trace(pdc_core *pdc, const char *fmt, ...)
{
    FILE *fp;
    const char *filename;
    va_list ap;

    if (!pdc || !pdc->trace)
	return;

    filename = pdc->tracefilename ? pdc->tracefilename : DEBUG_TRACE_FILE;

    if ((fp = fopen(filename, APPENDMODE)) == NULL)
	return;

    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);

    fclose(fp);
}

