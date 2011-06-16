/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* CUPS data directory. */
/* Not used in Haiku */
#define CUPS_DATADIR "/boot/common/data/cups"

/* Not used in Haiku */
#define CUPS_MODELDIR "/boot/common/data/model/gutenprint/5.2/"

/* */
/* Not used in Haiku */
#define CUPS_PPD_NICKNAME_STRING " - CUPS+Gutenprint v"

/* CUPS PPD PostScript level */
/* Not used in Haiku */
#define CUPS_PPD_PS_LEVEL 2

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #define ENABLE_NLS 1 */

/* */
#define GUTENPRINT_RELEASE_VERSION "5.2"

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if GCC special attributes are supported */
#define HAVE_GCC_ATTRIBUTES 1

#if !defined(HAVE_GCC_ATTRIBUTES) && !defined(__attribute__)
/* This should really be a C99 anonymous variadic macro. */
#define __attribute__(attr)
#endif

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if GNU ld is present. */
/* #undef HAVE_GNU_LD */

/* Define if you have the iconv() function. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Is libreadline present? */
#define HAVE_LIBREADLINE 0

/* Define if libz is present. */
/* #undef HAVE_LIBZ */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <ltdl.h> header file. */
#define HAVE_LTDL_H 1

/* Define if maintainer-mode is to be used. */
/* #undef HAVE_MAINTAINER_MODE */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Define if libreadline header is present. */
/* #undef HAVE_READLINE_READLINE_H */

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Build a modular libgutenprint */
/* #undef MODULE */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* The operating system to build for */
#define OSTYPE "haiku"

/* Name of package */
#define PACKAGE "gutenprint"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "gimp-print-devel@lists.sourceforge.net"

/* */
#define PACKAGE_DATA_DIR "/system/data/gutenprint"

/* */
#define PACKAGE_LIB_DIR "/boot/common/lib/gutenprint"

/* */
#define PACKAGE_LOCALE_DIR "/NOT_USED/locale"

/* Define to the full name of this package. */
#define PACKAGE_NAME "gutenprint"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "gutenprint 5.2.7"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "gutenprint"

/* Define to the version of this package. */
#define PACKAGE_VERSION "5.2.7"

/* */
#define PKGMODULEDIR "/boot/common/lib/gutenprint/5.2/modules"

/* */
#define PKGXMLDATADIR "/system/data/gutenprint"

/* Package release date. */
#define RELEASE_DATE "01 May 2011"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Use libdl/dlopen as module loader */
/* #undef USE_DLOPEN */

/* Use GNU libltdl as module loader */
/* #undef USE_LTDL */

/* Version number of package */
#define VERSION "5.2.7"

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
