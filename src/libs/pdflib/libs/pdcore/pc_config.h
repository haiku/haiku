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

/* $Id: pc_config.h 29803 2009-03-30 14:48:57Z bonefish $
 *
 * PDFlib portability and configuration definitions
 *
 */

#ifndef PC_CONFIG_H
#define PC_CONFIG_H

/* ------------------------ feature configuration  ------------------- */

/* zlib compression support */
#define HAVE_LIBZ

/* ---------------------------- platform definitions ------------------------ */

/* #undef this if your platform doesn't support environment variables */
#define HAVE_ENVVARS

/* Compilers which are not strictly ANSI conforming can set PDF_VOLATILE
 * to an empty value.
 */
#ifndef PDF_VOLATILE
#define PDF_VOLATILE	volatile
#endif

/* byte order */
#undef PDC_ISBIGENDIAN
#ifdef WORDS_BIGENDIAN
#define PDC_ISBIGENDIAN 1
#else
#define PDC_ISBIGENDIAN 0
#endif

/* ---------------------------------- WIN32  -------------------------------- */

/* try to identify Windows compilers */

#if (defined _WIN32 || defined __WATCOMC__ || defined __BORLANDC__ ||	\
	(defined(__MWERKS__) && defined(__INTEL__))) && !defined WIN32
#define	WIN32
#endif	/* <Windows compiler>  && !defined WIN32 */

#ifdef	WIN32
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"

#undef PDC_PATHSEP
#define PDC_PATHSEP     "\\"

#if defined(_WIN32_WCE) && (_WIN32_WCE >= 300)
#define PDF_PLATFORM    "Windows CE"
#define WINCE
#undef HAVE_SETLOCALE
#undef HAVE_ENVVARS
#else
#if defined(WIN64)
#define PDF_PLATFORM    "Win64"
#else
#define PDF_PLATFORM    "Win32"
#endif
#endif

#endif	/* WIN32 */

/* --------------------------------- Cygnus  -------------------------------- */

#ifdef __CYGWIN__
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"
#ifdef DLL_EXPORT
    #define PDFLIB_EXPORTS
#endif

#endif /* __CYGWIN__ */

/* ---------------------------------- DJGPP  -------------------------------- */

#ifdef __DJGPP__
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"
#define PDF_PLATFORM	"Win32/DJGPP"
#endif /* __DJGPP__ */

/* ----------------------------------- OS/2  -------------------------------- */

/*
 * Try to identify OS/2 compilers.
 */

#if (defined __OS2__ || defined __EMX__) && !defined OS2
#define OS2
#endif

#ifdef	OS2
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"
#define PDF_PLATFORM	"OS/2"
#endif	/* OS2 */

/* --------------------------------- Mac OS X ------------------------------- */

/* try to identify the Mac OS X command line compiler */

#if defined(__ppc__) && defined(__APPLE__)

#define MACOSX

/*
 * By default we always build a carbonized version of the library,
 * but allow non-Carbon builds to be triggered by setting the
 * PDF_TARGET_API_MAC_OS8 symbol externally.
 */

#ifndef PDF_TARGET_API_MAC_OS8
#define PDF_TARGET_API_MAC_CARBON
#endif

#if defined(PDF_TARGET_API_MAC_CARBON) && !defined(TARGET_API_MAC_CARBON)
#define TARGET_API_MAC_CARBON 1
#endif

/* Mac OS X 10.2 (Jaguar) defines this, but we use it for Mac OS 9 below */
#undef MAC

#endif /* Mac OS X */

/* --------------------------------- Mac OS 9 ------------------------------- */

/* try to identify Mac OS 9 compilers */

#if (defined macintosh || defined __POWERPC__ || defined __CFM68K__) && \
	!defined MAC && !defined MACOSX && !defined(__BEOS__) && !defined(__HAIKU__)
#define MAC
#endif

#ifdef	MAC
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"
#define PDC_PATHSEP     ":"

#undef HAVE_ENVVARS

#define PDF_PLATFORM	"Mac OS"
#endif	/* MAC */

/* ----------------------------------- BeOS --------------------------------- */

#if defined(__BEOS__) || defined(__HAIKU__)
#define PDF_PLATFORM	"BeOS"

#ifdef __POWERPC__
#define WORDS_BIGENDIAN
#undef PDC_ISBIGENDIAN
#define PDC_ISBIGENDIAN 1
#endif	/* __POWERPC__ */

#endif /* __BEOS__ || __HAIKU__ */

/* --------------------------------- AS/400 --------------------------------- */

/* try to identify the AS/400 compiler */

#if	defined __ILEC400__ && !defined AS400
#define	AS400
#endif

#ifdef AS400

#pragma comment(copyright, \
	"(C) PDFlib GmbH, Muenchen, Germany (www.pdflib.com)")

#define READTMODE       "rb"
#define READBMODE       "rb"
#define WRITEMODE	"wb"
#define APPENDMODE	"ab"

#define PDF_PLATFORM	"iSeries"

#define WORDS_BIGENDIAN
#undef PDC_ISBIGENDIAN
#define PDC_ISBIGENDIAN 1

#endif	/* AS400 */

/* --------------------- S/390 with Unix System Services -------------------- */

#ifdef	OS390

#define WRITEMODE	"wb"
#define APPENDMODE	"ab"

#endif	/* OS390 */

/* -------------------------------- S/390 with MVS -------------------------- */

/* try to identify MVS (__MVS__ is #defined on USS and MVS!)
 * I370 is used by SAS C
 */

#if !defined(OS390) && (defined __MVS__ || defined I370) && !defined MVS
#define	MVS
#endif

#ifdef	MVS

#if defined(MVS) && defined(I370)
#define READBMODE       "rb"
#define PDC_FILEQUOT    ""
#else
#define READBMODE       "rb,byteseek"
#define PDC_FILEQUOT    "'"
#endif
#define WRITEMODE       "wb"
#define APPENDMODE	"ab,recfm=v"

#undef PDC_PATHSEP
#define PDC_PATHSEP     "("

#undef PDC_PATHTERM
#define PDC_PATHTERM    ")"

#define PDF_PLATFORM	"zSeries MVS"
#define PDF_OS390_MVS_RESOURCE

#define WORDS_BIGENDIAN
#undef PDC_ISBIGENDIAN
#define PDC_ISBIGENDIAN 1

#endif	/* MVS */

/* ------------------------------------ VMS --------------------------------- */

/* No special handling required */

#ifdef	VMS
#define PDF_PLATFORM	"VMS"
#endif	/* VMS */

/* --------------------------------- Defaults ------------------------------- */

#ifndef READTMODE
#define READTMODE       "r"
#endif  /* !READTMODE */

#ifndef READBMODE
#define READBMODE       "rb"
#endif  /* !READBMODE */

#ifndef WRITEMODE
#define WRITEMODE	"w"
#endif	/* !WRITEMODE */

#ifndef APPENDMODE
#define APPENDMODE	"a"
#endif	/* !APPENDMODE */

#ifndef PDC_PATHSEP
#define PDC_PATHSEP     "/"
#endif  /* !PDC_PATHSEP */

#ifdef	_DEBUG
#define DEBUG
#endif	/* _DEBUG */

#define PDC_FLOAT_MAX   ((double) 1e+37)
#define PDC_FLOAT_MIN   ((double) -1e+37)
#define PDC_FLOAT_PREC  ((double) 1e-6)

#define PDC_OFFSET(type, field) ((unsigned int) &(((type *)NULL)->field))

#endif	/* PC_CONFIG_H */
