/* Some autoconf-unrelated preprocessor magic that needs to be done
   *before* including the system includes and therefore cannot belong
   in sysdep.h.

   Everything else related to system tweaking belongs to sysdep.h.

   This file is included at the bottom of config.h.  */

/* Testing for __sun is not enough because it's also defined on SunOS.  */
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

/* Under glibc-based systems we want all GNU extensions as well.  This
   declares some unnecessary cruft, but also useful functions such as
   timegm, FNM_CASEFOLD extension to fnmatch, memrchr, etc.  */
#define _GNU_SOURCE

#endif /* NAMESPACE_TWEAKS */


/* Alloca declaration, based on recommendation in the Autoconf manual.
   These have to be after the above namespace tweaks, but before any
   non-preprocessor code.  */

#if HAVE_ALLOCA_H
# include <alloca.h>
#elif defined WINDOWS
# include <malloc.h>
# ifndef alloca
#  define alloca _alloca
# endif
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif
