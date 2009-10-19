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

/* $Id: pdflib_tcl.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * in sync with pdflib.h 1.151.2.22
 *
 * Wrapper code for the PDFlib Tcl binding
 *
 */

/*
 * Build with STUBS enabled
 *
 * if building with older TCL Versions than 8.2 you have to undef this
 */
#define USE_TCL_STUBS

#include <tcl.h>

#include <string.h>
#include <stdlib.h>

#if defined(__WIN32__)
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   undef WIN32_LEAN_AND_MEAN

#   if defined(__WIN32__) && \
	(defined(_MSC_VER) || (defined(__GNUC__) && defined(__declspec)))
#	define SWIGEXPORT(a,b) __declspec(dllexport) a b
#   else
#	if defined(__BORLANDC__)
#	    define SWIGEXPORT(a,b) a _export b
#	else
#	    define SWIGEXPORT(a,b) a b
#	endif
#   endif
#else
#   define SWIGEXPORT(a,b) a b
#endif

#include <stdlib.h>

#ifdef SWIG_GLOBAL
#ifdef __cplusplus
#define SWIGSTATIC extern "C"
#else
#define SWIGSTATIC
#endif
#endif

#ifndef SWIGSTATIC
#define SWIGSTATIC static
#endif

/* SWIG pointer structure */

typedef struct SwigPtrType {
  char               *name;               /* Datatype name                  */
  int               len;                /* Length (used for optimization) */
  void               *(*cast)(void *);    /* Pointer casting function       */
  struct SwigPtrType *next;               /* Linked list pointer            */
} SwigPtrType;

/* Pointer cache structure */

typedef struct {
  int               stat;               /* Status (valid) bit             */
  SwigPtrType        *tp;                 /* Pointer to type structure      */
  char                name[256];          /* Given datatype name            */
  char                mapped[256];        /* Equivalent name                */
} SwigCacheType;

/* Some variables  */

static int SwigPtrMax  = 64;           /* Max entries that can be currently held */
                                       /* This value may be adjusted dynamically */
static int SwigPtrN    = 0;            /* Current number of entries              */
static int SwigPtrSort = 0;            /* Status flag indicating sort            */
static int SwigStart[256];             /* Starting positions of types            */

/* Pointer table */
static SwigPtrType *SwigPtrTable = 0;  /* Table containing pointer equivalences  */

/* Cached values */

#define SWIG_CACHESIZE  8
#define SWIG_CACHEMASK  0x7
static SwigCacheType SwigCache[SWIG_CACHESIZE];
static int SwigCacheIndex = 0;
static int SwigLastCache = 0;

/* Sort comparison function */
static int swigsort(const void *data1, const void *data2) {
	SwigPtrType *d1 = (SwigPtrType *) data1;
	SwigPtrType *d2 = (SwigPtrType *) data2;
	return strcmp(d1->name,d2->name);
}

/* Binary Search function */
static int swigcmp(const void *key, const void *data) {
  char *k = (char *) key;
  SwigPtrType *d = (SwigPtrType *) data;
  return strncmp(k,d->name,d->len);
}

/* Register a new datatype with the type-checker */

SWIGSTATIC
void SWIG_RegisterMapping(char *origtype, char *newtype, void *(*cast)(void *)) {

  int i;
  SwigPtrType *t = 0,*t1;

  /* Allocate the pointer table if necessary */

  if (!SwigPtrTable) {
    SwigPtrTable = (SwigPtrType *) malloc(SwigPtrMax*sizeof(SwigPtrType));
    SwigPtrN = 0;
  }
  /* Grow the table */
  if (SwigPtrN >= SwigPtrMax) {
    SwigPtrMax = 2*SwigPtrMax;
    SwigPtrTable = (SwigPtrType *) realloc((char *) SwigPtrTable,SwigPtrMax*sizeof(SwigPtrType));
  }
  for (i = 0; i < SwigPtrN; i++)
    if (strcmp(SwigPtrTable[i].name,origtype) == 0) {
      t = &SwigPtrTable[i];
      break;
    }
  if (!t) {
    t = &SwigPtrTable[SwigPtrN];
    t->name = origtype;
    t->len = strlen(t->name);
    t->cast = 0;
    t->next = 0;
    SwigPtrN++;
  }

  /* Check for existing entry */

  while (t->next) {
    if ((strcmp(t->name,newtype) == 0)) {
      if (cast) t->cast = cast;
      return;
    }
    t = t->next;
  }

  /* Now place entry (in sorted order) */

  t1 = (SwigPtrType *) malloc(sizeof(SwigPtrType));
  t1->name = newtype;
  t1->len = strlen(t1->name);
  t1->cast = cast;
  t1->next = 0;
  t->next = t1;
  SwigPtrSort = 0;
}

/* Make a pointer value string */

SWIGSTATIC
void SWIG_MakePtr(char *_c, const void *_ptr, char *type) {
  static char _hex[16] =
  {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'a', 'b', 'c', 'd', 'e', 'f'};
  unsigned long _p, _s;
  char _result[20], *_r;    /* Note : a 64-bit hex number = 16 digits */
  _r = _result;
  _p = (unsigned long) _ptr;
  if (_p > 0) {
    while (_p > 0) {
      _s = _p & 0xf;
      *(_r++) = _hex[_s];
      _p = _p >> 4;
    }
    *_r = '_';
    while (_r >= _result)
      *(_c++) = *(_r--);
  } else {
    strcpy (_c, "NULL");
  }
  if (_ptr)
    strcpy (_c, type);
}

/* Function for getting a pointer value */

SWIGSTATIC
char *SWIG_GetPtr(char *_c, void **ptr, char *_t)
{
  unsigned long _p;
  char temp_type[256];
  char *name;
  int i, len;
  SwigPtrType *sp,*tp;
  SwigCacheType *cache;
  int start, end;
  _p = 0;

  /* Pointer values must start with leading underscore */
  if (*_c == '_') {
      _c++;
      /* Extract hex value from pointer */
      while (*_c) {
	  if ((*_c >= '0') && (*_c <= '9'))
	    _p = (_p << 4) + (*_c - '0');
	  else if ((*_c >= 'a') && (*_c <= 'f'))
	    _p = (_p << 4) + ((*_c - 'a') + 10);
	  else
	    break;
	  _c++;
      }

      if (_t) {
	if (strcmp(_t,_c)) {
	  if (!SwigPtrSort) {
	    qsort((void *) SwigPtrTable, SwigPtrN, sizeof(SwigPtrType), swigsort);
	    for (i = 0; i < 256; i++) {
	      SwigStart[i] = SwigPtrN;
	    }
	    for (i = SwigPtrN-1; i >= 0; i--) {
	      SwigStart[(int) (SwigPtrTable[i].name[1])] = i;
	    }
	    for (i = 255; i >= 1; i--) {
	      if (SwigStart[i-1] > SwigStart[i])
		SwigStart[i-1] = SwigStart[i];
	    }
	    SwigPtrSort = 1;
	    for (i = 0; i < SWIG_CACHESIZE; i++)
	      SwigCache[i].stat = 0;
	  }

	  /* First check cache for matches.  Uses last cache value as starting point */
	  cache = &SwigCache[SwigLastCache];
	  for (i = 0; i < SWIG_CACHESIZE; i++) {
	    if (cache->stat) {
	      if (strcmp(_t,cache->name) == 0) {
		if (strcmp(_c,cache->mapped) == 0) {
		  cache->stat++;
		  *ptr = (void *) _p;
		  if (cache->tp->cast) *ptr = (*(cache->tp->cast))(*ptr);
		  return (char *) 0;
		}
	      }
	    }
	    SwigLastCache = (SwigLastCache+1) & SWIG_CACHEMASK;
	    if (!SwigLastCache) cache = SwigCache;
	    else cache++;
	  }
	  /* We have a type mismatch.  Will have to look through our type
	     mapping table to figure out whether or not we can accept this datatype */

	  start = SwigStart[(int) _t[1]];
	  end = SwigStart[(int) _t[1]+1];
	  sp = &SwigPtrTable[start];
	  while (start < end) {
	    if (swigcmp(_t,sp) == 0) break;
	    sp++;
	    start++;
	  }
	  if (start >= end) sp = 0;
	  /* Try to find a match for this */
	  if (sp) {
	    while (swigcmp(_t,sp) == 0) {
	      name = sp->name;
	      len = sp->len;
	      tp = sp->next;
	      /* Try to find entry for our given datatype */
	      while(tp) {
		if (tp->len >= 255) {
		  return _c;
		}
		strcpy(temp_type,tp->name);
		strncat(temp_type,_t+len,255-tp->len);
		if (strcmp(_c,temp_type) == 0) {

		  strcpy(SwigCache[SwigCacheIndex].mapped,_c);
		  strcpy(SwigCache[SwigCacheIndex].name,_t);
		  SwigCache[SwigCacheIndex].stat = 1;
		  SwigCache[SwigCacheIndex].tp = tp;
		  SwigCacheIndex = SwigCacheIndex & SWIG_CACHEMASK;

		  /* Get pointer value */
		  *ptr = (void *) _p;
		  if (tp->cast) *ptr = (*(tp->cast))(*ptr);
		  return (char *) 0;
		}
		tp = tp->next;
	      }
	      sp++;
	      /* Hmmm. Didn't find it this time */
	    }
	  }
	  /* Didn't find any sort of match for this data.
	     Get the pointer value and return the received type */
	  *ptr = (void *) _p;
	  return _c;
	} else {
	  /* Found a match on the first try.  Return pointer value */
	  *ptr = (void *) _p;
	  return (char *) 0;
	}
      } else {
	/* No type specified.  Good luck */
	*ptr = (void *) _p;
	return (char *) 0;
      }
  } else {
    if (strcmp (_c, "NULL") == 0) {
	*ptr = (void *) 0;
	return (char *) 0;
    }
    *ptr = (void *) 0;
    return _c;
  }
}

#ifdef __cplusplus
extern "C" {
#endif
#ifdef MAC_TCL
#pragma export on
#endif
SWIGEXPORT(int,Pdflib_Init)(Tcl_Interp *);
SWIGEXPORT(int,Pdflib_SafeInit)(Tcl_Interp *);
SWIGEXPORT(int,Pdflib_tcl_SafeInit)(Tcl_Interp *);
SWIGEXPORT(int,Pdflib_tcl_Init)(Tcl_Interp *);
SWIGEXPORT(int,Pdf_tcl_Init)(Tcl_Interp *);
SWIGEXPORT(int,Pdf_tcl_SafeInit)(Tcl_Interp *);
#ifdef MAC_TCL
#pragma export off
#endif
#ifdef __cplusplus
}
#endif

#include <setjmp.h>

#include "pdflib.h"

/* Exception handling */

#define try	PDF_TRY(p)
#define catch	PDF_CATCH(p) {\
		char errmsg[1024];\
		sprintf(errmsg, "PDFlib Error [%d] %s: %s", PDF_get_errnum(p),\
		    PDF_get_apiname(p), PDF_get_errmsg(p));\
		Tcl_SetResult(interp, errmsg, TCL_STATIC);\
		    return TCL_ERROR;			\
	    }

/* Unicode support is only available in Tcl 8.2 and higher */

#if 10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION >= 82

/*
 * Unicode strings for page description text and hyper text strings
 */

static char *
GetStringUnicodePDFChars(PDF *p, Tcl_Interp *interp, Tcl_Obj *objPtr, int *lenP)
{
    Tcl_UniChar *unistring = NULL;
    size_t len = 0;

    if (objPtr) {
        unistring = Tcl_GetUnicode(objPtr);
	if (unistring) {
	    len = (size_t) Tcl_UniCharLen(unistring);
	}
    }

    *lenP = (int) (2 * len);
    return (char *) unistring;
}


#else /* Tcl version older than 8.2 */

/* The cheap version doesn't know about Unicode strings */

#define GetStringUnicodePDFChars(p, interp, objPtr, lenP) \
	    Tcl_GetStringFromObj(objPtr, NULL)

#endif /* Tcl version older than 8.2 */

/* export the PDFlib routines to the shared library */
#ifdef __MWERKS__
#pragma export on
#endif

/* p_annots.c */

static int
_wrap_PDF_add_launchlink(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char *_arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_launchlink p llx lly urx ury filename ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_launchlink. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = argv[6];

    try {     PDF_add_launchlink(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_add_locallink(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    int _arg5;
    char *_arg6;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_locallink p llx lly urx ury page optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_locallink. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    if (Tcl_GetInt(interp, argv[6], &_arg5) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 6 of PDF_add_locallink.", TCL_STATIC);
	return TCL_ERROR;
    }
    _arg6 = argv[7];

    try {     PDF_add_locallink(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_add_note(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    double  llx;
    double  lly;
    double  urx;
    double  ury;
    char *contents;
    char *title;
    char *icon;
    int open;
    char *res;
    int len_cont;
    int len_title;

    if (objc != 10) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_note p llx lly urx ury contents title icon open ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_note. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[2], &llx) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve llx in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[3], &lly) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve lly in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[4], &urx) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve urx in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[5], &ury) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve ury in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((contents = GetStringUnicodePDFChars(p, interp, objv[6], &len_cont)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve contents argument in PDF_add_note", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((title = GetStringUnicodePDFChars(p, interp, objv[7], &len_title)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve title argument in PDF_add_note", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((icon = Tcl_GetStringFromObj(objv[8], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve icon in PDF_add_note", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[9], &open) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve open argument in PDF_add_note", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_add_note2(p, (float) llx, (float) lly, (float) urx,
		    (float) ury,contents,len_cont,title,len_title,icon,open);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_add_pdflink(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char *_arg5;
    int _arg6;
    char *_arg7;

    if (argc != 9) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_pdflink p llx lly urx ury filename page optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_pdflink. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = argv[6];
    if (Tcl_GetInt(interp, argv[7], &_arg6) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 7 of PDF_add_pdflink.", TCL_STATIC);
	return TCL_ERROR;
    }
    _arg7 = argv[8];

    try {     PDF_add_pdflink(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_add_weblink(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char *_arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_weblink p llx lly urx ury url ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_weblink. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = argv[6];

    try {     PDF_add_weblink(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_attach_file(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    double  llx;
    double  lly;
    double  urx;
    double  ury;
    char *filename;
    char *description;
    char *author;
    char *mimetype;
    char *icon;
    char *res;
    int len_descr;
    int len_autho;

    if (objc != 11) {
        Tcl_SetResult(interp, "Wrong # args. PDF_attach_file p llx lly urx ury filename description author mimetype icon ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res,(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_attach_file. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[2], &llx) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve llx in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[3], &lly) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve lly in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[4], &urx) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve urx in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[5], &ury) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve ury in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((filename = Tcl_GetStringFromObj(objv[6], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve filename in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((description = GetStringUnicodePDFChars(p, interp, objv[7], &len_descr)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve description argument in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((author = GetStringUnicodePDFChars(p, interp, objv[8], &len_autho)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve author argument in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((mimetype = Tcl_GetStringFromObj(objv[9], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve mimetype in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((icon = Tcl_GetStringFromObj(objv[10], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve icon in PDF_attach_file", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_attach_file2(p, (float) llx, (float) lly,
                    (float) urx, (float) ury,filename,0,description,len_descr,
		    author,len_autho,mimetype,icon);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_set_border_color(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_border_color p red green blue ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_border_color. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);

    try {     PDF_set_border_color(p,_arg1,_arg2,_arg3);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_set_border_dash(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_border_dash p b w ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_border_dash. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_set_border_dash(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_set_border_style(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *_arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_border_style p style width ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_border_style. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    _arg2 = (float) atof(argv[3]);

    try {     PDF_set_border_style(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

/* p_basic.c */

static int
_wrap_PDF_begin_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_begin_page p width height ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_begin_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_begin_page(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_close(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_close p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_close. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_close(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_delete(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_delete p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_delete. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    PDF_delete(p);

    return TCL_OK;
}

static int
_wrap_PDF_end_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_end_page p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_end_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_end_page(p);
    } catch;

    return TCL_OK;
}

static int _wrap_PDF_get_apiname(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    char volatile *_result = NULL;
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_apiname p",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_apiname. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {
	_result = (char *)PDF_get_apiname(p);
	Tcl_SetResult(interp, (char *) _result, TCL_VOLATILE);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_get_buffer(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {

    const char volatile *buffer = NULL;
    PDF *p;
    Tcl_Obj *result;
    char *res;
    long size;

    if (objc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_buffer p", TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
	"Type error in argument 1 of PDF_get_buffer. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    try {
	buffer = PDF_get_buffer(p, &size);
	result = Tcl_GetObjResult(interp);

/* ByteArrays appeared only in Tcl 8.1 */
#if 10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION >= 81
	Tcl_SetByteArrayObj(result, (unsigned char *) buffer, (int) size);
#else /* Tcl 8.0 */
	Tcl_SetStringObj(result, (char *) buffer, (int) size);
#endif /* Tcl 8.0 */

    } catch;

    return TCL_OK;
}

static int _wrap_PDF_get_errmsg(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    char volatile *_result = NULL;
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_errmsg p",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_errmsg. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {
	_result = (char *)PDF_get_errmsg(p);
	Tcl_SetResult(interp, (char *) _result, TCL_VOLATILE);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_get_errnum(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_errnum p",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_errnum. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    try {
	_result = (int)PDF_get_errnum(p);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_new(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char versionbuf[32];

    if (argc != 1) {
        Tcl_SetResult(interp, "Wrong # args. PDF_new ",TCL_STATIC);
        return TCL_ERROR;
    }

    p = (PDF *)PDF_new();

    if (p) {

/* The GetVersion API appeared in Tcl 8.1 */
#if 10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION >= 81
	int major, minor, type, patchlevel;
	Tcl_GetVersion(&major, &minor, &patchlevel, &type);
	sprintf(versionbuf, "Tcl %d.%d%c%d", major, minor, "ab."[type], patchlevel);
#else
#ifdef TCL_PATCH_LEVEL
	sprintf(versionbuf, "Tcl %s", TCL_PATCH_LEVEL);
#else
	sprintf(versionbuf, "Tcl (unknown)");
#endif
#endif
        PDF_set_parameter(p, "binding", versionbuf);
        PDF_set_parameter(p, "textformat", "auto2");
        PDF_set_parameter(p, "hypertextformat", "auto2");
        PDF_set_parameter(p, "hypertextencoding", "");
    }
    /* TODO: else show some error */

    SWIG_MakePtr(interp->result, (void *) p,"_PDF_p");
    return TCL_OK;
}

static int
_wrap_PDF_open_file(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_file p filename ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_file. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];

    try {     _result = (int)PDF_open_file(p,_arg1);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

/* p_block.c */

static int
_wrap_PDF_fill_imageblock(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    int len = 0;
    int page;
    char *blockname;
    int image;
    char *optlist;
    int error;

    if (objc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fill_imageblock p page blockname image optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_fill_imageblock. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[2], &page)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve page argument in PDF_fill_imageblock", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((blockname = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve blockname argument in PDF_fill_imageblock", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((error = Tcl_GetIntFromObj(interp, objv[4], &image)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve image argument in PDF_fill_imageblock", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((optlist = Tcl_GetStringFromObj(objv[5], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve optlist argument in PDF_fill_imageblock", TCL_STATIC);
	return TCL_ERROR;
    }


    try {
	_result = (int)PDF_fill_imageblock(p, page, blockname, image, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_fill_pdfblock(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    int len = 0;
    int page;
    char * blockname;
    int contents;
    char * optlist;
    int error;

    if (objc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fill_pdfblock p page blockname contents optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_fill_pdfblock. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[2], &page)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve page argument in PDF_fill_pdfblock", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((blockname = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve blockname argument in PDF_fill_pdfblock", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((error = Tcl_GetIntFromObj(interp, objv[4], &contents)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve contents argument in PDF_fill_pdfblock", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((optlist = Tcl_GetStringFromObj(objv[5], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve optlist argument in PDF_fill_pdfblock", TCL_STATIC);
	return TCL_ERROR;
    }


    try {
	_result = (int)PDF_fill_pdfblock(p, page, blockname, contents, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_fill_textblock(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    int len = 0;
    int page;
    char * blockname;
    char *text;
    int text_len = 0;
    char * optlist;
    int error;

    if (objc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fill_textblock p page blockname text optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_fill_textblock. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[2], &page)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve page argument in PDF_fill_textblock", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((blockname = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve blockname argument in PDF_fill_textblock", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((text = GetStringUnicodePDFChars(p, interp, objv[4], &text_len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_fill_textblock", TCL_STATIC);
	return TCL_ERROR;
    }


    if ((optlist = Tcl_GetStringFromObj(objv[5], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve optlist argument in PDF_fill_textblock", TCL_STATIC);
	return TCL_ERROR;
    }


    try {
	_result = (int)PDF_fill_textblock(p, page, blockname, text, text_len, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

/* p_color.c */

static int
_wrap_PDF_makespotcolor(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *spotname;
    int reserved = 0;

    if (argc != 4 && argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_makespotcolor p spotname",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_makespotcolor. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    spotname = argv[2];
    /* ignore "reserved" argument was a bug in V4 wrapper */
    /* reserved = (int) atol(argv[3]); */

    try {     _result = (int)PDF_makespotcolor(p, spotname, reserved);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_setcolor(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *_arg1;
    char *_arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setcolor p fstype colorspace c1 c2 c3 c4 ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setcolor. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = argv[2];
    _arg2 = argv[3];
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);
    _arg6 = (float) atof(argv[7]);

    try {     PDF_setcolor(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setgray(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setgray p g ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setgray. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_setcolor(p, "fillstroke", "gray", _arg1, 0, 0, 0);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setgray_fill(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setgray_fill p g ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setgray_fill. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_setcolor(p, "fill", "gray", _arg1, 0, 0, 0);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setgray_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setgray_stroke p g ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setgray_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_setcolor(p, "stroke", "gray", _arg1, 0, 0, 0);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setrgbcolor(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setrgbcolor p red green blue ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setrgbcolor. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);

    try {     PDF_setcolor(p, "fillstroke", "rgb", _arg1, _arg2, _arg3, 0);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setrgbcolor_fill(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setrgbcolor_fill p red green blue ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setrgbcolor_fill. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);

    try {     PDF_setcolor(p, "fill", "rgb", _arg1, _arg2, _arg3, 0);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setrgbcolor_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setrgbcolor_stroke p red green blue ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setrgbcolor_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);

    try {     PDF_setcolor(p, "stroke", "rgb", _arg1, _arg2, _arg3, 0);
    } catch;

    return TCL_OK;
}

/* p_draw.c */

static int
_wrap_PDF_arc(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_arc p x y r alpha beata ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_arc. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);

    try {     PDF_arc(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_arcn(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_arcn p x y r alpha beta ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_arcn. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);

    try {     PDF_arcn(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_circle(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_circle p x y r ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_circle. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);

    try {     PDF_circle(p,_arg1,_arg2,_arg3);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_clip(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_clip p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_clip. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_clip(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_closepath(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_closepath p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_closepath. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_closepath(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_closepath_fill_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_closepath_fill_stroke p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_closepath_fill_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_closepath_fill_stroke(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_closepath_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_closepath_stroke p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_closepath_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_closepath_stroke(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_curveto(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_curveto p x1 y1 x2 y2 x3 y3 ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_curveto. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);
    _arg6 = (float) atof(argv[7]);

    try {     PDF_curveto(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_endpath(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_endpath p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_endpath. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_endpath(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_fill(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fill p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_fill. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_fill(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_fill_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fill_stroke p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_fill_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_fill_stroke(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_lineto(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_lineto p x y ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_lineto. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_lineto(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_moveto(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_moveto p x y ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_moveto. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_moveto(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_rect(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_rect p x y width height ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_rect. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);

    try {     PDF_rect(p,_arg1,_arg2,_arg3,_arg4);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_stroke(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_stroke p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_stroke. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_stroke(p);
    } catch;

    return TCL_OK;
}

/* p_encoding.c */

static int
_wrap_PDF_encoding_set_char(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *encoding;
    int slot;
    char *glyphname;
    int uv;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_encoding_set_char p encoding slot glyphname uv ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_encoding_set_char. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    encoding = argv[2];
    slot = (int) atol(argv[3]);
    glyphname = argv[4];
    uv = (int) atol(argv[5]);

    try {     PDF_encoding_set_char(p, encoding, slot, glyphname, uv);
    } catch;

    return TCL_OK;
}

/* p_font.c */

static int
_wrap_PDF_findfont(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    char *_arg2;
    int _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_findfont p fontname encoding embed ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_findfont. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    _arg2 = argv[3];

    if (Tcl_GetInt(interp, argv[4], &_arg3) != TCL_OK) {
        Tcl_SetResult(interp, "Type error in argument 4 of PDF_findfont: ",
	    TCL_STATIC);
        Tcl_AppendResult(interp, argv[4], (char *) NULL);
	return TCL_ERROR;
    }

    try {     _result = (int)PDF_findfont(p,_arg1,_arg2,_arg3);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_load_font(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    int len = 0;
    int len1;
    char *fontname;
    char *encoding;
    char *optlist;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_load_font p fontname encoding optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_load_font. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((fontname = GetStringUnicodePDFChars(p, interp, objv[2], &len1)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve fontname argument in PDF_load_font", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((encoding = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve encoding argument in PDF_load_font", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((optlist = Tcl_GetStringFromObj(objv[4], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve optlist argument in PDF_load_font", TCL_STATIC);
	return TCL_ERROR;
    }


    try {
	_result = (int)PDF_load_font(p, fontname, len1, encoding, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_setfont(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setfont p font fontsize ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setfont. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &_arg1) != TCL_OK) {
        Tcl_SetResult(interp, "Type error in argument 2 of PDF_setfont: ",
	    TCL_STATIC);
        Tcl_AppendResult(interp, argv[2], (char *) NULL);
	return TCL_ERROR;
    }
    _arg2 = (float) atof(argv[3]);

    try {     PDF_setfont(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

/* p_gstate.c */

static int
_wrap_PDF_concat(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;

    if (argc != 8) {
        Tcl_SetResult(interp,
		"Wrong # args. PDF_concat p a b c d e f ", TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp,
	"Type error in argument 1 of PDF_concat. Expected _PDF_p, received ",
	    TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);
    _arg6 = (float) atof(argv[7]);

    try {     PDF_concat(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_initgraphics(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_initgraphics p ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_initgraphics. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_initgraphics(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_restore(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_restore p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_restore. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_restore(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_rotate(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_rotate p phi ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_rotate. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_rotate(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_save(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_save p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_save. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_save(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_scale(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_scale p sx sy ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_scale. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_scale(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setdash(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setdash p b w ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setdash. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_setdash(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setdashpattern(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *optlist;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setdashpattern p optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setdashpattern. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    optlist = argv[2];

    try {     PDF_setdashpattern(p, optlist);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setflat(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setflat p flatness ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setflat. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);

    try {     PDF_setflat(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setlinecap(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setlinecap p linecap ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setlinecap. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &_arg1) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 2 of PDF_setlinecap.", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_setlinecap(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setlinejoin(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setlinejoin p linejoin ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setlinejoin. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &_arg1) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 2 of PDF_setlinejoin.", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_setlinejoin(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setlinewidth(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setlinewidth p width ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setlinewidth. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_setlinewidth(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setmatrix(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setmatrix p a b c d e f ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setmatrix. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);
    _arg6 = (float) atof(argv[7]);

    try {     PDF_setmatrix(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setmiterlimit(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_setmiterlimit p miter ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_setmiterlimit. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);

    try {     PDF_setmiterlimit(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_setpolydash(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    float carray[MAX_DASH_LENGTH];
    double dval;
    int length, i;
    Tcl_Obj *val;
    char *res;

    if (objc != 3) {
        Tcl_SetResult(interp,
	    "Wrong # args. PDF_setpolydash p darray length ", TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
	Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
	Tcl_SetResult(interp,
	"Type error in argument 1 of PDF_setpolydash. Expected _PDF_p, received ", TCL_STATIC);
	Tcl_AppendResult(interp, res, (char *) NULL);
	return TCL_ERROR;
    }

    if (Tcl_ListObjLength(interp, objv[2], &length) != TCL_OK) {
	Tcl_SetResult(interp,
	    "Couldn't retrieve array length in PDF_setpolydash", TCL_STATIC);
	return TCL_ERROR;
    }
    if (length > MAX_DASH_LENGTH)
	length = MAX_DASH_LENGTH;

    for (i = 0; i < length; i++) {
	if (Tcl_ListObjIndex(interp, objv[2], i, &val) != TCL_OK ||
	    Tcl_GetDoubleFromObj(interp, val, &dval) != TCL_OK) {
	    Tcl_SetResult(interp,
		"Couldn't retrieve array value in PDF_setpolydash", TCL_STATIC);
	    return TCL_ERROR;
	}
	carray[i] = (float) dval;
    }

    try {     PDF_setpolydash(p, carray, length);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_skew(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_skew p alpha beta ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_skew. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_skew(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_translate(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_translate p tx ty ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_translate. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_translate(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

/* p_hyper.c */

static int
_wrap_PDF_add_bookmark(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    int _arg2;
    int _arg3;
    char *res;
    int error;
    int len;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_bookmark p text parent open ", TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer in PDF_add_bookmark", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_bookmark. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_add_bookmark", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[3], &_arg2)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve parent argument in PDF_add_bookmark", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[4], &_arg3)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve open argument in PDF_add_bookmark", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     _result = (int)PDF_add_bookmark2(p,_arg1,len,_arg2,_arg3);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_add_nameddest(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *name;
    char *optlist;
    char *res;
    int len;

    if (objc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_nameddest p name optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_nameddest. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((name = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve name argument in PDF_add_nameddest", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((optlist = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve optlist argument in PDF_add_nameddest", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_add_nameddest(p, name, 0, optlist);
    } catch;

    return TCL_OK;
}


static int
_wrap_PDF_set_info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *_arg1;
    char *_arg2;
    char *res;
    int len;

    if (objc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_info p key value ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_info. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve key argument in PDF_set_info", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((_arg2 = GetStringUnicodePDFChars(p, interp, objv[3], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve value argument in PDF_set_info", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_set_info2(p,_arg1,_arg2,len);
    } catch;

    return TCL_OK;
}

/* p_icc.c */

static int
_wrap_PDF_load_iccprofile(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *profilename;
    char *optlist;
    char *res;
    int len;

    if (objc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_load_iccprofile p profilename optlist", TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer in PDF_load_iccprofile", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_load_iccprofile. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((profilename = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve profilename argument in PDF_load_iccprofile", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((optlist = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve optlist argument in PDF_load_iccprofile", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     _result = (int)PDF_load_iccprofile(p, profilename, 0, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}


/* p_image.c */

static int
_wrap_PDF_add_thumbnail(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_add_thumbnail p image ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_add_thumbnail. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (int) atol(argv[2]);

    try {     PDF_add_thumbnail(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_close_image(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_close_image p image ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_close_image. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &_arg1) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 2 of PDF_close_image.", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_close_image(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_fit_image(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int image;
    float x;
    float y;
    char *optlist;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fit_image p image x y optlist",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_fit_image. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &image) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument image of PDF_fit_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    x = (float) atof(argv[3]);
    y = (float) atof(argv[4]);
    optlist = argv[5];

    try {     PDF_fit_image(p, image, x, y, optlist);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_load_image(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    char *imagetype;
    char *filename;
    char *optlist;
    int len = 0;
    int fnam_len = 0;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_load_image p imagetype filename optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_load_image. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((imagetype = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve imagetype argument in PDF_load_image", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((filename = Tcl_GetStringFromObj(objv[3], &fnam_len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve filename argument in PDF_load_image", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((optlist = Tcl_GetStringFromObj(objv[4], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve optlist argument in PDF_load_image", TCL_STATIC);
	return TCL_ERROR;
    }


    try {
	_result = (int)PDF_load_image(p, imagetype, filename, 0, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}


static int
_wrap_PDF_open_CCITT(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    int _arg2;
    int _arg3;
    int _arg4;
    int _arg5;
    int _arg6;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_CCITT p filename width height BitReverse K BlackIs1 ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_CCITT. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    if (Tcl_GetInt(interp, argv[3], &_arg2) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 3 of PDF_open_CCITT.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[4], &_arg3) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 4 of PDF_open_CCITT.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[5], &_arg4) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 5 of PDF_open_CCITT.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[6], &_arg5) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 6 of PDF_open_CCITT.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[7], &_arg6) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 7 of PDF_open_CCITT.", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     _result = (int)PDF_open_CCITT(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_open_image(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {

    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    char *_arg2;
    char *_arg3;
    long  _arg4;
    int _arg5;
    int _arg6;
    int _arg7;
    int _arg8;
    char *_arg9;
    int len = 0;
    char *res;

    if (objc != 11) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_image p imagetype source data length width height components bpc params", TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer in PDF_open_image",
	    TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_image. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve imagetype argument in PDF_open_image", TCL_STATIC);
        return TCL_ERROR;
    }
    if ((_arg2 = Tcl_GetStringFromObj(objv[3], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve source argument in PDF_open_image", TCL_STATIC);
        return TCL_ERROR;
    }

/* ByteArrays appeared only in Tcl 8.1 */

#if 10 * TCL_MAJOR_VERSION + TCL_MINOR_VERSION >= 81

    if ((_arg3 = (char *) Tcl_GetByteArrayFromObj(objv[4], &len)) == NULL) {

#else /* Tcl 8.0 */

    if ((_arg3 = (char *) Tcl_GetStringFromObj(objv[4], &len)) == NULL) {
#endif /* Tcl 8.0 */

        Tcl_SetResult(interp,
	    "Couldn't retrieve data argument in PDF_open_image", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetLongFromObj(interp, objv[5], &_arg4) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 5 of PDF_open_image.", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[6], &_arg5) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 6 of PDF_open_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[7], &_arg6) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 7 of PDF_open_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[8], &_arg7) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 8 of PDF_open_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[9], &_arg8) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 9 of PDF_open_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((_arg9 = Tcl_GetStringFromObj(objv[10], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve argument 10 in PDF_open_image", TCL_STATIC);
        return TCL_ERROR;
    }

    try {
	_result = (int)PDF_open_image(p,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_open_image_file(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    char *_arg2;
    char *_arg3;
    int _arg4;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_image_file p imagetype filename stringparam intparam ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_image_file. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    _arg2 = argv[3];
    _arg3 = argv[4];
    if (Tcl_GetInt(interp, argv[5], &_arg4) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 5 of PDF_open_image_file.", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     _result = (int)PDF_open_image_file(p,_arg1,_arg2,_arg3,_arg4);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_place_image(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;
    float _arg2;
    float _arg3;
    float _arg4;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_place_image p image x y scale ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_place_image. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    if (Tcl_GetInt(interp, argv[2], &_arg1) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Type error in argument 2 of PDF_place_image.", TCL_STATIC);
	return TCL_ERROR;
    }
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);

    try {     PDF_place_image(p,_arg1,_arg2,_arg3,_arg4);
    } catch;

    return TCL_OK;
}

/* p_params.c */


static int _wrap_PDF_get_parameter(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    char volatile *_result = NULL;
    PDF *p;
    char *_arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_parameter p key modifier ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_parameter. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = argv[2];
    _arg2 = (float) atof(argv[3]);

    try {
	_result = (char *)PDF_get_parameter(p,_arg1,_arg2);
	Tcl_SetResult(interp, (char *) _result, TCL_VOLATILE);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_get_value(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    float volatile _result = 0;
    PDF *p;
    char *_arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_value p key modifier ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        p = NULL;
    }
    _arg1 = argv[2];
    _arg2 = (float) atof(argv[3]);

    if (p != NULL) {
        try {     _result = (float)PDF_get_value(p,_arg1,_arg2);
        } catch;
    } else {
        _result = (float)PDF_get_value(p,_arg1,_arg2);
    }

    Tcl_PrintDouble(interp,(double) _result, interp->result);
    return TCL_OK;
}

static int
_wrap_PDF_set_parameter(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *_arg1;
    char *_arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_parameter p key value ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_parameter. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    _arg2 = argv[3];

    try {
            PDF_set_parameter(p, _arg1, _arg2);

    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_set_value(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;
    char *_arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_value p key value ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_value. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = argv[2];
    _arg2 = (float) atof(argv[3]);

    try {     PDF_set_value(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

/* p_pattern.c */

static int
_wrap_PDF_begin_pattern(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    int _arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_begin_pattern p width height xstep ystep painttype ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_begin_pattern. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (int) atol(argv[6]);

    try {     _result = (int)PDF_begin_pattern(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_end_pattern(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_end_pattern p ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_end_pattern. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_end_pattern(p);
    } catch;

    return TCL_OK;
}

/* p_pdi.c */

static int
_wrap_PDF_close_pdi(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_close_pdi p doc ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_close_pdi. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (int) atol(argv[2]);

    try {     PDF_close_pdi(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_close_pdi_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_close_pdi_page p page ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_close_pdi_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (int) atol(argv[2]);

    try {     PDF_close_pdi_page(p,_arg1);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_fit_pdi_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int page;
    float x;
    float y;
    char *optlist;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fit_pdi_page p page x y optlist",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_fit_pdi_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    page = (int) atol(argv[2]);
    x = (float) atof(argv[3]);
    y = (float) atof(argv[4]);
    optlist = argv[5];

    try {     PDF_fit_pdi_page(p, page, x, y, optlist);
    } catch;

    return TCL_OK;
}


static int
_wrap_PDF_get_pdi_parameter(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    char volatile * _result = NULL;
    PDF *p;
    char *_arg1;
    int _arg2;
    int _arg3;
    int _arg4;
    int len;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_pdi_parameter p key doc page reserved ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_pdi_parameter. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = argv[2];
    _arg2 = (int) atol(argv[3]);
    _arg3 = (int) atol(argv[4]);
    _arg4 = (int) atol(argv[5]);

    try {
	_result = (char *)PDF_get_pdi_parameter(p,_arg1,_arg2,_arg3,_arg4, &len);
	/* TODO: return a string object */
	Tcl_SetResult(interp, (char *) _result, TCL_VOLATILE);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_get_pdi_value(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    float volatile _result = -1;
    PDF *p;
    char *_arg1;
    int _arg2;
    int _arg3;
    int _arg4;

    if (argc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_get_pdi_value p key doc page reserved ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_get_pdi_value. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = argv[2];
    _arg2 = (int) atol(argv[3]);
    _arg3 = (int) atol(argv[4]);
    _arg4 = (int) atol(argv[5]);

    try {     _result = (float)PDF_get_pdi_value(p,_arg1,_arg2,_arg3,_arg4);
    } catch;

    Tcl_PrintDouble(interp,(double) _result, interp->result);
    return TCL_OK;
}

static int
_wrap_PDF_open_pdi(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *_arg1;
    char *_arg2;
    int _arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_pdi p filename stringparam reserved ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_pdi. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = argv[2];
    _arg2 = argv[3];
    _arg3 = (int) atol(argv[4]);

    try {     _result = (int)PDF_open_pdi(p,_arg1,_arg2,_arg3);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_open_pdi_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    int _arg1;
    int _arg2;
    char *_arg3;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_open_pdi_page p doc pagenumber optlist ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_open_pdi_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (int) atol(argv[2]);
    _arg2 = (int) atol(argv[3]);
    _arg3 = argv[4];

    try {     _result = (int)PDF_open_pdi_page(p,_arg1,_arg2,_arg3);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_place_pdi_page(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;

    if (argc != 7) {
        Tcl_SetResult(interp, "Wrong # args. PDF_place_pdi_page p page x y sx sy",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_place_pdi_page. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (int) atol(argv[2]);
    _arg2 = (float) atof(argv[3]);
    _arg3 = (float) atof(argv[4]);
    _arg4 = (float) atof(argv[5]);
    _arg5 = (float) atof(argv[6]);

    try {     PDF_place_pdi_page(p,_arg1,_arg2,_arg3,_arg4,_arg5);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_process_pdi(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    int doc;
    int page;
    char *optlist;

    if (argc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_process_pdi p doc page optlist ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_process_pdi. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    doc = (int) atol(argv[2]);
    page = (int) atol(argv[3]);
    optlist = argv[4];

    try {     _result = (int)PDF_process_pdi(p, doc, page, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}


/* p_resource.c */

static int
_wrap_PDF_create_pvf(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *filename;
    char *data;
    char *optlist;
    int len = 0;
    int dlen = 0;
    char *res;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_create_pvf p filename data optlist ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_create_pvf. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((filename = Tcl_GetStringFromObj(objv[2], &len)) == NULL) {
        Tcl_SetResult(interp,
            "Couldn't retrieve filename argument in PDF_create_pvf", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((data = Tcl_GetStringFromObj(objv[3], &dlen)) == NULL) {
        Tcl_SetResult(interp,
            "Couldn't retrieve data argument in PDF_create_pvf", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((optlist = Tcl_GetStringFromObj(objv[4], &len)) == NULL) {
        Tcl_SetResult(interp,
            "Couldn't retrieve optlist argument in PDF_create_pvf", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_create_pvf(p, filename, 0, (void *)data, (size_t)dlen, optlist);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_delete_pvf(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    int volatile _result = -1;
    PDF *p;
    char *filename;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_delete_pvf p filename ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_delete_pvf. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    filename = argv[2];

    try {     _result = (int)PDF_delete_pvf(p, filename, 0);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}


/* p_shading.c */

static int
_wrap_PDF_shading(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    int volatile _result = -1;
    PDF *p;
    char *shtype;
    float x0;
    float y0;
    float x1;
    float y1;
    float c1;
    float c2;
    float c3;
    float c4;
    char *optlist;

    if (argc != 12) {
        Tcl_SetResult(interp, "Wrong # args. PDF_shading p shtype x0 y0 x1 y1 c1 c2 c3 c4 optlist",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_shading. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    shtype = argv[2];
    x0 = (float) atof(argv[3]);
    y0 = (float) atof(argv[4]);
    x1 = (float) atof(argv[5]);
    y1 = (float) atof(argv[6]);
    c1 = (float) atof(argv[7]);
    c2 = (float) atof(argv[8]);
    c3 = (float) atof(argv[9]);
    c4 = (float) atof(argv[10]);
    optlist = argv[11];

    try {     _result = (int)PDF_shading(p, shtype, x0, y0, x1, y1, c1, c2, c3, c4, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_shading_pattern(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    int volatile _result = -1;
    PDF *p;
    int shading;
    char *optlist;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_shading_pattern p shading optlist",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_shading_pattern. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    shading = (int) atol(argv[2]);
    optlist = argv[3];

    try {     _result = (int)PDF_shading_pattern(p, shading, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_shfill(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;
    int shading;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_shfill p shading ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_shfill. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    shading = (int) atol(argv[2]);

    try {     PDF_shfill(p, shading);
    } catch;

    return TCL_OK;
}

/* p_template.c */

static int
_wrap_PDF_begin_template(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    int volatile _result = -1;
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_begin_template p width height ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_begin_template. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     _result = (int)PDF_begin_template(p,_arg1,_arg2);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_end_template(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_end_template p ",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_end_template. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_end_template(p);
    } catch;

    return TCL_OK;
}

/* p_text.c */

static int
_wrap_PDF_continue_text(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *_arg1;
    int len = 0;
    char *res;

    if (objc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_continue_text p text ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_continue_text. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_continue_text", TCL_STATIC);
	return TCL_ERROR;
    }

    try {
	PDF_continue_text2(p, _arg1, len);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_fit_textline(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *text;
    double x;
    double y;
    char *optlist;
    int textlen = 0;
    int len = 0;
    char *res;

    if (objc != 6) {
        Tcl_SetResult(interp, "Wrong # args. PDF_fit_textline p text x y optlist",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_fit_textline. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((text = GetStringUnicodePDFChars(p, interp, objv[2], &textlen)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_fit_textline", TCL_STATIC);
	return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[3], &x) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve x argument in PDF_fit_textline", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[4], &y) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve y argument in PDF_fit_textline", TCL_STATIC);
	return TCL_ERROR;
    }
    if ((optlist = Tcl_GetStringFromObj(objv[5], &len)) == NULL) {
        Tcl_SetResult(interp,
            "Couldn't retrieve optlist argument in PDF_fit_textline", TCL_STATIC);
	return TCL_ERROR;
    }

    try {
	PDF_fit_textline(p, text, textlen, (float) x, (float) y, optlist);
    } catch;

    return TCL_OK;
}


static int
_wrap_PDF_set_text_pos(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    float _arg1;
    float _arg2;

    if (argc != 4) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_text_pos p x y ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_text_pos. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    _arg1 = (float) atof(argv[2]);
    _arg2 = (float) atof(argv[3]);

    try {     PDF_set_text_pos(p,_arg1,_arg2);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_show(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *_arg1;
    int len = 0;
    char *res;

    if (objc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_show p text ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_show. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_show", TCL_STATIC);
	return TCL_ERROR;
    }

    try {     PDF_show2(p, _arg1, len);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_show_boxed(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    int volatile _result = -1;
    PDF *p;
    char *res;
    int len1 = 0;
    int len = 0;
    char *_arg1;
    double _arg2;
    double _arg3;
    double _arg4;
    double _arg5;
    char *_arg6;
    char *_arg7;

    if (objc != 9) {
        Tcl_SetResult(interp, "Wrong # args. PDF_show_boxed p text left top width height hmode feature ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp,
    "Type error in argument 1 of PDF_show_boxed. Expected _PDF_p, received ",
    	TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len1)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_show_boxed", TCL_STATIC);
        return TCL_ERROR;
    }

    if (Tcl_GetDoubleFromObj(interp, objv[3], &_arg2) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve x argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[4], &_arg3) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve y argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[5], &_arg4) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve width argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[6], &_arg5) != TCL_OK) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve height argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((_arg6 = Tcl_GetStringFromObj(objv[7], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve mode argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((_arg7 = Tcl_GetStringFromObj(objv[8], &len)) == NULL) {
        Tcl_SetResult(interp,
	    "Couldn't retrieve mode argument in PDF_show_boxed", TCL_STATIC);
	return TCL_ERROR;
    }

    try {
	_result = (int)PDF_show_boxed2(p, _arg1, len1, (float)_arg2, (float)_arg3, (float)_arg4, (float)_arg5,_arg6,_arg7);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}

static int
_wrap_PDF_show_xy(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    PDF *p;
    char *_arg1;
    double  _arg2;
    double  _arg3;
    int len = 0, error;
    char *res;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_show_xy p text x y ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p, "_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_show_xy. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_show_xy", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetDoubleFromObj(interp, objv[3], &_arg2)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve x argument in PDF_show_xy", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetDoubleFromObj(interp, objv[4], &_arg3)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve y argument in PDF_show_xy", TCL_STATIC);
	return TCL_ERROR;
    }

    try {
	PDF_show_xy2(p, _arg1, len, (float) _arg2, (float) _arg3);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_stringwidth(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[]) {
    float volatile _result = -1;
    PDF *p;
    char *_arg1;
    int _arg2;
    double  _arg3;
    int len = 0, error;
    char *res;
    Tcl_Obj *resultPtr;

    if (objc != 5) {
        Tcl_SetResult(interp, "Wrong # args. PDF_stringwidth p text font fontsize ",TCL_STATIC);
        return TCL_ERROR;
    }

    if ((res = Tcl_GetStringFromObj(objv[1], NULL)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve PDF pointer in PDF_stringwidth", TCL_STATIC);
	return TCL_ERROR;
    }

    if (SWIG_GetPtr(res, (void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_stringwidth. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, res, (char *) NULL);
        return TCL_ERROR;
    }

    if ((_arg1 = GetStringUnicodePDFChars(p, interp, objv[2], &len)) == NULL) {
        Tcl_SetResult(interp, "Couldn't retrieve text argument in PDF_stringwidth", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetIntFromObj(interp, objv[3], &_arg2)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve font argument in PDF_stringwidth", TCL_STATIC);
	return TCL_ERROR;
    }

    if ((error = Tcl_GetDoubleFromObj(interp, objv[4], &_arg3)) != TCL_OK) {
        Tcl_SetResult(interp, "Couldn't retrieve fontsize argument in PDF_stringwidth", TCL_STATIC);
	return TCL_ERROR;
    }

    try {
	_result = (float) PDF_stringwidth2(p, _arg1, len, _arg2, (float) _arg3);
    } catch;

    resultPtr = Tcl_GetObjResult(interp);
    Tcl_SetDoubleObj(resultPtr, (double) _result);

    return TCL_OK;
}

/* p_type3.c */

static int
_wrap_PDF_begin_font(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *fontname;
    float a, b, c, d, e, f;
    char *optlist;

    if (argc != 10) {
        Tcl_SetResult(interp, "Wrong # args. PDF_begin_font p fontname a b c d e f optlist",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_begin_font. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    fontname = argv[2];
    a = (float) atof(argv[3]);
    b = (float) atof(argv[4]);
    c = (float) atof(argv[5]);
    d = (float) atof(argv[6]);
    e = (float) atof(argv[7]);
    f = (float) atof(argv[8]);
    optlist = argv[9];

    try {     PDF_begin_font(p, fontname, 0, a, b, c, d, e, f, optlist);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_begin_glyph(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    char *name;
    float wx, llx, lly, urx, ury;

    if (argc != 8) {
        Tcl_SetResult(interp, "Wrong # args. PDF_begin_glyph p glyphname flag value ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_begin_glyph. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    name = argv[2];
    wx = (float) atof(argv[3]);
    llx = (float) atof(argv[4]);
    lly = (float) atof(argv[5]);
    urx = (float) atof(argv[6]);
    ury = (float) atof(argv[7]);

    try {     PDF_begin_glyph(p, name, wx, llx, lly, urx, ury);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_end_font(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_end_font p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_end_font. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_end_font(p);
    } catch;

    return TCL_OK;
}

static int
_wrap_PDF_end_glyph(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;

    if (argc != 2) {
        Tcl_SetResult(interp, "Wrong # args. PDF_end_glyph p ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_end_glyph. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }

    try {     PDF_end_glyph(p);
    } catch;

    return TCL_OK;
}

/* p_xgstate.c */

static int
_wrap_PDF_create_gstate(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {

    int volatile _result = -1;
    PDF *p;
    char *optlist;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_create_gstate p optlist",TCL_STATIC);
        return TCL_ERROR;
    }
    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_create_gstate. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    optlist = argv[2];

    try {     _result = (int)PDF_create_gstate(p, optlist);
    } catch;

    sprintf(interp->result,"%ld", (long) _result);
    return TCL_OK;
}


static int
_wrap_PDF_set_gstate(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[]) {
    PDF *p;
    int handle;

    if (argc != 3) {
        Tcl_SetResult(interp, "Wrong # args. PDF_set_gstate p gstate ",TCL_STATIC);
        return TCL_ERROR;
    }

    if (SWIG_GetPtr(argv[1],(void **) &p,"_PDF_p")) {
        Tcl_SetResult(interp, "Type error in argument 1 of PDF_set_gstate. Expected _PDF_p, received ", TCL_STATIC);
        Tcl_AppendResult(interp, argv[1], (char *) NULL);
        return TCL_ERROR;
    }
    handle = (int) atol(argv[2]);

    try {     PDF_set_gstate(p, handle);
    } catch;

    return TCL_OK;
}



/* This is required to make our extension work with safe Tcl interpreters */
SWIGEXPORT(int,Pdflib_SafeInit)(Tcl_Interp *interp)
{
    return TCL_OK;
}

/* This is required to satisfy pkg_mkIndex */
SWIGEXPORT(int,Pdflib_tcl_SafeInit)(Tcl_Interp *interp)
{
    return Pdflib_SafeInit(interp);
}

/* This is required to satisfy pkg_mkIndex */
SWIGEXPORT(int,Pdflib_tcl_Init)(Tcl_Interp *interp)
{
    return Pdflib_Init(interp);
}

/* This is required to satisfy pkg_mkIndex */
SWIGEXPORT(int,Pdf_tcl_Init)(Tcl_Interp *interp)
{
    return Pdflib_Init(interp);
}

/* This is required to satisfy pkg_mkIndex */
SWIGEXPORT(int,Pdf_tcl_SafeInit)(Tcl_Interp *interp)
{
    return Pdflib_SafeInit(interp);
}

SWIGEXPORT(int,Pdflib_Init)(Tcl_Interp *interp)
{
    if (interp == 0)
	return TCL_ERROR;

#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, "8.2", 0) == NULL) {
 	return TCL_ERROR;
    }
#else
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 1) == NULL) {
 	return TCL_ERROR;
    }
#endif

    /* Boot the PDFlib core */
    PDF_boot();

    /* Tell Tcl which package we are going to define */
    Tcl_PkgProvide(interp, "pdflib", PDFLIB_VERSIONSTRING);

    Tcl_CreateCommand(interp, "PDF_add_launchlink", (Tcl_CmdProc*) _wrap_PDF_add_launchlink, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_add_locallink", (Tcl_CmdProc*) _wrap_PDF_add_locallink, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_add_note", (Tcl_ObjCmdProc*) _wrap_PDF_add_note, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_add_pdflink", (Tcl_CmdProc*) _wrap_PDF_add_pdflink, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_add_weblink", (Tcl_CmdProc*) _wrap_PDF_add_weblink, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_attach_file", (Tcl_ObjCmdProc*) _wrap_PDF_attach_file, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_border_color", (Tcl_CmdProc*) _wrap_PDF_set_border_color, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_border_dash", (Tcl_CmdProc*) _wrap_PDF_set_border_dash, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_border_style", (Tcl_CmdProc*) _wrap_PDF_set_border_style, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_begin_page", (Tcl_CmdProc*) _wrap_PDF_begin_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_close", (Tcl_CmdProc*) _wrap_PDF_close, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_delete", (Tcl_CmdProc*) _wrap_PDF_delete, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_end_page", (Tcl_CmdProc*) _wrap_PDF_end_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
     Tcl_CreateObjCommand(interp, "PDF_get_buffer", (Tcl_ObjCmdProc*) _wrap_PDF_get_buffer, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_new", (Tcl_CmdProc*) _wrap_PDF_new, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_open_file", (Tcl_CmdProc*) _wrap_PDF_open_file, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "PDF_fill_imageblock", (Tcl_ObjCmdProc*) _wrap_PDF_fill_imageblock, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_fill_pdfblock", (Tcl_ObjCmdProc*) _wrap_PDF_fill_pdfblock, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_fill_textblock", (Tcl_ObjCmdProc*) _wrap_PDF_fill_textblock, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_makespotcolor", (Tcl_CmdProc*) _wrap_PDF_makespotcolor, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setcolor", (Tcl_CmdProc*) _wrap_PDF_setcolor, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setgray", (Tcl_CmdProc*) _wrap_PDF_setgray, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setgray_fill", (Tcl_CmdProc*) _wrap_PDF_setgray_fill, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setgray_stroke", (Tcl_CmdProc*) _wrap_PDF_setgray_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setrgbcolor", (Tcl_CmdProc*) _wrap_PDF_setrgbcolor, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setrgbcolor_fill", (Tcl_CmdProc*) _wrap_PDF_setrgbcolor_fill, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setrgbcolor_stroke", (Tcl_CmdProc*) _wrap_PDF_setrgbcolor_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_arc", (Tcl_CmdProc*) _wrap_PDF_arc, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_arcn", (Tcl_CmdProc*) _wrap_PDF_arcn, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_circle", (Tcl_CmdProc*) _wrap_PDF_circle, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_clip", (Tcl_CmdProc*) _wrap_PDF_clip, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_closepath", (Tcl_CmdProc*) _wrap_PDF_closepath, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_closepath_fill_stroke", (Tcl_CmdProc*) _wrap_PDF_closepath_fill_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_closepath_stroke", (Tcl_CmdProc*) _wrap_PDF_closepath_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_curveto", (Tcl_CmdProc*) _wrap_PDF_curveto, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_endpath", (Tcl_CmdProc*) _wrap_PDF_endpath, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_fill", (Tcl_CmdProc*) _wrap_PDF_fill, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_fill_stroke", (Tcl_CmdProc*) _wrap_PDF_fill_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_lineto", (Tcl_CmdProc*) _wrap_PDF_lineto, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_moveto", (Tcl_CmdProc*) _wrap_PDF_moveto, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_rect", (Tcl_CmdProc*) _wrap_PDF_rect, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_stroke", (Tcl_CmdProc*) _wrap_PDF_stroke, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_encoding_set_char", (Tcl_CmdProc*) _wrap_PDF_encoding_set_char, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_findfont", (Tcl_CmdProc*) _wrap_PDF_findfont, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_load_font", (Tcl_ObjCmdProc*) _wrap_PDF_load_font, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setfont", (Tcl_CmdProc*) _wrap_PDF_setfont, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_concat", (Tcl_CmdProc*) _wrap_PDF_concat, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_initgraphics", (Tcl_CmdProc*) _wrap_PDF_initgraphics, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_restore", (Tcl_CmdProc*) _wrap_PDF_restore, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_rotate", (Tcl_CmdProc*) _wrap_PDF_rotate, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_save", (Tcl_CmdProc*) _wrap_PDF_save, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_scale", (Tcl_CmdProc*) _wrap_PDF_scale, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setdash", (Tcl_CmdProc*) _wrap_PDF_setdash, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setdashpattern", (Tcl_CmdProc*) _wrap_PDF_setdashpattern, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setflat", (Tcl_CmdProc*) _wrap_PDF_setflat, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setlinecap", (Tcl_CmdProc*) _wrap_PDF_setlinecap, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setlinejoin", (Tcl_CmdProc*) _wrap_PDF_setlinejoin, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setlinewidth", (Tcl_CmdProc*) _wrap_PDF_setlinewidth, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setmatrix", (Tcl_CmdProc*) _wrap_PDF_setmatrix, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_setmiterlimit", (Tcl_CmdProc*) _wrap_PDF_setmiterlimit, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_setpolydash", (Tcl_ObjCmdProc*) _wrap_PDF_setpolydash, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_skew", (Tcl_CmdProc*) _wrap_PDF_skew, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_translate", (Tcl_CmdProc*) _wrap_PDF_translate, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "PDF_add_bookmark", (Tcl_ObjCmdProc*) _wrap_PDF_add_bookmark, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_add_nameddest", (Tcl_ObjCmdProc*) _wrap_PDF_add_nameddest, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_set_info", (Tcl_ObjCmdProc*) _wrap_PDF_set_info, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "PDF_load_iccprofile", (Tcl_ObjCmdProc*) _wrap_PDF_load_iccprofile, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_add_thumbnail", (Tcl_CmdProc*) _wrap_PDF_add_thumbnail, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_close_image", (Tcl_CmdProc*) _wrap_PDF_close_image, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_fit_image", (Tcl_CmdProc*) _wrap_PDF_fit_image, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_load_image", (Tcl_ObjCmdProc*) _wrap_PDF_load_image, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_open_CCITT", (Tcl_CmdProc*) _wrap_PDF_open_CCITT, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_open_image", (Tcl_ObjCmdProc*) _wrap_PDF_open_image, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_open_image_file", (Tcl_CmdProc*) _wrap_PDF_open_image_file, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_place_image", (Tcl_CmdProc*) _wrap_PDF_place_image, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_get_parameter", (Tcl_CmdProc*) _wrap_PDF_get_parameter, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_get_value", (Tcl_CmdProc*) _wrap_PDF_get_value, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_parameter", (Tcl_CmdProc*) _wrap_PDF_set_parameter, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_value", (Tcl_CmdProc*) _wrap_PDF_set_value, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_begin_pattern", (Tcl_CmdProc*) _wrap_PDF_begin_pattern, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_end_pattern", (Tcl_CmdProc*) _wrap_PDF_end_pattern, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_close_pdi", (Tcl_CmdProc*) _wrap_PDF_close_pdi, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_close_pdi_page", (Tcl_CmdProc*) _wrap_PDF_close_pdi_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_fit_pdi_page", (Tcl_CmdProc*) _wrap_PDF_fit_pdi_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_get_pdi_parameter", (Tcl_CmdProc*) _wrap_PDF_get_pdi_parameter, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_get_pdi_value", (Tcl_CmdProc*) _wrap_PDF_get_pdi_value, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_open_pdi", (Tcl_CmdProc*) _wrap_PDF_open_pdi, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_open_pdi_page", (Tcl_CmdProc*) _wrap_PDF_open_pdi_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_place_pdi_page", (Tcl_CmdProc*) _wrap_PDF_place_pdi_page, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_process_pdi", (Tcl_CmdProc*) _wrap_PDF_process_pdi, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "PDF_create_pvf", (Tcl_ObjCmdProc*) _wrap_PDF_create_pvf, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_delete_pvf", (Tcl_CmdProc*) _wrap_PDF_delete_pvf, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_shading", (Tcl_CmdProc*) _wrap_PDF_shading, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_shading_pattern", (Tcl_CmdProc*) _wrap_PDF_shading_pattern, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_shfill", (Tcl_CmdProc*) _wrap_PDF_shfill, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_begin_template", (Tcl_CmdProc*) _wrap_PDF_begin_template, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_end_template", (Tcl_CmdProc*) _wrap_PDF_end_template, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateObjCommand(interp, "PDF_continue_text", (Tcl_ObjCmdProc*) _wrap_PDF_continue_text, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_fit_textline", (Tcl_ObjCmdProc*) _wrap_PDF_fit_textline, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_text_pos", (Tcl_CmdProc*) _wrap_PDF_set_text_pos, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_show", (Tcl_ObjCmdProc*) _wrap_PDF_show, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_show_boxed", (Tcl_ObjCmdProc*) _wrap_PDF_show_boxed, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_show_xy", (Tcl_ObjCmdProc*) _wrap_PDF_show_xy, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateObjCommand(interp, "PDF_stringwidth", (Tcl_ObjCmdProc*) _wrap_PDF_stringwidth, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_begin_font", (Tcl_CmdProc*) _wrap_PDF_begin_font, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_begin_glyph", (Tcl_CmdProc*) _wrap_PDF_begin_glyph, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_end_font", (Tcl_CmdProc*) _wrap_PDF_end_font, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_end_glyph", (Tcl_CmdProc*) _wrap_PDF_end_glyph, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_create_gstate", (Tcl_CmdProc*) _wrap_PDF_create_gstate, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_set_gstate", (Tcl_CmdProc*) _wrap_PDF_set_gstate, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    Tcl_CreateCommand(interp, "PDF_get_errmsg", (Tcl_CmdProc*) _wrap_PDF_get_errmsg, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_get_errnum", (Tcl_CmdProc*) _wrap_PDF_get_errnum, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "PDF_get_apiname", (Tcl_CmdProc*) _wrap_PDF_get_apiname, (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

/*
 * These are the pointer type-equivalency mappings.
 * (Used by the SWIG pointer type-checker).
 */
    SWIG_RegisterMapping("_signed_long","_long",0);
    SWIG_RegisterMapping("_struct_PDF_s","_PDF",0);
    SWIG_RegisterMapping("_long","_unsigned_long",0);
    SWIG_RegisterMapping("_long","_signed_long",0);
    SWIG_RegisterMapping("_PDF","_struct_PDF_s",0);
    SWIG_RegisterMapping("_unsigned_long","_long",0);
    SWIG_RegisterMapping("_signed_int","_int",0);
    SWIG_RegisterMapping("_unsigned_short","_short",0);
    SWIG_RegisterMapping("_signed_short","_short",0);
    SWIG_RegisterMapping("_unsigned_int","_int",0);
    SWIG_RegisterMapping("_short","_unsigned_short",0);
    SWIG_RegisterMapping("_short","_signed_short",0);
    SWIG_RegisterMapping("_int","_unsigned_int",0);
    SWIG_RegisterMapping("_int","_signed_int",0);
    return TCL_OK;
}
