/* lib/config.h.  Generated from config.hin by configure.  */
/* lib/config.hin.  Generated from configure.ac by autoheader.  */

/* Define this to an absolute name of <fcntl.h>. */
#define ABSOLUTE_FCNTL_H "///boot/home/svnhaiku/trunk/headers/posix/fcntl.h"

/* Define this to an absolute name of <inttypes.h>. */
#define ABSOLUTE_INTTYPES_H "///boot/home/svnhaiku/trunk/headers/posix/inttypes.h"

/* Define this to an absolute name of <stdint.h>. */
#define ABSOLUTE_STDINT_H "///boot/home/svnhaiku/trunk/headers/posix/stdint.h"

/* Define this to an absolute name of <sys/stat.h>. */
#define ABSOLUTE_SYS_STAT_H "///boot/home/svnhaiku/trunk/headers/posix/sys/stat.h"

/* Define to the function xargmatch calls on failures. */
#define ARGMATCH_DIE usage (1)

/* Define to the declaration of the xargmatch failure function. */
#define ARGMATCH_DIE_DECL extern void usage ()

/* Define to the number of bits in type 'ptrdiff_t'. */
#define BITSIZEOF_PTRDIFF_T 32

/* Define to the number of bits in type 'sig_atomic_t'. */
#define BITSIZEOF_SIG_ATOMIC_T 32

/* Define to the number of bits in type 'size_t'. */
#define BITSIZEOF_SIZE_T 32

/* Define to the number of bits in type 'wchar_t'. */
#define BITSIZEOF_WCHAR_T 16

/* Define to the number of bits in type 'wint_t'. */
#define BITSIZEOF_WINT_T 32

/* Define if chown is not POSIX compliant regarding IDs of -1. */
#define CHOWN_FAILS_TO_HONOR_ID_OF_NEGATIVE_ONE 1

/* Define if chown modifies symlinks. */
/* #undef CHOWN_MODIFIES_SYMLINK */

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to 1 if using `getloadavg.c'. */
#define C_GETLOADAVG 1

/* Define the default level of POSIX conformance. The value is of the form
   YYYYMM, specifying the year and month the standard was adopted. If not
   defined here, it defaults to the value of _POSIX2_VERSION in <unistd.h>.
   Define to 199209 to default to POSIX 1003.2-1992, which makes standard
   programs like `head', `tail', and `sort' accept obsolete options like `+10'
   and `-10'. Define to 200112 to default to POSIX 1003.1-2001, which makes
   these standard programs treat leading-`+' operands as file names and
   require modern usages like `-n 10' instead of `-10'. Whether defined here
   or not, the default can be overridden at run time via the _POSIX2_VERSION
   environment variable. */
/* #undef DEFAULT_POSIX2_VERSION */

/* Define to 1 for DGUX with <sys/dg_sys_info.h>. */
/* #undef DGUX */

/* the name of the file descriptor member of DIR */
/* #undef DIR_FD_MEMBER_NAME */

#ifdef DIR_FD_MEMBER_NAME
# define DIR_TO_FD(Dir_p) ((Dir_p)->DIR_FD_MEMBER_NAME)
#else
# define DIR_TO_FD(Dir_p) -1
#endif


/* Define to 1 if // is a file system root distinct from /. */
#define DOUBLE_SLASH_IS_DISTINCT_ROOT 0

/* Define if there is a member named d_ino in the struct describing directory
   headers. */
#define D_INO_IN_DIRENT 1

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
/* #undef ENABLE_NLS */

/* Define as good substitute value for EOVERFLOW. */
/* #undef EOVERFLOW */

/* Define on systems for which file names may have a so-called `drive letter'
   prefix, define this to compute the length of that prefix, including the
   colon. */
#define FILE_SYSTEM_ACCEPTS_DRIVE_LETTER_PREFIX 0

/* Define if the backslash character may also serve as a file name component
   separator. */
#define FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR 0

/* Define if a drive letter prefix denotes a relative path if it is not
   followed by a file name component separator. */
#define FILE_SYSTEM_DRIVE_PREFIX_CAN_BE_RELATIVE 0

/* Define to nothing if C supports flexible array members, and to 1 if it does
   not. That way, with a declaration like `struct s { int n; double
   d[FLEXIBLE_ARRAY_MEMBER]; };', the struct hack can be used with pre-C99
   compilers. When computing the size of such an object, don't use 'sizeof
   (struct s)' as it overestimates the size. Use 'offsetof (struct s, d)'
   instead. Don't use 'offsetof (struct s, d[0])', as this doesn't work with
   MSVC and with C++ compilers. */
#define FLEXIBLE_ARRAY_MEMBER 1

/* Define to the type of elements in the array set by `getgroups'. Usually
   this is either `int' or `gid_t'. */
#define GETGROUPS_T gid_t

/* Define to 1 if the `getloadavg' function needs to be run setuid or setgid.
   */
/* #undef GETLOADAVG_PRIVILEGED */

/* Define to 1 when using the gnulib close-stream module. */
#define GNULIB_CLOSE_STREAM 1

/* Define to 1 when using the gnulib fcntl-safer module. */
#define GNULIB_FCNTL_SAFER 1

/* Define to 1 when using the gnulib fopen-safer module. */
#define GNULIB_FOPEN_SAFER 1

/* The concatenation of the strings `GNU ', and PACKAGE. */
#define GNU_PACKAGE "GNU coreutils"

/* Define if your system defines TIOCGWINSZ in sys/ioctl.h. */
/* #undef GWINSZ_IN_SYS_IOCTL */

/* Define if your system defines TIOCGWINSZ in sys/pty.h. */
/* #undef GWINSZ_IN_SYS_PTY */

/* Define to 1 if you have the `acl' function. */
/* #undef HAVE_ACL */

/* Define to 1 if you have the `acl_delete_def_file' function. */
/* #undef HAVE_ACL_DELETE_DEF_FILE */

/* Define to 1 if you have the `acl_entries' function. */
/* #undef HAVE_ACL_ENTRIES */

/* Define to 1 if you have the `acl_extended_file' function. */
/* #undef HAVE_ACL_EXTENDED_FILE */

/* Define to 1 if you have the `acl_free' function. */
/* #undef HAVE_ACL_FREE */

/* Define to 1 if you have the `acl_from_mode' function. */
/* #undef HAVE_ACL_FROM_MODE */

/* Define to 1 if you have the `acl_from_text' function. */
/* #undef HAVE_ACL_FROM_TEXT */

/* Define to 1 if you have the `acl_get_fd' function. */
/* #undef HAVE_ACL_GET_FD */

/* Define to 1 if you have the `acl_get_file' function. */
/* #undef HAVE_ACL_GET_FILE */

/* Define to 1 if you have the <acl/libacl.h> header file. */
/* #undef HAVE_ACL_LIBACL_H */

/* Define to 1 if you have the `acl_set_fd' function. */
/* #undef HAVE_ACL_SET_FD */

/* Define to 1 if you have the `acl_set_file' function. */
/* #undef HAVE_ACL_SET_FILE */

/* Define to 1 if you have the `acl_to_text' function. */
/* #undef HAVE_ACL_TO_TEXT */

/* Define to 1 if you have the `alarm' function. */
#define HAVE_ALARM 1

/* Define to 1 if you have 'alloca' after including <alloca.h>, a header that
   may be supplied by this distribution. */
#define HAVE_ALLOCA 1

/* Define HAVE_ALLOCA_H for backward compatibility with older code that
   includes <alloca.h> only if HAVE_ALLOCA_H is defined. */
#define HAVE_ALLOCA_H 1

/* Define if you have an arithmetic hrtime_t type. */
/* #undef HAVE_ARITHMETIC_HRTIME_T */

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT 1

/* Define to 1 if you have the <bp-sym.h> header file. */
/* #undef HAVE_BP_SYM_H */

/* Define to 1 if you have the `btowc' function. */
#define HAVE_BTOWC 1

/* Define to 1 if nanosleep mishandle large arguments. */
/* #undef HAVE_BUG_BIG_NANOSLEEP */

/* Define to 1 if strtold conforms to C99. */
/* #undef HAVE_C99_STRTOLD */

/* Define to 1 if your system has a GNU libc compatible `calloc' function, and
   to 0 otherwise. */
#define HAVE_CALLOC 0

/* Define to 1 if you have the `canonicalize_file_name' function. */
/* #undef HAVE_CANONICALIZE_FILE_NAME */

/* Define to 1 if you have the MacOS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the MacOS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define to 1 if your system has a working `chown' function. */
/* #undef HAVE_CHOWN */

/* Define to 1 if you have the `chroot' function. */
/* #undef HAVE_CHROOT */

/* Define to 1 if you have the `chsize' function. */
/* #undef HAVE_CHSIZE */

/* Define to 1 if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define to 1 if you have the `clock_settime' function. */
/* #undef HAVE_CLOCK_SETTIME */

/* Define if you have compound literals. */
#define HAVE_COMPOUND_LITERALS 1

/* FIXME */
#define HAVE_C_LINE 1

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
/* #undef HAVE_DCGETTEXT */

/* Define to 1 if you have the declaration of `canonicalize_file_name', and to
   0 if you don't. */
#define HAVE_DECL_CANONICALIZE_FILE_NAME 0

/* Define to 1 if you have the declaration of `clearerr_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_CLEARERR_UNLOCKED 1

/* Define to 1 if you have the declaration of `dirfd', and to 0 if you don't.
   */
#define HAVE_DECL_DIRFD 0

/* Define to 1 if you have the declaration of `euidaccess', and to 0 if you
   don't. */
#define HAVE_DECL_EUIDACCESS 0

/* Define to 1 if you have the declaration of `feof_unlocked', and to 0 if you
   don't. */
#define HAVE_DECL_FEOF_UNLOCKED 1

/* Define to 1 if you have the declaration of `ferror_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FERROR_UNLOCKED 1

/* Define to 1 if you have the declaration of `fflush_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FFLUSH_UNLOCKED 1

/* Define to 1 if you have the declaration of `fgets_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FGETS_UNLOCKED 0

/* Define to 1 if you have the declaration of `flockfile', and to 0 if you
   don't. */
#define HAVE_DECL_FLOCKFILE 1

/* Define to 1 if you have the declaration of `fputc_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FPUTC_UNLOCKED 1

/* Define to 1 if you have the declaration of `fputs_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FPUTS_UNLOCKED 0

/* Define to 1 if you have the declaration of `fread_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FREAD_UNLOCKED 0

/* Define to 1 if you have the declaration of `free', and to 0 if you don't.
   */
#define HAVE_DECL_FREE 1

/* Define to 1 if you have the declaration of `freeaddrinfo', and to 0 if you
   don't. */
#define HAVE_DECL_FREEADDRINFO 1

/* Define to 1 if you have the declaration of `funlockfile', and to 0 if you
   don't. */
#define HAVE_DECL_FUNLOCKFILE 1

/* Define to 1 if you have the declaration of `fwrite_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_FWRITE_UNLOCKED 0

/* Define to 1 if you have the declaration of `gai_strerror', and to 0 if you
   don't. */
#define HAVE_DECL_GAI_STRERROR 1

/* Define to 1 if you have the declaration of `getaddrinfo', and to 0 if you
   don't. */
#define HAVE_DECL_GETADDRINFO 1

/* Define to 1 if you have the declaration of `getchar_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_GETCHAR_UNLOCKED 1

/* Define to 1 if you have the declaration of `getcwd', and to 0 if you don't.
   */
#define HAVE_DECL_GETCWD 1

/* Define to 1 if you have the declaration of `getc_unlocked', and to 0 if you
   don't. */
#define HAVE_DECL_GETC_UNLOCKED 1

/* Define to 1 if you have the declaration of `getdelim', and to 0 if you
   don't. */
#define HAVE_DECL_GETDELIM 0

/* Define to 1 if you have the declaration of `getenv', and to 0 if you don't.
   */
#define HAVE_DECL_GETENV 1

/* Define to 1 if you have the declaration of `geteuid', and to 0 if you
   don't. */
#define HAVE_DECL_GETEUID 1

/* Define to 1 if you have the declaration of `getgrgid', and to 0 if you
   don't. */
#define HAVE_DECL_GETGRGID 1

/* Define to 1 if you have the declaration of `gethrtime', and to 0 if you
   don't. */
#define HAVE_DECL_GETHRTIME 0

/* Define to 1 if you have the declaration of `getline', and to 0 if you
   don't. */
#define HAVE_DECL_GETLINE 0

/* Define to 1 if you have the declaration of `getlogin', and to 0 if you
   don't. */
#define HAVE_DECL_GETLOGIN 1

/* Define to 1 if you have the declaration of `getnameinfo', and to 0 if you
   don't. */
#define HAVE_DECL_GETNAMEINFO 1

/* Define to 1 if you have the declaration of `getpass', and to 0 if you
   don't. */
#define HAVE_DECL_GETPASS 0

/* Define to 1 if you have the declaration of `getpwuid', and to 0 if you
   don't. */
#define HAVE_DECL_GETPWUID 1

/* Define to 1 if you have the declaration of `getuid', and to 0 if you don't.
   */
#define HAVE_DECL_GETUID 1

/* Define to 1 if you have the declaration of `getutent', and to 0 if you
   don't. */
/* #undef HAVE_DECL_GETUTENT */

/* Define to 1 if you have the declaration of `imaxabs', and to 0 if you
   don't. */
#define HAVE_DECL_IMAXABS 1

/* Define to 1 if you have the declaration of `imaxdiv', and to 0 if you
   don't. */
#define HAVE_DECL_IMAXDIV 1

/* Define to 1 if you have the declaration of `inet_ntop', and to 0 if you
   don't. */
#define HAVE_DECL_INET_NTOP 1

/* Define to 1 if you have the declaration of `isblank', and to 0 if you
   don't. */
#define HAVE_DECL_ISBLANK 1

/* Define to 1 if you have the declaration of `lchown', and to 0 if you don't.
   */
#define HAVE_DECL_LCHOWN 1

/* Define to 1 if you have the declaration of `lseek', and to 0 if you don't.
   */
#define HAVE_DECL_LSEEK 1

/* Define to 1 if you have the declaration of `malloc', and to 0 if you don't.
   */
#define HAVE_DECL_MALLOC 1

/* Define to 1 if you have a declaration of mbswidth() in <wchar.h>, and to 0
   otherwise. */
#define HAVE_DECL_MBSWIDTH_IN_WCHAR_H 0

/* Define to 1 if you have the declaration of `memchr', and to 0 if you don't.
   */
#define HAVE_DECL_MEMCHR 1

/* Define to 1 if you have the declaration of `memrchr', and to 0 if you
   don't. */
#define HAVE_DECL_MEMRCHR 0

/* Define to 1 if you have the declaration of `mkdir', and to 0 if you don't.
   */
#define HAVE_DECL_MKDIR 1

/* Define to 1 if you have the declaration of `nanosleep', and to 0 if you
   don't. */
#define HAVE_DECL_NANOSLEEP 0

/* Define to 1 if you have the declaration of `putchar_unlocked', and to 0 if
   you don't. */
#define HAVE_DECL_PUTCHAR_UNLOCKED 1

/* Define to 1 if you have the declaration of `putc_unlocked', and to 0 if you
   don't. */
#define HAVE_DECL_PUTC_UNLOCKED 1

/* Define to 1 if you have the declaration of `realloc', and to 0 if you
   don't. */
#define HAVE_DECL_REALLOC 1

/* Define to 1 if you have the declaration of `setregid', and to 0 if you
   don't. */
#define HAVE_DECL_SETREGID 0

/* Define to 1 if you have the declaration of `snprintf', and to 0 if you
   don't. */
#define HAVE_DECL_SNPRINTF 1

/* Define to 1 if you have the declaration of `strdup', and to 0 if you don't.
   */
#define HAVE_DECL_STRDUP 1

/* Define to 1 if you have the declaration of `strerror_r', and to 0 if you
   don't. */
#define HAVE_DECL_STRERROR_R 1

/* Define to 1 if you have the declaration of `strmode', and to 0 if you
   don't. */
#define HAVE_DECL_STRMODE 0

/* Define to 1 if you have the declaration of `strncasecmp', and to 0 if you
   don't. */
#define HAVE_DECL_STRNCASECMP 1

/* Define to 1 if you have the declaration of `strndup', and to 0 if you
   don't. */
#define HAVE_DECL_STRNDUP 0

/* Define to 1 if you have the declaration of `strnlen', and to 0 if you
   don't. */
#define HAVE_DECL_STRNLEN 1

/* Define to 1 if you have the declaration of `strsignal', and to 0 if you
   don't. */
#define HAVE_DECL_STRSIGNAL 1

/* Define to 1 if you have the declaration of `strtoimax', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOIMAX 1

/* Define to 1 if you have the declaration of `strtoll', and to 0 if you
   don't. */
/* #undef HAVE_DECL_STRTOLL */

/* Define to 1 if you have the declaration of `strtoull', and to 0 if you
   don't. */
/* #undef HAVE_DECL_STRTOULL */

/* Define to 1 if you have the declaration of `strtoumax', and to 0 if you
   don't. */
#define HAVE_DECL_STRTOUMAX 1

/* Define to 1 if you have the declaration of `sys_siglist', and to 0 if you
   don't. */
#define HAVE_DECL_SYS_SIGLIST 1

/* Define to 1 if you have the declaration of `ttyname', and to 0 if you
   don't. */
#define HAVE_DECL_TTYNAME 1

/* Define to 1 if you have the declaration of `tzname', and to 0 if you don't.
   */
/* #undef HAVE_DECL_TZNAME */

/* Define to 1 if you have the declaration of `wcwidth', and to 0 if you
   don't. */
#define HAVE_DECL_WCWIDTH 1

/* Define to 1 if you have the declaration of `_sys_siglist', and to 0 if you
   don't. */
#define HAVE_DECL__SYS_SIGLIST 0

/* Define to 1 if you have the declaration of `__fpending', and to 0 if you
   don't. */
#define HAVE_DECL___FPENDING 0

/* Define to 1 if you have the declaration of `__fsetlocking', and to 0 if you
   don't. */
#define HAVE_DECL___FSETLOCKING 0

/* Define to 1 if you have the declaration of `__sys_siglist', and to 0 if you
   don't. */
#define HAVE_DECL___SYS_SIGLIST 0

/* Define to 1 if you have the `directio' function. */
/* #undef HAVE_DIRECTIO */

/* Define to 1 if you have the `dirfd' function. */
#define HAVE_DIRFD 1

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the <dustat.h> header file. */
/* #undef HAVE_DUSTAT_H */

/* Define to 1 if you have the `eaccess' function. */
/* #undef HAVE_EACCESS */

/* Define to 1 if you have the `endgrent' function. */
#define HAVE_ENDGRENT 1

/* Define to 1 if you have the `endpwent' function. */
#define HAVE_ENDPWENT 1

/* Define if you have the declaration of environ. */
/* #undef HAVE_ENVIRON_DECL */

/* Define to 1 if you have the `euidaccess' function. */
/* #undef HAVE_EUIDACCESS */

/* Define to 1 if you have the `fchdir' function. */
#define HAVE_FCHDIR 1

/* Define to 1 if you have the `fchmod' function. */
#define HAVE_FCHMOD 1

/* Define to 1 if you have the `fchown' function. */
#define HAVE_FCHOWN 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdatasync' function. */
/* #undef HAVE_FDATASYNC */

/* Define to 1 if you have the `fdopendir' function. */
/* #undef HAVE_FDOPENDIR */

/* Define to 1 if pipes are FIFOs, 0 if sockets. Leave undefined if not known.
   */
#define HAVE_FIFO_PIPES 1

/* Define to 1 if you have the `flockfile' function. */
/* #undef HAVE_FLOCKFILE */

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* Define to 1 if you have the <fs_info.h> header file. */
#define HAVE_FS_INFO_H 1

/* Define to 1 if you have the `fs_stat_dev' function. */
#define HAVE_FS_STAT_DEV 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if you have the `funlockfile' function. */
/* #undef HAVE_FUNLOCKFILE */

/* Define to 1 if you have the `futimes' function. */
/* #undef HAVE_FUTIMES */

/* Define to 1 if you have the `futimesat' function. */
/* #undef HAVE_FUTIMESAT */

/* Define to 1 if you have the `gai_strerror' function. */
/* #undef HAVE_GAI_STRERROR */

/* Define to 1 if you have the `getaddrinfo' function. */
/* #undef HAVE_GETADDRINFO */

/* Define to 1 if you have the `getdelim' function. */
#define HAVE_GETDELIM 1

/* Define to 1 if your system has a working `getgroups' function. */
#define HAVE_GETGROUPS 1

/* Define to 1 if you have the `gethostbyname' function. */
/* #undef HAVE_GETHOSTBYNAME */

/* Define to 1 if you have the `gethostid' function. */
/* #undef HAVE_GETHOSTID */

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getloadavg' function. */
/* #undef HAVE_GETLOADAVG */

/* Define to 1 if you have the `getmntent' function. */
/* #undef HAVE_GETMNTENT */

/* Define to 1 if you have the `getmntinfo' function. */
/* #undef HAVE_GETMNTINFO */

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long_only' function. */
#define HAVE_GETOPT_LONG_ONLY 1

/* Define to 1 if you have the `getpagesize' function. */
/* #undef HAVE_GETPAGESIZE */

/* Define to 1 if you have the `getspnam' function. */
/* #undef HAVE_GETSPNAM */

/* Define to 1 if you have the `getsysinfo' function. */
/* #undef HAVE_GETSYSINFO */

/* Define if the GNU gettext() function is already present or preinstalled. */
/* #undef HAVE_GETTEXT */

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define to 1 if you have the `getusershell' function. */
/* #undef HAVE_GETUSERSHELL */

/* Define to 1 if you have the <grp.h> header file. */
#define HAVE_GRP_H 1

/* Define to 1 if you have the `hasmntopt' function. */
/* #undef HAVE_HASMNTOPT */

/* Define to 1 if you have the <hurd.h> header file. */
/* #undef HAVE_HURD_H */

/* Define if you have the iconv() function. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the `inet_ntop' function. */
/* #undef HAVE_INET_NTOP */

/* Define to 1 if you have the `initgroups' function. */
/* #undef HAVE_INITGROUPS */

/* Define to 1 if the compiler supports one of the keywords 'inline',
   '__inline__', '__inline' and effectively inlines functions marked as such.
   */
#define HAVE_INLINE 1

/* Define if you have the 'intmax_t' type in <stdint.h> or <inttypes.h>. */
#define HAVE_INTMAX_T 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if <inttypes.h> exists, doesn't clash with <sys/types.h>, and
   declares uintmax_t. */
#define HAVE_INTTYPES_H_WITH_UINTMAX 1

/* Define to 1 if you have the <io.h> header file. */
/* #undef HAVE_IO_H */

/* Define to 1 if <sys/socket.h> defines AF_INET. */
#define HAVE_IPV4 1

/* Define to 1 if <sys/socket.h> defines AF_INET6. */
/* #undef HAVE_IPV6 */

/* Define to 1 if you have the `isapipe' function. */
/* #undef HAVE_ISAPIPE */

/* Define to 1 if you have the `isascii' function. */
#define HAVE_ISASCII 1

/* Define to 1 if you have the `iswcntrl' function. */
#define HAVE_ISWCNTRL 1

/* Define to 1 if you have the `iswctype' function. */
#define HAVE_ISWCTYPE 1

/* Define to 1 if you have the `iswprint' function. */
#define HAVE_ISWPRINT 1

/* Define to 1 if you have the `iswspace' function. */
#define HAVE_ISWSPACE 1

/* Define if you have <langinfo.h> and nl_langinfo(CODESET). */
/* #undef HAVE_LANGINFO_CODESET */

/* Define to 1 if you have the `lchmod' function. */
/* #undef HAVE_LCHMOD */

/* Define to 1 if you have the `lchown' function. */
/* #undef HAVE_LCHOWN */

/* Define to 1 if you have the `dgc' library (-ldgc). */
/* #undef HAVE_LIBDGC */

/* Define to 1 if you have the <libgen.h> header file. */
/* #undef HAVE_LIBGEN_H */

/* Define to 1 if you have the `kstat' library (-lkstat). */
/* #undef HAVE_LIBKSTAT */

/* Define to 1 if you have the `ldgc' library (-lldgc). */
/* #undef HAVE_LIBLDGC */

/* Define to 1 if you have the `os' library (-los). */
/* #undef HAVE_LIBOS */

/* Define to 1 if you have the `ypsec' library (-lypsec). */
/* #undef HAVE_LIBYPSEC */

/* Define to 1 if you have the `listmntent' function. */
/* #undef HAVE_LISTMNTENT */

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if the type `long double' works and has more range or precision
   than `double'. */
#define HAVE_LONG_DOUBLE 1

/* Define to 1 if the type `long double' works and has more range or precision
   than `double'. */
/* #undef HAVE_LONG_DOUBLE_WIDER */

/* Define to 1 if you support file names longer than 14 characters. */
#define HAVE_LONG_FILE_NAMES 1

/* Define if you have the 'long long' type. */
#define HAVE_LONG_LONG 1

/* Define to 1 if the system has the type `long long int'. */
#define HAVE_LONG_LONG_INT 1

/* Define to 1 if you have the `lstat' function. */
#define HAVE_LSTAT 1

/* Define to 1 if `lstat' has the bug that it succeeds when given the
   zero-length file name argument. */
/* #undef HAVE_LSTAT_EMPTY_STRING_BUG */

/* Define to 1 if you have the <machine/hal_sysinfo.h> header file. */
/* #undef HAVE_MACHINE_HAL_SYSINFO_H */

/* Define to 1 if you have the <mach/mach.h> header file. */
/* #undef HAVE_MACH_MACH_H */

/* Define to 1 if your system has a GNU libc compatible `malloc' function, and
   to 0 otherwise. */
#define HAVE_MALLOC 1

/* Define to 1 if you have the `mblen' function. */
#define HAVE_MBLEN 1

/* Define to 1 if you have the `mbrlen' function. */
#define HAVE_MBRLEN 1

/* Define to 1 if you have the `mbrtowc' function. */
#define HAVE_MBRTOWC 1

/* Define to 1 if you have the `mbsinit' function. */
#define HAVE_MBSINIT 1

/* Define to 1 if you have the `mbsrtowcs' function. */
#define HAVE_MBSRTOWCS 1

/* Define to 1 if <wchar.h> declares mbstate_t. */
#define HAVE_MBSTATE_T 1

/* Define to 1 if you have the `memchr' function. */
#define HAVE_MEMCHR 1

/* Define to 1 if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mempcpy' function. */
/* #undef HAVE_MEMPCPY */

/* Define to 1 if you have the `memrchr' function. */
/* #undef HAVE_MEMRCHR */

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `microuptime' function. */
/* #undef HAVE_MICROUPTIME */

/* Define to 1 if you have the `mkfifo' function. */
#define HAVE_MKFIFO 1

/* Define to 1 if you have the <mntent.h> header file. */
/* #undef HAVE_MNTENT_H */

/* Define to 1 if you have the `nanotime' function. */
/* #undef HAVE_NANOTIME */

/* Define to 1 if you have the `nanouptime' function. */
/* #undef HAVE_NANOUPTIME */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Define to 1 if you have the `next_dev' function. */
#define HAVE_NEXT_DEV 1

/* Define to 1 if you have the <nfs/nfs_client.h> header file. */
/* #undef HAVE_NFS_NFS_CLIENT_H */

/* Define to 1 if you have the <nfs/vfs.h> header file. */
/* #undef HAVE_NFS_VFS_H */

/* Define to 1 if you have the `nice' function. */
/* #undef HAVE_NICE */

/* Define to 1 if you have the <nlist.h> header file. */
/* #undef HAVE_NLIST_H */

/* Define to 1 if libc includes obstacks. */
#define HAVE_OBSTACK 1

/* Define to 1 if you have the `openat' function. */
/* #undef HAVE_OPENAT */

/* Define to 1 if you have the <OS.h> header file. */
#define HAVE_OS_H 1

/* Define to 1 if getcwd works, except it sometimes fails when it shouldn't,
   setting errno to ERANGE, ENAMETOOLONG, or ENOENT. If __GETCWD_PREFIX is not
   defined, it doesn't matter whether HAVE_PARTLY_WORKING_GETCWD is defined.
   */
/* #undef HAVE_PARTLY_WORKING_GETCWD */

/* Define to 1 if you have the `pathconf' function. */
#define HAVE_PATHCONF 1

/* Define to 1 if you have the <paths.h> header file. */
/* #undef HAVE_PATHS_H */

/* Define to 1 if you have the `pipe' function. */
#define HAVE_PIPE 1

/* Define to 1 if you have the <priv.h> header file. */
/* #undef HAVE_PRIV_H */

/* Define if your system has the /proc/uptime special file. */
/* #undef HAVE_PROC_UPTIME */

/* Define to 1 if you have the `pstat_getdynamic' function. */
/* #undef HAVE_PSTAT_GETDYNAMIC */

/* Define to 1 if you have the `pstat_getstatic' function. */
/* #undef HAVE_PSTAT_GETSTATIC */

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if you have the <pwd.h> header file. */
#define HAVE_PWD_H 1

/* Define to 1 if you have the `raise' function. */
#define HAVE_RAISE 1

/* Define to 1 if you have the `readlink' function. */
#define HAVE_READLINK 1

/* Define to 1 if your system has a GNU libc compatible `realloc' function,
   and to 0 otherwise. */
#define HAVE_REALLOC 1

/* Define to 1 if you have the `resolvepath' function. */
/* #undef HAVE_RESOLVEPATH */

/* Define to 1 if you have the `rmdir' function. */
#define HAVE_RMDIR 1

/* Define to 1 if you have the `rpmatch' function. */
/* #undef HAVE_RPMATCH */

/* Define to 1 if you have run the test for working tzset. */
#define HAVE_RUN_TZSET_TEST 1

/* Define to 1 if you have the <search.h> header file. */
/* #undef HAVE_SEARCH_H */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setgroups' function. */
/* #undef HAVE_SETGROUPS */

/* Define to 1 if you have the `sethostname' function. */
#define HAVE_SETHOSTNAME 1

/* Define to 1 if you have the `settimeofday' function. */
/* #undef HAVE_SETTIMEOFDAY */

/* Define to 1 if you have the <shadow.h> header file. */
/* #undef HAVE_SHADOW_H */

/* Define to 1 if you have the `sig2str' function. */
/* #undef HAVE_SIG2STR */

/* Define to 1 if you have the `siginterrupt' function. */
/* #undef HAVE_SIGINTERRUPT */

/* Define to 1 if 'sig_atomic_t' is a signed integer type. */
#define HAVE_SIGNED_SIG_ATOMIC_T 1

/* Define to 1 if 'wchar_t' is a signed integer type. */
/* #undef HAVE_SIGNED_WCHAR_T */

/* Define to 1 if 'wint_t' is a signed integer type. */
/* #undef HAVE_SIGNED_WINT_T */

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if stdbool.h conforms to C99. */
/* #undef HAVE_STDBOOL_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define if <stdint.h> exists, doesn't clash with <sys/types.h>, and declares
   uintmax_t. */
#define HAVE_STDINT_H_WITH_UINTMAX 1

/* Define to 1 if you have the <stdio_ext.h> header file. */
/* #undef HAVE_STDIO_EXT_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `stime' function. */
#define HAVE_STIME 1

/* Define to 1 if you have the `stpcpy' function. */
#define HAVE_STPCPY 1

/* Define to 1 if you have the `strcoll' function and it is properly defined.
   */
#define HAVE_STRCOLL 1

/* Define to 1 if you have the `strcspn' function. */
#define HAVE_STRCSPN 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror_r' function. */
#define HAVE_STRERROR_R 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define if you have the strndup() function and it works. */
/* #undef HAVE_STRNDUP */

/* Define to 1 if you have the <stropts.h> header file. */
/* #undef HAVE_STROPTS_H */

/* Define to 1 if you have the `strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define to 1 if you have the `strtoimax' function. */
#define HAVE_STRTOIMAX 1

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strtoll' function. */
#define HAVE_STRTOLL 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the `strtoull' function. */
#define HAVE_STRTOULL 1

/* Define to 1 if you have the `strtoumax' function. */
#define HAVE_STRTOUMAX 1

/* Define to 1 if the system has the type `struct addrinfo'. */
#define HAVE_STRUCT_ADDRINFO 1

/* Define if there is a member named d_type in the struct describing directory
   headers. */
/* #undef HAVE_STRUCT_DIRENT_D_TYPE */

/* Define to 1 if `f_fstypename' is member of `struct fsstat'. */
/* #undef HAVE_STRUCT_FSSTAT_F_FSTYPENAME */

/* Define to 1 if `n_un.n_name' is member of `struct nlist'. */
/* #undef HAVE_STRUCT_NLIST_N_UN_N_NAME */

/* Define to 1 if `sp_pwdp' is member of `struct spwd'. */
/* #undef HAVE_STRUCT_SPWD_SP_PWDP */

/* Define to 1 if `f_fstypename' is member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FSTYPENAME */

/* Define to 1 if `f_namelen' is member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_NAMELEN */

/* Define to 1 if `f_type' is member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_TYPE */

/* Define to 1 if `f_basetype' is member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_BASETYPE */

/* Define to 1 if `f_fstypename' is member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_FSTYPENAME */

/* Define to 1 if `f_namemax' is member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_NAMEMAX */

/* Define to 1 if `f_type' is member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_TYPE */

/* Define to 1 if `st_atimensec' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIMENSEC */

/* Define to 1 if `st_atimespec.tv_nsec' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC */

/* Define to 1 if `st_atim.st__tim.tv_nsec' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIM_ST__TIM_TV_NSEC */

/* Define to 1 if `st_atim.tv_nsec' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC */

/* Define to 1 if `st_author' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_AUTHOR */

/* Define to 1 if `st_blocks' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BLOCKS */

/* Define to 1 if `st_spare1' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_SPARE1 */

/* Define if struct timespec is declared in <time.h>. */
#define HAVE_STRUCT_TIMESPEC 1

/* Define to 1 if `tm_zone' is member of `struct tm'. */
#define HAVE_STRUCT_TM_TM_ZONE 1

/* Define if struct utimbuf is declared -- usually in <utime.h>. Some systems
   have utime.h but don't declare the struct anywhere. */
#define HAVE_STRUCT_UTIMBUF 1

/* Define to 1 if `ut_exit' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_EXIT */

/* Define to 1 if `ut_exit.e_exit' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_EXIT_E_EXIT */

/* Define to 1 if `ut_exit.e_termination' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_EXIT_E_TERMINATION */

/* Define to 1 if `ut_exit.ut_exit' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_EXIT_UT_EXIT */

/* Define to 1 if `ut_exit.ut_termination' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_EXIT_UT_TERMINATION */

/* Define to 1 if `ut_id' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_ID */

/* Define to 1 if `ut_name' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_NAME */

/* Define to 1 if `ut_pid' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_PID */

/* Define to 1 if `ut_type' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_TYPE */

/* Define to 1 if `ut_user' is member of `struct utmpx'. */
/* #undef HAVE_STRUCT_UTMPX_UT_USER */

/* Define to 1 if `ut_exit' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT */

/* Define to 1 if `ut_exit.e_exit' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT_E_EXIT */

/* Define to 1 if `ut_exit.e_termination' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT_E_TERMINATION */

/* Define to 1 if `ut_exit.ut_exit' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT_UT_EXIT */

/* Define to 1 if `ut_exit.ut_termination' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_EXIT_UT_TERMINATION */

/* Define to 1 if `ut_id' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_ID */

/* Define to 1 if `ut_name' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_NAME */

/* Define to 1 if `ut_pid' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_PID */

/* Define to 1 if `ut_type' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_TYPE */

/* Define to 1 if `ut_user' is member of `struct utmp'. */
/* #undef HAVE_STRUCT_UTMP_UT_USER */

/* Define to 1 if you have the `strverscmp' function. */
/* #undef HAVE_STRVERSCMP */

/* Define to 1 if you have the `strxfrm' function. */
#define HAVE_STRXFRM 1

/* Define to 1 if your `struct stat' has `st_blocks'. Deprecated, use
   `HAVE_STRUCT_STAT_ST_BLOCKS' instead. */
/* #undef HAVE_ST_BLOCKS */

/* Define if struct stat has an st_dm_mode member. */
/* #undef HAVE_ST_DM_MODE */

/* Define to 1 if you have the `sync' function. */
#define HAVE_SYNC 1

/* Define to 1 if you have the `sysctl' function. */
/* #undef HAVE_SYSCTL */

/* Define to 1 if you have the `sysinfo' function. */
/* #undef HAVE_SYSINFO */

/* FIXME */
/* #undef HAVE_SYSLOG */

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the `sysmp' function. */
/* #undef HAVE_SYSMP */

/* Define to 1 if you have the <sys/acl.h> header file. */
/* #undef HAVE_SYS_ACL_H */

/* Define to 1 if you have the <sys/bitypes.h> header file. */
/* #undef HAVE_SYS_BITYPES_H */

/* Define to 1 if you have the <sys/filsys.h> header file. */
/* #undef HAVE_SYS_FILSYS_H */

/* Define to 1 if you have the <sys/fs/s5param.h> header file. */
/* #undef HAVE_SYS_FS_S5PARAM_H */

/* Define to 1 if you have the <sys/fs_types.h> header file. */
/* #undef HAVE_SYS_FS_TYPES_H */

/* Define to 1 if you have the <sys/inttypes.h> header file. */
/* #undef HAVE_SYS_INTTYPES_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mntent.h> header file. */
/* #undef HAVE_SYS_MNTENT_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
/* #undef HAVE_SYS_MOUNT_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/pstat.h> header file. */
/* #undef HAVE_SYS_PSTAT_H */

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/statfs.h> header file. */
/* #undef HAVE_SYS_STATFS_H */

/* Define to 1 if you have the <sys/statvfs.h> header file. */
#define HAVE_SYS_STATVFS_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/sysinfo.h> header file. */
/* #undef HAVE_SYS_SYSINFO_H */

/* Define to 1 if you have the <sys/sysmp.h> header file. */
/* #undef HAVE_SYS_SYSMP_H */

/* Define to 1 if you have the <sys/systemcfg.h> header file. */
/* #undef HAVE_SYS_SYSTEMCFG_H */

/* Define to 1 if you have the <sys/systeminfo.h> header file. */
/* #undef HAVE_SYS_SYSTEMINFO_H */

/* Define to 1 if you have the <sys/table.h> header file. */
/* #undef HAVE_SYS_TABLE_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/ucred.h> header file. */
/* #undef HAVE_SYS_UCRED_H */

/* Define to 1 if you have the <sys/vfs.h> header file. */
/* #undef HAVE_SYS_VFS_H */

/* Define to 1 if you have the <sys/wait.h> header file. */
#define HAVE_SYS_WAIT_H 1

/* Define to 1 if you have the `table' function. */
/* #undef HAVE_TABLE */

/* Define to 1 if you have the `tcgetattr' function. */
#define HAVE_TCGETATTR 1

/* Define to 1 if you have the `tcgetpgrp' function. */
#define HAVE_TCGETPGRP 1

/* Define to 1 if you have the `tcsetattr' function. */
#define HAVE_TCSETATTR 1

/* Define to 1 if you have the <termios.h> header file. */
#define HAVE_TERMIOS_H 1

/* Define to 1 if localtime_r, etc. have the type signatures that POSIX
   requires. */
#define HAVE_TIME_R_POSIX 1

/* Define if struct tm has the tm_gmtoff member. */
#define HAVE_TM_GMTOFF 1

/* Define to 1 if your `struct tm' has `tm_zone'. Deprecated, use
   `HAVE_STRUCT_TM_TM_ZONE' instead. */
#define HAVE_TM_ZONE 1

/* Define to 1 if you have the `tsearch' function. */
/* #undef HAVE_TSEARCH */

/* Define to 1 if you don't have `tm_zone' but do have the external array
   `tzname'. */
/* #undef HAVE_TZNAME */

/* Define to 1 if you have the `tzset' function. */
#define HAVE_TZSET 1

/* Define to 1 if you have the `uname' function. */
#define HAVE_UNAME 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#define HAVE_UNSETENV 1

/* Define to 1 if the system has the type `unsigned long long int'. */
#define HAVE_UNSIGNED_LONG_LONG_INT 1

/* Define if utimes accepts a null argument */
/* #undef HAVE_UTIMES_NULL */

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if `utime(file, NULL)' sets file's timestamp to the present. */
#define HAVE_UTIME_NULL 1

/* Define to 1 if you have the `utmpname' function. */
/* #undef HAVE_UTMPNAME */

/* Define to 1 if you have the `utmpxname' function. */
/* #undef HAVE_UTMPXNAME */

/* Define to 1 if you have the <utmpx.h> header file. */
/* #undef HAVE_UTMPX_H */

/* Define to 1 if you have the <utmp.h> header file. */
/* #undef HAVE_UTMP_H */

/* FIXME */
/* #undef HAVE_UT_HOST */

/* Define to 1 if you have the `vasnprintf' function. */
/* #undef HAVE_VASNPRINTF */

/* Define to 1 if you have the `vasprintf' function. */
#define HAVE_VASPRINTF 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define if you have the 'wchar_t' type. */
#define HAVE_WCHAR_T 1

/* Define to 1 if you have the `wcrtomb' function. */
#define HAVE_WCRTOMB 1

/* Define to 1 if you have the `wcscoll' function. */
#define HAVE_WCSCOLL 1

/* Define to 1 if you have the `wcslen' function. */
#define HAVE_WCSLEN 1

/* Define to 1 if you have the <wctype.h> header file. */
#define HAVE_WCTYPE_H 1

/* Define to 1 if you have the `wcwidth' function. */
/* #undef HAVE_WCWIDTH */

/* Define to 1 if you have the <winsock2.h> header file. */
/* #undef HAVE_WINSOCK2_H */

/* Define if you have the 'wint_t' type. */
#define HAVE_WINT_T 1

/* Define to 1 if you have the `wmemchr' function. */
#define HAVE_WMEMCHR 1

/* Define to 1 if you have the `wmemcpy' function. */
#define HAVE_WMEMCPY 1

/* Define to 1 if you have the `wmempcpy' function. */
/* #undef HAVE_WMEMPCPY */

/* Define to 1 if O_NOATIME works. */
#define HAVE_WORKING_O_NOATIME 0

/* Define to 1 if O_NOFOLLOW works. */
#define HAVE_WORKING_O_NOFOLLOW 0

/* Define if utimes works properly. */
/* #undef HAVE_WORKING_UTIMES */

/* Define to 1 if you have the <ws2tcpip.h> header file. */
/* #undef HAVE_WS2TCPIP_H */

/* Define to 1 if the system has the type `_Bool'. */
/* #undef HAVE__BOOL */

/* Define to 1 if you have the external variable, _system_configuration with a
   member named physmem. */
/* #undef HAVE__SYSTEM_CONFIGURATION */

/* Define to 1 if you have the `__fpending' function. */
/* #undef HAVE___FPENDING */

/* Define to 1 if you have the `__fsetlocking' function. */
/* #undef HAVE___FSETLOCKING */

/* The host operating system. */
#define HOST_OPERATING_SYSTEM "BeOS"

/* Define as const if the declaration of iconv() needs const. */
/* #undef ICONV_CONST */

#if FILE_SYSTEM_BACKSLASH_IS_FILE_NAME_SEPARATOR
# define ISSLASH(C) ((C) == '/' || (C) == '\\')
#else
# define ISSLASH(C) ((C) == '/')
#endif

/* Define if `link(2)' dereferences symbolic links. */
#define LINK_FOLLOWS_SYMLINKS 1

/* FIXME */
#define LOCALTIME_CACHE 1

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
#define LSTAT_FOLLOWS_SLASHED_SYMLINK 1

/* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>.
   */
/* #undef MAJOR_IN_MKDEV */

/* Define to 1 if `major', `minor', and `makedev' are declared in
   <sysmacros.h>. */
//#define MAJOR_IN_SYSMACROS 1

/* If malloc(0) is != NULL, define this to 1. Otherwise define this to 0. */
#define MALLOC_0_IS_NONNULL 1

/* Define if there is no specific function for reading the list of mounted
   file systems. fread will be used to read /etc/mnttab. (SVR2) */
/* #undef MOUNTED_FREAD */

/* Define if (like SVR2) there is no specific function for reading the list of
   mounted file systems, and your system has these header files: <sys/fstyp.h>
   and <sys/statfs.h>. (SVR3) */
/* #undef MOUNTED_FREAD_FSTYP */

/* Define if there are functions named next_dev and fs_stat_dev for reading
   the list of mounted file systems. (BeOS) */
#define MOUNTED_FS_STAT_DEV 1

/* Define if there is a function named getfsstat for reading the list of
   mounted file systems. (DEC Alpha running OSF/1) */
/* #undef MOUNTED_GETFSSTAT */

/* Define if there is a function named getmnt for reading the list of mounted
   file systems. (Ultrix) */
/* #undef MOUNTED_GETMNT */

/* Define if there is a function named getmntent for reading the list of
   mounted file systems, and that function takes a single argument. (4.3BSD,
   SunOS, HP-UX, Dynix, Irix) */
/* #undef MOUNTED_GETMNTENT1 */

/* Define if there is a function named getmntent for reading the list of
   mounted file systems, and that function takes two arguments. (SVR4) */
/* #undef MOUNTED_GETMNTENT2 */

/* Define if there is a function named getmntinfo for reading the list of
   mounted file systems and it returns an array of 'struct statfs'. (4.4BSD,
   Darwin) */
/* #undef MOUNTED_GETMNTINFO */

/* Define if there is a function named getmntinfo for reading the list of
   mounted file systems and it returns an array of 'struct statvfs'. (NetBSD
   3.0) */
/* #undef MOUNTED_GETMNTINFO2 */

/* Define if there is a function named listmntent that can be used to list all
   mounted file systems. (UNICOS) */
/* #undef MOUNTED_LISTMNTENT */

/* Define if there is a function named mntctl that can be used to read the
   list of mounted file systems, and there is a system header file that
   declares `struct vmount.' (AIX) */
/* #undef MOUNTED_VMOUNT */

/* Define to 1 if assertions should be disabled. */
/* #undef NDEBUG */

/* Define to 1 if your `struct nlist' has an `n_un' member. Obsolete, depend
   on `HAVE_STRUCT_NLIST_N_UN_N_NAME */
/* #undef NLIST_NAME_UNION */

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "coreutils"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "bug-coreutils@gnu.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "GNU coreutils"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "GNU coreutils 6.7"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "coreutils"

/* Define to the version of this package. */
#define PACKAGE_VERSION "6.7"

/* the number of pending output bytes on stream `fp' */
#define PENDING_OUTPUT_N_BYTES fp->_IO_write_ptr - fp->_IO_write_base

/* Define to the maximum link count that a true pipe can have. */
#define PIPE_LINK_COUNT_MAX (1)

/* Define this if you prefer euidaccess to return the correct result even if
   this would make it nonreentrant. Define this only if your entire
   application is safe even if the uid or gid might temporarily change. If
   your application uses signal handlers or threads it is probably not safe.
   */
#define PREFER_NONREENTRANT_EUIDACCESS 1

/* Define if <inttypes.h> exists and defines unusable PRI* macros. */
/* #undef PRI_MACROS_BROKEN */

/* Define to 1 if the C compiler supports function prototypes. */
#define PROTOTYPES 1

/* Define to 1 to provide canonicalize_filename_mode. */
#define PROVIDE_CANONICALIZE_FILENAME_MODE 1

/* Define to l, ll, u, ul, ull, etc., as suitable for constants of type
   'ptrdiff_t'. */
#define PTRDIFF_T_SUFFIX l

/* Define if rename does not work for source file names with a trailing slash,
   like the one from SunOS 4.1.1_U1. */
#define RENAME_TRAILING_SLASH_BUG 1

/* the value to which errno is set when rmdir fails on a nonempty directory */
#define RMDIR_ERRNO_NOT_EMPTY -2147459066

/* Define to 1 if the `setvbuf' function takes the buffering type as its
   second argument and the buffer pointer as the third, as on System V before
   release 3. */
/* #undef SETVBUF_REVERSED */

/* Define to l, ll, u, ul, ull, etc., as suitable for constants of type
   'sig_atomic_t'. */
#define SIG_ATOMIC_T_SUFFIX 

/* Define to l, ll, u, ul, ull, etc., as suitable for constants of type
   'size_t'. */
#define SIZE_T_SUFFIX ul

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define if the block counts reported by statfs may be truncated to 2GB and
   the correct values may be stored in the f_spare array. (SunOS 4.1.2, 4.1.3,
   and 4.1.3_U1 are reported to have this problem. SunOS 4.1.1 seems not to be
   affected.) */
/* #undef STATFS_TRUNCATES_BLOCK_COUNTS */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define if there is no specific function for reading file systems usage
   information and you have the <sys/filsys.h> header file. (SVR2) */
/* #undef STAT_READ_FILSYS */

/* Define if statfs takes 2 args and struct statfs has a field named f_bsize.
   (4.3BSD, SunOS 4, HP-UX, AIX PS/2) */
/* #undef STAT_STATFS2_BSIZE */

/* Define if statfs takes 2 args and struct statfs has a field named f_fsize.
   (4.4BSD, NetBSD) */
/* #undef STAT_STATFS2_FSIZE */

/* Define if statfs takes 2 args and the second argument has type struct
   fs_data. (Ultrix) */
/* #undef STAT_STATFS2_FS_DATA */

/* Define if statfs takes 3 args. (DEC Alpha running OSF/1) */
/* #undef STAT_STATFS3_OSF1 */

/* Define if statfs takes 4 args. (SVR3, Dynix, Irix, Dolphin) */
/* #undef STAT_STATFS4 */

/* Define if there is a function named statvfs. (SVR4) */
#define STAT_STATVFS 1

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if strerror_r returns char *. */
/* #undef STRERROR_R_CHAR_P */

/* Define to 1 if the f_fsid member of struct statfs is an integer. */
/* #undef STRUCT_STATFS_F_FSID_IS_INTEGER */

/* Define to 1 if the f_fsid member of struct statvfs is an integer. */
/* #undef STRUCT_STATVFS_F_FSID_IS_INTEGER */

/* Define to 1 on System V Release 4. */
/* #undef SVR4 */

/* FIXME */
/* #undef TERMIOS_NEEDS_XOPEN_SOURCE */

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Define to 1 if the type of the st_atim member of a struct stat is struct
   timespec. */
/* #undef TYPEOF_STRUCT_STAT_ST_ATIM_IS_STRUCT_TIMESPEC */

/* Define if tzset clobbers localtime's static buffer. */
#define TZSET_CLOBBERS_LOCALTIME_BUFFER 1

/* Define to 1 for Encore UMAX. */
/* #undef UMAX */

/* Define to 1 for Encore UMAX 4.3 that has <inq_status/cpustats.h> instead of
   <sys/cpustats.h>. */
/* #undef UMAX4_3 */

/* Define to 1 if unlink (dir) cannot possibly succeed. */
/* #undef UNLINK_CANNOT_UNLINK_DIR */

/* Define if you want access control list support. */
#define USE_ACL 0

/* Define to 1 if you want getc etc. to use unlocked I/O if available.
   Unlocked I/O can improve performance in unithreaded apps, but it is not
   safe for multithreaded apps. */
#define USE_UNLOCKED_IO 1

/* Version number of package */
#define VERSION "6.7"

/* Define if unsetenv() returns void, not int. */
/* #undef VOID_UNSETENV */

/* Define to l, ll, u, ul, ull, etc., as suitable for constants of type
   'wchar_t'. */
#define WCHAR_T_SUFFIX 

/* Define if sys/ptem.h is required for struct winsize. */
/* #undef WINSIZE_IN_PTEM */

/* Define to l, ll, u, ul, ull, etc., as suitable for constants of type
   'wint_t'. */
#define WINT_T_SUFFIX u

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

/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if on MINIX. */
/* #undef _MINIX */

/* Define to 2 if the system does not provide POSIX.1 features except with
   this defined. */
/* #undef _POSIX_1_SOURCE */

/* Define to 1 if you need to in order for `stat' and other things to work. */
/* #undef _POSIX_SOURCE */

/* Define if you want regoff_t to be at least as wide POSIX requires. */
#define _REGEX_LARGE_OFFSETS 1

/* Enable extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif

/* Define to rpl_ if the getcwd replacement function should be used. */
#define __GETCWD_PREFIX rpl_

/* Define to rpl_ if the getopt replacement functions and variables should be
   used. */
/* #undef __GETOPT_PREFIX */

/* Define to rpl_ if the mkstemp replacement function should be used. */
/* #undef __MKSTEMP_PREFIX */

/* Define to rpl_ if the openat replacement function should be used. */
#define __OPENAT_PREFIX rpl_

/* Define like PROTOTYPES; this can be used by system headers. */
#define __PROTOTYPES 1

/* Define to rpl_calloc if the replacement function should be used. */
#define calloc rpl_calloc

/* Define to rpl_chown if the replacement function should be used. */
#define chown rpl_chown

/* Define to a replacement function name for fnmatch(). */
#define fnmatch gnu_fnmatch

/* Define to rpl_free if the replacement function should be used. */
/* #undef free */

/* Define as rpl_getgroups if getgroups doesn't work right. */
/* #undef getgroups */

/* Define to a replacement function name for getline(). */
/* #undef getline */

/* Define to a replacement function name for getpass(). */
#define getpass gnu_getpass

/* Define to rpl_gettimeofday if the replacement function should be used. */
/* #undef gettimeofday */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* A replacement for va_copy, if needed.  */
#define gl_va_copy(a,b) ((a) = (b))

/* Define to rpl_gmtime if the replacement function should be used. */
#define gmtime rpl_gmtime

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `unsigned long int' if <sys/types.h> does not define. */
/* #undef ino_t */

/* Define to long or long long if <stdint.h> and <inttypes.h> don't define. */
/* #undef intmax_t */

/* Define to rpl_localtime if the replacement function should be used. */
#define localtime rpl_localtime

/* Define to `unsigned int' if <sys/types.h> does not define. */
#define major_t unsigned int

/* Define to rpl_malloc if the replacement function should be used. */
/* #undef malloc */

/* Define to a type if <wchar.h> does not define. */
/* #undef mbstate_t */

/* Define to rpl_memcmp if the replacement function should be used. */
/* #undef memcmp */

/* Define to `unsigned int' if <sys/types.h> does not define. */
#define minor_t unsigned int

/* Define to rpl_mkdir if the replacement function should be used. */
#define mkdir rpl_mkdir

/* Define to rpl_mktime if the replacement function should be used. */
#define mktime rpl_mktime

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to the name of the strftime replacement function. */
#define my_strftime nstrftime

/* Define to rpl_nanosleep if the replacement function should be used. */
#define nanosleep rpl_nanosleep

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to rpl_putenv if the replacement function should be used. */
#define putenv rpl_putenv

/* Define to rpl_re_comp if the replacement should be used. */
#define re_comp rpl_re_comp

/* Define to rpl_re_compile_fastmap if the replacement should be used. */
#define re_compile_fastmap rpl_re_compile_fastmap

/* Define to rpl_re_compile_pattern if the replacement should be used. */
#define re_compile_pattern rpl_re_compile_pattern

/* Define to rpl_re_exec if the replacement should be used. */
#define re_exec rpl_re_exec

/* Define to rpl_re_match if the replacement should be used. */
#define re_match rpl_re_match

/* Define to rpl_re_match_2 if the replacement should be used. */
#define re_match_2 rpl_re_match_2

/* Define to rpl_re_search if the replacement should be used. */
#define re_search rpl_re_search

/* Define to rpl_re_search_2 if the replacement should be used. */
#define re_search_2 rpl_re_search_2

/* Define to rpl_re_set_registers if the replacement should be used. */
#define re_set_registers rpl_re_set_registers

/* Define to rpl_re_set_syntax if the replacement should be used. */
#define re_set_syntax rpl_re_set_syntax

/* Define to rpl_re_syntax_options if the replacement should be used. */
#define re_syntax_options rpl_re_syntax_options

/* Define to rpl_realloc if the replacement function should be used. */
/* #undef realloc */

/* Define to rpl_regcomp if the replacement should be used. */
#define regcomp rpl_regcomp

/* Define to rpl_regerror if the replacement should be used. */
#define regerror rpl_regerror

/* Define to rpl_regexec if the replacement should be used. */
#define regexec rpl_regexec

/* Define to rpl_regfree if the replacement should be used. */
#define regfree rpl_regfree

/* Define to rpl_rename_dest_slash if the replacement function should be used.
   */
#define rename rpl_rename_dest_slash

/* Define to equivalent of C99 restrict keyword, or to nothing if this is not
   supported. Do not define if restrict is supported directly. */
#define restrict __restrict

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* type to use in place of socklen_t if not defined */
/* #undef socklen_t */

/* Define as a signed type of the same size as size_t. */
/* #undef ssize_t */

/* Define to rpl_strnlen if the replacement function should be used. */
/* #undef strnlen */

/* Define to rpl_strtod if the replacement function should be used. */
/* #undef strtod */

/* Define to rpl_tzset if the wrapper function should be used. */
#define tzset rpl_tzset

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define to rpl_utime if the replacement function should be used. */
/* #undef utime */

/* Define as a macro for copying va_list variables. */
#define va_copy __va_copy

/* Define to empty if the keyword `volatile' does not work. Warning: valid
   code using `volatile' can become incorrect without. Disable with care. */
/* #undef volatile */
