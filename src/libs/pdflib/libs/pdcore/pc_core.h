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

/* $Id: pc_core.h 14574 2005-10-29 16:27:43Z bonefish $
 *
 * PDFlib core services:
 * - memory management
 * - exception handling
 * - internal try/catch
 * - debug tracing
 */

#ifndef PC_CORE_H
#define PC_CORE_H

/* Built-in metric support */
#define PDF_BUILTINMETRIC_SUPPORTED

/* Built-in encoding support */
#define PDF_BUILTINENCODING_SUPPORTED


#define PDF_FEATURE_NOT_PUBLIC


/* ------------------------- general  ------------------------- */

typedef struct pdc_core_s pdc_core;

typedef int            pdc_bool;
typedef long           pdc_id;
typedef char           pdc_char;
typedef unsigned char  pdc_byte;
typedef short          pdc_short;
typedef unsigned short pdc_ushort;
typedef int            pdc_long;
typedef unsigned int   pdc_ulong;

typedef short          pdc_sint16;
typedef unsigned short pdc_uint16;
typedef int            pdc_sint32;
typedef unsigned int   pdc_uint32;

#define pdc_undef      -1
#define pdc_false       0
#define pdc_true	1

#define PDC_BOOLSTR(a) (a != 0 ? "true" : "false")

#ifndef MIN
#define MIN(a,b) (a <= b ? a : b)
#endif
#ifndef MAX
#define MAX(a,b) (a >= b ? a : b)
#endif

#define	PDC_1_3			13		/* PDF 1.3 = Acrobat 4 */
#define	PDC_1_4			14		/* PDF 1.4 = Acrobat 5 */
#define	PDC_1_5			15		/* PDF 1.5 = Acrobat 6 */
#define PDC_X_X_LAST            15


typedef void  (*pdc_error_fp)(void *opaque, int type, const char *msg);
typedef void* (*pdc_alloc_fp)(void *opaque, size_t size, const char *caller);
typedef void* (*pdc_realloc_fp)(void *opaque, void *mem, size_t size,
						const char *caller);
typedef void  (*pdc_free_fp)(void *opaque, void *mem);

pdc_core *pdc_init_core(pdc_error_fp errorhandler, pdc_alloc_fp allocproc,
    pdc_realloc_fp reallocproc, pdc_free_fp freeproc, void *opaque);

void pdc_delete_core(pdc_core *pdc);

/* this is used by pdflib and pdi sources, so i put it here...
*/
typedef enum {
    use_none = 0, use_art, use_bleed, use_crop, use_media, use_trim
} pdc_usebox;

/* ------------------------- memory management  ------------------------- */

void	*pdc_malloc(pdc_core *pdc, size_t size, const char *caller);
void	*pdc_realloc(pdc_core *pdc, void *mem, size_t size, const char *caller);
void	*pdc_calloc(pdc_core *pdc, size_t size, const char *caller);
void	pdc_free(pdc_core *pdc, void *mem);

/* --------------------------- exception handling --------------------------- */

/* per-library error table base numbers.
*/
#define PDC_ET_CORE	1000
#define PDC_ET_PDFLIB	2000
#define PDC_ET_PDI	4000
#define PDC_ET_PSP	5000
#define PDC_ET_PDPAGE	6000

#define PDC_ET_LAST	6000

/* core error numbers.
*/
enum
{
#define		pdc_genNames	1
#include	"pc_generr.h"

    PDC_E_dummy
};

typedef struct
{
    int		nparms;		/* number of error parameters	*/
    int		errnum;		/* error number			*/
    const char *errmsg;		/* default error message	*/
    const char *ce_msg;		/* custom error message		*/
} pdc_error_info;

void		pdc_register_errtab(pdc_core *pdc, int et, pdc_error_info *ei,
		    int n_entries);

pdc_bool	pdc_enter_api(pdc_core *pdc, const char *apiname);
pdc_bool	pdc_in_error(pdc_core *pdc);
void		pdc_set_warnings(pdc_core *pdc, pdc_bool on);

const char *	pdc_errprintf(pdc_core *pdc, const char *format, ...);

void		pdc_set_errmsg(pdc_core *pdc, int errnum, const char *parm1,
		    const char *parm2, const char *parm3, const char *parm4);

void		pdc_error(pdc_core *pdc, int errnum, const char *parm1,
		    const char *parm2, const char *parm3, const char *parm4);

void		pdc_warning(pdc_core *pdc, int errnum, const char *parm1,
		    const char *parm2, const char *parm3, const char *parm4);

int		pdc_get_errnum(pdc_core *pdc);
const char *	pdc_get_errmsg(pdc_core *pdc);
const char *	pdc_get_apiname(pdc_core *pdc);

/* ----------------------------- try/catch ---------------------------- */

#include <setjmp.h>

typedef struct
{
    jmp_buf	jbuf;
} pdc_jmpbuf;

pdc_jmpbuf *	pdc_jbuf(pdc_core *pdc);
void		pdc_exit_try(pdc_core *pdc);
int		pdc_catch_intern(pdc_core *pdc);
int		pdc_catch_extern(pdc_core *pdc);
void		pdc_rethrow(pdc_core *pdc);

#define PDC_TRY(pdc)		if (setjmp(pdc_jbuf(pdc)->jbuf) == 0)

#define PDC_EXIT_TRY(pdc)	pdc_exit_try(pdc)

#define PDC_CATCH(pdc)		if (pdc_catch_intern(pdc))

#define PDC_RETHROW(pdc)	pdc_rethrow(pdc)


/* --------------------------- debug trace  --------------------------- */

# ifndef DEBUG_TRACE_FILE
#  if defined(MVS)
#   define DEBUG_TRACE_FILE	"pdftrace"
#  elif defined(MAC) || defined(AS400)
#   define DEBUG_TRACE_FILE	"PDFlib.trace"
#  elif defined(WIN32)
#   define DEBUG_TRACE_FILE	"/PDFlib.trace"
#  else
#   define DEBUG_TRACE_FILE     "/tmp/PDFlib.trace"
#  endif
# endif
# define PDF_TRACE(ARGS)	pdc_trace ARGS

int	pdc_vsprintf(pdc_core *pdc, char *buf, const char *fmt, va_list args);
void	pdc_set_floatdigits(pdc_core *pdc, int val);
void	pdc_set_tracefile(pdc_core *pdc, const char *filename);
void	pdc_set_trace(pdc_core *pdc, const char *client);
void    pdc_trace(pdc_core *pdc, const char *fmt, ...);
void    pdc_trace_api(pdc_core *pdc, const char *funame,
			const char *fmt, va_list args);

/* --------------------------- scope  --------------------------- */

/*
 * An arbitrary number used for sanity checks.
 * Actually, we use the hex representation of pi in order to avoid
 * the more common patterns.
 */

#define PDC_MAGIC	((unsigned long) 0x126960A1)

#endif	/* PC_CORE_H */
