/* Some autoconf-unrelated preprocessor magic that needs to be done
   before including the system includes and therefore cannot belong in
   sysdep.h.  This file is included at the bottom of config.h.  */

/* Alloca-related defines, straight out of the Autoconf manual. */

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
void *alloca ();
#   endif
#  endif
# endif
#endif

#ifdef __sun
# ifdef __SVR4
#  define solaris
# endif
#endif

/* The "namespace tweaks" below attempt to set a friendly "compilation
   environment" under popular operating systems.  Default compilation
   environment often means that some functions that are "extensions"
   are not declared -- `strptime' is one example.

   But non-default environments can expose bugs in the system header
   files, crippling compilation in _very_ non-obvious ways.  Because
   of that, we define them only on well-tested architectures where we
   know they will work.  */

#undef NAMESPACE_TWEAKS

#ifdef solaris
# define NAMESPACE_TWEAKS
#endif

#ifdef __linux__
# define NAMESPACE_TWEAKS
#endif

#ifdef NAMESPACE_TWEAKS

/* Request the "Unix 98 compilation environment". */
#define _XOPEN_SOURCE 500

/* For Solaris: request everything else that is available and doesn't
   conflict with the above.  */
#define __EXTENSIONS__

/* For Linux: request features of 4.3BSD and SVID (System V Interface
   Definition). */
#define _SVID_SOURCE
#define _BSD_SOURCE

#endif /* NAMESPACE_TWEAKS */

/* Determine whether to use stdarg.  Use it only if the compiler
   supports ANSI C and stdarg.h is present.  We check for both because
   there are configurations where stdarg.h exists, but doesn't work.
   This check cannot be in sysdep.h because we use it to choose which
   system headers to include.  */
#ifndef WGET_USE_STDARG
# ifdef __STDC__
#  ifdef HAVE_STDARG_H
#   define WGET_USE_STDARG
#  endif
# endif
#endif /* not WGET_USE_STDARG */
