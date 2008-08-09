/* src/config.h.  Manually generated from src/config.h.in to
   accomodate the 32-bit Windows compilation environments.  */

/* Define if you want the debug output support compiled in. */
#define ENABLE_DEBUG 1

/* Define if you want the HTTP Digest Authorization compiled in. */
#define ENABLE_DIGEST 1

/* Define if IPv6 support is enabled. */
/* #undef ENABLE_IPV6 */

/* Define if you want the NTLM authorization support compiled in. */
#ifdef HAVE_OPENSSL
# define ENABLE_NTLM 1
#endif

/* Define if you want the Opie support for FTP compiled in. */
#define ENABLE_OPIE 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
/* #undef HAVE_ALLOCA_H */

/* Define when using built-in MD5. */
#define HAVE_BUILTIN_MD5

/* Define to 1 if you have the `drand48' function. */
/* #undef HAVE_DRAND48 */

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
/* #undef HAVE_FSEEKO */

/* Define to 1 if you have the `ftello' function. */
/* #undef HAVE_FTELLO */

/* Define to 1 if you have the `getaddrinfo' function. */
/* #undef HAVE_GETADDRINFO */

/* Define if support for the GnuTLS library is being compiled in. */
/* #undef HAVE_GNUTLS */

/* Define to 1 if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define to 1 if you have the `isatty' function. */
#define HAVE_ISATTY 1

/* Define to 1 if you have the <libintl.h> header file. */
/* #undef HAVE_LIBINTL_H */

/* Define if we're compiling support for MD5. */
/* #undef HAVE_MD5 */

/* Define to 1 if you have a working `mmap' system call. */
/* #undef HAVE_MMAP */

/* Define if you have the nanosleep function. */
/* #undef HAVE_NANOSLEEP */

/* Define this if you want the NLS support. */
/* #undef HAVE_NLS */

/* Define if support for the OpenSSL library is being compiled in. */
/* #undef HAVE_OPENSSL */

/* Define when using OpenSSL MD5. */
/* #undef HAVE_OPENSSL_MD5 */

/* Define to 1 if you have the `sigblock' function. */
/* #undef HAVE_SIGBLOCK */

/* Define to 1 if you have the `sigsetjmp' function. */
/* #undef HAVE_SIGSETJMP */

/* Define to 1 if the system has the type `sig_atomic_t'. */
#define HAVE_SIG_ATOMIC_T 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define if struct sockaddr_in6 has the sin6_scope_id member */
#define HAVE_SOCKADDR_IN6_SCOPE_ID 1

/* Define when using Solaris MD5. */
/* #undef HAVE_SOLARIS_MD5 */

/* Define to 1 if stdbool.h conforms to C99. */
/* #undef HAVE_STDBOOL_H */

/* Define to 1 if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define to 1 if you have the `strcasecmp' function. */
/* #undef HAVE_STRCASECMP */

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strncasecmp' function. */
/* #undef HAVE_STRNCASECMP */

/* Define to 1 if you have the `strptime' function. */
/* #undef HAVE_STRPTIME */

/* Define to 1 if the system has the type `struct sockaddr_in6'. */
/* #undef HAVE_STRUCT_SOCKADDR_IN6 */

/* Define to 1 if the system has the type `struct sockaddr_storage'. */
/* #undef HAVE_STRUCT_SOCKADDR_STORAGE */

/* Define to 1 if the system has the type `struct utimbuf'. */
#define HAVE_STRUCT_UTIMBUF 1

/* Define to 1 if you have the `symlink' function. */
/* #undef HAVE_SYMLINK */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/select.h> header file. */
/* #undef HAVE_SYS_SELECT_H */

/* Define to 1 if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define to 1 if you have the `timegm' function. */
/* #undef HAVE_TIMEGM */

/* Define to 1 if the system has the type `uint32_t'. */
/* #undef HAVE_UINT32_T */

/* Define to 1 if you have the <unistd.h> header file. */
/* #undef HAVE_UNISTD_H */

/* Define to 1 if you have the `usleep' function. */
/* #undef HAVE_USLEEP */

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if you have the <sys/utime.h> header file. */
#define HAVE_SYS_UTIME_H 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define if fnmatch.h can be included. */
/* #undef HAVE_WORKING_FNMATCH_H */

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* The size of a `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of a `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of a `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 0

/* The size of a `off_t', as computed by sizeof. */
#define SIZEOF_OFF_T 4

/* The size of a `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Define to 1 if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* # undef _ALL_SOURCE */
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
# define inline __inline
#endif

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to int or size_t on systems without socklen_t. */
#define socklen_t int

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */

/* Include compiler-specific defines. */
#include "config-compiler.h"

#include "config-post.h"

