/* auto/config.h.  Generated automatically by configure.  */
/*
 * config.h.in.  Generated automatically from configure.in by autoheader, and
 * manually changed after that.
 */

/* Define if we have EBCDIC code */
/* #undef EBCDIC */

/* Define unless no X support found */
/* #undef HAVE_X11 */

/* Define when terminfo support found */
/* #undef TERMINFO */

/* Define when termcap.h contains ospeed */
#define HAVE_OSPEED 1

/* Define when ospeed can be extern */
/* #undef OSPEED_EXTERN */

/* Define when termcap.h contains UP, BC and PC */
#define HAVE_UP_BC_PC 1

/* Define when UP, BC and PC can be extern */
/* #undef UP_BC_PC_EXTERN */

/* Define when termcap.h defines outfuntype */
/* #undef HAVE_OUTFUNTYPE */

/* Define when __DATE__ " " __TIME__ can be used */
#define HAVE_DATE_TIME 1

/* defined always when using configure */
#define UNIX 1

/* Defined to the size of an int */
#define SIZEOF_INT 4

/*
 * If we cannot trust one of the following from the libraries, we use our
 * own safe but probably slower vim_memmove().
 */
/* #undef USEBCOPY */
#define USEMEMMOVE 1
/* #undef USEMEMCPY */

/* Define when "man -s 2" is to be used */
/* #undef USEMAN_S */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef ino_t */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef dev_t */

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
#define rlim_t unsigned long

/* Define to `struct sigaltstack' if <signal.h> doesn't define.  */
/* #undef stack_t */

/* Define if stack_t has the ss_base field. */
/* #undef HAVE_SS_BASE */

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if you can safely include both <sys/time.h> and <sys/select.h>.  */
#define SYS_SELECT_WITH_SYS_TIME 1

/* Define if you have /dev/ptc */
/* #undef HAVE_DEV_PTC */

/* Define if you have Sys4 ptys */
/* #undef HAVE_SVR4_PTYS */

/* Define to range of pty names to try */
/* #undef PTYRANGE0 */
/* #undef PTYRANGE1 */

/* Define mode for pty */
/* #undef PTYMODE */

/* Define group for pty */
/* #undef PTYGROUP */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define as the command at the end of signal handlers ("" or "return 0;").  */
#define SIGRETURN return

/* Define if struct sigcontext is present */
#define HAVE_SIGCONTEXT 1

/* Define if touuper/tolower only work on lower/upercase characters */
/* #undef BROKEN_TOUPPER */

/* Define if stat() ignores a trailing slash */
/* #undef STAT_IGNORES_SLASH */

/* Define if tgetstr() has a second argument that is (char *) */
/* #undef TGETSTR_CHAR_P */

/* Define if tgetent() returns zero for an error */
#define TGETENT_ZERO_ERR 0

/* Define if the getcwd() function should not be used.  */
/* #undef BAD_GETCWD */

/* Define if you the function: */
#define HAVE_BCMP 1
#define HAVE_FCHDIR 1
#define HAVE_FCHOWN 1
#define HAVE_FSEEKO 1
#define HAVE_FSYNC 1
#define HAVE_FTELLO 1
#define HAVE_GETCWD 1
/* #undef HAVE_GETPSEUDOTTY */
#define HAVE_GETPWNAM 1
#define HAVE_GETPWUID 1
/* #undef HAVE_GETRLIMIT */
#define HAVE_GETTIMEOFDAY 1
/* #undef HAVE_GETWD */
/* #undef HAVE_ICONV */
/* #undef HAVE_NL_LANGINFO_CODESET */
#define HAVE_LSTAT 1
#define HAVE_MEMCMP 1
#define HAVE_MEMSET 1
/* #undef HAVE_NANOSLEEP */
#define HAVE_OPENDIR 1
#define HAVE_PUTENV 1
#define HAVE_QSORT 1
#define HAVE_READLINK 1
#define HAVE_RENAME 1
#define HAVE_SELECT 1
#define HAVE_SETENV 1
#define HAVE_SETPGID 1
#define HAVE_SETSID 1
#define HAVE_SIGACTION 1
/* #undef HAVE_SIGALTSTACK */
/* #undef HAVE_SIGSET */
#define HAVE_SIGSETJMP 1
/* #undef HAVE_SIGSTACK */
/* #undef HAVE_SIGVEC */
#define HAVE_SNPRINTF 1
#define HAVE_STRCASECMP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
/* #undef HAVE_STRICMP */
#define HAVE_STRNCASECMP 1
/* #undef HAVE_STRNICMP */
#define HAVE_STRPBRK 1
#define HAVE_STRTOL 1
#define HAVE_ST_BLKSIZE 1
/* #undef HAVE_SYSCONF */
/* #undef HAVE_SYSCTL */
/* #undef HAVE_SYSINFO */
#define HAVE_TGETENT 1
#define HAVE_TOWLOWER 1
#define HAVE_TOWUPPER 1
/* #undef HAVE_USLEEP */
#define HAVE_UTIME 1
/* #undef HAVE_BIND_TEXTDOMAIN_CODESET */

/* Define if you do not have utime(), but do have the utimes() function. */
/* #undef HAVE_UTIMES */

/* Define if you have the header file: */
#define HAVE_DIRENT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
/* #undef HAVE_FRAME_H */
/* #undef HAVE_ICONV_H */
/* #undef HAVE_LANGINFO_H */
/* #undef HAVE_LIBC_H */
/* #undef HAVE_LIBGEN_H */
/* #undef HAVE_LIBINTL_H */
#define HAVE_LOCALE_H 1
/* #undef HAVE_NDIR_H */
/* #undef HAVE_POLL_H */
/* #undef HAVE_PTHREAD_NP_H */
#define HAVE_PWD_H 1
#define HAVE_SETJMP_H 1
/* #undef HAVE_SGTTY_H */
#define HAVE_STRINGS_H 1
/* #undef HAVE_STROPTS_H */
/* #undef HAVE_SYS_ACCESS_H */
/* #undef HAVE_SYS_ACL_H */
/* #undef HAVE_SYS_DIR_H */
#define HAVE_SYS_IOCTL_H 1
/* #undef HAVE_SYS_NDIR_H */
#define HAVE_SYS_PARAM_H 1
/* #undef HAVE_SYS_POLL_H */
/* #undef HAVE_SYS_PTEM_H */
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_SELECT_H 1
/* #undef HAVE_SYS_STATFS_H */
/* #undef HAVE_SYS_STREAM_H */
/* #undef HAVE_SYS_SYSCTL_H */
/* #undef HAVE_SYS_SYSINFO_H */
/* #undef HAVE_SYS_SYSTEMINFO_H */
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_UTSNAME_H 1
#define HAVE_WCHAR_H 1
#define HAVE_TERMCAP_H 1
#define HAVE_TERMIOS_H 1
/* #undef HAVE_TERMIO_H */
#define HAVE_UNISTD_H 1
/* #undef HAVE_UTIL_DEBUG_H */
/* #undef HAVE_UTIL_MSGI18N_H */
#define HAVE_UTIME_H 1
/* #undef HAVE_X11_SUNKEYSYM_H */
/* #undef HAVE_XM_XM_H */
/* #undef HAVE_XM_XPMP_H */
/* #undef HAVE_X11_XPM_H */
/* #undef HAVE_X11_XMU_EDITRES_H */
/* #undef HAVE_X11_SM_SMLIB_H */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have a <sys/wait.h> that is not POSIX.1 compatible. */
/* #undef HAVE_UNION_WAIT */

/* This is currently unused in vim: */
/* Define if you have the ANSI C header files. */
/* #undef STDC_HEADERS */

/* instead, we check a few STDC things ourselves */
#define HAVE_STDARG_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1

/* Define if strings.h cannot be included when strings.h already is */
/* #undef NO_STRINGS_WITH_STRING_H */

/* Define if you want tiny features. */
/* #undef FEAT_TINY */

/* Define if you want small features. */
/* #undef FEAT_SMALL */

/* Define if you want normal features. */
/* #undef FEAT_NORMAL */

/* Define if you want big features. */
/* #undef FEAT_BIG */

/* Define if you want huge features. */
#define FEAT_HUGE 1

/* Define if you want to include the Perl interpreter. */
/* #undef FEAT_PERL */

/* Define if you want to include the Python interpreter. */
/* #undef FEAT_PYTHON */

/* Define if you want to include the Ruby interpreter. */
/* #undef FEAT_RUBY */

/* Define if you want to include the Tcl interpreter. */
/* #undef FEAT_TCL */

/* Define if you want to include the Sniff interface. */
/* #undef FEAT_SNIFF */

/* Define if you want to add support for ACL */
/* #undef HAVE_POSIX_ACL */
/* #undef HAVE_SOLARIS_ACL */
/* #undef HAVE_AIX_ACL */

/* Define if you want to add support of GPM (Linux console mouse daemon) */
/* #undef HAVE_GPM */

/* Define if you want to include the Cscope interface. */
#define FEAT_CSCOPE 1

/* Define if you want to include multibyte support. */
#define FEAT_MBYTE 1

/* Define if you want to include fontset support. */
/* #undef FEAT_XFONTSET */

/* Define if you want to include XIM support. */
/* #undef FEAT_XIM */

/* Define if you want to include Hangul input support. */
/* #undef FEAT_HANGULIN */

/* Define if you use GTK and want GNOME support. */
/* #undef FEAT_GUI_GNOME */

/* Define if GTK+ 2 is available. */
/* #undef HAVE_GTK2 */

/* Define if GTK+ multihead support is available (requires GTK+ >= 2.1.1). */
/* #undef HAVE_GTK_MULTIHEAD */

/* Define if your X has own locale library */
/* #undef X_LOCALE */

/* Define if we have dlfcn.h. */
/* #undef HAVE_DLFCN_H */

/* Define if there is a working gettext(). */
/* #undef HAVE_GETTEXT */

/* Define if _nl_msg_cat_cntr is present. */
/* #undef HAVE_NL_MSG_CAT_CNTR */

/* Define if we have dlopen() */
/* #undef HAVE_DLOPEN */

/* Define if we have dlsym() */
/* #undef HAVE_DLSYM */

/* Define if we have dl.h. */
/* #undef HAVE_DL_H */

/* Define if we have shl_load() */
/* #undef HAVE_SHL_LOAD */

/* Define if you want to include Sun Visual Workshop support. */
/* #undef FEAT_SUN_WORKSHOP */

/* Define if you want to include NetBeans integration. */
/* #undef FEAT_NETBEANS_INTG */

/* Define default global runtime path */
/* #undef RUNTIME_GLOBAL */

/* Define name of who modified a released Vim */
/* #undef MODIFIED_BY */

/* Define if you want XSMP interaction as well as vanilla swapfile safety */
#define USE_XSMP_INTERACT 1

/* Define if vsnprintf() works */
#define HAVE_VSNPRINTF 1
