/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

/* $Id: lisp.h 10525 2004-12-23 21:23:50Z korli $ */

#include "canna.h"

#include <stdio.h>
#include "symbolname.h"

#define	YES	1
#define NO	0

#define VALGET  1
#define VALSET  0

#ifdef WIN
#define CELLSIZE	5120	/* size of cell area (byte)		*/
#else
#define CELLSIZE	10240	/* size of cell area (byte)		*/
#endif

#define STKSIZE		1024	/* the depth of value & parameter stack	*/
#define BUFSIZE	 	256	/* universal buffer size (byte)		*/

#ifdef NIL
#undef NIL
#endif
#define NIL	   0L		/* internal expression of NIL		*/
#define UNBOUND	  -2L		/* unbound mark of variable		*/
#define NON	  -1L		/* the mark of No. (unable to use NO)	*/

#define UNDEF		0
#define	SPECIAL		1
#define	SUBR		2
#define	EXPR		3
#define	CMACRO		4
#define	MACRO		5

#define TAG_MASK	0x07000000L
#define CELL_MASK	0x00ffffffL
#define GC_MASK		0x08000000L

#define NIL_TAG		0L
#define NUMBER_TAG	0x01000000L
#define STRING_TAG	0x02000000L
#define SYMBOL_TAG	0x03000000L
#define CONS_TAG	0x04000000L

#define MAX_DEPTH	20

/* define macros */

#define null(x)		!(x)
#define tag(x)		((x) & TAG_MASK)
#define atom(x)		(tag(x) < CONS_TAG)
#define constp(x)	(tag(x) < SYMBOL_TAG)
#define numberp(x)	(tag(x) == NUMBER_TAG)
#define stringp(x)	(tag(x) == STRING_TAG)
#define symbolp(x)	(tag(x) == SYMBOL_TAG)
#define consp(x)	(tag(x) == CONS_TAG)

#define gcfield(x)	(((struct gccell *)x)->tagfield)
#define mkcopied(x)	((x) | GC_MASK)
#define alreadycopied(x) (gcfield(x) & GC_MASK)
#define newaddr(x)	((x) & ~GC_MASK)

typedef	POINTERINT	list;
typedef POINTERINT	pointerint;

/* cell area */

#define celloffset(x)	((x) & CELL_MASK)

#define car(x)		((struct cell *)(celltop + celloffset(x)))->head
#define cdr(x)		((struct cell *)(celltop + celloffset(x)))->tail
#define caar(x)		car(car(x))
#define cadr(x)		car(cdr(x))
#define cdar(x)		cdr(car(x))
#define cddr(x)		cdr(cdr(x))

#define symbolpointer(x) ((struct atomcell *)(celltop + celloffset(x)))

#define mknum(x)	(NUMBER_TAG | ((x) & CELL_MASK))

#ifdef BIGPOINTER
#define xnum(x)   ((((x) & 0x00800000)) ? (x | 0xffffffffff000000) : (x & 0x00ffffff))
#else
#define xnum(x)   ((((x) & 0x00800000)) ? (x | 0xff000000) : (x & 0x00ffffff))
#endif

#define xstring(x) (((struct stringcell *)(celltop + celloffset(x)))->str)
#define xstrlen(x) (((struct stringcell *)(celltop + celloffset(x)))->length)

#define argnchk(fn,x)	if (n != x) argnerr(fn)

/* data type definitions */

struct cell {
  list tail;
  list head;
};

struct atomcell {
  list	plist;
  list	value;
  char	*pname;
  int	ftype;
  list 	(*func)(...);
  list  (*valfunc)(...);
  int	mid;
  int	fid;
  list	hlink;
};

struct stringcell {
  int length;
  char str[4]; /* dummy array */
};

struct gccell {
  list	tagfield;
};

struct atomdefs {
	char	*symname;
	int	symtype;
	list	(*symfunc)(...);
};

struct cannafndefs {
  char *fnname;
  int  fnid;
};

struct cannamodedefs {
  char *mdname;
  int  mdid;
};

struct cannavardefs {
  char *varname;
  list (*varfunc)(...);
};
