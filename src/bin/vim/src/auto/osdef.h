/*
 * osdef.h is automagically created from osdef?.h.in by osdef.sh -- DO NOT EDIT
 */
/* autoconf cannot fiddle out declarations. Use our homebrewn tools. (jw) */
/*
 * Declarations that may cause conflicts belong here so that osdef.sh
 * can clean out the forest. Everything else belongs in os_unix.h
 *
 * How this works:
 * - This file contains all unix prototypes that Vim might need.
 * - The shell script osdef.sh is executed at compile time to remove all the
 *   prototypes that are in an include file. This results in osdef.h.
 * - osdef.h is included in vim.h.
 *
 * sed cannot always handle so many commands, this is file 1 of 2
 */

#ifndef fopen	/* could be redefined to fopen64() */
#endif
#ifdef HAVE_FSEEKO
extern int	fseeko __ARGS((FILE *, off_t, int));
#endif
#ifdef HAVE_FTELLO
extern off_t	ftello __ARGS((FILE *));
#endif
#ifndef ferror	/* let me say it again: "macros should never have prototypes" */
#endif
#if defined(sun) || defined(_SEQUENT_)
/* used inside of stdio macros getc(), puts(), putchar()... */
extern int	_flsbuf __ARGS((int, FILE *));
extern int	_filbuf __ARGS((FILE *));
#endif

#if !defined(HAVE_SELECT)
struct pollfd;			/* for poll __ARGS */
extern int	poll __ARGS((struct pollfd *, long, int));
#endif

#ifdef HAVE_MEMSET
#endif
#ifdef HAVE_BCMP
extern int	bcmp __ARGS((void *, void *, size_t));
#endif
#ifdef HAVE_MEMCMP
#endif
#ifdef HAVE_STRPBRK
#endif
#ifdef USEBCOPY
extern void	bcopy __ARGS((char *, char *, int));
#else
# ifdef USEMEMCPY
# else
#  ifdef USEMEMMOVE
#  endif
# endif
#endif
/* used inside of FD_ZERO macro: */
extern void	bzero __ARGS((void *, size_t));
#ifdef HAVE_SETSID
#endif
#ifdef HAVE_SETPGID
#endif
#ifdef HAVE_STRTOL
#endif
#ifdef HAVE_STRFTIME
#endif
#ifdef HAVE_STRCASECMP
#endif
#ifdef HAVE_STRNCASECMP
#endif
#ifndef strdup
#endif

#ifndef USE_SYSTEM
#endif


#ifdef HAVE_SIGSET
extern RETSIGTYPE (*sigset __ARGS((int, RETSIGTYPE (*func) SIGPROTOARG))) __ARGS(SIGPROTOARG);
#endif

#if defined(HAVE_SETJMP_H)
# ifdef HAVE_SIGSETJMP
extern int	sigsetjmp __ARGS((sigjmp_buf, int));
# else
extern int	setjmp __ARGS((jmp_buf));
# endif
#endif


#if defined(HAVE_GETCWD) && !defined(sun)
#else
extern char	*getwd __ARGS((char *));
#endif
#ifndef __alpha	/* suggested by Campbell */
#endif
/*
 * osdef2.h.in - See osdef1.h.in for a description.
 */



extern int	usleep __ARGS((unsigned int));
#ifndef stat	/* could be redefined to stat64() */
#endif
#ifndef lstat	/* could be redefined to lstat64() */
#endif



#ifdef HAVE_TERMIOS_H
struct termios;		/* for tcgetattr __ARGS */
extern int	tcgetattr __ARGS((int, struct termios *));
#endif

#ifdef HAVE_SYS_STATFS_H
struct statfs;		/* for fstatfs __ARGS */
extern int	fstatfs __ARGS((int, struct statfs *, int, int));
#endif

#ifdef HAVE_GETTIMEOFDAY
#endif

#ifdef HAVE_GETPWNAM
#endif

#ifdef USE_TMPNAM
#else
#endif

#ifdef ISC
extern int	_Xmblen __ARGS((char const *, size_t));
#else
		/* This is different from the header but matches mblen() */
extern int	_Xmblen __ARGS((char *, size_t));
#endif
