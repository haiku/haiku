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

/* $Id: pdflib_pl.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * Wrapper code for the PDFlib Perl binding
 *
 * in sync with pdflib.h 1.151.2.22
 *
 */

/*
 * The PERL_OBJECT #define is only required for ActivePerl < 5.6
 *
 * IMPORTANT: in this case the code must be compiled in C++ mode!
 */
#if defined(WIN32) && \
	defined(PERL_REVISION) && defined(PERL_VERSION) && \
	PERL_REVISION == 5 && PERL_VERSION < 6
#define PERL_OBJECT
#endif

#ifdef __cplusplus
#include <math.h>
#include <stdlib.h>

extern "C" {
#endif /* __cplusplus */

#undef DEBUG
#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>
#undef free
#undef malloc
#include <string.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Compilers which are not strictly ANSI conforming can set PDF_VOLATILE
 * to an empty value.
 */
#ifndef PDF_VOLATILE
#define PDF_VOLATILE    volatile
#endif

/* Definitions for compiling Perl extensions on a variety of machines */

#if defined(WIN32) || defined(__WIN32__)
#   if defined(_MSC_VER)
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

/* #defines for older Perl versions < 5.005_50 */
#if !defined(PERL_REVISION) || \
	((PERL_REVISION >= 5) && \
	((PERL_VERSION<5) || ((PERL_VERSION==5) && (PERL_SUBVERSION<50))))
#ifndef PL_sv_yes
#define PL_sv_yes sv_yes
#endif
#ifndef PL_na
#define PL_na na
#endif
#endif

#ifdef PERL_OBJECT
#define MAGIC_PPERL  CPerlObj *pPerl = (CPerlObj *) this;
#define MAGIC_CAST   (int (CPerlObj::*)(SV *, MAGIC *))
#define SWIGCLASS_STATIC
#else
#define MAGIC_PPERL
#define MAGIC_CAST
#define SWIGCLASS_STATIC static
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

typedef struct SwigPtrType {
  char               *name;
  int               len;
  void               *(*cast)(void *);
  struct SwigPtrType *next;
} SwigPtrType;

/* Pointer cache structure */
typedef struct {
  int               stat;               /* Status (valid) bit             */
  SwigPtrType        *tp;                 /* Pointer to type structure      */
  char                name[256];          /* Given datatype name            */
  char                mapped[256];        /* Equivalent name                */
} SwigCacheType;

static int SwigPtrMax  = 64;	/* Max entries that can be currently held */
static int SwigPtrN    = 0;	/* Current number of entries              */
static int SwigPtrSort = 0;	/* Status flag indicating sort            */
static SwigPtrType *SwigPtrTable = 0;  /* Table containing pointer equivalences  */
static int SwigStart[256];	/* Table containing starting positions    */

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

#ifndef PERL_OBJECT
SWIGSTATIC
void SWIG_RegisterMapping(char *origtype, char *newtype, void *(*cast)(void *)) {
#else
SWIGSTATIC
#define SWIG_RegisterMapping(a,b,c) _SWIG_RegisterMapping(pPerl, a,b,c)
void _SWIG_RegisterMapping(CPerlObj *pPerl, char *origtype, char *newtype, void *(*cast)(void *)) {
#endif

  int i;
  SwigPtrType *t = 0, *t1;

  if (!SwigPtrTable) {
    SwigPtrTable = (SwigPtrType *) malloc(SwigPtrMax*sizeof(SwigPtrType));
    SwigPtrN = 0;
  }
  if (SwigPtrN >= SwigPtrMax) {
    SwigPtrMax = 2*SwigPtrMax;
    SwigPtrTable = (SwigPtrType *) realloc(SwigPtrTable,SwigPtrMax*sizeof(SwigPtrType));
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
  while (t->next) {
    if (strcmp(t->name,newtype) == 0) {
      if (cast) t->cast = cast;
      return;
    }
    t = t->next;
  }
  t1 = (SwigPtrType *) malloc(sizeof(SwigPtrType));
  t1->name = newtype;
  t1->len = strlen(t1->name);
  t1->cast = cast;
  t1->next = 0;
  t->next = t1;
  SwigPtrSort = 0;
}

/* Function for getting a pointer value */

#ifndef PERL_OBJECT
SWIGSTATIC
char *SWIG_GetPtr(SV *sv, void **ptr, char *_t)
#else
SWIGSTATIC
#define SWIG_GetPtr(a,b,c) _SWIG_GetPtr(pPerl,a,b,c)
char *_SWIG_GetPtr(CPerlObj *pPerl, SV *sv, void **ptr, char *_t)
#endif
{
  char temp_type[256];
  char *name,*_c;
  int len,i,start,end;
  IV   tmp = 0;
  SwigPtrType *sp,*tp;
  SwigCacheType *cache;

  /* If magical, apply more magic */

  if (SvGMAGICAL(sv))
    mg_get(sv);

  /* Check to see if this is an object */
  if (sv_isobject(sv)) {
    SV *tsv = (SV*) SvRV(sv);
    if ((SvTYPE(tsv) == SVt_PVHV)) {
      MAGIC *mg;
      if (SvMAGICAL(tsv)) {
	mg = mg_find(tsv,'P');
	if (mg) {
	  SV *rsv = mg->mg_obj;
	  if (sv_isobject(rsv)) {
	    tmp = SvIV((SV*)SvRV(rsv));
	  }
	}
      } else {
	return "Not a valid pointer value";
      }
    } else {
      tmp = SvIV((SV*)SvRV(sv));
    }
    if (!_t) {
      *(ptr) = (void *) tmp;
      return (char *) 0;
    }
  } else if (! SvOK(sv)) {            /* Check for undef */
    *(ptr) = (void *) 0;
    return (char *) 0;
  } else if (SvTYPE(sv) == SVt_RV) {       /* Check for NULL pointer */
    *(ptr) = (void *) 0;
    if (!SvROK(sv))
      return (char *) 0;
    else
      return "Not a valid pointer value";
  } else {                                 /* Don't know what it is */
      *(ptr) = (void *) 0;
      return "Not a valid pointer value";
  }
  if (_t) {
    /* Now see if the types match */

    if (!sv_isa(sv,_t)) {
      _c = HvNAME(SvSTASH(SvRV(sv)));
      if (!SwigPtrSort) {
	qsort((void *) SwigPtrTable, SwigPtrN, sizeof(SwigPtrType), swigsort);
	for (i = 0; i < 256; i++) {
	  SwigStart[i] = SwigPtrN;
	}
	for (i = SwigPtrN-1; i >= 0; i--) {
	  SwigStart[(int) SwigPtrTable[i].name[0]] = i;
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
	      *ptr = (void *) tmp;
	      if (cache->tp->cast) *ptr = (*(cache->tp->cast))(*ptr);
	      return (char *) 0;
	    }
	  }
	}
	SwigLastCache = (SwigLastCache+1) & SWIG_CACHEMASK;
	if (!SwigLastCache) cache = SwigCache;
	else cache++;
      }

      start = SwigStart[(int) _t[0]];
      end = SwigStart[_t[0]+1];
      sp = &SwigPtrTable[start];
      while (start < end) {
	if (swigcmp(_t,sp) == 0) break;
	sp++;
	start++;
      }
      if (start >= end) sp = 0;
      if (sp) {
	while (swigcmp(_t,sp) == 0) {
	  name = sp->name;
	  len = sp->len;
	  tp = sp->next;
	  while(tp) {
	    if (tp->len >= 255) {
	      return _c;
	    }
	    strcpy(temp_type,tp->name);
	    strncat(temp_type,_t+len,255-tp->len);
	    if (sv_isa(sv,temp_type)) {
	      /* Get pointer value */
	      *ptr = (void *) tmp;
	      if (tp->cast) *ptr = (*(tp->cast))(*ptr);

	      strcpy(SwigCache[SwigCacheIndex].mapped,_c);
	      strcpy(SwigCache[SwigCacheIndex].name,_t);
	      SwigCache[SwigCacheIndex].stat = 1;
	      SwigCache[SwigCacheIndex].tp = tp;
	      SwigCacheIndex = SwigCacheIndex & SWIG_CACHEMASK;
	      return (char *) 0;
	    }
	    tp = tp->next;
	  }
	  /* Hmmm. Didn't find it this time */
 	  sp++;
	}
      }
      /* Didn't find any sort of match for this data.
	 Get the pointer value and return the received type */
      *ptr = (void *) tmp;
      return _c;
    } else {
      /* Found a match on the first try.  Return pointer value */
      *ptr = (void *) tmp;
      return (char *) 0;
    }
  }
  *ptr = (void *) tmp;
  return (char *) 0;
}

#ifdef __cplusplus
extern "C"
#endif
#ifndef PERL_OBJECT
/* If we don't use the XS macro it doesn't work with all Perl configurations */
SWIGEXPORT(/* */, XS(boot_pdflib_pl));
#else
SWIGEXPORT(void,boot_pdflib_pl)(CV *cv, CPerlObj *pPerl);
#endif

#include <setjmp.h>

/* The following sick stuff is an attempt to make recent Perl versions
 * work with recent SWIG versions compiled with recent MS VC++ versions.
 */
#ifdef _MSC_VER
    #undef setjmp
    #define setjmp _setjmp
    #undef longjmp
#endif

#include "pdflib.h"

/* exception handling is currently not thread-safe! */
#define SWIG_exception(msg)   	croak(msg)
static jmp_buf			exception_buffer;
static int			exception_status;
static char			error_message[1024];

/* Exception handling */
#define try		if ((exception_status = setjmp(exception_buffer)) == 0)
#define catch(error)	else if (exception_status == error)
#define throw(error)	longjmp(exception_buffer, error)

/* PDFlib error handler */

static void
pdf_swig_errorhandler(PDF *p, int errortype, const char* shortmsg)
{
    sprintf(error_message, "PDFlib Error %s\n", shortmsg);

    /* Issue a warning message and continue for non-fatal errors */
    if (errortype == PDF_NonfatalError) {
	fprintf(stderr, error_message);
	return;
    }

    /* ...and throw an exception */
    throw(errortype);
}

/* export the PDFlib routines to the shared library */
#ifdef __MWERKS__
#pragma export on
#endif

/* p_annots.c */
XS(_wrap_PDF_add_launchlink) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char * PDF_VOLATILE _arg5;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_add_launchlink(p, llx, lly, urx, ury, filename);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_launchlink. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (char *) SvPV(ST(5),PL_na);

    try {     PDF_add_launchlink(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_add_locallink) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    int PDF_VOLATILE _arg5;
    char * PDF_VOLATILE _arg6;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_add_locallink(p, llx, lly, urx, ury, page, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_locallink. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (int )SvIV(ST(5));
    _arg6 = (char *) SvPV(ST(6),PL_na);

    try {     PDF_add_locallink(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_add_note) {
    PDF * p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE contents;
    size_t len_cont;
    char * PDF_VOLATILE title;
    size_t len_title;
    char * PDF_VOLATILE icon;
    int PDF_VOLATILE open;
    dXSARGS ;

    if (items != 9)
        croak("Usage: PDF_add_note(p, llx, lly, urx, ury, contents, title, icon, open);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_note. Expected PDFPtr.");
        XSRETURN(1);
    }
    llx = (float ) SvNV(ST(1));
    lly = (float ) SvNV(ST(2));
    urx = (float ) SvNV(ST(3));
    ury = (float ) SvNV(ST(4));
    contents = (char *) SvPV(ST(5),len_cont);
    title = (char *) SvPV(ST(6),len_title);
    icon = (char *) SvPV(ST(7),PL_na);
    open = (int )SvIV(ST(8));

    try {     PDF_add_note2(p,llx,lly,urx,ury,contents,(int)len_cont,title,(int)len_title,icon,open);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_add_pdflink) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char * PDF_VOLATILE _arg5;
    int PDF_VOLATILE _arg6;
    char * PDF_VOLATILE _arg7;
    dXSARGS ;

    if (items != 8)
        croak("Usage: PDF_add_pdflink(p, llx, lly, urx, ury, filename, page, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_pdflink. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (char *) SvPV(ST(5),PL_na);
    _arg6 = (int )SvIV(ST(6));
    _arg7 = (char *) SvPV(ST(7),PL_na);

    try {     PDF_add_pdflink(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_add_weblink) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    char * PDF_VOLATILE _arg5;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_add_weblink(p, llx, lly, urx, ury, url);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_weblink. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (char *) SvPV(ST(5),PL_na);

    try {     PDF_add_weblink(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_attach_file) {
    PDF * p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE filename;
    char * PDF_VOLATILE description;
    size_t len_descr;
    char * PDF_VOLATILE author;
    size_t len_auth;
    char * PDF_VOLATILE mimetype;
    char * PDF_VOLATILE icon;
    dXSARGS ;

    if (items != 10)
        croak("Usage: PDF_attach_file(p, llx, lly, urx, ury, filename, description, author, mimetype, icon);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_attach_file. Expected PDFPtr.");
        XSRETURN(1);
    }
    llx = (float ) SvNV(ST(1));
    lly = (float ) SvNV(ST(2));
    urx = (float ) SvNV(ST(3));
    ury = (float ) SvNV(ST(4));
    filename = (char *) SvPV(ST(5),PL_na);
    description = (char *) SvPV(ST(6),len_descr);
    author = (char *) SvPV(ST(7),len_auth);
    mimetype = (char *) SvPV(ST(8),PL_na);
    icon = (char *) SvPV(ST(9),PL_na);

    try {     PDF_attach_file2(p,llx,lly,urx,ury,filename,0,description,(int) len_descr,author,(int) len_auth,mimetype,icon);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_border_color) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_set_border_color(p, red, green, blue);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_border_color. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));

    try {     PDF_set_border_color(_arg0,_arg1,_arg2,_arg3);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_border_dash) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_border_dash(p, w, b);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_border_dash. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_set_border_dash(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_border_style) {
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_border_style(p, style, width);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_border_style. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_set_border_style(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_basic.c */
XS(_wrap_PDF_begin_page) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_begin_page(p, width, height);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_begin_page. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_begin_page(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_close) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_close(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_close. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_close(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_delete) {
    PDF * p;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_delete(p);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_delete. Expected PDFPtr.");
        XSRETURN(1);
    }

    /* no  try catch here */
    PDF_delete(p);

    XSRETURN(0);
}

XS(_wrap_PDF_end_page) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_end_page(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_end_page. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_end_page(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_get_apiname) {
    char * _result;
    PDF * _arg0;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_get_apiname(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_apiname. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {
	_result = (char *)PDF_get_apiname(_arg0);
    } else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++),(char *) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_get_buffer) {
    const char * _result;
    PDF * _arg0;
    long  size;
    dXSARGS ;

    if (items != 1) 
        croak("Usage: PDF_get_buffer(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0, "PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_buffer. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     _result = (char *)PDF_get_buffer(_arg0, &size);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(0) = sv_newmortal();
    sv_setpvn((SV*)ST(0), (char *) _result, size);
    XSRETURN(1);
}

XS(_wrap_PDF_get_errmsg) {
    char * _result;
    PDF * _arg0;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_get_errmsg(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_errmsg. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {
	_result = (char *)PDF_get_errmsg(_arg0);
    } else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++),(char *) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_get_errnum) {
    int _result;
    PDF * _arg0;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_get_errnum(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_errnum. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     _result = (int )PDF_get_errnum(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_new) {
    PDF * _result;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;
    char versionbuf[32];

    if (items != 0)
        croak("Usage: PDF_new();");

    try {
	_result = (PDF*)PDF_new2(pdf_swig_errorhandler, NULL, NULL, NULL, NULL);

	if (_result) {
#if defined(ACTIVEPERL_VERSION)
	    sprintf(versionbuf, "ASperl %s", PERLFILEVERSION);
#elif defined(PERL_REVISION) && defined(PERL_VERSION) && defined(PERL_SUBVERSION)
	    sprintf(versionbuf, "Perl %d.%d.%d",
	    	PERL_REVISION, PERL_VERSION, PERL_SUBVERSION);
#else
	    sprintf(versionbuf, "Perl");
#endif
	    PDF_set_parameter(_result, "binding", versionbuf);
	} else {
	    SWIG_exception("PDF_new: internal error");
	}
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setref_pv(ST(argvi++),"PDFPtr", (void *) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_open_file) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_open_file(p,filename);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_file. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);

    try {     _result = (int )PDF_open_file(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

/* p_block.c */

XS(_wrap_PDF_fill_imageblock) {
    int _result;
    PDF * p;
    int PDF_VOLATILE page;
    char * PDF_VOLATILE blockname;
    int PDF_VOLATILE image;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fill_imageblock(p, page, blockname, image, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fill_imageblock. Expected PDFPtr.");
        XSRETURN(1);
    }
    page = (int )SvIV(ST(1));
    blockname = (char *) SvPV(ST(2),PL_na);
    image = (int )SvIV(ST(3));
    optlist = (char *) SvPV(ST(4),PL_na);

    try {     _result = (int )PDF_fill_imageblock(p,page, blockname, image, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_fill_pdfblock) {
    int _result;
    PDF * p;
    int PDF_VOLATILE page;
    char * PDF_VOLATILE blockname;
    int PDF_VOLATILE contents;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fill_pdfblock(p, page, blockname, contents, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fill_pdfblock. Expected PDFPtr.");
        XSRETURN(1);
    }
    page = (int )SvIV(ST(1));
    blockname = (char *) SvPV(ST(2),PL_na);
    contents = (int )SvIV(ST(3));
    optlist = (char *) SvPV(ST(4),PL_na);

    try {     _result = (int )PDF_fill_pdfblock(p,page, blockname, contents, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_fill_textblock) {
    int _result;
    PDF * p;
    int PDF_VOLATILE page;
    char * PDF_VOLATILE blockname;
    char * PDF_VOLATILE text;
    size_t len;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fill_textblock(p, page, blockname, text, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fill_textblock. Expected PDFPtr.");
        XSRETURN(1);
    }
    page = (int )SvIV(ST(1));
    blockname = (char *) SvPV(ST(2),PL_na);
    text = (char *) SvPV(ST(3),len);
    optlist = (char *) SvPV(ST(4),PL_na);

    try {     _result = (int )PDF_fill_textblock(p,page, blockname, text, len, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


/* p_color.c */

XS(_wrap_PDF_makespotcolor) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE spotname;
    int PDF_VOLATILE reserved = 0;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    /* to be compatible to old buggy version we allow two or three parameters */
    if (items != 3 && items != 2)
        croak("Usage: PDF_makespotcolor(p, spotname);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_makespotcolor. Expected PDFPtr.");
        XSRETURN(1);
    }
    spotname = (char *) SvPV(ST(1),PL_na);
    /* Ignore "reserved" parameter, was buggy in V4 wrapper, V5 need 0 */
    /* reserved = (int )SvIV(ST(2)); */

    try {     _result = (int )PDF_makespotcolor(p, spotname, reserved);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_setcolor) {
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;
    int argvi = 0;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_setcolor(p, fstype, colorspace, c1, c2, c3, c4);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setcolor. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));
    _arg6 = (float ) SvNV(ST(6));

    try {     PDF_setcolor(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(argvi);
}

XS(_wrap_PDF_setgray) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setgray(p, gray);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setgray. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {	PDF_setcolor(_arg0, "fillstroke", "gray", _arg1, 0, 0, 0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setgray_fill) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setgray_fill(p, gray);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setgray_fill. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {	PDF_setcolor(_arg0, "fill", "gray", _arg1, 0, 0, 0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setgray_stroke) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setgray_stroke(p, gray);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setgray_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {	PDF_setcolor(_arg0, "stroke", "gray", _arg1, 0, 0, 0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setrgbcolor) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_setrgbcolor(p, red, green, blue);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setrgbcolor. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));

    try {	PDF_setcolor(_arg0, "fillstroke", "rgb", _arg1, _arg2, _arg3,0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setrgbcolor_fill) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_setrgbcolor_fill(p, red, green, blue);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setrgbcolor_fill. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));

    try {	PDF_setcolor(_arg0, "fill", "rgb", _arg1, _arg2, _arg3, 0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setrgbcolor_stroke) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_setrgbcolor_stroke(p,red,green,blue);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setrgbcolor_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));

    try {	PDF_setcolor(_arg0, "stroke", "rgb", _arg1, _arg2, _arg3, 0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_draw.c */

XS(_wrap_PDF_arc) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_arc(p, x, y, r, alpha, beta);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_arc. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));

    try {     PDF_arc(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_arcn) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    int argvi = 0;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_arcn(p, x, y, r, alpha, beta);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_arcn. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));

    try {     PDF_arcn(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_circle) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_circle(p, x, y, r);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_circle. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));

    try {     PDF_circle(_arg0,_arg1,_arg2,_arg3);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_clip) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_clip(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_clip. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_clip(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_closepath) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_closepath(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_closepath. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_closepath(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_closepath_fill_stroke) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_closepath_fill_stroke(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_closepath_fill_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_closepath_fill_stroke(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_closepath_stroke) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_closepath_stroke(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_closepath_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_closepath_stroke(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_curveto) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_curveto(p, x1, y1, x2, y2, x3, y3);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_curveto. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));
    _arg6 = (float ) SvNV(ST(6));

    try {     PDF_curveto(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_endpath) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_endpath(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_endpath. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_endpath(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_fill) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_fill(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fill. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_fill(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_fill_stroke) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_fill_stroke(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fill_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_fill_stroke(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_lineto) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_lineto(p, x, y);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_lineto. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_lineto(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_moveto) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_moveto(p, x, y);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_moveto. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_moveto(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_rect) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_rect(p, x, y, width, height);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_rect. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));

    try {     PDF_rect(_arg0,_arg1,_arg2,_arg3,_arg4);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_stroke) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_stroke(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_stroke. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_stroke(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_encoding.c */

XS(_wrap_PDF_encoding_set_char) {
    PDF * p;
    char * PDF_VOLATILE encoding;
    int PDF_VOLATILE slot;
    char * PDF_VOLATILE glyphname;
    int PDF_VOLATILE uv;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_encoding_set_char(p, encoding, slot, glyphname, uv);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_encoding_set_char. Expected PDFPtr.");
        XSRETURN(1);
    }

    encoding = (char *) SvPV(ST(1),PL_na);
    slot = (int )SvIV(ST(2));
    glyphname = (char *) SvPV(ST(3),PL_na);
    uv = (int )SvIV(ST(4));

    try {     PDF_encoding_set_char(p, encoding, slot, glyphname, uv);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}


/* p_font.c */

XS(_wrap_PDF_findfont) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    int PDF_VOLATILE _arg3;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_findfont(p, fontname, encoding, embed);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_findfont. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _arg3 = (int )SvIV(ST(3));

    try {     _result = (int )PDF_findfont(_arg0,_arg1,_arg2,_arg3);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_load_font) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE fontname;
    char * PDF_VOLATILE encoding;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_load_font(p, fontname, encoding, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_load_font. Expected PDFPtr.");
        XSRETURN(1);
    }
    fontname = (char *) SvPV(ST(1),PL_na);
    encoding = (char *) SvPV(ST(2),PL_na);
    optlist = (char *) SvPV(ST(3),PL_na);

    try {     _result = (int )PDF_load_font(p, fontname, 0, encoding, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_setfont) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_setfont(p, font, fontsize);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setfont. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_setfont(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_gstate.c */

XS(_wrap_PDF_concat) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_concat(p, a, b, c, d, e, f);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_concat. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));
    _arg6 = (float ) SvNV(ST(6));

    try {     PDF_concat(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_initgraphics) {
    PDF * _arg0;
    int argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_initgraphics(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_initgraphics. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_initgraphics(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_restore) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_restore(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_restore. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_restore(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_rotate) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_rotate(p, phi);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_rotate. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {     PDF_rotate(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_save) {
    PDF * _arg0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_save(p);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_save. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_save(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_scale) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_scale(p, sx, sy);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_scale. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_scale(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setdash) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_setdash(p, b, w);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setdash. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_setdash(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setdashpattern) {
    PDF * p;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setdashpattern(p, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setdashpattern. Expected PDFPtr.");
        XSRETURN(1);
    }
    optlist = (char *) SvPV(ST(1),PL_na);

    try {     PDF_setdashpattern(p, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setflat) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setflat(p, flatness);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setflat. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {     PDF_setflat(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setlinecap) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setlinecap(p, linecap);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setlinecap. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_setlinecap(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setlinejoin) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setlinejoin(p, linejoin);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setlinejoin. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_setlinejoin(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setlinewidth) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setlinewidth(p, width);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setlinewidth. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {     PDF_setlinewidth(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setmatrix) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    float _arg6;
    int argvi = 0;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_setmatrix(p, a, b, c, d, e, f);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setmatrix. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));
    _arg6 = (float ) SvNV(ST(6));

    try {     PDF_setmatrix(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_setmiterlimit) {
    PDF * _arg0;
    float _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setmiterlimit(p, miter);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_setmiterlimit. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));

    try {     PDF_setmiterlimit(_arg0,_arg1);
}
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_setpolydash) {
    PDF *p;
    SV *val;
    AV *av;
    float carray[MAX_DASH_LENGTH];
    int PDF_VOLATILE length;
    int i;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_setpolydash(p, arrayref);");

    if (SWIG_GetPtr(ST(0),(void **) &p, "PDFPtr")) {
        croak("Type error in argument 1 of PDF_setpolydash. Expected PDFPtr.");
        XSRETURN(1);
    }

    if (!SvROK(ST(1))) {
        croak("Type error in argument 2 of PDF_setpolydash. Expected reference to array.");
        XSRETURN(1);
    }

    av = (AV *) SvRV(ST(1));
    length = (int) av_len(av) + 1;

    if (length > MAX_DASH_LENGTH)
	length = MAX_DASH_LENGTH;

    for (i = 0; i < length; i++) {
	val = *av_fetch(av, i, 0);
	if ((!SvNOK(val)) && (!SvIOK(val))) {
	    croak("expected a reference to a float array in PDF_setpolydash\n");
	}
	carray[i] = SvNV(val);
    }

    try {     PDF_setpolydash(p, carray, length);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}

XS(_wrap_PDF_skew) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_skew(p, alpha, beta);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_skew. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_skew(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_translate) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_translate(p, tx, ty);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_translate. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_translate(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_hyper.c */

XS(_wrap_PDF_add_bookmark) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE text;
    size_t len;
    int PDF_VOLATILE parent;
    int PDF_VOLATILE open;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_add_bookmark(p, text, parent, open);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_bookmark. Expected PDFPtr.");
        XSRETURN(1);
    }
    text = (char *) SvPV(ST(1),len);
    parent = (int )SvIV(ST(2));
    open = (int )SvIV(ST(3));

    try {     _result = PDF_add_bookmark2(p,text,(int) len,parent,open);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_add_nameddest) {
    PDF * p;
    char * PDF_VOLATILE name;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_add_nameddest(p, name, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_nameddest. Expected PDFPtr.");
        XSRETURN(1);
    }
    name = (char *) SvPV(ST(1),PL_na);
    optlist = (char *) SvPV(ST(2),PL_na);

    try {     PDF_add_nameddest(p, name, 0, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_info) {
    PDF * p;
    char * PDF_VOLATILE key;
    char * PDF_VOLATILE value;
    size_t len;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_info(p, key, value);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_info. Expected PDFPtr.");
        XSRETURN(1);
    }
    key = (char *) SvPV(ST(1),PL_na);
    value = (char *) SvPV(ST(2),len);

    try {     PDF_set_info2(p,key,value,(int) len);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_icc.c */

XS(_wrap_PDF_load_iccprofile) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE profilename;
    char * PDF_VOLATILE optlist;
    int argvi = 0;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_load_iccprofile(p, profilename, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_load_iccprofile. Expected PDFPtr.");
        XSRETURN(1);
    }
    profilename = (char *) SvPV(ST(1),PL_na);
    optlist = (char *) SvPV(ST(2),PL_na);

    try {     _result = PDF_load_iccprofile(p, profilename, 0, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


/* p_image.c */

XS(_wrap_PDF_add_thumbnail) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    int argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_add_thumbnail(p, image);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_add_thumbnail. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_add_thumbnail(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_close_image) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_close_image(p, image);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_close_image. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_close_image(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_fit_image) {
    PDF * p;
    int image;
    float x;
    float y;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fit_image(p, image,  x, y, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fit_image. Expected PDFPtr.");
        XSRETURN(1);
    }
    image = (int )SvIV(ST(1));
    x = (float ) SvNV(ST(2));
    y = (float ) SvNV(ST(3));
    optlist = (char *) SvPV(ST(4),PL_na);

    try {     PDF_fit_image(p, image,  x, y, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_load_image) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE imagetype;
    char * PDF_VOLATILE filename;
    char * PDF_VOLATILE optlist;
    int argvi = 0;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_load_image(p, imagetype, filename, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_load_image. Expected PDFPtr.");
        XSRETURN(1);
    }
    imagetype = (char *) SvPV(ST(1),PL_na);
    filename = (char *) SvPV(ST(2),PL_na);
    optlist = (char *) SvPV(ST(3),PL_na);

    try {     _result = PDF_load_image(p, imagetype, filename, 0, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


XS(_wrap_PDF_open_CCITT) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    int PDF_VOLATILE _arg2;
    int PDF_VOLATILE _arg3;
    int PDF_VOLATILE _arg4;
    int PDF_VOLATILE _arg5;
    int PDF_VOLATILE _arg6;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_open_CCITT(p, filename, width, height, BitReverse, K, BlackIs1);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_CCITT. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (int )SvIV(ST(2));
    _arg3 = (int )SvIV(ST(3));
    _arg4 = (int )SvIV(ST(4));
    _arg5 = (int )SvIV(ST(5));
    _arg6 = (int )SvIV(ST(6));

    try {     _result = (int )PDF_open_CCITT(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_open_image) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    char * PDF_VOLATILE _arg3;
    long  PDF_VOLATILE _arg4;
    int PDF_VOLATILE _arg5;
    int PDF_VOLATILE _arg6;
    int PDF_VOLATILE _arg7;
    int PDF_VOLATILE _arg8;
    char * PDF_VOLATILE _arg9;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 10)
        croak("Usage: PDF_open_image(p, imagetype, source, data, length, width, height, components, bpc, params);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_image. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _arg3 = (char *) SvPV(ST(3),PL_na);
    _arg4 = (long )SvIV(ST(4));
    _arg5 = (int )SvIV(ST(5));
    _arg6 = (int )SvIV(ST(6));
    _arg7 = (int )SvIV(ST(7));
    _arg8 = (int )SvIV(ST(8));
    _arg9 = (char *) SvPV(ST(9),PL_na);

    try {     _result = (int )PDF_open_image(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8,_arg9);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_open_image_file) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    char * PDF_VOLATILE _arg3;
    int PDF_VOLATILE _arg4;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_open_image_file(p, imagetype, filename, stringparam, intparam);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_image_file. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _arg3 = (char *) SvPV(ST(3),PL_na);
    _arg4 = (int )SvIV(ST(4));

    try {     _result = (int )PDF_open_image_file(_arg0,_arg1,_arg2,_arg3,_arg4);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_place_image) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_place_image(p, image, x, y, scale);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_place_image. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));

    try {     PDF_place_image(_arg0,_arg1,_arg2,_arg3,_arg4);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

/* p_params.c */

XS(_wrap_PDF_get_parameter) {
    char * _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    float _arg2;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_get_parameter(p, key, modifier);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_parameter. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (float ) SvNV(ST(2));

    try {     _result = (char *)PDF_get_parameter(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++),(char *) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_get_value) {
    float _result;
    PDF * p;
    char * PDF_VOLATILE key;
    float value;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_get_value(p, key, modifier);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
	/* allow get_value with null-pointer */
	p = (PDF *)0;
        /*croak("Type error in argument 1 of PDF_get_value. Expected PDFPtr.");
        XSRETURN(1);
	*/
    }
    key = (char *) SvPV(ST(1),PL_na);
    value = (float ) SvNV(ST(2));

    try {     _result = (float )PDF_get_value(p,key,value);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setnv(ST(argvi++), (double) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_set_parameter) {
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_parameter(p, key, value);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_parameter. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);

    try {     PDF_set_parameter(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_value) {
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    float _arg2;
    int argvi = 0;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_value(p, key, value);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_value. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_set_value(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

/* p_pattern.c */

XS(_wrap_PDF_begin_pattern) {
    int _result;
    PDF * _arg0;
    float _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    int PDF_VOLATILE _arg5;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_begin_pattern(p, width, height, xstep, ystep, painttype);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_begin_pattern. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (int )SvIV(ST(5));

    try {     _result = (int )PDF_begin_pattern(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_end_pattern) {
    PDF * _arg0;
    int argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_end_pattern(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_end_pattern. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_end_pattern(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

/* p_pdi.c */
XS(_wrap_PDF_close_pdi) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    int argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_close_pdi(p, doc);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_close_pdi. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_close_pdi(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_close_pdi_page) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    int argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_close_pdi_page(p, page);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_close_pdi_page. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));

    try {     PDF_close_pdi_page(_arg0,_arg1);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(argvi);
}

XS(_wrap_PDF_fit_pdi_page) {
    PDF * p;
    int PDF_VOLATILE page;
    float x;
    float y;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fit_pdi_page(p, page, x, y, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fit_pdi_page. Expected PDFPtr.");
        XSRETURN(1);
    }
    page = (int )SvIV(ST(1));
    x = (float ) SvNV(ST(2));
    y = (float ) SvNV(ST(3));
    optlist = (char *) SvPV(ST(4),PL_na);

    try {     PDF_fit_pdi_page(p, page, x, y, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}


XS(_wrap_PDF_get_pdi_parameter) {
    char * _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    int PDF_VOLATILE _arg2;
    int PDF_VOLATILE _arg3;
    int PDF_VOLATILE _arg4;
    int len;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_get_pdi_parameter(p, key, doc, page, reserved);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_pdi_parameter. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (int )SvIV(ST(2));
    _arg3 = (int )SvIV(ST(3));
    _arg4 = (int )SvIV(ST(4));

    try {     _result = (char *)PDF_get_pdi_parameter(_arg0,_arg1,_arg2,_arg3,_arg4, &len);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(0) = sv_newmortal();
    sv_setpvn((SV*)ST(0), (char *) _result, len);
    XSRETURN(1);
}

XS(_wrap_PDF_get_pdi_value) {
    float _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    int PDF_VOLATILE _arg2;
    int PDF_VOLATILE _arg3;
    int PDF_VOLATILE _arg4;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_get_pdi_value(p, key, doc, page, reserved);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_get_pdi_value. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (int )SvIV(ST(2));
    _arg3 = (int )SvIV(ST(3));
    _arg4 = (int )SvIV(ST(4));

    try {     _result = (float )PDF_get_pdi_value(_arg0,_arg1,_arg2,_arg3,_arg4);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setnv(ST(argvi++), (double) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_open_pdi) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    char * PDF_VOLATILE _arg2;
    int PDF_VOLATILE _arg3;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4) 
        croak("Usage: PDF_open_pdi(p, filename, stringparam, reserved);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_pdi. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (char *) SvPV(ST(2),PL_na);
    _arg3 = (int )SvIV(ST(3));

    try {     _result = (int )PDF_open_pdi(_arg0,_arg1,_arg2,_arg3);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_open_pdi_page) {
    int _result;
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    int PDF_VOLATILE _arg2;
    char * PDF_VOLATILE _arg3;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_open_pdi_page(p, doc, page, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_open_pdi_page. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));
    _arg2 = (int )SvIV(ST(2));
    _arg3 = (char *) SvPV(ST(3),PL_na);

    try {     _result = (int )PDF_open_pdi_page(_arg0,_arg1,_arg2,_arg3);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_place_pdi_page) {
    PDF * _arg0;
    int PDF_VOLATILE _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    dXSARGS ;

    if (items != 6)
        croak("Usage: PDF_place_pdi_page(p, image, x, y, sx, sy);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_place_pdi_page. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (int )SvIV(ST(1));
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));

    try {     PDF_place_pdi_page(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_process_pdi) {
    int _result;
    PDF * p;
    int PDF_VOLATILE doc;
    int PDF_VOLATILE page;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 4) 
        croak("Usage: PDF_process_pdi(p, doc, page, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_process_pdi. Expected PDFPtr.");
        XSRETURN(1);
    }
    doc = (int )SvIV(ST(1));
    page = (int )SvIV(ST(2));
    optlist = (char *) SvPV(ST(3),PL_na);

    try {     _result = (int )PDF_process_pdi(p, doc, page, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


/* p_resource.c */

XS(_wrap_PDF_create_pvf) {
    PDF * p;
    char * PDF_VOLATILE filename;
    char * PDF_VOLATILE data;
    size_t size;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_create_pvf(p, filename, data, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_create_pvf. Expected PDFPtr.");
        XSRETURN(1);
    }
    filename = (char *) SvPV(ST(1),PL_na);
    data = (char *) SvPV(ST(2),size);
    optlist = (char *) SvPV(ST(3),PL_na);

    try {     PDF_create_pvf(p, filename, 0, data, size, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_delete_pvf) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE filename;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 2) 
        croak("Usage: PDF_delete_pvf(p, filename);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_delete_pvf. Expected PDFPtr.");
        XSRETURN(1);
    }
    filename = (char *) SvPV(ST(1),PL_na);

    try {     _result = (int )PDF_delete_pvf(p, filename, 0);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


/* p_shading.c */

XS(_wrap_PDF_shading) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE shtype;
    float x0;
    float y0;
    float x1;
    float y1;
    float c1;
    float c2;
    float c3;
    float c4;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 11) 
        croak("Usage: PDF_shading(p, shtype, x0, y0, x1, y1, c1, c2, c3, c4, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_shading. Expected PDFPtr.");
        XSRETURN(1);
    }
    shtype = (char *) SvPV(ST(1),PL_na);
    x0 = (float ) SvNV(ST(2));
    y0 = (float ) SvNV(ST(3));
    x1 = (float ) SvNV(ST(4));
    y1 = (float ) SvNV(ST(5));
    c1 = (float ) SvNV(ST(6));
    c2 = (float ) SvNV(ST(7));
    c3 = (float ) SvNV(ST(8));
    c4 = (float ) SvNV(ST(9));
    optlist = (char *) SvPV(ST(10),PL_na);

    try {     _result = (int )PDF_shading(p, shtype, x0, y0, x1, y1, c1, c2, c3, c4, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_shading_pattern) {
    int _result;
    PDF * p;
    int PDF_VOLATILE shading;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 3) 
        croak("Usage: PDF_shading_pattern(p, shading, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_shading_pattern. Expected PDFPtr.");
        XSRETURN(1);
    }
    shading = (int )SvIV(ST(1));
    optlist = (char *) SvPV(ST(2),PL_na);

    try {     _result = (int )PDF_shading_pattern(p, shading, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_shfill) {
    PDF * p;
    int PDF_VOLATILE shading;
    int argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_shfill(p, shading);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_shfill. Expected PDFPtr.");
        XSRETURN(1);
    }

    shading = (int )SvIV(ST(1));

    try {     PDF_shfill(p, shading);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(argvi);
}


/* p_template.c */

XS(_wrap_PDF_begin_template) {
    int _result;
    PDF * _arg0;
    float _arg1;
    float _arg2;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_begin_template(p, width, height);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_begin_template. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     _result = (int )PDF_begin_template(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_end_template) {
    PDF * _arg0;
    int argvi = 0;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_end_template(p);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_end_template. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_end_template(_arg0);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(argvi);
}

/* p_text.c */

XS(_wrap_PDF_continue_text) {
    PDF * p;
    char * PDF_VOLATILE text;
    size_t len;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_continue_text(p, text);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_continue_text. Expected PDFPtr.");
        XSRETURN(1);
    }
    text = (char *) SvPV(ST(1), len);

    try {     PDF_continue_text2(p, text, (int) len);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_fit_textline) {
    PDF * p;
    char * PDF_VOLATILE text;
    size_t len;
    float x;
    float y;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 5)
        croak("Usage: PDF_fit_textline(p, text, x, y, optlist);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_fit_textline. Expected PDFPtr.");
        XSRETURN(1);
    }
    text = (char *) SvPV(ST(1), len);
    x = (float ) SvNV(ST(2));
    y = (float ) SvNV(ST(3));
    optlist = (char *) SvPV(ST(4), PL_na);

    try {     PDF_fit_textline(p, text, (int) len, x, y, optlist);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_set_text_pos) {
    PDF * _arg0;
    float _arg1;
    float _arg2;
    dXSARGS ;

    if (items != 3)
        croak("Usage: PDF_set_text_pos(p, x, y);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_text_pos. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (float ) SvNV(ST(1));
    _arg2 = (float ) SvNV(ST(2));

    try {     PDF_set_text_pos(_arg0,_arg1,_arg2);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_show) {
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    dXSARGS ;
    size_t len;

    if (items != 2)
        croak("Usage: PDF_show(p, text);");
    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_show. Expected PDFPtr.");
        XSRETURN(1);
    }
    _arg1 = (char *) SvPV(ST(1), len);

    try {     PDF_show2(_arg0,_arg1, (int) len);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_show_boxed) {
    int _result;
    PDF * _arg0;
    char * PDF_VOLATILE _arg1;
    float _arg2;
    float _arg3;
    float _arg4;
    float _arg5;
    char * PDF_VOLATILE _arg6;
    char * PDF_VOLATILE _arg7;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 8)
        croak("Usage: PDF_show_boxed(p, text, left, top, width, height, hmode, feature);");

    if (SWIG_GetPtr(ST(0),(void **) &_arg0,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_show_boxed. Expected PDFPtr.");
        XSRETURN(1);
    }

    _arg1 = (char *) SvPV(ST(1),PL_na);
    _arg2 = (float ) SvNV(ST(2));
    _arg3 = (float ) SvNV(ST(3));
    _arg4 = (float ) SvNV(ST(4));
    _arg5 = (float ) SvNV(ST(5));
    _arg6 = (char *) SvPV(ST(6),PL_na);
    _arg7 = (char *) SvPV(ST(7),PL_na);

    try {     _result = (int )PDF_show_boxed(_arg0,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7);
 }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}

XS(_wrap_PDF_show_xy) {
    PDF * p;
    char * PDF_VOLATILE text;
    float x;
    float y;
    size_t len;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_show_xy(p, text, x, y);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_show_xy. Expected PDFPtr.");
        XSRETURN(1);
    }
    text = (char *) SvPV(ST(1), len);
    x = (float ) SvNV(ST(2));
    y = (float ) SvNV(ST(3));

    try {     PDF_show_xy2(p,text, (int) len, x,y);
    }
    else {
	SWIG_exception(error_message);
    }
    XSRETURN(0);
}

XS(_wrap_PDF_stringwidth) {
    float _result;
    PDF * p;
    char * PDF_VOLATILE text;
    int PDF_VOLATILE font;
    float fontsize;
    int PDF_VOLATILE argvi = 0;
    size_t len;
    dXSARGS ;

    if (items != 4)
        croak("Usage: PDF_stringwidth(p, text, font, fontsize);");
    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_stringwidth. Expected PDFPtr.");
        XSRETURN(1);
    }
    text = (char *) SvPV(ST(1), len);
    font = (int )SvIV(ST(2));
    fontsize = (float ) SvNV(ST(3));

    try {     _result = (float )PDF_stringwidth2(p,text, (int) len, font,fontsize);
    }
    else {
	SWIG_exception(error_message);
    }
    ST(argvi) = sv_newmortal();
    sv_setnv(ST(argvi++), (double) _result);
    XSRETURN(argvi);
}

/* p_type3.c */

XS(_wrap_PDF_begin_font) {
    PDF * p;
    char * PDF_VOLATILE fontname;
    float a, b, c, d, e, f;
    char * PDF_VOLATILE optlist;
    dXSARGS ;

    if (items != 9)
        croak("Usage: PDF_begin_font(p, fontname, a, b, c, d, e, f, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_begin_font. Expected PDFPtr.");
        XSRETURN(1);
    }

    fontname = (char *) SvPV(ST(1),PL_na);
    a = (float )SvNV(ST(2));
    b = (float )SvNV(ST(3));
    c = (float )SvNV(ST(4));
    d = (float )SvNV(ST(5));
    e = (float )SvNV(ST(6));
    f = (float )SvNV(ST(7));
    optlist = (char *) SvPV(ST(8),PL_na);

    try {     PDF_begin_font(p, fontname, 0, a, b, c, d, e, f, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}

XS(_wrap_PDF_begin_glyph) {
    PDF * p;
    char * PDF_VOLATILE glyphname;
    float wx, llx, lly, urx, ury;
    dXSARGS ;

    if (items != 7)
        croak("Usage: PDF_begin_glyph(p, glyphname, wx, llx, lly, urx, ury);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_begin_glyph. Expected PDFPtr.");
        XSRETURN(1);
    }

    glyphname = (char *) SvPV(ST(1),PL_na);
    wx = (float )SvIV(ST(2));
    llx = (float )SvIV(ST(3));
    lly = (float )SvIV(ST(4));
    urx = (float )SvIV(ST(5));
    ury = (float )SvIV(ST(6));

    try {     PDF_begin_glyph(p, glyphname, wx, llx, lly, urx, ury);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}

XS(_wrap_PDF_end_font) {
    PDF * p;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_end_font(p);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_end_font. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_end_font(p);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}

XS(_wrap_PDF_end_glyph) {
    PDF * p;
    dXSARGS ;

    if (items != 1)
        croak("Usage: PDF_end_glyph(p);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_end_glyph. Expected PDFPtr.");
        XSRETURN(1);
    }

    try {     PDF_end_glyph(p);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}

/* p_xgstate.c */

/*
5   int PDF_create_gstate (p, char *optlist)
*/
XS(_wrap_PDF_create_gstate) {
    int _result;
    PDF * p;
    char * PDF_VOLATILE optlist;
    int PDF_VOLATILE argvi = 0;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_create_gstate(p, optlist);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_create_gstate. Expected PDFPtr.");
        XSRETURN(1);
    }
    optlist = (char *) SvPV(ST(1),PL_na);

    try {     _result = (int )PDF_create_gstate(p, optlist);
    }
    else {
	SWIG_exception(error_message);
    }

    ST(argvi) = sv_newmortal();
    sv_setiv(ST(argvi++),(IV) _result);
    XSRETURN(argvi);
}


XS(_wrap_PDF_set_gstate) {
    PDF * p;
    int PDF_VOLATILE gstate;
    dXSARGS ;

    if (items != 2)
        croak("Usage: PDF_set_gstate(p, gstate);");

    if (SWIG_GetPtr(ST(0),(void **) &p,"PDFPtr")) {
        croak("Type error in argument 1 of PDF_set_gstate. Expected PDFPtr.");
        XSRETURN(1);
    }

    gstate = (int )SvIV(ST(1));

    try {     PDF_set_gstate(p, gstate);
    }
    else {
	SWIG_exception(error_message);
    }

    XSRETURN(0);
}



XS(_wrap_perl5_pdflib_var_init) {
    dXSARGS;
    (void) items;

    XSRETURN(1);
}
#ifdef __cplusplus
extern "C"
#endif
XS(boot_pdflib_pl) {
    dXSARGS;
    char *file = __FILE__;
    newXS("pdflibc::var_pdflib_init", _wrap_perl5_pdflib_var_init, file);

    (void) items;

    /* Boot the PDFlib core */
    PDF_boot();

    newXS("pdflibc::PDF_add_launchlink", _wrap_PDF_add_launchlink, file);
    newXS("pdflibc::PDF_add_locallink", _wrap_PDF_add_locallink, file);
    newXS("pdflibc::PDF_add_note", _wrap_PDF_add_note, file);
    newXS("pdflibc::PDF_add_pdflink", _wrap_PDF_add_pdflink, file);
    newXS("pdflibc::PDF_add_weblink", _wrap_PDF_add_weblink, file);
    newXS("pdflibc::PDF_attach_file", _wrap_PDF_attach_file, file);
    newXS("pdflibc::PDF_set_border_color", _wrap_PDF_set_border_color, file);
    newXS("pdflibc::PDF_set_border_dash", _wrap_PDF_set_border_dash, file);
    newXS("pdflibc::PDF_set_border_style", _wrap_PDF_set_border_style, file);

    newXS("pdflibc::PDF_begin_page", _wrap_PDF_begin_page, file);
    newXS("pdflibc::PDF_close", _wrap_PDF_close, file);
    newXS("pdflibc::PDF_delete", _wrap_PDF_delete, file);
    newXS("pdflibc::PDF_end_page", _wrap_PDF_end_page, file);
    newXS("pdflibc::PDF_get_buffer", _wrap_PDF_get_buffer, file);
    newXS("pdflibc::PDF_new", _wrap_PDF_new, file);
    newXS("pdflibc::PDF_open_file", _wrap_PDF_open_file, file);
    newXS("pdflibc::PDF_get_errmsg", _wrap_PDF_get_errmsg, file);
    newXS("pdflibc::PDF_get_apiname", _wrap_PDF_get_apiname, file);
    newXS("pdflibc::PDF_get_errnum", _wrap_PDF_get_errnum, file);

    newXS("pdflibc::PDF_fill_imageblock", _wrap_PDF_fill_imageblock, file);
    newXS("pdflibc::PDF_fill_pdfblock", _wrap_PDF_fill_pdfblock, file);
    newXS("pdflibc::PDF_fill_textblock", _wrap_PDF_fill_textblock, file);

    newXS("pdflibc::PDF_makespotcolor", _wrap_PDF_makespotcolor, file);
    newXS("pdflibc::PDF_setcolor", _wrap_PDF_setcolor, file);
    newXS("pdflibc::PDF_setgray", _wrap_PDF_setgray, file);
    newXS("pdflibc::PDF_setgray_fill", _wrap_PDF_setgray_fill, file);
    newXS("pdflibc::PDF_setgray_stroke", _wrap_PDF_setgray_stroke, file);
    newXS("pdflibc::PDF_setrgbcolor", _wrap_PDF_setrgbcolor, file);
    newXS("pdflibc::PDF_setrgbcolor_fill", _wrap_PDF_setrgbcolor_fill, file);
    newXS("pdflibc::PDF_setrgbcolor_stroke", _wrap_PDF_setrgbcolor_stroke, file);

    newXS("pdflibc::PDF_arc", _wrap_PDF_arc, file);
    newXS("pdflibc::PDF_arcn", _wrap_PDF_arcn, file);
    newXS("pdflibc::PDF_circle", _wrap_PDF_circle, file);
    newXS("pdflibc::PDF_clip", _wrap_PDF_clip, file);
    newXS("pdflibc::PDF_closepath", _wrap_PDF_closepath, file);
    newXS("pdflibc::PDF_closepath_fill_stroke", _wrap_PDF_closepath_fill_stroke, file);
    newXS("pdflibc::PDF_closepath_stroke", _wrap_PDF_closepath_stroke, file);
    newXS("pdflibc::PDF_curveto", _wrap_PDF_curveto, file);
    newXS("pdflibc::PDF_endpath", _wrap_PDF_endpath, file);
    newXS("pdflibc::PDF_fill", _wrap_PDF_fill, file);
    newXS("pdflibc::PDF_fill_stroke", _wrap_PDF_fill_stroke, file);
    newXS("pdflibc::PDF_lineto", _wrap_PDF_lineto, file);
    newXS("pdflibc::PDF_moveto", _wrap_PDF_moveto, file);
    newXS("pdflibc::PDF_rect", _wrap_PDF_rect, file);
    newXS("pdflibc::PDF_stroke", _wrap_PDF_stroke, file);

    newXS("pdflibc::PDF_encoding_set_char", _wrap_PDF_encoding_set_char, file);

    newXS("pdflibc::PDF_findfont", _wrap_PDF_findfont, file);
    newXS("pdflibc::PDF_load_font", _wrap_PDF_load_font, file);
    newXS("pdflibc::PDF_setfont", _wrap_PDF_setfont, file);

    newXS("pdflibc::PDF_concat", _wrap_PDF_concat, file);
    newXS("pdflibc::PDF_initgraphics", _wrap_PDF_initgraphics, file);
    newXS("pdflibc::PDF_restore", _wrap_PDF_restore, file);
    newXS("pdflibc::PDF_rotate", _wrap_PDF_rotate, file);
    newXS("pdflibc::PDF_save", _wrap_PDF_save, file);
    newXS("pdflibc::PDF_scale", _wrap_PDF_scale, file);
    newXS("pdflibc::PDF_setdash", _wrap_PDF_setdash, file);
    newXS("pdflibc::PDF_setdashpattern", _wrap_PDF_setdashpattern, file);
    newXS("pdflibc::PDF_setflat", _wrap_PDF_setflat, file);
    newXS("pdflibc::PDF_setlinecap", _wrap_PDF_setlinecap, file);
    newXS("pdflibc::PDF_setlinejoin", _wrap_PDF_setlinejoin, file);
    newXS("pdflibc::PDF_setlinewidth", _wrap_PDF_setlinewidth, file);
    newXS("pdflibc::PDF_setmatrix", _wrap_PDF_setmatrix, file);
    newXS("pdflibc::PDF_setmiterlimit", _wrap_PDF_setmiterlimit, file);
    newXS("pdflibc::PDF_setpolydash", _wrap_PDF_setpolydash, file);
    newXS("pdflibc::PDF_skew", _wrap_PDF_skew, file);
    newXS("pdflibc::PDF_translate", _wrap_PDF_translate, file);

    newXS("pdflibc::PDF_add_bookmark", _wrap_PDF_add_bookmark, file);
    newXS("pdflibc::PDF_add_nameddest", _wrap_PDF_add_nameddest, file);
    newXS("pdflibc::PDF_set_info", _wrap_PDF_set_info, file);

    newXS("pdflibc::PDF_load_iccprofile", _wrap_PDF_load_iccprofile, file);

    newXS("pdflibc::PDF_add_thumbnail", _wrap_PDF_add_thumbnail, file);
    newXS("pdflibc::PDF_close_image", _wrap_PDF_close_image, file);
    newXS("pdflibc::PDF_fit_image", _wrap_PDF_fit_image, file);
    newXS("pdflibc::PDF_load_image", _wrap_PDF_load_image, file);
    newXS("pdflibc::PDF_open_CCITT", _wrap_PDF_open_CCITT, file);
    newXS("pdflibc::PDF_open_image", _wrap_PDF_open_image, file);
    newXS("pdflibc::PDF_open_image_file", _wrap_PDF_open_image_file, file);
    newXS("pdflibc::PDF_place_image", _wrap_PDF_place_image, file);

    newXS("pdflibc::PDF_get_parameter", _wrap_PDF_get_parameter, file);
    newXS("pdflibc::PDF_get_value", _wrap_PDF_get_value, file);
    newXS("pdflibc::PDF_set_parameter", _wrap_PDF_set_parameter, file);
    newXS("pdflibc::PDF_set_value", _wrap_PDF_set_value, file);

    newXS("pdflibc::PDF_begin_pattern", _wrap_PDF_begin_pattern, file);
    newXS("pdflibc::PDF_end_pattern", _wrap_PDF_end_pattern, file);

    newXS("pdflibc::PDF_close_pdi", _wrap_PDF_close_pdi, file);
    newXS("pdflibc::PDF_close_pdi_page", _wrap_PDF_close_pdi_page, file);
    newXS("pdflibc::PDF_fit_pdi_page", _wrap_PDF_fit_pdi_page, file);
    newXS("pdflibc::PDF_get_pdi_parameter", _wrap_PDF_get_pdi_parameter, file);
    newXS("pdflibc::PDF_get_pdi_value", _wrap_PDF_get_pdi_value, file);
    newXS("pdflibc::PDF_open_pdi", _wrap_PDF_open_pdi, file);
    newXS("pdflibc::PDF_open_pdi_page", _wrap_PDF_open_pdi_page, file);
    newXS("pdflibc::PDF_place_pdi_page", _wrap_PDF_place_pdi_page, file);
    newXS("pdflibc::PDF_process_pdi", _wrap_PDF_process_pdi, file);

    newXS("pdflibc::PDF_create_pvf", _wrap_PDF_create_pvf, file);
    newXS("pdflibc::PDF_delete_pvf", _wrap_PDF_delete_pvf, file);

    newXS("pdflibc::PDF_shading", _wrap_PDF_shading, file);
    newXS("pdflibc::PDF_shading_pattern", _wrap_PDF_shading_pattern, file);
    newXS("pdflibc::PDF_shfill", _wrap_PDF_shfill, file);

    newXS("pdflibc::PDF_begin_template", _wrap_PDF_begin_template, file);
    newXS("pdflibc::PDF_end_template", _wrap_PDF_end_template, file);

    newXS("pdflibc::PDF_continue_text", _wrap_PDF_continue_text, file);
    newXS("pdflibc::PDF_fit_textline", _wrap_PDF_fit_textline, file);
    newXS("pdflibc::PDF_set_text_pos", _wrap_PDF_set_text_pos, file);
    newXS("pdflibc::PDF_show", _wrap_PDF_show, file);
    newXS("pdflibc::PDF_show_boxed", _wrap_PDF_show_boxed, file);
    newXS("pdflibc::PDF_show_xy", _wrap_PDF_show_xy, file);
    newXS("pdflibc::PDF_stringwidth", _wrap_PDF_stringwidth, file);

    newXS("pdflibc::PDF_begin_font", _wrap_PDF_begin_font, file);
    newXS("pdflibc::PDF_begin_glyph", _wrap_PDF_begin_glyph, file);
    newXS("pdflibc::PDF_end_font", _wrap_PDF_end_font, file);
    newXS("pdflibc::PDF_end_glyph", _wrap_PDF_end_glyph, file);

    newXS("pdflibc::PDF_create_gstate", _wrap_PDF_create_gstate, file);
    newXS("pdflibc::PDF_set_gstate", _wrap_PDF_set_gstate, file);

/*
 * These are the pointer type-equivalency mappings.
 * (Used by the SWIG pointer type-checker).
 */
    SWIG_RegisterMapping("unsigned short","short",0);
    SWIG_RegisterMapping("PDF","struct PDF_s",0);
    SWIG_RegisterMapping("long","unsigned long",0);
    SWIG_RegisterMapping("long","signed long",0);
    SWIG_RegisterMapping("signed short","short",0);
    SWIG_RegisterMapping("signed int","int",0);
    SWIG_RegisterMapping("short","unsigned short",0);
    SWIG_RegisterMapping("short","signed short",0);
    SWIG_RegisterMapping("unsigned long","long",0);
    SWIG_RegisterMapping("int","unsigned int",0);
    SWIG_RegisterMapping("int","signed int",0);
    SWIG_RegisterMapping("unsigned int","int",0);
    SWIG_RegisterMapping("struct PDF_s","PDF",0);
    SWIG_RegisterMapping("signed long","long",0);
    ST(0) = &PL_sv_yes;
    XSRETURN(1);
}
