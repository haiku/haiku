/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* A GNU-like <signal.h>.

   Copyright (C) 2006-2010 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#if defined __need_sig_atomic_t || defined __need_sigset_t
/* Special invocation convention inside glibc header files.  */

# include_next <signal.h>

#else
/* Normal invocation convention.  */

#ifndef _GL_SIGNAL_H

/* The include_next requires a split double-inclusion guard.  */
#include_next <signal.h>

#ifndef _GL_SIGNAL_H
#define _GL_SIGNAL_H

/* The definition of GL_LINK_WARNING is copied here.  */
/* GL_LINK_WARNING("literal string") arranges to emit the literal string as
   a linker warning on most glibc systems.
   We use a linker warning rather than a preprocessor warning, because
   #warning cannot be used inside macros.  */
#ifndef GL_LINK_WARNING
  /* This works on platforms with GNU ld and ELF object format.
     Testing __GLIBC__ is sufficient for asserting that GNU ld is in use.
     Testing __ELF__ guarantees the ELF object format.
     Testing __GNUC__ is necessary for the compound expression syntax.  */
# if defined __GLIBC__ && defined __ELF__ && defined __GNUC__
#  define GL_LINK_WARNING(message) \
     GL_LINK_WARNING1 (__FILE__, __LINE__, message)
#  define GL_LINK_WARNING1(file, line, message) \
     GL_LINK_WARNING2 (file, line, message)  /* macroexpand file and line */
#  define GL_LINK_WARNING2(file, line, message) \
     GL_LINK_WARNING3 (file ":" #line ": warning: " message)
#  define GL_LINK_WARNING3(message) \
     ({ static const char warning[sizeof (message)]             \
          __attribute__ ((__unused__,                           \
                          __section__ (".gnu.warning"),         \
                          __aligned__ (1)))                     \
          = message "\n";                                       \
        (void)0;                                                \
     })
# else
#  define GL_LINK_WARNING(message) ((void) 0)
# endif
#endif

/* The definition of _GL_ARG_NONNULL is copied here.  */
/* _GL_ARG_NONNULL((n,...,m)) tells the compiler and static analyzer tools
   that the values passed as arguments n, ..., m must be non-NULL pointers.
   n = 1 stands for the first argument, n = 2 for the second argument etc.  */
#ifndef _GL_ARG_NONNULL
# if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ > 3
#  define _GL_ARG_NONNULL(params) __attribute__ ((__nonnull__ params))
# else
#  define _GL_ARG_NONNULL(params)
# endif
#endif

/* Define pid_t, uid_t.
   Also, mingw defines sigset_t not in <signal.h>, but in <sys/types.h>.  */
#include <sys/types.h>

/* On AIX, sig_atomic_t already includes volatile.  C99 requires that
   'volatile sig_atomic_t' ignore the extra modifier, but C89 did not.
   Hence, redefine this to a non-volatile type as needed.  */
#if ! 1
typedef int rpl_sig_atomic_t;
# undef sig_atomic_t
# define sig_atomic_t rpl_sig_atomic_t
#endif

/* A set or mask of signals.  */
#if !1
typedef unsigned int sigset_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif


#if 0
# ifndef SIGPIPE
/* Define SIGPIPE to a value that does not overlap with other signals.  */
#  define SIGPIPE 13
#  define GNULIB_defined_SIGPIPE 1
/* To actually use SIGPIPE, you also need the gnulib modules 'sigprocmask',
   'write', 'stdio'.  */
# endif
#endif


#if 1
# if !1

/* Maximum signal number + 1.  */
#  ifndef NSIG
#   define NSIG 32
#  endif

/* This code supports only 32 signals.  */
typedef int verify_NSIG_constraint[2 * (NSIG <= 32) - 1];

/* Test whether a given signal is contained in a signal set.  */
extern int sigismember (const sigset_t *set, int sig) _GL_ARG_NONNULL ((1));

/* Initialize a signal set to the empty set.  */
extern int sigemptyset (sigset_t *set) _GL_ARG_NONNULL ((1));

/* Add a signal to a signal set.  */
extern int sigaddset (sigset_t *set, int sig) _GL_ARG_NONNULL ((1));

/* Remove a signal from a signal set.  */
extern int sigdelset (sigset_t *set, int sig) _GL_ARG_NONNULL ((1));

/* Fill a signal set with all possible signals.  */
extern int sigfillset (sigset_t *set) _GL_ARG_NONNULL ((1));

/* Return the set of those blocked signals that are pending.  */
extern int sigpending (sigset_t *set) _GL_ARG_NONNULL ((1));

/* If OLD_SET is not NULL, put the current set of blocked signals in *OLD_SET.
   Then, if SET is not NULL, affect the current set of blocked signals by
   combining it with *SET as indicated in OPERATION.
   In this implementation, you are not allowed to change a signal handler
   while the signal is blocked.  */
#  define SIG_BLOCK   0  /* blocked_set = blocked_set | *set; */
#  define SIG_SETMASK 1  /* blocked_set = *set; */
#  define SIG_UNBLOCK 2  /* blocked_set = blocked_set & ~*set; */
extern int sigprocmask (int operation, const sigset_t *set, sigset_t *old_set);

#  define signal rpl_signal
/* Install the handler FUNC for signal SIG, and return the previous
   handler.  */
extern void (*signal (int sig, void (*func) (int))) (int);

#  if GNULIB_defined_SIGPIPE

/* Raise signal SIG.  */
#   undef raise
#   define raise rpl_raise
extern int raise (int sig);

#  endif

# endif /* !1 */
#elif defined GNULIB_POSIXCHECK
# undef sigaddset
# define sigaddset(s,n) \
  (GL_LINK_WARNING ("sigaddset is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigaddset (s, n))
# undef sigdelset
# define sigdelset(s,n) \
  (GL_LINK_WARNING ("sigdelset is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigdelset (s, n))
# undef sigemptyset
# define sigemptyset(s) \
  (GL_LINK_WARNING ("sigemptyset is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigemptyset (s))
# undef sigfillset
# define sigfillset(s) \
  (GL_LINK_WARNING ("sigfillset is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigfillset (s))
# undef sigismember
# define sigismember(s,n) \
  (GL_LINK_WARNING ("sigismember is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigismember (s, n))
# undef sigpending
# define sigpending(s) \
  (GL_LINK_WARNING ("sigpending is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigpending (s))
# undef sigprocmask
# define sigprocmask(h,s,o)                               \
  (GL_LINK_WARNING ("sigprocmask is unportable - "        \
                    "use gnulib module sigprocmask for portability"),   \
   sigprocmask (h, s, o))
#endif /* 1 */


#if 1
# if !1

#  if !1
/* Present to allow compilation, but unsupported by gnulib.  */
union sigval
{
  int sival_int;
  void *sival_ptr;
};

/* Present to allow compilation, but unsupported by gnulib.  */
struct siginfo_t
{
  int si_signo;
  int si_code;
  int si_errno;
  pid_t si_pid;
  uid_t si_uid;
  void *si_addr;
  int si_status;
  long si_band;
  union sigval si_value;
};
typedef struct siginfo_t siginfo_t;
#  endif /* !1 */

/* We assume that platforms which lack the sigaction() function also lack
   the 'struct sigaction' type, and vice versa.  */

struct sigaction
{
  union
  {
    void (*_sa_handler) (int);
    /* Present to allow compilation, but unsupported by gnulib.  POSIX
       says that implementations may, but not must, make sa_sigaction
       overlap with sa_handler, but we know of no implementation where
       they do not overlap.  */
    void (*_sa_sigaction) (int, siginfo_t *, void *);
  } _sa_func;
  sigset_t sa_mask;
  /* Not all POSIX flags are supported.  */
  int sa_flags;
};
#  define sa_handler _sa_func._sa_handler
#  define sa_sigaction _sa_func._sa_sigaction
/* Unsupported flags are not present.  */
#  define SA_RESETHAND 1
#  define SA_NODEFER 2
#  define SA_RESTART 4

extern int sigaction (int, const struct sigaction *restrict,
                      struct sigaction *restrict);

# elif !0

#  define sa_sigaction sa_handler

# endif /* !1, !0 */
#elif defined GNULIB_POSIXCHECK
# undef sigaction
# define sigaction(s,a,o)                               \
  (GL_LINK_WARNING ("sigaction is unportable - "        \
                    "use gnulib module sigaction for portability"),   \
   sigaction (s, a, o))
#endif

/* Some systems don't have SA_NODEFER.  */
#ifndef SA_NODEFER
# define SA_NODEFER 0
#endif


#ifdef __cplusplus
}
#endif

#endif /* _GL_SIGNAL_H */
#endif /* _GL_SIGNAL_H */
#endif
